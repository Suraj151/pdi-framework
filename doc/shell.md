# 7. Command Line / Terminal

The shell is the universal control plane. The same commands are reachable over serial, over telnet on port 23, and over SSH on port 22, with login, history, tab completion, pipes and redirection, variables, scripting, in-place editing and file transfer. One implementation covers every channel because each source presents itself as an `iTerminalInterface`, and the CLI binds one session per terminal.

Start reading at [src/service_provider/cmd/](../src/service_provider/cmd/); the parser lives in `src/utility/CommandBase.h`.

## 7.1 Layered model

```
   serial port        telnet session        ssh channel
        │                   │                    │
        └───────────────────┴────────────────────┘
                            │  iTerminalInterface*
                            ▼
              CommandLineServiceProvider
                ├─ binds a session per terminal
                ├─ line editing, history, completion
                └─ dispatch
                            │
                            ▼
                    CommandBase registry
                ├─ every command self-registers by name
                ├─ parses  cmd opt=val[,opt=val]  or positional args
                └─ supports held options, multi-tick commands, abort
                            │
                            ▼
                   services, scheduler, filesystem
```

## 7.2 The terminal contract

Anything that wants to feed the CLI implements `iTerminalInterface` — byte-level reads and writes overloaded for every primitive type, plus the terminal affordances. Four implementations ship: the serial port, the client object a TCP server hands back for telnet, the SSH channel wrapper, and `__i_null_terminal`.

The null terminal swallows everything written to it and never has anything to read. It exists because a session is bound to a terminal, so work that must run under a session with nobody watching — a cron job, say — would otherwise have to borrow the console's terminal, and would be refused whenever somebody was logged in on it. Attaching to the sink instead makes such work independent of who is signed in.

When a telnet or SSH client connects, its service calls `useTerminal(client)`, which attaches a fresh session and draws a login prompt on that client alone. Serial holds its slot from boot. Nothing leaks between sessions.

## 7.3 Input sequences

Line editing happens in-process, so the CLI recognises control sequences byte by byte:

| Sequence | Action |
|---|---|
| Enter | submit the line — `\r`, `\n`, or the two in sequence all count as one |
| Backspace, Delete | edit |
| ←, → | move within the line |
| ↑, ↓ | walk history (needs storage) |
| Home, End | jump to line start or end |
| Page Up, Page Down | scroll long output |
| Tab | complete the word under the cursor; again to list the candidates |
| Esc | cancel the line; inside `fedit`, open the save/cancel/delete menu |
| Ctrl+C, Ctrl+Z | abort the running command |

Clients disagree about what Enter is: a raw serial terminal sends `\n`, telnet sends CR LF or CR NUL. Every ending is accepted, and a pair is taken as the single key press it represents rather than as a key press followed by an empty line — the two halves may arrive in separate reads, so which ending was taken is remembered on the session.

Only real characters reach the line. Control bytes the editor has no meaning for, and the stray `0xff` a board can emit as it comes out of reset, are dropped rather than typed — so a line that starts arriving mid-reset is still the line you meant.

A line has no fixed length; it grows on the heap. A key the heap has no room for is not echoed, and the line it was typed into is not run: Enter prints `line too long, not run` and gives a fresh prompt, so a long pasted command is never run with part of it missing.

A long-running command receives these mid-execution by overriding `executeTermInputAction`.

**Completion.** Tab completes the word the cursor is in, and the word is found through the same tokenizer the parser uses, so the rest of the line is left exactly as typed and completion works mid-line as well as at the end. The first press types as much as every candidate agrees on; a single match is finished off and followed by a space. When nothing more can be typed, a second press lists the candidates in columns below and redraws the prompt and the line.

What is offered depends on where the word sits. A word in command position — the start of the line, or after `|`, `&&` or `;` — completes against the command names. A redirect target completes against paths. Anything else is an argument, and the command decides what its arguments name by overriding `completionFor`:

```cpp
/* argument 0 is the subcommand, everything after it names a service */
cmd_complete_t completionFor(uint8_t argindex) const override {
    return 0 == argindex ? CMD_COMPLETE_NONE : CMD_COMPLETE_SERVICE;
}
```

Commands that say nothing complete paths, which suits most of them. A path is completed one component at a time: the word splits at its last `/`, the leading part names the directory to read, and a directory match gains a trailing `/` so the next component can be typed straight away. On a board with no filesystem there is no grammar to read the line with, so completion there offers command names and nothing else.

An empty line completes nothing. Every command would match, and a console has nothing to page a screenful of them with.

## 7.4 The command contract

Every command is a `CommandBase`. The base owns parsing; you write `execute()`.

```cpp
struct TempCommand : public CommandBase {
    TempCommand() {
        Clear();
        SetCommand(CMD_NAME_TEMP);      // name, at most 15 characters
        setArgs(RODT_ATTR(""), 1, true); // the words it takes: no flags, one name=value field
        AddOption(CMD_OPTION_NAME_T);   // up to CMD_OPTION_MAX options, names shorter than CMD_OPTION_SIZE_MAX
    }
    const char* getUsage() const override {
        return RODT_ATTR("temp [t=C|F]  read the temperature sensor (default Celsius)");
    }
    bool needauth() override { return true; }
    cmd_result_t execute(cmd_term_inseq_t) override { … }
};
```

**`CMD_OPTION_MAX` is the most options any command can have**, named `x=y` ones and positional values alike, and every command shares the one figure. A board with memory to spare defines a larger value in its `device_config.h`. It costs no static RAM, since a command's options exist only while that command runs, and a line carrying more positional values than the limit sets `m_argsoverflow` rather than passing the extra ones on, so a command such as `source` can refuse it instead of running with part of what was typed.

**Every command declares the words it takes** with `setArgs(flags, operands, keyvalue)` in its constructor, and the base checks a line against that before `execute()` runs: a `-x` whose letters are not among the declared flags is refused as `invalid option : <word>` (`CMD_ERROR_OPT`), a word past the declared number of operands as `extra operand : <word>` (`CMD_ERROR_INVAL`), and for a command that takes `name=value` fields any word without `=`, or with a name it does not have, as `invalid option : <word>`; nothing runs in any of these cases and the usage line follows. A quoted or escaped run is one word, `""` is a word that is given and empty, and a quoted word starting with `-` is an operand, which is how a file named `-x` is reached. A lone `-` and, where no flags are declared, a negative number are operands too. A command that declares nothing takes any words, as `echo` does. A command with sub-commands (`service`, `db`, `udevadm`, `net`, `iot`) names one it does not have as `unknown sub-command : <word>`. Numbers are read whole with `takeNumber()` (or `ParseUint32`, `ParseInt64`, `ParseOctal`) and refused when they are not one number inside the range, naming the word and the range taken (`invalid pid : 32768 (0 to 32767)`), never wrapped, cut or defaulted, and a failure is written with `writeFailure(prefix, what, code)`, which names what failed and gives the reason in words from `ErrorToText()` — the number only for a code that has no words yet — with an empty name shown as `''`.

`getUsage()` is the single source of truth for that command's help. It is what `help` prints, and what the dispatcher prints automatically whenever the command returns an argument error — so no error branch ever needs to repeat a usage string.

Parsing works like this:

```
   "gpio p=4,m=3,v=500"
      │     └──┬──┘
      │        └─ split on the option separator (',' by default, ' ' for a command
      │           that reads positionally) outside any quotes, then match each key
      │           to a declared option. Never separate on ';' — that is the shell's
      │           own command separator, and a command using it loses every option
      │           after the first
      └─ command name, up to the first space

   "chmod 0644 /etc/passwd"
      └─ a command that opted into positional args gets them in declaration order
```

Inside `execute`, options come back by name:

```cpp
auto pin = RetrieveOption(CMD_OPTION_NAME_P);
if (pin == nullptr) return CMD_ERROR_ARGS_MISSING;
```

A command that cannot finish in one tick — `watch`, `fedit`, an interactive prompt — returns `CMD_ERROR_AGAIN`. The dispatcher keeps the instance alive, counts the iteration, and re-enters `execute` on the next input.

A command that waits for **time** rather than input does the same and never blocks the loop: it reserves the slot it waits on with `reservePositionalSlots`, sets a scheduler timeout that calls `m_cmdexecinterface->wakeCommand(this)` when it is due, and answers `holdsInput()` while it waits so keys typed meanwhile stay for the shell. `keepLineTail`/`takeLineTail` carry the rest of its line until it is done. `sleep` ([SleepCommand.h](../src/service_provider/cmd/commands/SleepCommand.h)) is the model.

One thing to know when you do that: a parsed option value points into the live receive buffer, which is gone by the next keystroke. `holdOptionValue("c")` copies the bytes into storage the option owns, freed when the command clears. Any value that has to survive into the next tick needs it.

## 7.5 Result codes

A command returns a `pdi_err_t`, from the command band declared in [src/utility/pdi_types.h](../src/utility/pdi_types.h). Everything in the framework uses the same type, so a code from storage or the network can travel back out through a command unchanged.

| Result | Meaning | Dispatcher's response |
|---|---|---|
| `PDI_OK` | done | blank line, options cleared |
| `CMD_ERROR_AGAIN` | keep me alive | nothing cleared; wait for more input |
| `CMD_ERROR_INVAL`, `CMD_ERROR_ARGS_MISSING`, `CMD_ERROR_OPT` | bad argument, missing argument, unrecognised option | error line plus the command's usage string |
| `CMD_ERROR_NOENT`, `CMD_ERROR_INVALID` | no such command, or unparsable | error line |
| `CMD_ERROR_PERM`, `CMD_ERROR_ACCES` | login required, or the credential given was wrong | back to the login flow |
| `CMD_ERROR_CANCELED`, `CMD_ERROR_INTR` | Ctrl+C or Ctrl+Z, and terminal abort | error line, iteration stops |
| `CMD_ERROR_FAILED` | the command ran and did not succeed | error line |
| `CMD_RESULT_FALSE` | the command was asked a question and the answer is no | nothing printed; only `$?` carries it |
| `CMD_ERROR_NOTTY`, `CMD_ERROR_HOLD_BUFFER`, `CMD_ERROR_UNSET` | no terminal, the terminal is holding the line buffer, no result recorded yet | handled by the service |

## 7.6 The dispatcher

The service owns the list of in-flight commands and the history file path. Everything else — receive buffer, cursor, history position, completion index, working directory — lives on the session. Each input tick resolves the session from the terminal, makes it current, and then runs the command.

History is persisted only when storage is available, in a file capped at 25 lines. Completion walks the command registry for names matching the typed prefix and cycles on repeated Tab, and works with or without storage because the registry is in RAM.

## 7.7 Pipes and redirection

A command does not know where its output is going, and a command reading input does not know where it came from. That is the whole trick: the shell puts a stream in front of each end, and the command writes and reads as it always did.

```
   ps | grep ssh > /tmp/found.txt

   ┌──────────┐   stdout     ┌──────────┐   stdout     ┌───────────────┐
   │   ps     │ ───────────▶ │  grep    │ ───────────▶ │ /tmp/found.txt│
   └──────────┘   a pipe     └──────────┘   a file     └───────────────┘
                             ▲   stdin
                             └── the pipe the stage before it filled
```

Each session carries a small descriptor table — `stdin`, `stdout`, `stderr` — that normally points at the terminal. The shell repoints an entry for the length of one line and puts it back afterwards, so nothing leaks into the next command:

```
                        fd 0 · stdin        fd 1 · stdout

   plain command        the terminal        the terminal

   cat /etc/passwd  ┬─  cat: the terminal   cat: the pipe
             | wc   ┴─  wc : the pipe       wc : the terminal
```

| Operator | Meaning |
|---|---|
| `>` | send output to a file, replacing it |
| `>>` | send output to a file, appending |
| `\|` | send output to the next command |
| `<` | read input from a file |
| `;` | run the next command whatever this one returned |
| `&&` | run the next command only if this one succeeded |
| `\|\|` | run the next command only if this one failed |

`cat`, `head`, `tail`, `wc`, `grep`, `sort`, `tee` and `read` read the input descriptor when no filename is given, which is what makes them useful on the right of a pipe. Everything else keeps writing to `stdout` without a line of its own changing.

A command ends its lines with a carriage return and a newline for the terminal; when its output goes to a pipe or a file the carriage return is dropped, so what lands there ends lines the linux way. The bytes of a file are not a command's lines: `cat` hands them on exactly as stored, so `cat a.txt > b.txt` is a byte for byte copy whatever line ends `a.txt` uses.

Two limits are worth knowing. A pipe is a fixed buffer of `PDI_PIPE_CAPACITY` bytes (1024 by default), the way a real one is; a stage that outruns it marks the pipe overflowed instead of growing until the heap is gone. The stages run one after another, so nothing drains a full pipe while its writer runs: a command reading a file or a device stops once its pipe is full, the way a linux one stops when the reader closes the pipe, so `hexdump /dev/mmcblk0 | head 2` reads a block or two rather than the whole card. A reader that goes on to the end of a pipe that was cut, as `wc` or `grep` do, answered on part of the output only, so the line ends with `pipe full, output cut at <bytes> bytes`; one that stops early, as `head` does, needed no more and nothing is said. And a write the filesystem refuses — a redirect into read-only `/proc`, or a full disk — is reported as `cannot write <path> : <reason>` (`Read-only file system`, `No space left on device`, `Is a directory`) rather than passing silently, because the answer only arrives when the last block is committed.

`;`, `&&` and `||` join whole pipelines rather than single commands, so `echo counted | wc && echo done` runs the pipeline first and the right-hand side on its result. `$?` carries that result, which is what a following `&&` reads and what `if` and `while` branch on ([§7.15](#715-shell-scripts-and-etcrclocal)). An operator with nothing after it is a syntax error, and one inside quotes is text.

## 7.8 Built-in command inventory

`if`, `elif`, `else`, `fi`, `while`, `for`, `done`, `break`, `continue` and `exit` are not in this table. They are read by the script runner rather than dispatched as commands, so they work in a file and not at the prompt — see [§7.15](#715-shell-scripts-and-etcrclocal).

| Command | Options | Brief |
|---|---|---|
| ls [-l] [-a] [-h] [\<path>] | -l, -a, -h | The names in a directory, and nothing else: on the terminal in columns as wide as the longest name across `LS_LINE_WIDTH` (80), one per line into a pipe or a file. Names starting with `.` are left out unless `-a` is given, which also lists `.` and `..` first. `-l` writes one long row per entry instead — mode, owner, group, size, mtime and name — each column as wide as its widest entry, nothing cut, the size whole up to the largest 64-bit value and right-aligned; `-h` with `-l` shows a size of 1024 bytes or more as `1.5K`, `3.8G` and so on. Flags combine (`-la`, `-lah`); another option is refused with `invalid option`, a second path with `extra operand`. No arg lists the current directory; relative paths join it. A file lists just itself, under the name given. A filesystem mounted in the directory listed shows as a directory of its own name, so `ls /` lists `proc`, `sys`, `dev` and `tmp`, and `find` and `du` cross into them. Owner and group show names, falling back to numbers; the first letter of the mode is `d` for a directory, `c` or `b` for a character or block device and `-` for a file; a path that cannot be listed prints `Failed : <path> : <reason>`. e.g. **ls**, **ls -la /etc**, **ls -lh /mnt** |
| mkdir \<dir> | | Create a directory, `0755` masked by umask, owned by the session. A name already there, directory or file, is refused with `File exists` (`PDI_ERR_EXISTS`), a name longer than the filesystem allows with `File name too long`. e.g. **mkdir /scripts** |
| touch \<file> | | Create empty at `0644` masked by umask, or bump mtime if it exists; a name longer than the filesystem allows is refused as `mkdir` refuses it. e.g. **touch /notes.txt** |
| mv \<src> \<dst> | | Move or rename, across mounts if needed; onto a directory it moves into it under its own name. An existing file or directory is not replaced: `mv: <dst> : File exists`, both left as they were. A path onto itself is refused with `mv: <src> and <dst> are the same file`, a source that is not there with `mv: <src> : Not found`. e.g. **mv /a.txt /b.txt**, **mv /a.txt /logs** |
| cp \<src> \<dst> | | Copy a file, across mounts if needed; onto a directory it copies into it under its own name. An existing file is not replaced: `cp: <dst> : File exists`. A file onto itself is refused with `cp: <src> and <dst> are the same file`, a directory as the source with `Is a directory`, a source that is not there with `Not found`. e.g. **cp /a.txt /b.txt**, **cp /a.txt /backup** |
| pwd | | Print the working directory. |
| rm [-r] \<path> | -r | Remove a file or an empty directory; a directory with anything in it is refused with `Directory not empty` and kept whole, and `-r` removes it with everything in it. Needs write permission. A name that is not there prints `Failed to delete : <path> : No such file or directory`. e.g. **rm /notes.txt**, **rm -r /logs** |
| cat [\<file>] | | Print a file, or the piped input when no file is named; needs read permission. e.g. **cat /proc/uptime** |
| echo \<text> | | Print text. Like every command it can be redirected or piped ([§7.7](#77-pipes-and-redirection)), which is how a file is written from the shell. e.g. **echo 1 > /sys/class/gpio/5/value**, **echo second line >> /notes.txt** |
| fedit \<file> | | Scrolling in-place line editor. A status bar shows the path; ←/→/Home/End/Backspace edit the active line, ↑/↓ move through the file, Enter splits at the cursor. Esc opens the menu: **!w** save, **!c** cancel, **!d** delete line. Edits stream to a temp copy and commit on save. e.g. **fedit /notes.txt** |
| head [\<file>] [N] | | First N lines, default 10, in constant memory; a line of any length is printed whole as it is read. Reads the piped input when no file is named. N is a whole number up to 4294967295; `0` prints nothing, and anything else is refused with `invalid count : <N>`. A directory is refused with `Is a directory`. |
| tail [\<file>] [N] | | Last N lines, default 10, in constant memory; a line of any length is printed whole as it is read. Reads the piped input when no file is named. N is read as `head` reads it. |
| wc [\<file>] | | Lines, words, bytes, of a file or of the piped input. |
| df [-h] | -h | One row per mount: total, used, free, in bytes, or in K, M, G, T, P or E with `-h` (e.g. `3.8G`). Every column is as wide as its widest value, numbers right-aligned under their heading, nothing cut. The mount list comes from `/proc/mounts` and the sizes from the backend behind each prefix. Another option or an operand is refused. |
| mount [-a \| \<device> \<dir>] | positional | The mount table: prefix, type, backend. With a disk or partition node and an existing directory, root mounts the FAT or littlefs volume the node carries there, or prints `mount: <device> : <code>`; `-a` mounts what `/etc/fstab` lists that is not mounted yet. See [§6.2.4](services.md#624-serialserviceprovider--__serial_service). e.g. **mount /dev/mmcblk0p1 /mnt** |
| umount \<dir> | positional | Detach the filesystem mounted at runtime at `dir`, freeing its slot in the mount table; root only, like `mount`, so anyone else is refused with `PDI_ERR_PERM`. The mounts made at boot are refused with `PDI_ERR_PERM` too, a filesystem still holding open files or a session's working directory with `PDI_ERR_BUSY`, and a directory nothing is mounted at with `PDI_ERR_NOT_FOUND`, each as `umount: <dir> : <code>`. A filesystem mounted at runtime counts its open files, which is what keeps one from being detached under them. |
| chmod \<octal> \<path> | | Set permission bits; owner or root. The mode is octal digits up to `0777`; anything else is refused with `chmod: invalid mode : <mode>` and the mode stays. e.g. **chmod 0644 /etc/passwd** |
| chown \<uid>[:\<gid>] \<path> | | Change owner, root only; gid defaults to uid. uid and gid are numbers up to 65535; anything else is refused with `chown: invalid user : <word>` and the owner stays. e.g. **chown 1001 /alice** |
| umask [\<octal>] | | Show or set this session's umask, default `0022`; a mask that is not octal up to `0777` is refused with `umask: invalid mode : <mask>`. |
| hexdump \<file> | | Offset, sixteen hex bytes, ASCII. |
| dd if=\<file> of=\<file> [bs=512] [count=N] [skip=N] [seek=N] | key=value, as linux | Copy `count` blocks of `bs` bytes (1 to `DD_BLOCK_MAX`, 4 KB) from block `skip` of the input to block `seek` of the output, then print the whole and part records read and written and the bytes copied. Disks and other device nodes are written in place at the offset; a regular output file is replaced when no `seek` is given and written in place, not cut short, when one is. A stream node such as `/dev/zero` is read until `count` blocks are in, so it needs `count=`. A file that cannot be read or written ends the copy with `dd: <file> : <code>`. Ctrl+C stops it after the record under way and prints what it copied; `of=/dev/null` reads without keeping anything, to time a read. e.g. **dd if=/dev/mmcblk0 of=/boot.bin count=1** |
| grep \<pattern> [\<path>] | | Search a file or directory tree, or the piped input, printing `path:line:col:content`. Regex subset: `.` `*` `+` `?` `^` `$` `[abc]` `[a-z]` `[^abc]` and escapes. Succeeds when a line matched and answers `CMD_RESULT_FALSE` when none did, so `&&` and `\|\|` can branch on it. A line longer than the heap can hold is matched on the part held and reported as `<path>:<line>: too long, matched on its first <n> bytes`, never answered as no match in silence. An empty pattern (`""`) matches every line; a pattern with a `[` left open or a `\` with nothing after it is refused with `invalid pattern : <pattern>`. e.g. **grep ^ERROR /log.txt**, **ps \| grep ssh** |
| find [\<path>] [-name \<glob>] [-type f\|d] | predicates, as linux | Every entry under the path, the path itself first and each directory before what it holds, printed relative to the path as typed (`.` when none is given). `-name` matches the last part of each path against a glob — `*`, `?`, `[abc]`, `[a-z]`, `[!...]` — and `-type` keeps files or directories; both narrow together. An unknown predicate is refused with `CMD_ERROR_OPT`. e.g. **find /etc -name '*.conf'**, **find . -type d** |
| du [-s] [-b] [-h] [\<path>] | -s, -b, -h | The space each directory takes on its disk, its subdirectories included, children listed before their parent, in KiB: every file counts the blocks its disk stores it in, at least 1 KiB, so a directory never shows less than the rows below it; files on `/proc`, `/sys` and `/dev` take none. `-b` shows the bytes the files hold instead, `-h` a human size, `-s` the total only; the flags combine (`-sh`). The size column is as wide as its widest row. A file prints its own size. e.g. **du -s /var/log**, **du -b /etc**, **du -sh /** |
| stat \<path>... | | Name, size and type, mode in octal and symbolic form, owner and group as number and name, modify and birth time; a time the filesystem does not keep shows `-`. The type reads `regular file`, `directory`, `character special file` or `block special file`. Several paths print one block each, and one that cannot be read prints `Failed : <path> : <reason>` without stopping the rest. e.g. **stat /etc/shadow** |
| sort [-r] [-n] [-u] [\<file>] | -r, -n, -u | Sort the lines of a file or of the piped input by their bytes; `-n` by the leading number, a line with none reading as zero, equal numbers ordered by their text; `-r` reverses; `-u` keeps the first of each run of equal keys. Flags combine, as **-rn**. The lines are held in RAM, so input the heap cannot take is refused with `out of memory` rather than sorted in part. e.g. **sort -n /data.csv**, **ps \| sort** |
| tee [-a] [\<file>...] | -a | Copy the piped input to the output and to each file, replacing them or, with `-a`, appending. A file that cannot be opened or written is reported as `tee: <file> : <reason>` (a directory as `Is a directory`) and the copy to the output carries on. e.g. **cat /proc/meminfo \| tee /mem.txt \| grep Free** |
| diff \<file1> \<file2> | | The lines that differ, in the normal diff format: `NcM`, `NaM` and `NdM` hunks with `<` lines from the first file and `>` lines from the second. Succeeds when the files match and answers `CMD_RESULT_FALSE` when they differ, so `&&` and `\|\|` can branch on it. Each line is compared by a hash and the edit search keeps a trace that grows with the square of the number of edits, so files that differ in very many places are refused with `out of memory`. e.g. **diff /etc/hosts /hosts.bak** |
| cls | | Clear the screen. |
| cd \<dir> | | Change directory; `~` and `-` work. A bare `cd` or `~` goes to the logged in user's home. One that cannot be entered prints `Failed to change directory: <dir> : <reason>` — `Not found`, `Not a directory` or `Permission denied` — and the directory stays as it was. |
| login | u=, p= | Interactive login, or inline with both options. The session starts in the user's home. |
| logout | | End the session — serial returns to the prompt, telnet and SSH close. The working directory and the session's variables reset either way, so the next login starts clean on a terminal that outlives the login. |
| whoami | | The session's username. |
| id | | `uid=N(name) gid=N`. |
| who | | Active sessions across all channels: user, tty, sid, login time, idle. |
| groups | | The current user's primary group. |
| su [\<user> [\<pass>]] | | Switch user in this session, prompting when arguments are omitted. On success the identity, uid, gid and home directory all follow, and the session's environment starts clean. |
| passwd p=\<curr> n=\<new> c=\<confirm> | p, n, c | Change your own password; prompts in three echo-suppressed phases when arguments are omitted. |
| useradd u=\<user> p=\<pass> | u, p | Root only. Next free uid, gid equal to uid, home `/home/<user>`, created `0755` and owned by the new user (`/home` itself is made `0755 root` when missing). Writes both user files. A home already there is kept as it is; one that cannot be made takes the account back out and answers with the filesystem's error. |
| userdel u=\<user> | u | Root only. Removes from both files and deletes `/home/<user>` with everything in it; a home anywhere else is left alone. Refuses root and self. |
| service list \| status \| start \| stop \| restart \| enable \| disable | positional | Service control. `list` shows state, enabled and task count per service; `status <name>` adds tracked pids and that service's own detail, with any password shown as `********`; `start`/`stop`/`restart` bring a service up from cold and tear it down for real; `enable`/`disable` persist `enabled` for the next boot. Root for all but the first two. e.g. **service status MDNS** |
| ps [\<sid>] | | The task table — owner, state, %CPU, run count, interval and name, read from `/proc` — under its column names; optional owner filter, 0 to 254, anything else refused with `invalid sid : <sid>`. The summary line with the free heap is `top`'s; the heap itself is in `/proc/meminfo`. |
| top | i=, n=, u= | The `ps` view on a repeating tick under a summary line of uptime, task count and free heap — interval in ms, 500 or more, iteration bound from 1, owner filter. A value out of range is refused with `invalid interval`, `invalid count` or `invalid sid`. Ctrl+C stops it. |
| kill [\<sig>] \<pid> | | Signal a task: 9 KILL, 15 TERM, 18 CONT, 19 STOP. One argument is a pid, two are signal then pid. A pid that is not a number up to 32767 is refused with `invalid pid : <pid>`, one no task has with `no such pid : <pid>`, any other signal with `unsupported signal : <sig>`. |
| pkill [\<sig>] \<name> | | Same, matched by the whole name across every task carrying it. |
| killall [\<sig>] \<name> | | Same as `pkill`, defaulting to KILL. |
| renice \<nice> \<pid> | | Change nice and re-sort immediately. A nice outside -20..19 is refused with `invalid nice : <nice> (-20 to 19)` and the task keeps its own. |
| sshkgen t=\<algo>[,f=\<dir>] | t, f | Generate an SSH key pair. `t=1` Ed25519, `t=2` or `3` RSA. Without `f` it prompts for the directory — **a]** `~/.ssh` for client keys, **b]** `/etc/ssh` for host keys, the default — and an explicit path also works. Ed25519 is instant; RSA keygen runs into minutes and how many depends on the device. e.g. **sshkgen t=1**, **sshkgen t=2,f=b** |
| net \<option> | ip, scansta, connsta | Network state and control. e.g. **net connsta,\<ssid>,\<password>** |
| host \<name> | | Resolve a name: IP literal, then `/etc/hosts`, then DNS. |
| ping \<host> [count] | | ICMP echo, default four packets and at most ten, streaming each reply and finishing with a loss and rtt summary. |
| wget [\<path>] \<url> | | Download an `http` or `https` url to a file, redrawing a transfer bar as it goes. The path may name a file or a directory; left out, the file lands in the working directory under the name the url ends with. An existing file is replaced. The transfer is refused before anything is fetched when the name is longer than the filesystem allows or the body will not fit in the free space of the filesystem the file goes to, and a failed transfer leaves no partial file. A body is saved only when it arrived whole: one shorter than its `Content-Length`, or a chunked one that ends before its last chunk, fails as `Failed to download` (`HTTP_ERROR_BODY_INCOMPLETE`). `https` needs the TLS service ([§6.2.18](services.md#6218-tls-no-provider-transport-hookup--cert-provisioning)). e.g. **wget https://host/app.bin**, **wget /app.bin https://host/app.bin** |
| date [-u] [-n] [-s \<epoch>] [+\<fmt>] | | Show or set the clock. `-u` for UTC, `+fmt` for a custom format, `-s` to set, `-n` to force an NTP resync. |
| timedatectl | | Clock status: local and universal time, zone, sync state, server. `date` and `timedatectl` are present where the board keeps network time (`DEVICE_SUPPORTS_NTP`); the Mega 2560 has neither. |
| reboot | | Reboot, root only — anyone else is refused with `root required` and the device keeps running, as restarting it is a privileged action. Any word after it is refused and the device keeps running. |
| watch | c=, i=, n= | Run a command repeatedly. Options are comma separated, as every other command's are, so `;` stays the shell's own command separator. e.g. **watch c=net ip,i=3000,n=10**. Quote the inner command when it carries a comma of its own — **watch c="login u=a,p=b",i=3000** — and the quotes bound the value rather than ending it |
| db status \| list \| verify \| save \| restore | positional | Inspect the config record store: which medium is live and how full it is, one line per record, a checksum pass over all of them, and saving or restoring the defaults tier. `restore` is root only. Never prints a record's contents. See [§5.10](database.md#510-from-the-terminal). |
| iot \<option> | setid, getid, sethost, gethost | Device unique id and IoT host. Changing either drops the device key; the next OTP request generates a new one. |
| env | | Every variable this session can see, one `NAME=value` per line, each from the tier that wins. See [§7.14](#714-environment-variables). |
| export \<name>=\<value> | | Set a variable for this session. Names the session answers for itself are refused. e.g. **export GREETING=hello** |
| unset \<name> | | Drop a variable from this session, leaving `/.env` alone. |
| read \<name> [\<name>..] | positional | Take one line of input and put it in variables, so a command's output can be used on the following lines. Needs a pipe or a file to read from. e.g. **date \| read TODAY** then **echo $TODAY**. See [§7.14](#714-environment-variables). |
| source \<file> [args] | | Run each line of a file in this session, so a `cd` or an `export` it performs is still in force afterwards; the arguments are `$1`, `$2` and so on, up to `CMD_OPTION_MAX - 1` of them. A line naming a command there is none of prints `<name>: command not found`, naming that command. See [§7.15](#715-shell-scripts-and-etcrclocal) and its [limits](#script-limits). |
| test \<a> \<op> \<b> | | Answer a question through `$?` and print nothing either way: `=` `!=` on strings, `-eq -ne -lt -le -gt -ge` on numbers, `-z` `-n` on emptiness, `-e` `-f` `-d` on paths. Also written `[ ... ]`. e.g. **test $MODE = debug && echo debugging** |
| sleep \<seconds> | positional | Wait that many whole seconds before the next line runs, the POSIX way: one whole number, 0 up to `SLEEP_MAX_SECONDS` (about 24 days); a fraction, a unit or a word is refused with the range. The session and the script it runs wait; every other session, the portal and the services keep running. What follows it on its line runs after it by its result (`sleep 5 && reboot`), a line typed meanwhile runs once it is done, and ctrl+c ends the wait and drops the rest of its line. Inside a pipe, `$(...)`, a redirect, a cron job or `rc.local` nothing can come back to a waiting session, so there the board itself waits, for at most `SLEEP_DETACHED_MAX_MS` (10 s); a longer wait there is refused with a line in `/var/log/syslog.warning`. e.g. **sleep 2**, **sleep 1 && echo done** |
| which \<name>... | positional | Print each name that is a command and nothing for one that is not; succeeds only when every name is one, so a script can check before it runs something. e.g. **which wget && wget /x.bin http://host/x.bin** |
| type \<name>... | positional | Say for each name `<name> is a shell builtin` or `type: <name>: not found`; succeeds only when every name is a command. |
| basename \<path> [suffix] | positional | The last part of a path, from its text alone, with the suffix taken off when the name is longer than it: `/a/b/c.txt .txt` gives `c`, `/a/b/` gives `b`, `/` gives `/`. e.g. **echo $(basename $1 .log)** |
| dirname \<path> | positional | The path less its last part, from its text alone: `/a/b/c` gives `/a/b`, `a` gives `.`, `/a` and `/` give `/`, and doubled slashes count as one. |
| seq [first [increment]] \<last> | positional | Whole numbers one a line from first (1 when not given) to last, by the increment, down as well as up (`seq 5 -1 1`), in the 32-bit signed range. A run that cannot reach last prints nothing; a zero increment, a fraction or a word is refused, naming it. ctrl+c or a reader that takes no more (`seq 100000 \| head 3`) ends it. |
| logger [-p \<priority>] [--] [message] | -p | Write a line to the system log, tagged with your user name: the words given, or each line of a file or the piped input when none are (`df \| logger -p info`). `-p` takes `emerg`, `alert`, `crit`, `err`, `warning`, `notice` (the default), `info` or `debug`, alone or after a facility (`user.err`), and picks the file: `emerg` to `err` go to `syslog.error`, `warning` to `syslog.warning`, `notice` to `syslog.success`, `info` and `debug` to `syslog.info`. `--` ends the options, for a message that starts with `-`. A line is cut at `SYSLOG_LINE_MAX` (200 bytes). Present with the syslog service. e.g. **logger -p err disk is full** |
| history [n] | positional | The lines typed at any session, numbered from the oldest, or only the last n. The shell keeps the last `CMD_TERMINAL_HISTORY_MAX_LINES` (25) in `/.term_history`, shared by every user, the same lines up-arrow walks. Present with storage. |
| true | | Answer yes and print nothing. e.g. **while true** |
| false | | Answer no and print nothing. |
| crontab | | List the scheduled jobs `/etc/crontab` holds, as the device parsed them, and what is due this minute. See [§7.16](#716-scheduled-jobs-and-etccrontab). |
| udevadm info\|trigger\|monitor | | Devices on the serial interfaces. `info` lists each one with its interface, linux-style name, driver and state, with `manual` after the state of one left out of the scan; `trigger` reads `/etc/devices.conf` again, puts every device left out back in, and looks for devices at once; `monitor` prints `add` and `remove` lines as devices come and go, until Ctrl+C. See [§6.2.4](services.md#624-serialserviceprovider--__serial_service). |
| lsblk | | List the disks bound drivers present and their partitions, with the name of each one's node under `/dev`, its size in bytes, `disk` or `part`, and the filesystem it carries. See [§6.2.4](services.md#624-serialserviceprovider--__serial_service). |
| blkid [node] | positional | Name the filesystem (`TYPE`) and partition table (`PTTYPE`) each disk and partition carries, one line per node that carries either, or only the node given; a node that is not there is refused with `blkid: <node> : Not found`, and one that carries neither answers `CMD_RESULT_FALSE`. See [§6.2.4](services.md#624-serialserviceprovider--__serial_service). |
| wipefs [-a] \<node> | positional | List the filesystem and partition table signatures a disk or partition carries; with `-a`, erase them, one line per signature with the bytes it held. Root only. See [§6.2.4](services.md#624-serialserviceprovider--__serial_service). e.g. **wipefs -a /dev/mmcblk0** |
| sfdisk \<disk> \| -l [disk] \| --delete \<disk> [n] | positional | Write a dos partition table from `start, size, type, bootable` lines on its input, list a disk's table the way `fdisk -l` does, or empty one slot or every slot. Root only to change a table. See [§6.2.4](services.md#624-serialserviceprovider--__serial_service). e.g. **echo ",,c" \| sfdisk /dev/mmcblk0** |
| mkfs -t vfat\|littlefs \<node> | positional | Make a FAT or littlefs filesystem on a disk or partition. Root only. See [§6.2.4](services.md#624-serialserviceprovider--__serial_service). e.g. **mkfs -t vfat /dev/mmcblk0p1** |
| arecord -d \<seconds> \<file> | positional | Record the first microphone bound into a 16 bit mono WAV file, in the background, and say how many bytes it kept. The file is replaced. A recording that will not fit on the filesystem the file goes to — the card for a file under a mounted card — is refused before it starts, `not enough space: needs N bytes, M free`, counting the header and `SOUND_CAPTURE_SPACE_RESERVE` (8 KB) the filesystem needs for itself. Samples are saved to storage every `SOUND_CAPTURE_SYNC_BYTES` (4 KB). Ctrl+C stops it early, and a recording cut short by Ctrl+C, a write that fails or the mic going away keeps what was saved, with the reason and its code ([§6.2.4](services.md#624-serialserviceprovider--__serial_service)). Present where the I2S bus and storage are both built. e.g. **arecord -d 5 /voice.wav** |
| help | | Every registered command with its usage line. Works before login. |
| uptime | | `up Xd Yh Zm Ws`. |
| free [-b\|-k\|-h] | -b, -k, -h | The free memory and the largest block of it the heap can still hand out, under `free` and `largest` on a `Mem:` row, each right-aligned under its name: in KiB, in bytes with `-b`, as `15K`-style sizes with `-h`; the last of them given wins. No port reports a total, so there is no `total` or `used` column. The same figures as `/proc/meminfo`. e.g. **free -h** |
| hostname | | Print the host name, the one in `/etc/hostname`, which mDNS writes from the board's mac when it starts. |
| uname [-a\|-s\|-n\|-r\|-v\|-m] | positional | Print the system name, or with options the system name, host name, release, version and machine, always in that order; `-a` prints all five. e.g. **uname -a** |
| tls q=1,t=,l=,n=,i= | | On-device certificate generation. Present only where the port declares `DEVICE_SUPPORTS_TLS_CERT_GENERATION` and the flag is on; elsewhere certificates are generated off-device and uploaded ([§6.2.18](services.md#6218-tls-no-provider-transport-hookup--cert-provisioning)). e.g. **tls q=1,t=0,l=256,n=device.local,i=192.168.1.50** |
| exec \<path> | | Load a program image from the filesystem and run it as a background task, returning its pid. Present only where the port registers a loader; a board without one simply has no `exec`. See [§7.13](#713-dynamic-app-loading). |

Path arguments behave the POSIX way everywhere: a leading `/` is absolute, anything else resolves against the session's working directory, and `cd` also takes `~` and `-`.

On argument style: commands with at most two arguments take them positionally, which is both shorter to type and what muscle memory expects — `chmod 0644 /etc/passwd`, `renice -5 10`. Where a leading argument is optional, the count disambiguates: one argument is the target, two are signal then target. Named `x=y` options are kept for commands with many optional slots, for anything that takes a password, and for patterns where the optional argument sits in the middle.

**Commands that read differently from linux.** Most commands take what their linux namesake takes. These take named fields or a comma list instead, so muscle memory needs the form on the right:

| You might type | Here it is | Why |
|---|---|---|
| `useradd bob` | `useradd u=bob p=<pass>` | user and password in one line, root only |
| `userdel bob` | `userdel u=bob` | same field as `useradd` |
| `passwd` | `passwd`, or `passwd p=<current> n=<new> c=<new>` | with no fields it prompts with echo off |
| `login bob` | `login`, or `login u=bob,p=<pass>` | with no fields it prompts |
| `top -d 2 -n 10` | `top i=2000,n=10` | interval in ms, count of refreshes, `u=<sid>` for one session |
| `watch -n 2 ps` | `watch c=ps,i=2000,n=5` | the command, the interval in ms, how many runs |
| `nmcli dev wifi connect MyWifi password secret` | `net connsta,MyWifi,secret` | the WiFi command takes a comma list; `net ip` shows the addresses |
| `ssh-keygen -t ed25519` | `sshkgen t=1,f=b` | `t=1` ed25519, `t=2` rsa; `f=b` writes the host key directory |
| `openssl req ...` | `tls q=1,t=0,l=256,n=<name>,i=<ip>` | on-device certificate, where the port supports it |

## 7.9 Multi-terminal session lifecycle

Several sessions run at once with fully independent state. There is a single dispatcher and an array of session slots; each slot holds its own line buffer, cursor, history and completion position, working directory, umask, identity, descriptor table, environment variables and position in any script it is running. The serial console holds one slot from boot, and telnet and SSH each keep a pool of their own, so two people over telnet and four over SSH are all just slots in the same table.

```
  boot ──▶ filesystem up
        ──▶ user store bootstraps /etc/passwd and /etc/shadow if absent
        ──▶ serial terminal attached, login prompt drawn

  telnet connect ──▶ free pool slot? ──▶ accept ──▶ useTerminal(client)
                  │                                  ├─ session attached to the next free slot
                  │                                  └─ login prompt on that client only
                  │  pool full ──▶ short grace, then "no telnet session available"
                  ──▶ each tick: find session by terminal, make current, dispatch
                  ──▶ disconnect: session detached, both slots freed

  ssh connect    ──▶ user auth succeeds ──▶ session attached and marked authorised
                  ──▶ channel opens     ──▶ useTerminal, prompt
                  ──▶ each tick: same session-scoped path
                  ──▶ close: session detached
```

The table and the pools are sized together — see the note in [§6.2.16](services.md#6216-commandlineserviceprovider--__cmd_service). A pool larger than the table it draws from would leave one transport unable to claim the slots it promises, so a device config that names too few sessions fails the build rather than finding out at runtime.

Two invariants carry the whole design. First, the current session is switched once per tick, at the top of input handling, and the static terminal pointer moves with it — so prompt drawing, new command instances and the auth delegators all see the right session without anyone passing it around. Second, every in-flight command records which session created it, and the lookups that find waiting commands filter on that. A telnet login prompt waiting for a username cannot be fed by SSH keystrokes.

Long-running commands capture their terminal and owner when they start, so their output keeps flowing to the right session no matter what the other sessions are doing.

## 7.10 SFTP and SCP file transfer

The SSH service opens an SFTP subsystem on demand, and the same handlers serve `scp -s` for single files, interactive `sftp`, and graphical clients like FileZilla and WinSCP — including editing a remote file in place, which works because the session pool serves the second connection those clients open.

```
scp -s <local-file>  pdiStack@<device-ip>:<remote-path>     # upload
scp -s pdiStack@<device-ip>:<remote-path>  <local-file>     # download
sftp -P 22 pdiStack@<device-ip>                             # interactive
```

| Operation | What you type | Purpose |
|---|---|---|
| REALPATH | `pwd`, `cd` | resolve `.`, `..` and relative paths |
| STAT, LSTAT, FSTAT | `ls`, `stat` | size, type, attributes |
| OPENDIR, READDIR, CLOSE | `ls` | directory listing, 16 entries per response |
| OPEN, READ, WRITE, CLOSE | `get`, `put` | file transfer |
| MKDIR, RMDIR | `mkdir`, `rmdir` | directories |
| REMOVE | `rm` | delete |
| RENAME | `rename` | rename without overwriting |

A file stays open on its handle from OPEN to CLOSE, so each READ and WRITE seeks to its offset rather than opening the file again; a file a session leaves open when the client drops without closing it is closed when the session ends. A READ answers with as much of what it asked for as `SSH_SFTP_READ_MAX` allows (2 KB by default, 16 KB on boards with the RAM) and the client's largest packet holds. When the heap is short the reply is cut to what it can spare, but not below `SSH_SFTP_READ_MIN` (256 bytes), and the client asks for the rest in its next READ. A reply the heap leaves no room to frame for the channel, because another session or the portal took the memory meanwhile, is kept and sent on a later turn before any further request is taken; only when the heap has no room for it for `SSH_TX_STALL_MS` is the session closed, with `SSH: session closed, the heap had no room for an sftp reply for <ms> ms`. A channel tells the client the largest packet it takes, `SSH_RX_PACKET_MAX` from the board's config (4 KB by default and on boards with little RAM, 16 KB on boards with more, 32 KB on a host); every packet that arrives is put together across the service's turns, `SSH_RX_CHUNK` (512 bytes) at a time, so a packet still on its way never holds the other sessions up, and its MAC is checked before any byte of it is used. A packet larger than that plus `SSH_RX_PACKET_SLACK` (1 KB, for the other messages a client sends) closes the session with its reason in the system log. The server also offers the `limits@openssh.com` extension, so an OpenSSH `scp` or `sftp` sizes its reads to `SSH_SFTP_READ_MAX` and its writes to fit one packet. A write's data is written to the file as it arrives, whatever the packet boundaries and however many requests the client has in flight, so an upload never needs more than one packet in memory and a transfer never needs a whole-file buffer; each write is answered with its own status once all its bytes are stored. The channel window a client may fill is `SSH_CHANNEL_WINDOW` (2 MB), topped up whenever half of it is used, so an upload of any size keeps going. A packet that reaches the board in several pieces is waited for until it is whole; only when no more of it arrives for `SSH_RX_STALL_MS` (30 seconds) is the connection closed, so a WiFi link that stalls for a few seconds slows a transfer without ending it. A packet the heap cannot hold whole is never taken in part: the reader keeps its first bytes and tries again on the next turns, the rest waiting in the connection, and only a heap that stays short for `SSH_RX_STALL_MS` closes the session, with the reason in the system log, `SSH: session closed, a packet could not be read: <code>`; output the queue has no room for waits the same way for the queue to drain, so a moment of memory pressure from another session or the portal slows a transfer rather than ending it. Output is queued per session and written as fast as the connection takes it; while a reply is still going out, no further sftp request is taken, so a slow link holds the client back through TCP instead of piling replies up, and a session holds at most `SSH_TX_QUEUE_MAX` of output (8 KB by default, 4 KB and 32 KB on the boards) before its producer waits. File data never goes past the window the client has given (`SSH_MSG_CHANNEL_WINDOW_ADJUST` tops it up). Output the heap leaves no room for is kept and tried again every `SSH_TX_RETRY_MS` (20 ms); output that cannot go for `SSH_TX_STALL_MS` (30 seconds), or that the heap cannot hold at all, closes the session with `SSH: session closed, <n> bytes of output could not be sent` (or `held`, or `queued`), so a session never shows output with a part missing. Keepalive and other global requests are answered with `SSH_MSG_REQUEST_FAILURE`, as the protocol asks, and a message the server does not know is answered with `SSH_MSG_UNIMPLEMENTED`, so a client that checks the connection with keepalives keeps it open. A login the heap cannot give a session to is refused with `SSH: session closed, the heap cannot hold a new session`. A login whose key exchange the heap cannot build is refused the same way, `SSH: session closed, the key exchange reply could not be built`. All of these are set in [src/config/SshConfig.h](../src/config/SshConfig.h) and can be overridden per board in its device config. Over WiFi a file moves to or from an SD card at about 35 to 58 KB/s, and an upload at about 52 KB/s; firmware still goes over OTA, which writes the image straight into the update partition.

Directory listings are read once when the directory opens and paginated across responses, released when it closes. One handle is tracked per session, which is what an interactive client uses. Idle SFTP sessions are reaped after a minute so a suspended client cannot hold a pool slot; interactive shell sessions are never idle-reaped, because a person may sit at a prompt for as long as they like.

### 7.10.1 SSH authentication

Password and public key are both accepted, and `/etc/ssh/ssh.conf` decides which are offered:

```
# /etc/ssh/ssh.conf
PasswordAuthentication yes
PubkeyAuthentication   yes
```

Passwords are checked against `/etc/shadow` — the same credentials as serial and telnet. Public keys are Ed25519 or 2048-bit RSA, listed one per line in the device's `~/.ssh/authorized_keys` in the standard OpenSSH format, which is exactly what you copy out of your own `id_ed25519.pub`:

```
fedit ~/.ssh/authorized_keys        # paste the line, then !w
```

Set either option to `no` to switch that method off — `yes`/`on`/`1` and `no`/`off`/`0` all read the way you would expect, and a word that means neither leaves the option at its default rather than silently turning it off. Whether the SSH service starts at all is a separate question, answered by `/etc/service.conf` ([§3.10](configuration.md#310-the-etc-config-surface)). With passwords off, only a holder of an authorised private key gets in:

```
ssh -i ~/.ssh/id_ed25519 pdiStack@<device-ip>
```

The device's own host keys are separate, in `/etc/ssh`. Ed25519 lives in `/etc/ssh/ed25519` with its `.pub` and `.seed`, and is created automatically the first time the SSH service starts. RSA lives in `/etc/ssh/rsa` and is generated only when you ask for it with `sshkgen t=2,f=b`, since 2048-bit keygen costs minutes and how many depends on the device. Either way the key goes onto the wire in standard SSH format during the handshake. `authorized_keys` is one file under the device home directory, shared by all users.

## 7.11 Background commands and Ctrl+C

`watch` and `top` run as scheduler tasks behind a shell session, and they are the template for any long-running command:

```
  1  parse arguments, holdOptionValue() anything that must outlive this tick
  2  register a scheduler task under the command's name, so it shows up in ps
  3  set m_runinbackground and return INCOMPLETE, so the instance stays alive
  4  override stopRunningInBackground() to remove that task and clear the flag
```

Ctrl+C handling is generic: the dispatcher walks its command list and calls `stopRunningInBackground()` on every backgrounded command owned by the current session. A new background command inherits that behaviour with no wiring in the shell. `ps` shows what is currently running, under the command's own name.

A background command belongs to the session that started it, the way a linux job belongs to its login: each run of its task goes through `runAsOwner(this, …)`, which makes that session current and its terminal the one the shell prints on, then puts back what was there. So `watch` runs its command as the user who started it, with their access and working directory, and shows it only on their terminal, whoever typed last. When the session ends its background commands go with it.

A command that works through a long file or device in one go — `cat`, `head`, `tail`, `grep`, `wc`, `hexdump`, `dd`, `cp`, `find`, `du`, `sort`, `diff` — never returns to the shell until it is done, so it asks `commandStopped()` between chunks instead. That looks at the session's own terminal at most every `CMD_INTERRUPT_POLL_MS` (50 ms): a Ctrl+C there ends the command with `CMD_ERROR_CANCELED` (`dd` still prints the records it copied, and `cp` removes its partial copy), and any other key typed meanwhile is kept, up to `CMD_TYPEAHEAD_MAX` (128), for the shell to read once the command is done. Ctrl+C drops the keys typed ahead of it, as linux does. A new long command gets the same by checking `commandStopped(m_terminal)` in its read loop and returning `stoppedResult(result)`.

## 7.12 Adding a command

Say you want `temp`.

1. Add the name — `#define CMD_NAME_TEMP "temp"` — keeping it shorter than `CMD_SIZE_MAX` (15 characters).
2. Write the command:
   ```cpp
   struct TempCommand : public CommandBase {
       TempCommand() {
           Clear();
           SetCommand(CMD_NAME_TEMP);
           setArgs(RODT_ATTR(""), 1, true);
           AddOption(CMD_OPTION_NAME_T);
       }
       static void RegisterCommand() {
           CommandBase::RegisterCommand(CMD_NAME_TEMP, [](void *arg)->void *{
               return pdiutil::safe_new<TempCommand>();
           });
       }
       const char* getUsage() const override {
           return RODT_ATTR("temp [t=C|F]  read the temperature sensor (default Celsius)");
       }
       bool needauth() override { return true; }
       pdi_err_t execute(cmd_term_inseq_t terminputaction) {
           CommandOption *unit = RetrieveOption(CMD_OPTION_NAME_T);
           bool celsius = !(unit && unit->optionval && unit->optionval[0] == 'F');
           m_terminal->writeln(readSensor(celsius));
           return PDI_OK;
       }
   };
   ```
3. Include the header in the CLI service and call `TempCommand::RegisterCommand()` in its constructor, next to its siblings.

That is the whole job. Tab completion, history, help, argument-error usage printing and Ctrl+C all come from the base. `setArgs` declares what the command takes — here no flags, one `name=value` field — so the base refuses an unknown option, a bare word or a second field with the word named and the usage line before `execute` runs (the contract is in [§7.4](#74-the-command-contract)); a command that declares nothing takes any words, as `echo` does.

**Two argument shapes.** `AddOption` declares named `x=y` slots, as above. A command that takes its arguments positionally calls `setAcceptArgsOptions(true)` instead and reads `m_options[0]`, `[1]` and so on; `setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE)` makes those the space-separated words, which is how `su <user> <pass>` and `test <a> = <b>` read theirs. ⚠ Without `setAcceptArgsOptions(true)` a positional argument is never captured and every slot looks empty.

**A command that answers a question** returns `CMD_RESULT_FALSE` for "no" rather than an error code. The dispatcher prints nothing for it, so a false result can sit inside a script or an `&&` chain without an error line, while `$?` still carries it — `test` is the one that does this today ([§7.15](#715-shell-scripts-and-etcrclocal)).

A few limits shape command design. Names are capped at `CMD_SIZE_MAX` less one (15 characters) and options at `CMD_OPTION_MAX`, each option name shorter than `CMD_OPTION_SIZE_MAX`, so a verb that wants more either splits into sub-commands or takes positional arguments. Options are comma separated, and a value that needs to carry a comma, an `=` or a space is **quoted** — single or double, as a shell quotes — so `watch c="login u=a,p=b",i=3000` passes the whole inner command through as one value. The quotes bound the value and are removed before the command sees it. And `needauth()` is the only permission gate, so put it on anything that changes state.

Parsing stays index-based rather than building an argv: the parser hands each command the offsets and lengths of its options within the line it already holds, and quote removal compacts in place inside that span. On a device measured in kilobytes a second copy of every argument is a cost with nothing to show for it, so the index model is the standard here and argv is deliberately not built.

## 7.13 Dynamic app loading

With `ENABLE_PROGRAM_EXEC`, `exec` reads a program image off the filesystem, hands it to whatever loader the port registered, and launches it as a background preemptive task:

```
exec /apps/hello.app.elf
```

The command knows nothing about image formats. A port declares `DEVICE_SUPPORTS_PROGRAM_EXEC` and registers an `iProgramLoaderInterface`, which owns what an image is and how it is relocated — so a board with a different architecture needs no change on the command side, and a board with no loader simply has no `exec`. The loader that exists today works over a relocatable ELF image.

The command returns to the prompt immediately with a pid. The app runs concurrently and its output arrives asynchronously; `ps` lists it and `kill <pid>` ends it. It also ends when `main()` returns, and the image is freed on either path. Because it is a task rather than a scheduler entry, stop and continue don't apply to it.

The path goes through the normal VFS, so apps arrive by SFTP or HTTP upload like any other file. The feature needs storage and brings contextual execution with it.

The program must be a small position-independent ELF built against the loader — not a firmware image — calling only symbols the firmware exports; the common libc entries are already there. How you build one depends on the SDK behind the port, and the loader ships with a template project for it:

```bash
# with the port's SDK environment active
<sdk> create-project-from-example "<loader template>"
cd <project>                     # edit main.c, keeping int main(int argc, char *argv[])
<sdk> set-target <board>         # must match the loader's own target
<sdk> elf                        # → build/<name>.app.elf
```

Two lines in the project's build file are what make the output loadable rather than a normal binary: `include(elf_loader)` and `project_elf(<name>)`. Upload the result and run it. A port without a loader simply has no `exec` command, and nothing else changes.

## 7.14 Environment variables

A variable can come from three places, and a lookup takes the first one that carries it:

```
  $NAME
    │
    ├─ 1. derived     PWD, USER, UID, HOSTNAME — answered from the session itself
    ├─ 2. session     set with export, gone when the session ends
    └─ 3. base file   /.env, shared by every session and kept across reboots
```

`env` prints all three merged, `export NAME=value` sets one for this session, and `unset NAME` drops it again. A session variable shadows the base file, so dropping it uncovers the file's value rather than deleting it. The base file is edited like any other config — `fedit /.env` — and is created with `HOME` on first boot.

**The first tier is resolved, never stored.** `PWD`, `USER`, `UID` and `HOSTNAME` are facts the session already knows, so they are read at lookup time and assigning them is refused:

```
pdiStack@12580169:(/): export PWD=/nope
the session answers for that name, it cannot be set
```

That is what keeps them honest — a stored `PWD` would be wrong the moment anyone ran `cd`, and two sources of truth for one fact is the thing worth avoiding, not the extra lookup.

The shell expands `$NAME` wherever it expands `$?`, with the same quoting rules: a single-quoted run is literal, a double-quoted one still expands, and a name the environment does not carry expands to nothing rather than erroring.

```
pdiStack@12580169:(/): export GREETING=hello
pdiStack@12580169:(/): echo $USER at $HOSTNAME
pdiStack at pdi-bff549
pdiStack@12580169:(/): echo pre$GREETING-post
prehello-post
pdiStack@12580169:(/): echo '$GREETING'
$GREETING
```

Braces mark where a name ends and take two forms of their own:

| Form | Expands to |
|---|---|
| `${NAME}` | the value, so it can run straight into text: `${N}th` |
| `${NAME:-word}` | the value, or `word` when the name is unset or empty; `word` may hold blanks, `$NAME` and `$(...)` |
| `${#NAME}` | the length of the value, `0` for an unset name |

A `${` that is never closed, or one holding anything else, is left as typed.

`$(( ... ))` works out whole-number arithmetic and puts the result in its place: `+ - * / %`, unary signs and parentheses, with the usual precedence. A name can be written bare or with its `$`, and one that is unset or empty counts as `0`:

```
pdiStack@12580169:(/): export N=5
pdiStack@12580169:(/): echo $((N * 2 + 1)) $(( (N + 1) / 4 ))
11 1
```

Numbers are 32-bit and wrap on overflow. Anything it cannot work out — division by zero, a name holding a word, an operator it does not know — leaves the text as typed, so the mistake is visible in the output rather than turned into a number. Parentheses nest to `SHELL_ARITH_DEPTH` (8).

**A command's answer can go into a variable.** `read` takes one line from wherever the input descriptor points — the left of a pipe, or a file — and binds it to the names given:

```
pdiStack@12580169:(/): date | read TODAY
pdiStack@12580169:(/): echo backed up on $TODAY
backed up on Sat Sep 27 11:04:18 2026
pdiStack@12580169:(/): read HOSTLINE < /etc/hostname
```

The line arrives as text and nothing else — the blank line a command opens with and the carriage return it ends with are both gone before the bytes leave the pipe, so the value runs together with whatever is printed around it.

Give it more than one name and the line is split on spaces, a field each, with the last name taking whatever is left: `echo one two three | read A B` leaves `A` as `one` and `B` as `two three`. It takes as many names as a command takes options, `CMD_OPTION_MAX` (see [Script limits](#script-limits)), and a name the line has nothing left for is set to empty.

Two things it will not do. It needs something feeding it, so `read NAME` on its own says so rather than waiting for a line to be typed at it. And when the input has already run out it leaves the names as they were and answers no through `$?`, so a script can tell the end of the input from a failure.

**A command's answer can also stand in the line itself.** `$(...)` runs what is inside the parentheses as a line of its own — pipes, `;`, `&&` and further `$(...)` included — and puts what it printed in its place, with the closing line break dropped:

```
pdiStack@12580169:(/): echo backed up on $(date)
backed up on Sat Sep 27 11:04:18 2026
pdiStack@12580169:(/): export STAMP="$(date)"
pdiStack@12580169:(/): echo hi > /tmp/log-$(cat /etc/hostname).txt
```

Quoting works as it does for `$NAME`: inside double quotes the answer stays one value however many blanks it holds, and inside single quotes nothing runs. The answer goes in as text after the line has been read, so it can never become part of the line — an answer holding `; reboot` is printed, not run.

Each substitution runs once, just before the pipeline holding it, so `echo a > f ; echo $(cat f)` reads the file the first half wrote. An answer is bounded by `SHELL_CAPTURE_MAX` (256 bytes); a longer one fails the line rather than being cut short. A substitution may open inside another to `SHELL_CAPTURE_DEPTH` (2) levels. A command inside one that stops to ask for input is cancelled rather than left waiting. Everything a substitution uses is taken while the line runs and released after it. A `for` list does not run substitutions yet: `for X in $(...)` walks the literal words.

A name is a letter or underscore followed by letters, digits or underscores. A session holds `ENV_SESSION_MAX` variables, eight by default; names are bounded by `ENV_NAME_MAX`, and `read` stops a value at `ENV_VALUE_MAX` so a long line cannot become a long variable. The base file needs storage, so a board without a filesystem keeps the first two tiers and simply has none.

The session tier ends with the login. `logout` and `su` both clear it, which matters most on the serial console: telnet and SSH close their channel and release the session with it, while the serial terminal outlives every login on it, so without an explicit reset one user's variables would follow the next one in. Only the session tier is dropped — the base file is untouched, so clearing uncovers `/.env` rather than deleting from it.

## 7.15 Shell scripts and `/etc/rc.local`

A script is a file of shell lines, and there is no second grammar. Each line goes through the same executor a typed line goes through, so pipes, redirection, `&&` and `||`, quoting, `$NAME` and `$?` all mean inside a script exactly what they mean at the prompt. The runner reads only a few things for itself: a blank line, a `#` comment, and the control-flow keywords described below.

```
pdiStack@12580169:(/): source /demo.sh
```

**`source` runs in the session that called it**, not in a subshell — the framework has none. A `cd` or an `export` a script performs is still in force when it returns, which is the whole reason the POSIX name fits:

```
# /setup.sh
export MODE=debug
cd /var/log
```
```
pdiStack@12580169:(/): source /setup.sh
pdiStack@12580169:(/var/log): echo $MODE
debug
```

A script that fails a line keeps going, the way `rc.local` does; use `&&` for a line that must gate the next. Scripts can call scripts, bounded by `SCRIPT_MAX_DEPTH`, so a file that sources itself stops rather than recursing until the heap gives out.

### Arguments

Words after the file name are the script's arguments. `$1` to `$9` read them, `$#` counts them and `$@` gives all of them separated by blanks:

```
# /blink.sh
echo pin $1 blinks $2 times from $# arguments
```
```
pdiStack@12580169:(/): source /blink.sh 13 5
pin 13 blinks 5 times from 2 arguments
```

An argument the script was not given is empty, and outside a script `$#` is `0`. Quoting decides what one argument is, so `source /notify.sh "door open"` is one. A value goes in as text and is never read as shell again, so an argument holding `;` or `$(...)` cannot run anything. A `source` with no arguments of its own sees its caller's; one with arguments has its own until it returns.

**The number of arguments is a hard limit**: `source` takes its file and arguments as command options, so it accepts `CMD_OPTION_MAX - 1` arguments and refuses a line with more with `too many arguments` rather than dropping the extra ones. A board with memory to spare defines a larger `CMD_OPTION_MAX` in its `device_config.h`, so check your port's file for the figure your board runs with. The value sets how many options every command takes, not only `source`. `$1` to `$9` reach the first nine; with a limit above that, the rest are reachable only through `$@`. Every other limit a script meets is in [Script limits](#script-limits).

### Branching and repeating

`if`/`elif`/`else`/`fi`, `while`/`done` and `for`/`done` are handled by the runner as a line-level state machine, not by the line parser — control flow spans lines, and the parser deliberately understands one line at a time. There is no `then` and no `do`: the runner is line based, so a keyword on a line of its own would be noise.

A close has to match what it closes. `fi` ends an `if`, `done` ends a `while` or a `for`, and a mismatched or stray one fails the script rather than quietly ending a block it does not belong to — a `for` closed by `fi` would otherwise run a single pass and read as a loop that simply had one word in its list.

`if` and `while` branch on **a command's result**, which is what `$?` already carries for `&&`:

```
# /boot-checks.sh
if test -f /etc/wifi/wifi.conf
echo wifi settings present
else
echo running on stored defaults
fi

export TRIES=a
while test $TRIES != c
echo attempt
if test $TRIES = a
export TRIES=b
else
export TRIES=c
fi
done
```

`elif <command>` tries another condition when none before it held, and `else` runs when none did. A counter is plain arithmetic, so a loop can count its passes directly:

```
# /retry.sh
export TRY=0
while [ $TRY -lt 3 ]
export TRY=$((TRY + 1))
if wget /fw.bin http://10.0.0.5/fw.bin
break
elif [ $TRY = 2 ]
echo last attempt next
fi
done
```

`break` leaves the innermost `while` or `for`, and `continue` skips the rest of its body and starts the next pass; either one outside a loop fails the script. `exit [n]` ends the script it is in with status `n`, or with the last command's status when no number is given, and a script that sourced it carries on from the line after its `source`. `exit` at the prompt is not a command.

`!` in front of a pipeline turns its answer around, at the prompt as well as in a condition: `if ! test -f /etc/site.sh`. `true` and `false` answer yes and no and print nothing, so `while true` loops until a `break`. `[ ... ]` is `test` written with brackets — `[ $TRY -lt 3 ]` is `test $TRY -lt 3` — and an opening bracket with no closing one is refused.

`test` is what makes that useful. Every other command reports success or a *fault* — `grep` returns success whether or not it matched — so without `test` a condition could only ask whether a command malfunctioned, never what a value is. It answers through `$?` and prints nothing either way, so a false condition inside a loop leaves no output:

| Form | True when |
|---|---|
| `test <a> = <b>`, `test <a> != <b>` | the strings match, or differ |
| `test <a> -eq -ne -lt -le -gt -ge <b>` | the numbers compare so; a non-number is refused rather than answered |
| `test -z <s>`, `test -n <s>` | the string is empty, or is not |
| `test -e <path>`, `-f`, `-d` | the path exists, is a plain file, or is a directory |

`for` walks a list of words instead of a condition, binding each one to a name in turn:

```
# /rotate.sh
for LEVEL in error warning info
echo clearing $LEVEL
rm /var/log/syslog.$LEVEL
done
```

The list is words, and quoting decides what a word is: `for X in "one two" three` is two passes, while an unquoted `$NAME` holding blanks is as many words as it reads as. A name the environment does not carry expands to nothing and contributes no word, so a list that goes empty simply runs the body no times. The name is bound for real, so it is still set to the last word after `done`; `for` refuses a name the session answers for itself, such as `PWD`.

A loop is capped at `SCRIPT_LOOP_MAX` passes, so a condition that never goes false fails the script instead of holding the session — at boot there is nobody to interrupt one. A `for` over a fixed list ends itself, but the cap still applies, because the list is read again on each pass and a body that adds to its own list would otherwise never finish. Nesting is bounded by `SCRIPT_BLOCK_MAX`, and a block left unclosed at the end of a file is reported rather than passing quietly.

### A line that asks for something

A command inside a script can stop for input — `su` and `passwd` both do. Where the script was typed by someone, the terminal is handed back so they can answer, and the script carries on at the next line afterwards. Where nobody is watching it, waiting would hold the session forever and the line after would be read as the answer, so the waiting command is cancelled and the script stops. Boot and scheduled scripts take the second path, which is what keeps `rc.local` from wedging a boot.

Where a script has got to lives on the session, so two transports can each be part-way through one without touching the other, and a script pending when someone logs out or runs `su` is abandoned rather than resumed under the next login.

### `/etc/rc.local`

If `/etc/rc.local` exists it runs at the end of boot, after every service has started and before the login prompt appears. It is not created for you.

It names the identity it runs under, in its **leading comment block**:

```
# rc.local - runs at the end of boot
# UID 0
echo bringing up the workshop config
source /etc/site.sh
```

| The header says | What happens |
|---|---|
| no `UID` line | runs as root |
| `# UID <n>` and the user exists | runs as that user, with their home as the working directory |
| `# UID <n>` and no such user | **the file does not run at all** |

An identity that cannot be resolved is never quietly downgraded to root — a script asking to run as somebody specific and silently getting more privilege than it asked for is the wrong failure. A `UID` line below the first command is documentation, not policy, so a comment further down cannot change who the script runs as.

The session `rc.local` runs in is built for it and given back afterwards. That is deliberate: reusing the console's session would leave a logged-in root shell sitting on the serial port at boot. It also means `export` inside `rc.local` does not reach your login — put anything that must outlive it in `/.env`.

### Example scripts

A script gets onto the board in one of three ways: typed at the prompt a line at a time with `echo`, written in `fedit`, or copied over with `sftp`/`scp` (see [§7.10](#710-sftp-and-scp-file-transfer)). Typed lines go in single quotes, so a `$` reaches the file as written instead of expanding while you type; `>` starts the file and `>>` adds each line after it. Every example below runs as shown with `source`.

**1. A status report, typed in at the prompt.**

```
pdiStack@12580169:(/): echo 'echo == $(cat /etc/hostname) at $(date) ==' > /health.sh
pdiStack@12580169:(/): echo 'uptime' >> /health.sh
pdiStack@12580169:(/): echo 'df' >> /health.sh
pdiStack@12580169:(/): echo 'service list' >> /health.sh
pdiStack@12580169:(/): cat /health.sh
echo == $(cat /etc/hostname) at $(date) ==
uptime
df
service list
pdiStack@12580169:(/): source /health.sh
```

**2. Repeat something a given number of times**, with arguments, a `while` and a counter:

```
# /repeat.sh — source /repeat.sh <times> <text>
export N=0
while [ $N -lt $1 ]
export N=$((N + 1))
echo $N: $2
done
unset N
```
```
pdiStack@12580169:(/): source /repeat.sh 3 hello
1: hello

2: hello

3: hello
pdiStack@12580169:(/): source /repeat.sh 2 "two words"
1: two words

2: two words
```

Each `echo` opens with a line break of its own, which is why lines printed one after another by a script come out a blank line apart.

**3. Back up a file before editing it**, refusing a path that is not there. `exit 1` makes the failure visible to whoever ran it, through `$?`:

```
# /backup.sh — source /backup.sh <file>
if ! test -f $1
echo no such file $1
exit 1
fi
cp $1 $1.bak
echo saved $1.bak
```
```
pdiStack@12580169:(/): source /backup.sh /etc/mqtt/mqtt.conf
saved /etc/mqtt/mqtt.conf.bak
pdiStack@12580169:(/): source /backup.sh /nothere
no such file /nothere

CmdErr : 1
```

**4. One script, several actions**: check the argument count first, then pick the action with `elif`:

```
# /svc.sh — source /svc.sh up|down <service>
if [ $# -ne 2 ]
echo usage: source /svc.sh up or down and a service
exit 2
elif [ $1 = up ]
service start $2
elif [ $1 = down ]
service stop $2
else
echo unknown action $1
fi
```
```
pdiStack@12580169:(/): source /svc.sh down OTA
stopped OTA, released 1 task(s)
pdiStack@12580169:(/): source /svc.sh sideways OTA
unknown action sideways
```

**5. Report which services are running**: a `for` over the names, with a command's answer as the condition:

```
# /states.sh
for S in MQTT OTA Email
if [ -n "$(service status $S | grep inactive)" ]
echo $S is stopped
else
echo $S is running
fi
done
```
```
pdiStack@12580169:(/): source /states.sh
MQTT is running

OTA is running

Email is stopped
```

**6. Bring a stopped service back, and note that it happened.** On its own it does nothing while the service runs; give it a line in `/etc/crontab` and it checks every ten minutes (see [§7.16](#716-scheduled-jobs-and-etccrontab)):

```
# /keepalive.sh — source /keepalive.sh <service>
if [ -n "$(service status $1 | grep inactive)" ]
service start $1
echo $(date) restarted $1 >> /var/log/keepalive.log
fi
```
```
# /etc/crontab
*/10 * * * * source /keepalive.sh MQTT
```

**7. Download with retries**, stopping at the first one that works:

```
# /fetch.sh — source /fetch.sh <path> <url>
export TRY=0
while [ $TRY -lt 3 ]
export TRY=$((TRY + 1))
if wget $1 $2
echo fetched $1 on attempt $TRY
break
fi
done
unset TRY
```
```
pdiStack@12580169:(/): source /fetch.sh /tmp/fw.bin http://10.0.0.5/fw.bin
```

Each attempt prints `wget`'s own progress and result; the last line is `fetched /tmp/fw.bin on attempt 1`, or nothing more when all three failed.

**8. Count boots**, keeping the count in a file between them. Run it from `/etc/rc.local` and every boot adds one:

```
# /bootcount.sh
export COUNT=0
if test -f /etc/bootcount
read COUNT < /etc/bootcount
fi
export COUNT=$((COUNT + 1))
echo $COUNT > /etc/bootcount
echo boot number $COUNT
unset COUNT
```

**9. Keep a log from growing**: count its lines with `wc`, take the first field with `read`, and move the file aside past a limit:

```
# /rotate.sh — source /rotate.sh <file> <lines>
wc $1 | read LINES REST
if [ $LINES -gt $2 ]
mv $1 $1.old
echo rotated $1 at $LINES lines
fi
unset LINES
unset REST
```
```
pdiStack@12580169:(/): source /rotate.sh /var/log/keepalive.log 100
```

**10. A one-line log entry**, the kind a cron job writes:

```
# /stamp.sh
echo $(date) $(uptime) >> /var/log/uptime.log
```
```
# /etc/crontab
0 * * * * source /stamp.sh
```

**11. Boot-time setup**, putting the scripts above together in `/etc/rc.local`:

```
# rc.local - runs at the end of boot
# UID 0
source /bootcount.sh
source /keepalive.sh MQTT
source /stamp.sh
```

**12. Lines worth typing straight at the prompt**, no file needed:

```
pdiStack@12580169:(/): [ -f /etc/rc.local ] && echo rc.local present || echo no rc.local
no rc.local
pdiStack@12580169:(/): test -d /etc && echo /etc is a directory
/etc is a directory
pdiStack@12580169:(/): echo a day is $((60 * 60 * 24)) seconds
a day is 86400 seconds
pdiStack@12580169:(/): echo ${#HOSTNAME} characters in $HOSTNAME
10 characters in pdi-abcdef
pdiStack@12580169:(/): echo running as ${USER:-nobody} in $PWD
running as pdiStack in /
pdiStack@12580169:(/): export STAMP="$(date)"
```

`if`, `while` and `for` belong to scripts; at the prompt the same decisions are made with `&&`, `||` and `[ ... ]`. A variable a script sets stays in the session after it returns, which is what lets `/setup.sh` above work, and it holds one of the session's `ENV_SESSION_MAX` slots. A script that only needs a value while it runs gives the slot back with `unset` at the end, as several of the examples do.

### Script limits

Every limit a script can reach, in one place. Each one is a hard limit: reaching it refuses or fails, it never quietly cuts something short. The figures are the defaults; a port can define any of them in its `device_config.h`, and some ports ship larger ones, so your port's file has the figures your board runs with.

| Limit | Default | What it bounds | When it is reached |
|---|---|---|---|
| `CMD_OPTION_MAX` | per port | options a command takes, so a script gets `CMD_OPTION_MAX - 1` arguments | `source` prints `too many arguments` and does not run the script |
| `SCRIPT_MAX_DEPTH` | 3 | scripts open at once, the first one and everything it sources | that `source` fails and the script that called it carries on with its next line |
| `SCRIPT_BLOCK_MAX` | 8 | `if`, `while` and `for` blocks open inside each other | the script fails |
| `SCRIPT_LOOP_MAX` | 1000 | passes one `while` or `for` may make | the script fails |
| `SHELL_CAPTURE_MAX` | 256 bytes | the answer of one `$(...)` | the line fails |
| `SHELL_CAPTURE_DEPTH` | 2 | `$(...)` open inside another | the line fails |
| `SHELL_ARITH_DEPTH` | 8 | parentheses inside one `$((...))` | the text is left as typed |
| `ENV_SESSION_MAX` | 8 | variables a session holds | `export` or `read` past it is refused |
| `ENV_NAME_MAX` | 32 | length of a variable name | the name is refused |
| `ENV_VALUE_MAX` | 128 | length of a value `read` stores | `read` stops the value there |
| `PDI_PIPE_CAPACITY` | 1024 bytes | what one stage of a pipe passes to the next | the pipe is marked overflowed |

Arguments past the ninth are reachable only through `$@`, since `$1` to `$9` are single digits.

The whole of this section is behind `ENABLE_SCRIPT_RUNNER`, on by default. Comment it out and `source`, control flow and `rc.local` go with it, including the per-session state they use; the rest of the shell is unaffected.

## 7.16 Scheduled jobs and `/etc/crontab`

`rc.local` covers "run this once at boot". `/etc/crontab` covers "run this every so often" — one job per line, five time fields and the command:

```
# min hour dom mon dow  command
*/5  *    *   *   *     echo tick >> /tmp/heartbeat
0    3    *   *   *     source /etc/nightly.sh
30   4    *   *   0     db backup
```

Each field takes the standard grammar: `*`, a number, a range `10-20`, a step `*/5` or `10-20/2`, and comma-separated lists of any of those. Day-of-week runs 0–6 from Sunday. Month and day-of-month names (`JAN`, `MON`) and `@daily`-style macros are not read — `rc.local` is the `@reboot` equivalent.

| field | range |
|---|---|
| minute | 0–59 |
| hour | 0–23 |
| day of month | 1–31 |
| month | 1–12 |
| day of week | 0–6, Sunday 0 |

**A spec that does not parse matches nothing**, rather than reading as zero and firing on the hour. That distinction matters because the two look identical from outside: a job that never runs and a job that has not come round yet both do nothing.

**The table is read from the file every minute and nothing is kept between runs.** An edit takes effect on the next minute with no reload step, and a board carries no memory cost for jobs it is not running. The cost is one pass over the file per minute, which is why `CRON_MAX_ENTRIES` (8 by default) bounds it — a line is found by scanning from the start of the file, so a long table costs more than its length suggests.

**Jobs only run while the clock is trustworthy.** A board that cannot reach a time server still counts forward from boot but never calls that time valid, and a calendar cannot be matched against a count from boot, so nothing fires. `crontab` says so rather than leaving you to wonder.

Each job runs in a session built for it, under root, and given back afterwards. **That session is attached to the null terminal, not the console**, which is what lets a job run at all while somebody is logged in — a job must never borrow a live session, and waiting for the console to be free would mean never running on a board anyone uses. Job output therefore goes where the job sends it: to a file if it redirects, and nowhere otherwise, the way `cron` has always behaved. A job that stops for input is interrupted rather than left holding the terminal. A session the job ran in the middle of — a command part way through on ssh, say — is current again on its own terminal once the job is done.

`rc.local` keeps the console instead, so its output is still visible at boot — it runs before anyone can be logged in, so there is no session to collide with.

`crontab` lists the table as the device parsed it, which is what makes it worth more than `cat /etc/crontab` — a row missing a field, or a typo in a spec, simply does not appear:

```
$ crontab
*/5 * * * * echo tick >> /tmp/heartbeat
0 3 * * * source /etc/nightly.sh

due this minute:
  none
```

Editing is `fedit /etc/crontab`. The whole of this is behind `ENABLE_CRON_SERVICE`, on by default and switched off automatically without the cmd and storage services or the script runner, which is what runs each job.
