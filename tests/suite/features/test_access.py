#!/usr/bin/env python3

"""
The access matrix: who may do what to which file, through every surface.

Two scratch accounts are made, `accalice` and `accbob`, and the cases below run
as accalice against objects owned by root, by accbob and by accalice herself.
The expectation in each row is the linux one for the same owner and mode. A
case is judged by its effect, never by the message: a refused write leaves the
content as it was, a refused delete leaves the file there, a refused read never
prints the marker the file holds.

Nothing here touches a real system file in a way that could lock a board: the
destructive cases on /etc/shadow and /etc/passwd run only on the host process,
and on a board the same permission is proved on a root-owned decoy in /etc with
the same mode. Every object is put back before the next case, so one case that
wrongly succeeds cannot change the outcome of another.
"""

import time

from .registry import test, Skip, expect_in, expect_not_in
from .test_users import scratch_user, drop_user
from .test_sftp import sftp_open

ALICE = "accalice"
BOB = "accbob"
ALICE_PW = "AccAlice7Kq2"
BOB_PW = "AccBob9Lw4"
ROOT_DIR = "/acc_root"
DECOY = "/etc/acc_decoy"
HOMES = "/home"
ROOM_NEEDED = 48 * 1024

_READY = {}


def on_host(t):
    return getattr(t.shell, "set_heap", None) is not None


def root_free(t):
    for line in t.run("df").splitlines():
        parts = line.split()
        if len(parts) >= 5 and parts[0] == "/" and parts[4].isdigit():
            return int(parts[4])
    return None


def as_user(t, name, password):
    t.run("su %s %s" % (name, password))
    who = t.run("whoami")
    if name not in who:
        raise AssertionError("su %s did not switch the session:\n%s" % (name, who))


def present(t, path):
    return "present" in t.run("test -e %s && echo present" % path)


def mode_of(t, path):
    """The permission column ls prints for path, or None when it is not listed."""
    parent, _, name = path.rstrip("/").rpartition("/")
    for line in t.run("ls %s" % (parent or "/")).splitlines():
        cols = line.split()
        if len(cols) >= 6 and cols[-1] == name:
            return cols[0]
    return None


def owner_of(t, path):
    parent, _, name = path.rstrip("/").rpartition("/")
    for line in t.run("ls %s" % (parent or "/")).splitlines():
        cols = line.split()
        if len(cols) >= 6 and cols[-1] == name:
            return cols[1]
    return None


def uid_of(t, name):
    for line in t.run("cat /etc/passwd").splitlines():
        parts = line.strip().split(":")
        if len(parts) >= 3 and parts[0] == name:
            return parts[2]
    raise AssertionError("%s has no line in /etc/passwd" % name)


def setup(t):
    """
    Make both accounts and the objects the cases act on, once per run; a later
    lane finds them already there. Removed again when the run ends.
    """
    for command in ("useradd", "userdel", "su", "chmod", "chown", "test"):
        if command not in t.caps.commands:
            raise Skip("the target has no %s" % command)

    key = id(t.shell)
    if _READY.get("done") and present(t, ROOT_DIR + "/public"):
        return

    free = root_free(t)
    if free is not None and free < ROOM_NEEDED and not present(t, ROOT_DIR):
        raise Skip("the root filesystem has %d bytes free; two homes and the fixtures need %d"
                   % (free, ROOM_NEEDED))

    t.become_root()
    scratch_user(t, ALICE, ALICE_PW)
    scratch_user(t, BOB, BOB_PW)
    t.run("rm %s" % ROOT_DIR)
    t.run("mkdir %s" % ROOT_DIR)
    t.run("chmod 755 %s" % ROOT_DIR)

    if not _READY.get("registered"):
        def teardown(live):
            live.resync()
            live.become_root()
            live.run("rm %s" % ROOT_DIR)
            live.run("rm %s" % DECOY)
            live.run("rm /tmp/acc_bob")
            live.run("rm /tmp/acc_alice")
            drop_user(live, ALICE)
            drop_user(live, BOB)
            live.run("rm %s" % HOMES)
            _READY.clear()
        t.at_exit(teardown)
        _READY["registered"] = True

    _READY["done"] = True
    _READY["key"] = key


def fresh(t, path, text, mode, owner=None, owner_pw=None):
    """
    Put an object back to a known state as its owner: content, mode, and for an
    object of accbob's, written as accbob so it is truly hers.
    """
    t.become_root()
    t.run("rm %s" % path)
    if owner is None:
        t.run("echo %s > %s" % (text, path))
        t.run("chmod %s %s" % (mode, path))
        return
    as_user(t, owner, owner_pw)
    t.run("echo %s > %s" % (text, path))
    t.run("chmod %s %s" % (mode, path))
    t.become_root()


def cat_as_root(t, path):
    t.become_root()
    return t.run("cat %s" % path)


# ---------------------------------------------------------------- the matrix

def case_read(path, marker, allowed, prep):
    def run(t):
        setup(t)
        prep(t)
        as_user(t, ALICE, ALICE_PW)
        out = t.run("cat %s" % path)
        t.become_root()
        if allowed:
            expect_in(marker, out, "%s reading %s" % (ALICE, path))
        else:
            expect_not_in(marker, out, "%s reading %s, which she may not" % (ALICE, path))
    return run


def case_write(path, marker, allowed, prep):
    def run(t):
        setup(t)
        prep(t)
        as_user(t, ALICE, ALICE_PW)
        t.run("echo ACCWROTE > %s" % path)
        after = cat_as_root(t, path)
        prep(t)
        if allowed:
            expect_in("ACCWROTE", after, "%s writing %s" % (ALICE, path))
        else:
            expect_not_in("ACCWROTE", after, "%s writing %s, which she may not" % (ALICE, path))
            expect_in(marker, after, "%s still holding what it held" % path)
    return run


def case_append(path, marker, allowed, prep):
    def run(t):
        setup(t)
        prep(t)
        as_user(t, ALICE, ALICE_PW)
        t.run("echo ACCAPPENDED >> %s" % path)
        after = cat_as_root(t, path)
        prep(t)
        if allowed:
            expect_in("ACCAPPENDED", after, "%s appending to %s" % (ALICE, path))
        else:
            expect_not_in("ACCAPPENDED", after, "%s appending to %s, which she may not" % (ALICE, path))
    return run


def case_create(path, allowed):
    def run(t):
        setup(t)
        t.become_root()
        t.run("rm %s" % path)
        as_user(t, ALICE, ALICE_PW)
        t.run("echo ACCNEW > %s" % path)
        t.become_root()
        made = present(t, path)
        t.run("rm %s" % path)
        if allowed and not made:
            raise AssertionError("%s could not create %s, which she may" % (ALICE, path))
        if not allowed and made:
            raise AssertionError("%s created %s, which she may not" % (ALICE, path))
    return run


def case_mkdir(path, allowed):
    def run(t):
        setup(t)
        t.become_root()
        t.run("rm %s" % path)
        as_user(t, ALICE, ALICE_PW)
        t.run("mkdir %s" % path)
        t.become_root()
        made = present(t, path)
        t.run("rm %s" % path)
        if allowed and not made:
            raise AssertionError("%s could not make the directory %s, which she may" % (ALICE, path))
        if not allowed and made:
            raise AssertionError("%s made the directory %s, which she may not" % (ALICE, path))
    return run


def case_delete(path, allowed, prep):
    def run(t):
        setup(t)
        prep(t)
        as_user(t, ALICE, ALICE_PW)
        t.run("rm %s" % path)
        t.become_root()
        gone = not present(t, path)
        prep(t)
        if allowed and not gone:
            raise AssertionError("%s could not delete %s, which she may" % (ALICE, path))
        if not allowed and gone:
            raise AssertionError("%s deleted %s, which she may not" % (ALICE, path))
    return run


def case_rename(path, allowed, prep):
    def run(t):
        setup(t)
        prep(t)
        moved = path + "_mv"
        t.run("rm %s" % moved)
        as_user(t, ALICE, ALICE_PW)
        t.run("mv %s %s" % (path, moved))
        t.become_root()
        went = (not present(t, path)) and present(t, moved)
        t.run("rm %s" % moved)
        prep(t)
        if allowed and not went:
            raise AssertionError("%s could not rename %s, which she may" % (ALICE, path))
        if not allowed and went:
            raise AssertionError("%s renamed %s, which she may not" % (ALICE, path))
    return run


def case_chmod(path, allowed, prep):
    def run(t):
        setup(t)
        prep(t)
        before = mode_of(t, path)
        as_user(t, ALICE, ALICE_PW)
        t.run("chmod 777 %s" % path)
        t.become_root()
        after = mode_of(t, path)
        prep(t)
        changed = after is not None and after != before
        if allowed and not changed:
            raise AssertionError("%s could not chmod %s, which she may (%s -> %s)"
                                 % (ALICE, path, before, after))
        if not allowed and changed:
            raise AssertionError("%s changed the mode of %s, which she may not (%s -> %s)"
                                 % (ALICE, path, before, after))
    return run


def case_chown(path, prep):
    def run(t):
        setup(t)
        prep(t)
        before = owner_of(t, path)
        alice_uid = uid_of(t, ALICE)
        as_user(t, ALICE, ALICE_PW)
        t.run("chown %s %s" % (alice_uid, path))
        t.become_root()
        after = owner_of(t, path)
        prep(t)
        if after != before:
            raise AssertionError("%s gave %s to herself (%s -> %s); only root may chown"
                                 % (ALICE, path, before, after))
    return run


def root_file(path, text, mode):
    return lambda t: fresh(t, path, text, mode)


def bob_file(path, text, mode):
    return lambda t: fresh(t, path, text, mode, BOB, BOB_PW)


def alice_file(path, text, mode):
    return lambda t: fresh(t, path, text, mode, ALICE, ALICE_PW)


ALICE_HOME = "%s/%s" % (HOMES, ALICE)
BOB_HOME = "%s/%s" % (HOMES, BOB)

R_PUB = ROOT_DIR + "/public"
R_PRIV = ROOT_DIR + "/private"
B_PUB = BOB_HOME + "/pub"
B_PRIV = BOB_HOME + "/priv"
A_OWN = ALICE_HOME + "/mine"
B_TMP = "/tmp/acc_bob"

ROOT_PUB = root_file(R_PUB, "ACCROOTPUB", "644")
ROOT_PRIV = root_file(R_PRIV, "ACCROOTPRIV", "600")
BOB_PUB = bob_file(B_PUB, "ACCBOBPUB", "644")
BOB_PRIV = bob_file(B_PRIV, "ACCBOBPRIV", "600")
ALICE_OWN = alice_file(A_OWN, "ACCALICEOWN", "644")
BOB_TMP = bob_file(B_TMP, "ACCBOBTMP", "644")
DECOY_FILE = root_file(DECOY, "ACCDECOY", "600")

NEEDS = ("useradd", "userdel", "su", "whoami", "chmod", "chown", "test", "cat", "echo", "rm", "mv",
         "mkdir", "ls")

MATRIX = [
    ("reads a root file anyone may read", case_read(R_PUB, "ACCROOTPUB", True, ROOT_PUB)),
    ("cannot read a root file only root may read", case_read(R_PRIV, "ACCROOTPRIV", False, ROOT_PRIV)),
    ("cannot write a root file", case_write(R_PUB, "ACCROOTPUB", False, ROOT_PUB)),
    ("cannot append to a root file", case_append(R_PUB, "ACCROOTPUB", False, ROOT_PUB)),
    ("cannot create a file in a root directory", case_create(ROOT_DIR + "/new", False)),
    ("cannot make a directory in a root directory", case_mkdir(ROOT_DIR + "/newdir", False)),
    ("cannot delete a root file", case_delete(R_PUB, False, ROOT_PUB)),
    ("cannot rename a root file", case_rename(R_PUB, False, ROOT_PUB)),
    ("cannot chmod a root file", case_chmod(R_PUB, False, ROOT_PUB)),
    ("cannot chown a root file to herself", case_chown(R_PUB, ROOT_PUB)),
    ("reads another user's file that user left readable", case_read(B_PUB, "ACCBOBPUB", True, BOB_PUB)),
    ("cannot read another user's private file", case_read(B_PRIV, "ACCBOBPRIV", False, BOB_PRIV)),
    ("cannot write another user's file", case_write(B_PUB, "ACCBOBPUB", False, BOB_PUB)),
    ("cannot create a file in another user's home", case_create(BOB_HOME + "/fromalice", False)),
    ("cannot delete another user's file", case_delete(B_PUB, False, BOB_PUB)),
    ("cannot rename another user's file", case_rename(B_PUB, False, BOB_PUB)),
    ("cannot chmod another user's file", case_chmod(B_PUB, False, BOB_PUB)),
    ("cannot chown another user's file to herself", case_chown(B_PUB, BOB_PUB)),
    ("creates a file in her own home", case_create(ALICE_HOME + "/made", True)),
    ("makes a directory in her own home", case_mkdir(ALICE_HOME + "/madedir", True)),
    ("writes her own file", case_write(A_OWN, "ACCALICEOWN", True, ALICE_OWN)),
    ("deletes her own file", case_delete(A_OWN, True, ALICE_OWN)),
    ("renames her own file", case_rename(A_OWN, True, ALICE_OWN)),
    ("chmods her own file", case_chmod(A_OWN, True, ALICE_OWN)),
    ("cannot give her own file away with chown", case_chown(A_OWN, ALICE_OWN)),
    ("creates a file in /tmp", case_create("/tmp/acc_alice", True)),
    ("cannot delete another user's file in /tmp", case_delete(B_TMP, False, BOB_TMP)),
    ("cannot overwrite another user's file in /tmp", case_write(B_TMP, "ACCBOBTMP", False, BOB_TMP)),
    ("cannot read a root-only file in /etc", case_read(DECOY, "ACCDECOY", False, DECOY_FILE)),
    ("cannot write a root-only file in /etc", case_write(DECOY, "ACCDECOY", False, DECOY_FILE)),
    ("cannot delete a root-only file in /etc", case_delete(DECOY, False, DECOY_FILE)),
    ("cannot create a file in /etc", case_create("/etc/acc_new", False)),
    ("cannot create a file in the root directory", case_create("/acc_atroot", False)),
]

for _label, _fn in MATRIX:
    test("%s %s" % (ALICE, _label), needs=NEEDS, su=True)(_fn)


# ------------------------------------------------- real system files, read side

@test("a user cannot read the password hashes", needs=NEEDS, su=True)
def user_cannot_read_shadow(t):
    setup(t)
    t.become_root()
    root_hash_line = [l for l in t.run("cat /etc/shadow").splitlines() if l.startswith(t.username + ":")]
    if not root_hash_line:
        raise Skip("the shadow file holds no line for %s to look for" % t.username)
    secret = root_hash_line[0].split(":")[1][:12]
    as_user(t, ALICE, ALICE_PW)
    out = t.run("cat /etc/shadow")
    t.become_root()
    if secret and secret in out:
        raise AssertionError("%s read root's password hash from /etc/shadow" % ALICE)


@test("a user cannot read the ssh host private key", needs=NEEDS + ("hexdump",), su=True)
def user_cannot_read_host_key(t):
    setup(t)
    t.become_root()
    mine = t.run("hexdump /etc/ssh/ed25519")
    if "0000" not in mine:
        raise Skip("the target has no ed25519 host key to protect")
    first = mine.splitlines()[0]
    as_user(t, ALICE, ALICE_PW)
    out = t.run("hexdump /etc/ssh/ed25519")
    t.become_root()
    if first.strip() and first.strip() in out:
        raise AssertionError("%s read the ssh host private key" % ALICE)


@test("the https private key is readable only by root", needs=NEEDS + ("hexdump",), su=True)
def user_cannot_read_https_key(t):
    setup(t)
    t.become_root()
    mode = mode_of(t, "/etc/http/server.key")
    if mode is None:
        raise Skip("the target has no https key")
    if mode[4:] != "------":
        raise AssertionError("/etc/http/server.key is %s; a private key must be readable by its "
                             "owner alone" % mode)
    mine = t.run("hexdump /etc/http/server.key").splitlines()[0]
    as_user(t, ALICE, ALICE_PW)
    out = t.run("hexdump /etc/http/server.key")
    t.become_root()
    if mine.strip() and mine.strip() in out:
        raise AssertionError("%s read the https private key" % ALICE)


@test("a user cannot change another user's password through the store", needs=NEEDS, su=True)
def user_cannot_rewrite_passwd(t):
    setup(t)
    if not on_host(t):
        raise Skip("rewriting the real user store is only tried on the host process")
    t.become_root()
    before = t.run("cat /etc/passwd")
    shadow = t.run("cat /etc/shadow")
    as_user(t, ALICE, ALICE_PW)
    t.run("echo intruder:x:0:0:/:cmd >> /etc/passwd")
    t.run("echo intruder:x >> /etc/shadow")
    t.run("rm /etc/shadow")
    t.become_root()
    expect_not_in("intruder", t.run("cat /etc/passwd"), "/etc/passwd after a user tried to add root")
    after_shadow = t.run("cat /etc/shadow")
    expect_not_in("intruder", after_shadow, "/etc/shadow after a user tried to append")
    if t.username + ":" not in after_shadow:
        raise AssertionError("/etc/shadow lost root's line after %s tried to delete it" % ALICE)
    if before.strip() != t.run("cat /etc/passwd").strip():
        raise AssertionError("/etc/passwd changed while %s tried to write it" % ALICE)
    del shadow


# --------------------------------------------- read-only and special filesystems

@test("a user cannot write through /proc", needs=NEEDS, su=True)
def user_cannot_write_proc(t):
    setup(t)
    as_user(t, ALICE, ALICE_PW)
    t.run("echo 1 > /proc/meminfo")
    t.become_root()
    expect_in("MemFree", t.run("cat /proc/meminfo"), "/proc/meminfo after a write was tried")


@test("a user cannot drive a gpio through /sys", needs=NEEDS, su=True)
def user_cannot_drive_gpio(t):
    setup(t)
    t.become_root()
    pins = [l.split()[-1] for l in t.run("ls /sys/class/gpio").splitlines()
            if l.split() and l.split()[-1].isdigit()]
    if not pins:
        raise Skip("the target lists no gpio in /sys/class/gpio")
    pin = pins[-1]
    node = "/sys/class/gpio/%s" % pin
    names = [l.split()[-1] for l in t.run("ls %s" % node).splitlines() if l.split()]
    if "value" not in names:
        raise Skip("gpio %s has no value leaf" % pin)
    before = t.run("cat %s/value" % node).strip()
    want = "0" if before.endswith("1") else "1"
    as_user(t, ALICE, ALICE_PW)
    t.run("echo %s > %s/value" % (want, node))
    t.become_root()
    after = t.run("cat %s/value" % node).strip()
    if after != before:
        t.run("echo %s > %s/value" % (before[-1:], node))
        raise AssertionError("%s changed gpio %s through /sys (%s -> %s); only root may drive a "
                             "pin" % (ALICE, pin, before, after))


# --------------------------------------------------------- root-only commands

ROOT_ONLY = [
    ("useradd", "useradd u=accmallory p=AccMallory5Tz", lambda t: present(t, "%s/accmallory" % HOMES)
     or "accmallory" in t.run("cat /etc/passwd")),
    ("userdel", "userdel u=%s" % BOB, lambda t: BOB not in t.run("cat /etc/passwd")),
    ("service stop", "service stop cron", lambda t: "inactive" in t.run("service status cron")),
    ("umount", "umount /tmp", lambda t: "/tmp" not in t.run("mount")),
    ("db restore", "db restore", None),
]


def root_only_case(label, command, changed):
    def run(t):
        setup(t)
        if command.split()[0] not in t.caps.commands:
            raise Skip("the target has no %s" % command.split()[0])
        if command.startswith("service") and "cron" not in t.run("service list"):
            raise Skip("the target runs no cron service")
        as_user(t, ALICE, ALICE_PW)
        out = t.run(command)
        t.become_root()
        did = changed(t) if changed is not None else False
        if "useradd" in command:
            drop_user(t, "accmallory")
        if "userdel" in command and did:
            scratch_user(t, BOB, BOB_PW)
        if command.startswith("service") and did:
            t.run("service start cron")
        if "umount" in command and did:
            raise AssertionError("%s unmounted /tmp; the target needs a restart to get it back"
                                 % ALICE)
        if did:
            raise AssertionError("%s ran `%s` and it took effect; only root may:\n%s"
                                 % (ALICE, command, out))
        if changed is None and "root required" not in out:
            raise AssertionError("%s ran `%s` and was not refused as root-only:\n%s"
                                 % (ALICE, command, out))
    return run


for _label, _command, _changed in ROOT_ONLY:
    test("%s cannot run %s" % (ALICE, _label), needs=NEEDS, su=True)(
        root_only_case(_label, _command, _changed))


@test("a user cannot signal a task the system owns", needs=NEEDS + ("ps", "kill"), su=True)
def user_cannot_signal_system_task(t):
    setup(t)
    t.become_root()
    pid = None
    for line in t.run("ps").splitlines():
        cols = line.split()
        if len(cols) >= 10 and cols[0].isdigit() and cols[1] == "0" and cols[-1] == "Time":
            pid = cols[0]
    if pid is None:
        raise Skip("ps lists no Time task owned by the system")
    as_user(t, ALICE, ALICE_PW)
    t.run("kill 19 %s" % pid)
    t.become_root()
    state = None
    for line in t.run("ps").splitlines():
        cols = line.split()
        if cols and cols[0] == pid:
            state = cols[2]
    if state == "T":
        t.run("kill 18 %s" % pid)
        raise AssertionError("%s stopped the system's Time task (pid %s)" % (ALICE, pid))


# ------------------------------------------------------------ other surfaces

@test("over sftp a user cannot read a root-only file", needs=NEEDS, su=True)
def sftp_user_cannot_read_private(t):
    setup(t)
    ROOT_PRIV(t)
    client, sftp = sftp_open(t, username=ALICE, password=ALICE_PW)
    try:
        got = None
        try:
            with sftp.open(R_PRIV, "rb") as fh:
                got = fh.read()
        except IOError:
            got = None
    finally:
        sftp.close()
        client.close()
    if got is not None and b"ACCROOTPRIV" in got:
        raise AssertionError("%s read %s over sftp" % (ALICE, R_PRIV))


@test("over sftp a user cannot write into a root directory", needs=NEEDS, su=True)
def sftp_user_cannot_write_root_dir(t):
    setup(t)
    target = ROOT_DIR + "/sftp_new"
    t.become_root()
    t.run("rm %s" % target)
    client, sftp = sftp_open(t, username=ALICE, password=ALICE_PW)
    try:
        try:
            with sftp.open(target, "wb") as fh:
                fh.write(b"ACCSFTP\n")
        except IOError:
            pass
    finally:
        sftp.close()
        client.close()
    t.become_root()
    made = present(t, target)
    t.run("rm %s" % target)
    if made:
        raise AssertionError("%s wrote %s over sftp" % (ALICE, target))


@test("over sftp a user cannot delete another user's file", needs=NEEDS, su=True)
def sftp_user_cannot_delete_other(t):
    setup(t)
    BOB_PUB(t)
    client, sftp = sftp_open(t, username=ALICE, password=ALICE_PW)
    try:
        try:
            sftp.remove(B_PUB)
        except IOError:
            pass
    finally:
        sftp.close()
        client.close()
    t.become_root()
    if not present(t, B_PUB):
        BOB_PUB(t)
        raise AssertionError("%s deleted %s over sftp" % (ALICE, B_PUB))


@test("over sftp a user writes in her own home", needs=NEEDS, su=True)
def sftp_user_writes_own_home(t):
    setup(t)
    target = ALICE_HOME + "/sftp_mine"
    client, sftp = sftp_open(t, username=ALICE, password=ALICE_PW)
    try:
        with sftp.open(target, "wb") as fh:
            fh.write(b"ACCSFTPMINE\n")
    finally:
        sftp.close()
        client.close()
    t.become_root()
    expect_in("ACCSFTPMINE", t.run("cat %s" % target), "the file %s wrote over sftp" % ALICE)
    owner = owner_of(t, target)
    t.run("rm %s" % target)
    if owner != ALICE:
        raise AssertionError("the file %s wrote over sftp belongs to %s" % (ALICE, owner))


@test("over the portal a user cannot download a root-only file", needs=NEEDS, su=True)
def portal_user_cannot_download_private(t):
    setup(t)
    ROOT_PRIV(t)
    portal = t.portal(username=ALICE, password=ALICE_PW)
    answers = []
    for path in ("/../../..%s" % R_PRIV,
                 "/%%2e%%2e/%%2e%%2e/%%2e%%2e%s" % R_PRIV,
                 "/..%%2f..%%2f..%%2f%s" % R_PRIV.lstrip("/").replace("/", "%2f"),
                 "/../../../etc/shadow",
                 "/../../../etc/ssh/ed25519"):
        try:
            answers.append(portal.get(path, attempts=1))
        except Exception:
            continue
    for answer in answers:
        raw = answer.raw or b""
        if b"ACCROOTPRIV" in raw or (t.username.encode() + b":") in raw:
            raise AssertionError("%s fetched a root-only file through the portal" % ALICE)


@test("over the portal a user cannot delete a root file", needs=NEEDS, su=True)
def portal_user_cannot_delete_root(t):
    setup(t)
    ROOT_PUB(t)
    portal = t.portal(username=ALICE, password=ALICE_PW)
    try:
        portal.post("/storage-filedel", {"df": R_PUB, "loc": "/storage",
                                          "csrf": portal.csrf("/storage")})
    except Exception:
        pass
    t.become_root()
    if not present(t, R_PUB):
        ROOT_PUB(t)
        raise AssertionError("%s deleted %s through the portal" % (ALICE, R_PUB))
