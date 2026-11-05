# 4. Task Scheduler

The scheduler is the pacemaker. NTP refresh, MQTT reconnect, OTA polling, GPIO blinking, session expiry, watchdog feeding, log heartbeats — all of it is a task. Every board gets the inline scheduler for free; a port that implements the threading interfaces adds two more lanes on top.

Implementation is [src/utility/TaskScheduler.h](../src/utility/TaskScheduler.h), and the global is `__task_scheduler`.

## 4.1 Three modes

| Mode | Runs on | Stack | Needs port support | Right when |
|---|---|---|---|---|
| inline | the main loop's stack, inside `run()` | the loop's own | no | the task is short and never blocks — this covers nearly everything |
| cooperative | its own stack, advancing only when it yields or sleeps | one you size | yes | you want to write it as `while(1){ step(); sleep(N); }` and you trust it to yield |
| preemptive | its own stack, interrupted by a timer ISR | one you size | yes | the task may block or run long, and you can't trust it to yield |

All three coexist. Each pass of `serve()` ticks the lanes in order:

```
  serve()
    ├─ __task_scheduler.run()                     inline: at most one task, then out
    ├─ device yield + event dispatch
    ├─ cooperative_scheduler.tick_from_loop()     one slice
    └─ preemptive_scheduler.yield()               ISR already drives it; this is courtesy
```

`tick_from_loop()` is the main-loop entry point for a lane. Cooperative schedulers forward it to `run()`; preemptive ones ignore it because a hardware timer is already driving them. A port that needs a different main-loop hook overrides only that one call.

## 4.2 Policies

Independently of mode, each task carries a priority, a nice value and a policy. Together they decide who goes first when several inline tasks come due on the same tick.

Selection runs in three stages, and the first one that separates two tasks decides between them:

```
  1. due?          a task whose interval has elapsed beats one whose has not
  2. vruntime      of two equally due tasks, the one that has taken less service
                   — and policy and priority both feed the weight that charges it
  3. policy score  only when both of the above tie
```

**Due-ness is a gate, not a term.** Nothing about priority lets a task run before its interval has elapsed, so a 300-second task cannot sit at the head of the queue keeping a 1 ms listener off the CPU.

**Virtual runtime is what priority actually buys.** Each task carries `m_vruntime`, and after every run it is charged the time that run took, divided by the task's weight:

```
  vruntime += exec_µs × TASK_WEIGHT_NOMINAL / task.m_weight
```

A heavier task is charged less for the same microseconds, so it comes back round sooner. Priority therefore changes the *rate* a task accrues debt, never granting it a standing advantage — which is what makes starvation unreachable rather than merely unlikely. Period-awareness falls out for free: a 10 ms task is charged a hundred times as often as a 1 s one, with no rule about periods anywhere.

The weight is held on the task and recomputed only when priority, nice or policy changes — `setTaskPriority`, `setTaskNice` and `setTaskPolicy` own that, so no caller can move one of the three without the weight following. Nothing recomputes it per run.

`taskWeight()` maps effective priority — `priority - nice`, clamped to 0..`MAX_TASK_PRIORITY` — onto a geometric ladder, computed rather than tabulated so no table lands in RAM on a small board:

```
  TASK_WEIGHT_LEVELS   = MAX_TASK_PRIORITY + 1        one step per priority
  step ratio           = (LEVELS + TASK_WEIGHT_SPAN) / LEVELS
```

Both derive from `MAX_TASK_PRIORITY`, so raising the ceiling adds steps without stretching the ladder: `(1 + k/n)^n` tends to `e^k`, which makes `TASK_WEIGHT_SPAN` the natural log of the whole ladder's span whatever the level count. At 99 that is a ratio of 109/100 and a span of 9 → 69,531, about 7,700×.

**One step per priority means every `renice` registers.** The named rungs land at `LOW` 27, `MEDIUM` 939, `HIGH` 8,068 and `MAX` 69,531 — so a task at `MEDIUM` is charged about 35× less than one at the `LOW` default, and a `renice` of 1 moves any task about 9%.

A newly registered task starts at the scheduler's `m_min_vruntime` rather than at zero, so it cannot arrive owing nothing and monopolise the CPU until it catches up. That floor only ever moves forward.

**Policy selects the weight**, the same way a scheduling class does rather than by adding a term to a score. That is what keeps it effective: vruntime is an accumulator, so two tasks are almost never exactly level on it, and anything that only broke ties would never be consulted.

| Policy | Weight | Effect |
|---|---|---|
| FIFO (default) | the priority ladder | priority sets the rate the task accrues debt |
| fair share | the priority ladder | the same rate; differs from FIFO only in the tie-break below |
| round robin | **nominal, ignoring priority** | every round-robin task accrues at one rate, so peers get equal slices whatever their priority |
| deadline | the ladder × `TASK_WEIGHT_DEADLINE_BOOST` | charged that much less, so it comes back round that much sooner |

Round robin not consulting the ladder is the point of it — a round-robin task at `LOW` and one at `HIGH` get the same share. Use it where several tasks must progress together and priority would otherwise separate them.

Only when due-ness **and** vruntime leave two tasks level does the blended score break the tie, on lateness, recentness or cumulative CPU according to the policy. Policies apply to the inline lane; contextual lanes schedule themselves.

Priorities are larger-is-more-important, in the range 0..`MAX_TASK_PRIORITY` (99), with `HIGH_TASK_PRIORITY` 74, `MEDIUM_TASK_PRIORITY` 49 and `LOW_TASK_PRIORITY` 10 as the named rungs. `DEFAULT_TASK_PRIORITY` is `LOW`. Nice runs the other way, POSIX-style, and is subtracted.

Deadline working through the weight is what makes it period-independent: a 10 ms task gets the same proportional lift as a 1 s one, because nothing in the charge depends on how far the task is from its deadline.

## 4.3 What a task record holds

A task is a POD stored by value in a vector reserved to `MAX_SCHEDULABLE_TASKS`, so steady-state scheduling causes no heap churn.

| Group | Fields | Why |
|---|---|---|
| identity | id, name, owner | pid, a read-only name pointer (often in flash), and the owning session — 0 means kernel. Ids count up and start again at `MAX_TASK_ID`, stepping over any still in use, so a caller holding the id of a task that has ended cannot reach whatever registered next |
| callback | the function | what actually runs |
| schedule | duration, last run, remaining attempts, priority, nice, policy, mode, virtual runtime, weight | everything selection needs. Weight is derived from priority, nice and policy, and held rather than recomputed each run |
| lifecycle | state, pending signal | ready / running / sleeping / stopped / zombie, plus a queued signal |
| observability | created-at, run count, last and total exec time in µs | what `ps` and `top` render, including %CPU |
| contextual | the lane executive | present only in contextual builds |

Exec time is sampled in microseconds around each callback, so a task that finishes in well under a millisecond still registers a non-zero cost rather than rounding to zero.

## 4.4 The API, by what you want to do

Every registration call takes a trailing name and owner. Fill them in — that is what makes a task visible in `ps` and reachable by `pkill NAME`. Services normally don't call these directly; they go through the `ServiceProvider` wrappers, which supply the service name automatically.

**Registering and cancelling**

| Want to | Call | Note |
|---|---|---|
| run once after N ms | `setTimeout(fn, dur, now, prio, name, owner)` | auto-removed after it fires |
| run every N ms | `setInterval(fn, dur, now, prio, name, owner)` | runs until cancelled |
| reschedule a one-shot | `updateTimeout(id, fn, dur, now)` | replaces callback and duration in place |
| reschedule a periodic | `updateInterval(id, …)` | falls back to registering fresh if the id is gone |
| spell everything out | `register_task(fn, dur, prio, last, maxAtt, name, owner)` | the lowest-level entry point |
| cancel either kind | `clearTimeout(id)` / `clearInterval(id)` | marks for removal on the next sweep |
| look one up | `get_task(id)` | to change policy or mode after the fact |
| force a re-sort now | `rebaseAndRestartPrioTasks()` | the current sweep breaks after one task |

`register_task` returns `-1` when the slot table is full — check it before assuming the task exists.

**Metadata, signals, rendering**

| Want to | Call |
|---|---|
| name a task after the fact | `setTaskName(id, name)` — the pointer must outlive the task |
| attach an owning session | `setTaskOwner(id, sid)` |
| change nice, -20..19 | `setTaskNice(id, nice)`, then rebase for an immediate re-sort |
| change the priority | `setTaskPriority(id, prio)` — clamped to `MAX_TASK_PRIORITY` |
| change the policy | `setTaskPolicy(id, policy)` — see [§4.2](#42-policies) |
| queue a signal on one task | `sendSignal(id, sig)` |
| signal every task with a name | `sendSignalByName(name, sig, requester, is_root)` — this is what `pkill` and `killall` use |
| promote a task to a lane | `scheduleUnderExecSched(sched, id, mode, stack)` |

## 4.5 What happens on a tick

```
  run() ──▶ handle_tasks()
             │
             1. now = millis
             2. order every task — due first, then least virtual runtime,
                then policy score — into an index table (the vector never
                moves, so task ids stay stable)
             3. for each task in score order:
                  │
                  ├─ contextual task?  deliver its pending signal to the lane
                  │                    (STOP suspends, CONT resumes, KILL ends),
                  │                    reap it if finished, then move on
                  │
                  ├─ consume the pending signal under one critical section
                  │     KILL / TERM ─▶ zombie, callback dropped
                  │     STOP        ─▶ stopped, skipped until CONT
                  │     CONT        ─▶ back to sleeping
                  │
                  ├─ due?  sample µs, run the callback, sample µs again,
                  │        update exec time, total time, run count, state,
                  │        and charge the run against the task's weight
                  │
                  ├─ advance last-run by whole intervals (catch-up capped at 3)
                  └─ yield to the platform
             4. after one task, if a rebase was requested, break out —
                the next tick re-sorts from scratch
             5. drop everything whose attempts hit zero
```

Point 4 is why the scheduler keeps jitter low without preemption: each pass of `serve()` runs at most one inline task before handing control back to the platform, the web server and the contextual lanes. It also means a freshly registered high-priority task is picked up on the very next tick.

## 4.6 Picking a mode

```
  Does the task work for more than a few ms,
  or call a blocking SDK API?
   │
   ├─ No ──▶ inline. No port work, no stack to size. Default answer.
   │
   └─ Yes ─▶ Would you naturally write it as while(1){ step(); sleep(N); } ?
              │
              ├─ Yes ──▶ cooperative. Cheap switches, you control the yields,
              │          and peers depend on you actually yielding.
              │
              └─ No, it may block unpredictably ──▶ preemptive. A timer ISR
                         forces the switch. Costlier, and it needs the port to
                         supply context switching and a tick ISR.
```

On a port without the contextual interfaces, inline plus a hand-written state machine is the answer; promoting a task to a lane that isn't there does nothing.

## 4.7 Behaviour worth knowing before you lean on it

Cancelling a task marks it rather than erasing it — the vector entry is dropped in the sweep at the end of `handle_tasks`, not the moment you call `clearInterval`. Don't dereference an id in between.

Tasks are stored by value, so a lambda capturing `this` or a chunk of state is fine, but those captures live until the removal sweep runs.

The scheduler is not re-entrant. Never call `run()` from inside a task; that is what the contextual lanes are for.

A freshly registered interval stamps its last-run time on first consideration, so it fires one full interval later rather than immediately. Catch-up after a stall is capped at three rounds, so a scheduler frozen through an OTA or a deep sleep resumes fresh instead of firing a burst of back-runs — re-register if you need exact counts.

The slot table is `MAX_SCHEDULABLE_TASKS`, 25 by default. Raising it raises every service's worst-case footprint and the factory-reset hook count along with it.

Due-ness itself is an exact comparison against `millis_now()`. The 3 ms tolerance applies only in the deepest tie-break, where two tasks are equally due, level on virtual runtime and level on score, so clock jitter cannot reshuffle ordinary task order.

## 4.8 Promoting a task to a lane

With contextual execution on, registration and promotion are two steps — you register normally, get an id, then hand that id to a lane:

```cpp
int tid = __task_scheduler.register_task([&](){
    uint32_t i = 0;
    while (1) {
        __i_dvc_ctrl.getTerminal()->writeln((uint32_t)i++);
        __i_cooperative_scheduler.sleep(500);  // yields to peers
    }
});
__task_scheduler.scheduleUnderExecSched(
    &__i_cooperative_scheduler,
    tid,
    TASK_MODE_COOPERATIVE,
    1 * 1024                  // per-task stack, in bytes
);
```

Stack sizing is on you unless the port can measure it. A KiB for cooperative and two for preemptive is a reasonable starting point on a constrained device; raise it if the task touches `pdiutil::string`, parses JSON, or goes through TLS.

Across lanes: `sleep(ms)` and `yield()` are called from inside a cooperative task (the inline lane's `sleep` is deliberately a no-op), a preemptive task can yield voluntarily even though the ISR will preempt it anyway, and any data shared between lanes needs `iMutex` or `iConditionVar`. The inline lane needs neither.

## 4.9 Watching and steering it from the shell

`ps` prints every registered task — pid, owner, state (`R` running, `S` sleeping, `T` stopped, `Z` zombie), priority, nice, policy, rolling %CPU, run count, interval and name — and `ps <sid>` filters by owner. `top` re-renders the same view on a scheduler tick and stops on Ctrl+C. `watch` wraps any other command on an interval.

Both read `/proc` rather than reaching into the scheduler, so what a command prints and what a file says cannot drift apart. A build with no filesystem keeps `ps` and `top` by falling back to the scheduler directly, through the same renderer.

Signals mirror POSIX: `HUP=1`, `KILL=9`, `TERM=15`, `CONT=18`, `STOP=19`. They are queued on the task and consumed at the top of the next scheduler pass.

```
  kill    [sig] <pid>     one task, by pid          default TERM
  pkill   [sig] <name>    every task with a name    default TERM
  killall [sig] <name>    same, but default KILL
  renice  <nice> <pid>    -20..19, re-sorts on the next tick
```

Root may signal any task; anyone else only tasks owned by their own session. All of it compiles out when auth is off. Each number is read whole and refused, with the word and the range taken, when it is not one: `invalid pid : 32768 (0 to 32767)`, `unsupported signal : 3`, `invalid nice : 20 (-20 to 19)` — a value is never clamped or wrapped, and the task is left as it was.

Services are driven the same way, one level up:

```
  service list             every service: state, enabled, and task count
  service status <name>    that service's state, config file and the pids it owns
  service start <name>     bring it up from cold
  service stop <name>      real teardown: listener, connections and every tracked task
  service restart <name>   stop, then start
  service enable <name>    persist "<name> yes" in /etc/service.conf
  service disable <name>   persist "<name> no"
```

All of these except `list` and `status` need root. Ownership is tracked per service rather than per task name, so renaming a task never breaks service control. None of the verbs signal a task: `stop` releases what the service holds, so `service stop SSH` gives up port 22 and closes its sessions.

`stop` and `restart` refuse two services: an essential one, and the service carrying the session the command is typed on — otherwise `service stop SSH` over SSH would drop the connection mid-command. The essential set is time, database, serial, auth, user store, factory reset and cmd — a board must not be able to talk itself out of a console, a clock or an identity. `start` and `restart` refuse while a service the target `Requires` is inactive, and name it.

Each service's own start and stop record its state: `active` once its init reports success, `failed` when that init says it did not come up, `inactive` before the first start and after a stop. A service that merely holds a task does not count as running. `enable`/`disable` decide what happens on the next boot and are the pair that survives a restart — see [§3.10](configuration.md#310-the-etc-config-surface).
