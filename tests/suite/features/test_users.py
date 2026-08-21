#!/usr/bin/env python3

"""
Identity, and the commands that change it.

Mirrors tests/host/system/test_commands_users.cpp. Every account made here is
removed again, and the account the suite logged in as is never modified — on a
real board that is the only way back in.
"""

from .registry import test, expect_in, expect_not_in, Skip

# far above anything a device ships with, so a collision with a real account is
# not possible
TEST_UID_BASE = 4200


def scratch_user(t, name, password):
    """Make an account, having first removed any left by an interrupted run."""
    t.run("userdel u=%s" % name)

    out = t.run("useradd u=%s p=%s" % (name, password))
    if "root required" in out:
        raise Skip("not root on this target")
    if "added" not in out and "exists" not in out:
        raise AssertionError("useradd did not report success:\n%s" % out)

    return name


def drop_user(t, name):
    t.run("userdel u=%s" % name)


@test("whoami names the logged in user", needs=("whoami",))
def whoami_names(t):
    expect_in(t.username, t.run("whoami"), "whoami")


@test("id prints the uid and gid", needs=("id",))
def id_prints(t):
    out = t.run("id")
    expect_in("uid=", out, "id")
    expect_in("gid=", out, "id")


@test("groups prints the primary group", needs=("groups", "id"))
def groups_prints(t):
    # the session's gid is what is printed, since there is no group name store
    gid = None
    for part in t.run("id").replace(",", " ").split():
        if part.startswith("gid="):
            gid = part[len("gid="):].split("(")[0]

    if gid is None:
        raise AssertionError("id printed no gid to compare against")

    expect_in(gid, t.run("groups"), "groups agrees with id")


@test("who lists the open session", needs=("who",))
def who_lists(t):
    out = t.run("who")
    expect_in("USER", out, "who header")
    expect_in(t.username, out, "the session we are on")


@test("useradd creates an account that can be found", needs=("useradd", "userdel", "cat"))
def useradd_creates(t):
    name = scratch_user(t, "tadd", "taddpass")
    try:
        expect_in(name, t.run("cat /etc/passwd"), "the store after useradd")
    finally:
        drop_user(t, name)


@test("userdel removes an account", needs=("useradd", "userdel", "cat"))
def userdel_removes(t):
    name = scratch_user(t, "tdel", "tdelpass")
    drop_user(t, name)

    expect_not_in(name, t.run("cat /etc/passwd"), "the store after userdel")


@test("su switches the session to another user", needs=("useradd", "userdel", "su", "whoami"), su=True)
def su_switches(t):
    name = scratch_user(t, "tsu", "tsupass")
    try:
        t.run("su %s tsupass" % name)
        expect_in(name, t.run("whoami"), "whoami after su")
    finally:
        t.become_root()
        drop_user(t, name)


@test("su with a wrong password changes nothing", needs=("useradd", "userdel", "su", "whoami"), su=True)
def su_wrong_password(t):
    name = scratch_user(t, "tsuw", "tsuwpass")
    try:
        t.run("su %s notthepassword" % name)
        expect_in(t.username, t.run("whoami"), "still the original user")
    finally:
        t.become_root()
        drop_user(t, name)


@test("su to an unknown user changes nothing", needs=("su", "whoami"), su=True)
def su_unknown(t):
    t.run("su nobodyhere somepass")
    expect_in(t.username, t.run("whoami"), "still the original user")


@test("a switched session loses root privilege",
      needs=("useradd", "userdel", "su", "chmod", "chown", "cat", "echo"), su=True)
def su_loses_root(t):
    name = scratch_user(t, "tplain", "tplainpass")
    path = "/wu_secret.txt"
    try:
        t.run("echo root only > %s" % path)
        t.run("chown 0:0 %s" % path)
        t.run("chmod 600 %s" % path)
        expect_in("root only", t.run("cat %s" % path), "root can read it")

        t.run("su %s tplainpass" % name)
        expect_not_in("root only", t.run("cat %s" % path), "a plain user cannot")
        expect_in("root required", t.run("useradd u=tnope p=x"), "and cannot add users")
    finally:
        t.become_root()
        t.run("rm %s" % path)
        drop_user(t, name)


@test("passwd changes the password of the current user",
      needs=("useradd", "userdel", "su", "passwd"), su=True)
def passwd_changes(t):
    name = scratch_user(t, "tpw", "oldpass")
    try:
        t.run("su %s oldpass" % name)
        t.run("passwd p=oldpass n=newpass c=newpass")

        # the only way to check from here is to use it
        t.become_root()
        t.run("su %s newpass" % name)
        expect_in(name, t.run("whoami"), "the new password works")

        t.become_root()
        t.run("su %s oldpass" % name)
        expect_in(t.username, t.run("whoami"), "the old password does not")
    finally:
        t.become_root()
        drop_user(t, name)


@test("passwd needs the current password to be right",
      needs=("useradd", "userdel", "su", "passwd"), su=True)
def passwd_needs_current(t):
    name = scratch_user(t, "tpwc", "realpass")
    try:
        t.run("su %s realpass" % name)
        t.run("passwd p=wrongpass n=newpass c=newpass")

        t.become_root()
        t.run("su %s newpass" % name)
        expect_in(t.username, t.run("whoami"), "the rejected password was not set")

        t.run("su %s realpass" % name)
        expect_in(name, t.run("whoami"), "the real password still works")
    finally:
        t.become_root()
        drop_user(t, name)


@test("passwd needs the confirmation to match",
      needs=("useradd", "userdel", "su", "passwd"), su=True)
def passwd_needs_confirmation(t):
    name = scratch_user(t, "tpwm", "typopass")
    try:
        t.run("su %s typopass" % name)
        t.run("passwd p=typopass n=firstnew c=secondnew")

        t.become_root()
        t.run("su %s firstnew" % name)
        expect_in(t.username, t.run("whoami"), "a mismatched pair changed nothing")

        t.run("su %s typopass" % name)
        expect_in(name, t.run("whoami"), "the original password still works")
    finally:
        t.become_root()
        drop_user(t, name)


@test("the user store is not world readable", needs=("ls",))
def shadow_is_private(t):
    out = t.run("ls /etc")
    for line in out.splitlines():
        if "shadow" in line:
            expect_in("-rw-------", line, "the hash file's mode")
            return

    raise Skip("no /etc/shadow on this target")


def _shadow_line(t):
    for line in t.run("ls /etc").splitlines():
        if "shadow" in line:
            return line
    return None


@test("changing a password keeps the hash file private",
      needs=("passwd", "useradd", "userdel", "su", "ls"), su=True)
def passwd_keeps_shadow_private(t):
    """
    passwd rewrites /etc/shadow. A rewrite that replaces the file by rename can
    lose its mode and owner, which once left the hash file world readable and
    owned by whoever changed their password. After a non-root user changes their
    own password the file must still be private and owned by root.
    """
    if _shadow_line(t) is None:
        raise Skip("no /etc/shadow on this target")

    name = scratch_user(t, "tshadow", "oldpass")
    try:
        t.run("su %s oldpass" % name)
        t.run("passwd p=oldpass n=newpass c=newpass")
        t.become_root()

        line = _shadow_line(t)
        if line is None:
            raise AssertionError("/etc/shadow vanished after a password change")
        expect_in("-rw-------", line, "the hash file's mode after a password change")

        owner = line.split()[1]
        if owner not in (t.username, "root", "0"):
            raise AssertionError("a password change gave the hash file to %s" % owner)
    finally:
        t.become_root()
        drop_user(t, name)
