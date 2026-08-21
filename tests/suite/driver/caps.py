#!/usr/bin/env python3

"""
What the target in front of us can actually do.

A board is built with a subset of the services and commands, and which subset
is a compile time decision the test suite has no way to know in advance. So it
asks: `help` for the commands, `mount` and `df` for the filesystems, `srvc
list` for the services. A test that names something absent is skipped with the
reason, which is very different from failing.
"""

import re

# `  ls          ls [path]  list a directory` — name first, usage after the pad
HELP_ROW = re.compile(r"^\s{2,}(\S+)\s")
HELP_HEADER = re.compile(r"Registered commands \((\d+)\)")

# `mount` prints PREFIX / TYPE / NAME, `df` prints MOUNT / NAME / sizes
PATH_ROW = re.compile(r"^\s*(/\S*)\s+(\S+)")

# `srvc list` prints SERVICE / STATE / TASKS / R/S/Z
SERVICE_ROW = re.compile(r"^\s*(\S+)\s+(inactive|active|stopped|dead)\b")


class Capabilities(object):

    def __init__(self):
        self.commands = set()
        self.mounts = set()
        self.filesystems = set()
        self.services = {}
        self.notes = []

    @classmethod
    def probe(cls, shell):
        caps = cls()

        caps._probe_commands(shell)
        if "mount" in caps.commands:
            caps._probe_mounts(shell)
        if "df" in caps.commands:
            caps._probe_filesystems(shell)
        if "srvc" in caps.commands:
            caps._probe_services(shell)

        return caps

    def _safe(self, shell, command):
        try:
            return shell.run(command)
        except Exception as err:
            self.notes.append("%s did not answer: %s" % (command, err))
            return ""

    def _probe_commands(self, shell):
        out = self._safe(shell, "help")
        for line in out.splitlines():
            if HELP_HEADER.search(line):
                continue
            found = HELP_ROW.match(line.rstrip())
            if found:
                self.commands.add(found.group(1))

    def _probe_mounts(self, shell):
        for line in self._safe(shell, "mount").splitlines():
            found = PATH_ROW.match(line)
            if found:
                self.mounts.add(found.group(1))

    def _probe_filesystems(self, shell):
        for line in self._safe(shell, "df").splitlines():
            found = PATH_ROW.match(line)
            if found:
                self.filesystems.add(found.group(2))

    def _probe_services(self, shell):
        for line in self._safe(shell, "srvc list").splitlines():
            found = SERVICE_ROW.match(line)
            if found:
                self.services[found.group(1)] = found.group(2)

    def has_command(self, *names):
        return all(name in self.commands for name in names)

    def has_mount(self, *paths):
        return all(path in self.mounts for path in paths)

    def has_service(self, *names):
        return all(name in self.services for name in names)

    def why_skip(self, needs=(), mounts=(), services=()):
        """The reason this target cannot run a test, or None when it can."""
        absent = [name for name in needs if name not in self.commands]
        if absent:
            return "no %s command" % ", ".join(sorted(absent))

        unmounted = [path for path in mounts if path not in self.mounts]
        if unmounted:
            return "%s not mounted" % ", ".join(sorted(unmounted))

        off = [name for name in services if name not in self.services]
        if off:
            return "%s service not built in" % ", ".join(sorted(off))

        return None

    def summary(self):
        return "%d commands, %d mounts, %d services" % (
            len(self.commands), len(self.mounts), len(self.services))
