#!/usr/bin/env python3

"""
Name resolution order: IP literal, then /etc/hosts, then DNS.

`test_net.py` already covers each source answering on its own. What decides
whether a resolver is correct is what happens when two of them could answer the
same question, and that is what this suite is for. Precedence only shows when
the sources disagree, so every test here makes them disagree on purpose.

/etc/hosts is rewritten to do that, and put back afterwards. It is copied aside
rather than rebuilt from a literal, because `echo` can only write one line and
restoring a file from memory that never quite matches is how a test suite
leaves a board slightly wrong.
"""

from .registry import test, expect_in, expect_not_in, Skip

HOSTS = "/etc/hosts"
BACKUP = "/hosts.bak"

# TEST-NET-1, reserved for documentation and routed nowhere, so an entry
# pointing here can never be confused with a real answer
DOC_ADDRESS = "192.0.2.55"
OTHER_ADDRESS = "192.0.2.66"


def rewrite_hosts(t, line):
    """
    Replace /etc/hosts with a single entry, keeping the original aside.

    Returns True when the original was saved; the caller restores in a finally.
    """
    t.run("rm %s" % BACKUP)
    t.run("cp %s %s" % (HOSTS, BACKUP))

    saved = t.run("cat %s" % BACKUP)
    if "localhost" not in saved:
        raise Skip("could not copy %s aside, leaving it alone: %s" % (HOSTS, saved))

    t.run("echo %s > %s" % (line, HOSTS))

    written = t.run("cat %s" % HOSTS)
    if line.split()[0] not in written:
        raise Skip("the target would not rewrite %s: %s" % (HOSTS, written))

    return True


def restore_hosts(t):
    t.run("mv %s %s" % (BACKUP, HOSTS))

    body = t.run("cat %s" % HOSTS)
    if "localhost" not in body:
        # never leave the board without a usable hosts file
        t.run("echo 127.0.0.1 localhost > %s" % HOSTS)


@test("an address literal is resolved before the hosts file",
      needs=("host", "cat", "cp", "mv", "echo", "rm"))
def literal_beats_hosts(t):
    """
    A hosts entry whose *name* is itself an address must not shadow the literal.
    Nothing sane writes such an entry, which is the point: it is the only way to
    make the first two sources disagree and see which one answers.
    """
    rewrite_hosts(t, "%s 1.2.3.4" % OTHER_ADDRESS)
    try:
        out = t.run("host 1.2.3.4")
        expect_in("1.2.3.4", out, "the literal resolving to itself")
        expect_not_in(OTHER_ADDRESS, out,
                      "the hosts entry must not win over a literal")
    finally:
        restore_hosts(t)


@test("the hosts file is consulted before dns",
      needs=("host", "cat", "cp", "mv", "echo", "rm"))
def hosts_beats_dns(t):
    """
    A name that really does resolve in public dns, pointed somewhere else in
    /etc/hosts. Whichever address comes back names the source that answered.
    """
    rewrite_hosts(t, "%s example.com" % DOC_ADDRESS)
    try:
        out = t.run("host example.com", timeout=max(t.timeout, 30))
        expect_in(DOC_ADDRESS, out, "the hosts entry answering before dns")
    finally:
        restore_hosts(t)


@test("a commented line in the hosts file is not a mapping",
      needs=("host", "cat", "cp", "mv", "echo", "rm"))
def comment_is_not_a_mapping(t):
    rewrite_hosts(t, "# %s commented.invalid" % DOC_ADDRESS)
    try:
        out = t.run("host commented.invalid", timeout=max(t.timeout, 30))
        expect_not_in(DOC_ADDRESS, out, "a commented entry must not resolve")
    finally:
        restore_hosts(t)


@test("a hosts entry is what ping resolves through",
      needs=("ping", "host", "cat", "cp", "mv", "echo", "rm"), slow=True)
def ping_uses_the_resolver(t):
    """
    The resolver is only worth having if the commands go through it. Pointing a
    name at the target's own address makes the reply prove it end to end.
    """
    from .test_net import own_address

    mine = own_address(t)
    rewrite_hosts(t, "%s selftest.invalid" % mine)
    try:
        out = t.run("ping selftest.invalid 1", timeout=max(t.timeout, 40))
        expect_in(mine, out, "ping resolving a name through the hosts file")
    finally:
        restore_hosts(t)


@test("the hosts file survives being rewritten and restored",
      needs=("cat", "cp", "mv", "echo", "rm"))
def hosts_is_restored(t):
    """
    The other tests here rewrite a file the target needs. This one asserts the
    put-back works, so a failure points at the harness rather than showing up
    later as an unrelated resolution failure.
    """
    before = t.run("cat %s" % HOSTS)

    rewrite_hosts(t, "%s scratch.invalid" % DOC_ADDRESS)
    restore_hosts(t)

    after = t.run("cat %s" % HOSTS)
    if before.strip() != after.strip():
        raise AssertionError("%s was not put back:\nbefore: %r\nafter:  %r"
                             % (HOSTS, before, after))
