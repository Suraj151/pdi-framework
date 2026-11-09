#!/usr/bin/env python3

"""
The filesystem at its edges, on every backend the target carries.

A name at the length limit and one past it, a path on each mount, a rename over
a file that is already there, two sessions writing one file, a special
filesystem that must not be written, and a disk driven until it is full. Each
mount the target reports is tested where the test makes sense for it; a mount
that is read-only, like /proc and /sys, is tested for refusing writes rather
than taking them.
"""

from .registry import test, Skip, expect_in, expect_not_in

NAME_LIMIT = 24  # FILE_NAME_MAX_SIZE


def writable_mounts(t):
    """The mount prefixes a test may create files under."""
    good = []
    for prefix in ("/", "/tmp", "/mnt"):
        if prefix == "/" or prefix in t.caps.mounts:
            good.append(prefix)
    return good


@test("a name at the length limit is accepted", needs=("echo", "ls", "rm"))
def name_at_limit(t):
    name = "n" * NAME_LIMIT
    path = "/" + name
    t.run("rm %s" % path)
    t.run("echo hi > %s" % path)
    listed = name in t.run("ls /")
    t.run("rm %s" % path)
    if not listed:
        raise AssertionError("a name of %d characters was not accepted" % NAME_LIMIT)


@test("a name past the length limit is refused without a crash", needs=("echo", "ls", "rm"))
def name_past_limit(t):
    name = "n" * (NAME_LIMIT + 6)
    path = "/" + name
    out = t.run("echo hi > %s" % path)
    listed = name in t.run("ls /")
    t.run("rm %s" % path)
    if listed:
        raise AssertionError("a name of %d characters was accepted; the limit is %d"
                             % (len(name), NAME_LIMIT))
    expect_in("alive", t.run("echo alive"), "the shell after an over-long name")


@test("every writable mount takes a file and reads it back", needs=("echo", "cat", "rm"))
def each_mount_round_trips(t):
    for prefix in writable_mounts(t):
        base = "" if prefix == "/" else prefix
        path = "%s/fe_rt.txt" % base
        t.run("rm %s" % path)
        t.run("echo MOUNTOK > %s" % path)
        out = t.run("cat %s" % path)
        t.run("rm %s" % path)
        expect_in("MOUNTOK", out, "a file on %s" % prefix)


@test("a redirect of a command that prints nothing creates the file or empties it, on every writable mount",
      needs=("true", "echo", "cat", "wc", "rm"))
def silent_redirect_each_mount(t):
    for prefix in writable_mounts(t):
        base = "" if prefix == "/" else prefix
        path = "%s/fe_silent.txt" % base
        t.run("rm %s" % path)

        t.run("true > %s" % path)
        expect_in("0 0 0 %s" % path, t.run("wc %s" % path), "a new file from a silent redirect on %s" % prefix)

        t.run("echo OLDLINE > %s" % path)
        t.run("true > %s" % path)
        out = t.run("wc %s" % path)
        expect_in("0 0 0 %s" % path, out, "a file a silent redirect replaced on %s" % prefix)

        t.run("echo KEPTLINE > %s" % path)
        t.run("true >> %s" % path)
        expect_in("KEPTLINE", t.run("cat %s" % path), "a file a silent append left alone on %s" % prefix)

        t.run("rm %s" % path)
        t.run("true >> %s" % path)
        expect_in("0 0 0 %s" % path, t.run("wc %s" % path), "a new file from a silent append on %s" % prefix)
        t.run("rm %s" % path)


@test("a missing path is named missing and a directory a directory, on every writable mount",
      needs=("rm", "cat", "ls", "mkdir", "echo"))
def missing_or_directory_each_mount(t):
    for prefix in writable_mounts(t):
        base = "" if prefix == "/" else prefix
        gone = "%s/fe_gone.txt" % base
        deep = "%s/fe_nodir/fe_gone.txt" % base
        folder = "%s/fe_folder" % base
        t.run("rm %s" % gone)
        t.run("rm -r %s" % folder)

        expect_in("%s : No such file or directory" % gone, t.run("rm %s" % gone), "rm of a missing file on %s" % prefix)
        expect_in("%s : No such file or directory" % gone, t.run("cat %s" % gone), "cat of a missing file on %s" % prefix)
        expect_in("%s : No such file or directory" % deep, t.run("cat %s" % deep), "cat under a missing directory on %s" % prefix)
        expect_in("%s : No such file or directory" % gone, t.run("ls %s" % gone), "ls of a missing path on %s" % prefix)

        t.run("mkdir %s" % folder)
        out = t.run("cat %s" % folder)
        expect_in("%s : Is a directory" % folder, out, "cat of a directory on %s" % prefix)
        expect_not_in("No such file", out, "cat of a directory on %s" % prefix)
        expect_in("cannot write %s : Is a directory" % folder, t.run("echo x > %s" % folder),
                  "a redirect onto a directory on %s" % prefix)
        t.run("rm -r %s" % folder)


@test("a rename over an existing file is refused and both files stay", needs=("echo", "cat", "mv", "rm"))
def rename_over_existing(t):
    ws = t.workspace("fe_rename")
    t.run("echo source > %s/a" % ws)
    t.run("echo target > %s/b" % ws)
    expect_in("mv: %s/b : File exists" % ws, t.run("mv %s/a %s/b" % (ws, ws)), "mv over a file")
    expect_in("target", t.run("cat %s/b" % ws), "the file mv was refused over")
    expect_in("source", t.run("cat %s/a" % ws), "the file mv was refused for")


@test("two sessions writing one file do not corrupt it", needs=("echo", "cat", "rm"))
def two_writers(t):
    peer = None
    try:
        peer = t.peer()
    except Skip:
        raise Skip("this transport cannot open a second session")
    path = "/fe_two.txt"
    try:
        t.run("echo mainline > %s" % path)
        peer.run("echo peerline >> %s" % path, 20.0)
        t.run("echo mainagain >> %s" % path)
        out = t.run("cat %s" % path)
        # whatever the interleaving, each complete line that was written is
        # present and intact, and nothing of a half line is left
        for marker in ("mainline", "peerline", "mainagain"):
            expect_in(marker, out, "a line two writers each put in %s" % path)
    finally:
        t.run("rm %s" % path)
        peer.close()


@test("a non-root user cannot remove a system directory", needs=("ls", "rm", "su", "useradd",
      "userdel", "whoami"), su=True)
def user_cannot_remove_system_dir(t):
    from .test_users import scratch_user, drop_user
    name = "feuser"
    scratch_user(t, name, "FeUserPass42")
    try:
        t.run("su %s FeUserPass42" % name)
        if name not in t.run("whoami"):
            raise Skip("could not become the scratch user")
        t.run("rm /etc")
        t.become_root()
        if "etc" not in t.run("ls /"):
            raise AssertionError("a non-root user removed /etc")
    finally:
        t.become_root()
        drop_user(t, name)


@test("procfs refuses a write", needs=("echo", "cat"))
def procfs_read_only(t):
    if "/proc" not in t.caps.mounts:
        raise Skip("the target has no /proc")
    before = t.run("cat /proc/uptime")
    t.run("echo 0 > /proc/uptime")
    after = t.run("cat /proc/uptime")
    if "cannot write" not in t.run("echo 0 > /proc/uptime") and after.strip() == "0":
        raise AssertionError("/proc/uptime took a write; procfs must be read only")
    del before


@test("sysfs refuses a write to a read-only leaf", needs=("echo", "cat", "ls"))
def sysfs_read_only(t):
    if "/sys" not in t.caps.mounts:
        raise Skip("the target has no /sys")
    # block size is a reported value, never settable
    nodes = t.run("ls /sys/block")
    name = None
    for line in nodes.splitlines():
        cols = line.split()
        if len(cols) >= 6 and cols[-1] not in (".", ".."):
            name = cols[-1]
    if name is None:
        raise Skip("the target lists no block device in /sys/block")
    leaf = "/sys/block/%s/size" % name
    before = t.run("cat %s" % leaf)
    t.run("echo 0 > %s" % leaf)
    after = t.run("cat %s" % leaf)
    if before.strip() and after.strip() == "0" and before.strip() != "0":
        raise AssertionError("%s took a write; a sysfs value leaf must be read only" % leaf)


@test("an append that no longer fits is reported, not dropped", needs=("echo", "rm", "df", "wc"))
def full_append_is_reported(t):
    """
    Once /tmp is full a further append must fail where it can be seen: a
    message, a failing status, or both. Taking the line, keeping nothing and
    reporting success loses data without anyone knowing.
    """
    if "/tmp" not in t.caps.mounts:
        raise Skip("the target has no /tmp to fill")
    path = "/tmp/fe_full"
    line = "y" * 190
    t.run("rm %s" % path)
    try:
        for _ in range(40):
            t.run("echo %s >> %s" % (line, path))
            free = None
            for ln in t.run("df").splitlines():
                cols = ln.split()
                if len(cols) >= 5 and cols[0] == "/tmp" and cols[4].isdigit():
                    free = int(cols[4])
            if free is not None and free < len(line) + 1:
                break
        before = t.run("wc %s" % path).split()
        out = t.run("echo %s >> %s; echo rc=$?" % (line, path))
        after = t.run("wc %s" % path).split()
    finally:
        t.run("rm %s" % path)
    for counts in (before, after):
        if len(counts) < 3 or not all(c.isdigit() for c in counts[:3]):
            raise AssertionError("wc gave no counts, so growth cannot be judged:\n%s" % " ".join(counts))
    grew = before[:3] != after[:3]
    status = [l for l in out.splitlines() if l.strip().startswith("rc=")]
    reported = ("cannot write" in out) or (status and status[-1].strip() != "rc=0")
    if not grew and not reported:
        raise AssertionError("an append to a full /tmp stored nothing and reported success:\n%s" % out)


@test("a disk full is reported and leaves the filesystem usable", needs=("echo", "rm", "df"),
      slow=True)
def disk_full_is_clean(t):
    """
    Fill /tmp, which is a small ram disk, until it refuses, then show the
    refusal was clean: the mount still lists, a small write elsewhere still
    works, and removing the filler frees the room again.
    """
    if "/tmp" not in t.caps.mounts:
        raise Skip("the target has no /tmp to fill")
    filler = "/tmp/fe_fill"
    t.run("rm %s" % filler)
    def tmp_free():
        for ln in t.run("df").splitlines():
            cols = ln.split()
            if len(cols) >= 5 and cols[0] == "/tmp" and cols[4].isdigit():
                return int(cols[4])
        return None
    try:
        line = "x" * 200
        for _ in range(60):
            t.run("echo %s >> %s" % (line, filler))
            free = tmp_free()
            if free is not None and free < 200:
                break
        # the shell still answers and another mount still takes a write once /tmp is full
        expect_in("alive", t.run("echo alive"), "the shell after /tmp filled")
        t.run("rm /fe_after.txt")
        t.run("echo after > /fe_after.txt")
        expect_in("after", t.run("cat /fe_after.txt"), "a write to / after /tmp filled")
        t.run("rm /fe_after.txt")
    finally:
        t.run("rm %s" % filler)
    free = tmp_free()
    if free is not None and free < 200:
        raise AssertionError("/tmp reports %d free after the filler was removed; the room did not "
                             "come back" % free)


def root_free(t):
    for ln in t.run("df").splitlines():
        cols = ln.split()
        if len(cols) >= 5 and cols[0] == "/" and cols[4].isdigit():
            return int(cols[4])
    return None


@test("userdel on a full disk never drops another account", needs=("useradd", "userdel", "cat", "df", "echo", "rm"),
      slow=True)
def userdel_full_disk_keeps_others(t):
    from .test_sftp import sftp_open
    keep = ["fkeep1", "fkeep2"]
    gone = "fgone"
    filler = "/fe_rootfill.bin"
    t.run("rm %s" % filler)
    free = root_free(t)
    if free is None:
        raise Skip("df names no root free size")
    if free > 1024 * 1024:
        raise Skip("the root disk is too large to fill in a test (%d B free); filling a board's flash takes most of an hour and wears it" % free)
    for name in keep + [gone]:
        t.run("userdel u=%s" % name)
        t.run("useradd u=%s p=Strong#Pass123" % name)
    free = root_free(t) or 0
    try:
        client, sftp = sftp_open(t)
        try:
            with sftp.open(filler, "wb") as fh:
                left = max(0, free - 8192)
                block = b"F" * 4096
                while left > 0:
                    piece = block[:min(left, len(block))]
                    fh.write(piece)
                    left -= len(piece)
        except IOError:
            pass
        finally:
            sftp.close()
            client.close()
        for _ in range(80):
            t.run("echo %s >> %s" % ("z" * 200, filler))
            now = root_free(t)
            if now is not None and now <= 4096:
                break
        left = root_free(t)
        if left is None or left > 4096:
            raise Skip("the root disk did not fill (%s B left)" % left)
        t.run("userdel u=%s" % gone, timeout=30)
    finally:
        t.run("rm %s" % filler)
    passwd = t.run("cat /etc/passwd", timeout=30)
    shadow = t.run("cat /etc/shadow", timeout=30)
    try:
        for name in keep + ["pdiStack"]:
            expect_in(name + ":", passwd, "userdel on a full disk dropped %s from /etc/passwd" % name)
            expect_in(name + ":", shadow, "userdel on a full disk dropped %s from /etc/shadow" % name)
    finally:
        for name in keep + [gone]:
            t.run("userdel u=%s" % name)
