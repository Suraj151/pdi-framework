#!/usr/bin/env python3

"""
Registration, measurement and judgement for the performance tier.

A perf test measures one thing once and returns the number. The runner calls it
several times and keeps the median, because a wireless link alone moves a
transfer by tens of percent from one try to the next, and a single reading
would turn that into a verdict.

Each board keeps its own baselines, keyed by its hostname, in baselines.json
beside this file. A metric starts out settling: its first runs are recorded and
a slowdown is only a warning, since there is nothing trustworthy to compare it
with yet. Once three recorded runs agree within the metric's margin it is
settled, and from then on a run slower than the baseline by more than the
margin fails. A run never moves a settled baseline by itself; after a change
that is meant to be faster, --perf-record takes the new numbers on purpose, so a
regression can never become the baseline unnoticed.
"""

import csv
import json
import os
import statistics
import time

from ..features.registry import Skip, GREEN, RED, YELLOW, DIM, RESET

BASELINES = os.path.join(os.path.dirname(os.path.abspath(__file__)), "baselines.json")
HISTORY = os.environ.get("PDI_PERF_HISTORY",
                         os.path.expanduser("~/pdi-test-logs/perf-history.csv"))

SETTLE_RUNS = 3

PERFS = []


class Perf(object):

    def __init__(self, suite, name, fn, unit, better, margin, repeat, lane, needs):
        self.suite = suite
        self.name = name
        self.fn = fn
        self.unit = unit
        self.better = better
        self.margin = margin
        self.repeat = repeat
        self.lane = lane
        self.needs = tuple(needs)

    @property
    def full_name(self):
        return "%s.%s" % (self.suite, self.name)


def perf(name, unit, better="higher", margin=0.20, repeat=5, lane=None, needs=()):
    """
    Register one measurement.

    unit names what the number counts, better says which way is good, margin is
    the fraction a settled baseline may be missed by before the run fails. lane
    names the transport the measurement is about (telnet, ssh, serial); None
    takes whichever lane is open.
    """
    def register(fn):
        suite = fn.__module__.rsplit(".", 1)[-1]
        if suite.startswith("perf_"):
            suite = suite[len("perf_"):]
        PERFS.append(Perf(suite, name, fn, unit, better, margin, repeat, lane, needs))
        return fn
    return register


def discover():
    import importlib
    import pkgutil

    package = os.path.dirname(os.path.abspath(__file__))
    for entry in sorted(pkgutil.iter_modules([package])):
        if entry.name.startswith("perf_"):
            importlib.import_module("%s.%s" % (__package__, entry.name))
    return PERFS


def load_baselines(path=BASELINES):
    try:
        with open(path) as fh:
            return json.load(fh)
    except (IOError, ValueError):
        return {}


def save_baselines(data, path=BASELINES):
    tmp = path + ".tmp"
    with open(tmp, "w") as fh:
        json.dump(data, fh, indent=1, sort_keys=True)
        fh.write("\n")
    os.replace(tmp, path)


def settled(entry, margin):
    """The baseline a metric's recorded runs agree on, or None while they do not."""
    if entry.get("value") is not None:
        return entry["value"]

    runs = entry.get("runs", [])
    if len(runs) < SETTLE_RUNS:
        return None

    recent = runs[-SETTLE_RUNS:]
    middle = statistics.median(recent)
    if middle <= 0:
        return None
    if all(abs(run - middle) <= margin * middle for run in recent):
        return middle
    return None


def judge(item, value, entry):
    """
    (verdict, baseline, change) for one median against a metric's entry.

    verdict is "ok", "faster", "slower" (a warning while settling) or "FAIL".
    change is the fractional move from the baseline, positive when better.
    """
    baseline = settled(entry, item.margin)
    reference = baseline
    if reference is None and entry.get("runs"):
        reference = statistics.median(entry["runs"])

    if reference is None or reference <= 0:
        return ("new", None, None)

    change = (value - reference) / reference
    if item.better == "lower":
        change = -change

    if change < -item.margin:
        return ("FAIL" if baseline is not None else "slower", reference, change)
    if change > item.margin:
        return ("faster", reference, change)
    return ("ok", reference, change)


def append_history(board, item, samples, value, verdict, path=HISTORY):
    folder = os.path.dirname(path)
    if folder and not os.path.isdir(folder):
        return
    fresh = not os.path.exists(path)
    with open(path, "a", newline="") as fh:
        out = csv.writer(fh)
        if fresh:
            out.writerow(["when", "board", "metric", "unit", "median", "min", "max", "verdict"])
        out.writerow([time.strftime("%Y-%m-%d %H:%M:%S"), board, item.full_name, item.unit,
                      "%.3f" % value, "%.3f" % min(samples), "%.3f" % max(samples), verdict])


def run(lanes, board, only=None, record=False, verbose=False):
    """
    Measure every selected metric on the lanes given and judge it.

    lanes maps a transport label to a Target. Returns (ok, failed, skipped)
    counts. Baselines and history are written only for a run the caller
    confirms the target stayed up through, so the caller passes commit().
    """
    discover()
    data = load_baselines()
    mine = data.setdefault(board, {})
    measured = []
    failed = []
    skipped = []
    ok = 0

    for item in PERFS:
        if only and not item.full_name.startswith(only) and only not in item.full_name:
            continue

        if item.lane is not None:
            target = lanes.get(item.lane)
            if target is None:
                skipped.append((item.full_name, "no %s lane was opened" % item.lane))
                print("  %sskip%s %s %s(no %s lane)%s"
                      % (YELLOW, RESET, item.full_name, DIM, item.lane, RESET), flush=True)
                continue
        else:
            target = next(iter(lanes.values()))

        reason = target.caps.why_skip(item.needs, (), ())
        if reason:
            skipped.append((item.full_name, reason))
            print("  %sskip%s %s %s(%s)%s" % (YELLOW, RESET, item.full_name, DIM, reason, RESET),
                  flush=True)
            continue

        samples = []
        try:
            for _ in range(item.repeat):
                samples.append(float(item.fn(target)))
        except Skip as why:
            skipped.append((item.full_name, str(why)))
            print("  %sskip%s %s %s(%s)%s" % (YELLOW, RESET, item.full_name, DIM, why, RESET),
                  flush=True)
            continue
        except Exception as err:
            failed.append(item.full_name)
            print("  %sFAIL%s %s: the measurement itself failed" % (RED, RESET, item.full_name),
                  flush=True)
            print("       %s" % str(err).replace("\n", "\n       "), flush=True)
            continue
        finally:
            try:
                target.cleanup()
            except Exception:
                pass

        value = statistics.median(samples)
        entry = mine.get(item.full_name, {})
        verdict, reference, change = judge(item, value, entry)
        measured.append((item, samples, value, verdict))

        spread = "%.1f..%.1f" % (min(samples), max(samples))
        if reference is None:
            against = "first run"
        else:
            against = "baseline %.1f, %+.0f%%" % (reference, change * 100.0)
        state = "" if settled(entry, item.margin) is not None else ", settling"
        colour = {"FAIL": RED, "slower": YELLOW, "faster": GREEN}.get(verdict, GREEN)
        print("  %s%-6s%s %s: %.1f %s %s(%s; %s%s)%s"
              % (colour, verdict, RESET, item.full_name, value, item.unit, DIM, spread,
                 against, state, RESET), flush=True)

        if "FAIL" == verdict:
            failed.append(item.full_name)
        else:
            ok += 1

    def commit():
        for item, samples, value, verdict in measured:
            entry = mine.setdefault(item.full_name, {"unit": item.unit, "better": item.better,
                                                      "runs": []})
            entry["unit"] = item.unit
            entry["better"] = item.better
            if record:
                entry["value"] = value
                entry["runs"] = [value]
            elif settled(entry, item.margin) is None:
                entry.setdefault("runs", []).append(value)
                entry["runs"] = entry["runs"][-10:]
            append_history(board, item, samples, value, verdict)
        save_baselines(data)

    return ok, failed, skipped, commit
