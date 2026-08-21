#!/usr/bin/env python3

"""
The line editor, driven a keystroke at a time.

This is the suite that gains the most from running on real hardware. The C++
tests in test_terminal_editing.cpp feed the editor through a string terminal in
the same process; here the same keys cross a uart or an ssh channel, arrive in
whatever chunks the transport chooses, and the echo comes back the same way.
Escape sequences that work in-process have a real chance of not working here.

Everything is asserted on what the target did, never on what it echoed, so a
transport that redraws differently does not break the suite.
"""

from .registry import test, expect_in, expect_not_in
from ..driver.shell import PROMPT

UP = "\x1b[A"
DOWN = "\x1b[B"
LEFT = "\x1b[D"
RIGHT = "\x1b[C"
HOME = "\x1b[H"
END = "\x1b[F"
BACKSPACE = "\x7f"
DELETE = "\x1b[3~"
TAB = "\t"
CTRL_C = "\x03"
CTRL_Z = "\x1a"


def keys(t, text, settle=0.4):
    """Type without pressing enter, and return what came back."""
    t.shell.send_raw(text)
    return t.shell.drain(settle)


def enter(t, timeout=None):
    """Press enter and read to the next prompt."""
    t.shell.send_raw("\n")
    return t.shell.expect(PROMPT, timeout or t.timeout)


def settle(t):
    """
    Get back to a known state: a fresh prompt and an empty line, with nothing
    from the previous test still in flight.

    Draining for a fixed moment is not enough. A transport that echoes slowly —
    ssh through the crypto layer on a busy board — delivers the previous step's
    echo after the drain has given up, and it then lands in the *next* step's
    window and is read as its output. That produced two false failures over ssh
    while telnet and serial passed: a prompt from an earlier command counted as
    tab running the line. Reading up to a token only this call could have
    produced discards whatever was queued, however late it is.
    """
    t.resync()


@test("a whole command typed at once runs", needs=("pwd",))
def whole_command(t):
    settle(t)
    expect_in("/", t.run("pwd"), "pwd typed as a line")


@test("a line is held until enter", needs=("pwd",))
def held_until_enter(t):
    settle(t)

    keys(t, "pw")
    keys(t, "d")
    out = enter(t)

    expect_in("/", out, "the line ran only after enter")


@test("backspace removes the last character", needs=("pwd",))
def backspace_removes(t):
    settle(t)

    keys(t, "pwdx")
    keys(t, BACKSPACE)
    out = enter(t)

    expect_in("/", out, "pwd ran after the x was erased")
    expect_not_in("CmdErr", out, "no unknown command")


@test("backspace on an empty line does nothing", needs=("pwd",))
def backspace_empty(t):
    settle(t)

    keys(t, BACKSPACE * 3)
    keys(t, "pwd")
    out = enter(t)

    expect_in("/", out, "the line was still usable")


@test("every character can be erased and the line retyped", needs=("pwd",))
def erase_everything(t):
    settle(t)

    keys(t, "garbage")
    keys(t, BACKSPACE * 7)
    keys(t, "pwd")
    out = enter(t)

    # only that the right command ran: the typed text is still in the echo, and
    # asserting on the echo is what this suite exists to avoid
    expect_in("/", out, "pwd after the line was cleared by hand")


@test("a character can be inserted in the middle", needs=("pwd",))
def insert_middle(t):
    settle(t)

    # type "pd", go back one, insert "w"
    keys(t, "pd")
    keys(t, LEFT)
    keys(t, "w")
    out = enter(t)

    expect_in("/", out, "pwd was assembled by an insert")


@test("the cursor stops at the start of the line", needs=("pwd",))
def cursor_stops_at_start(t):
    settle(t)

    keys(t, "wd")
    keys(t, LEFT * 8)
    keys(t, "p")
    out = enter(t)

    expect_in("/", out, "the cursor did not run off the front")


@test("home and end jump to the ends", needs=("pwd",))
def home_and_end(t):
    settle(t)

    keys(t, "wd")
    keys(t, HOME)
    keys(t, "p")
    keys(t, END)
    out = enter(t)

    expect_in("/", out, "home then end assembled the line")


@test("backspace in the middle removes the character before the cursor", needs=("pwd",))
def backspace_middle(t):
    settle(t)

    keys(t, "pwXd")
    keys(t, LEFT)
    keys(t, BACKSPACE)
    out = enter(t)

    expect_in("/", out, "the X was taken out of the middle")


@test("ctrl c abandons the line", needs=("pwd",))
def ctrl_c_abandons(t):
    settle(t)

    keys(t, "nonsense")
    keys(t, CTRL_C)

    # an abandoned line runs nothing, and nothing is exactly what an unknown
    # command prints here, so the check is that the shell still answers
    out = t.run("pwd")
    expect_in("/", out, "the shell was usable again")


@test("a line can be retyped after an interrupt", needs=("pwd",))
def retype_after_interrupt(t):
    settle(t)

    keys(t, "half")
    keys(t, CTRL_C)
    keys(t, "pwd")
    out = enter(t)

    expect_in("/", out, "the new line ran")


@test("the previous command comes back on the up arrow", needs=("pwd",))
def history_recall(t):
    settle(t)
    t.run("pwd")

    keys(t, UP)
    out = enter(t)

    expect_in("/", out, "the recalled command ran")


@test("the down arrow walks back towards the newest", needs=("pwd", "whoami"))
def history_walk(t):
    settle(t)
    t.run("whoami")
    t.run("pwd")

    keys(t, UP)
    keys(t, UP)
    keys(t, DOWN)
    out = enter(t)

    expect_in("/", out, "walking up twice then down lands on pwd")


@test("tab completes a command prefix", needs=("help",))
def tab_completes(t):
    settle(t)

    keys(t, "hel")
    keys(t, TAB, settle=0.6)
    out = enter(t, timeout=max(t.timeout, 30))

    expect_in("Registered commands", out, "tab completed help and it ran")


@test("tab on a prefix that matches nothing leaves the line alone", needs=("pwd",))
def tab_no_match(t):
    settle(t)

    keys(t, "zzz")
    keys(t, TAB, settle=0.5)
    keys(t, BACKSPACE * 3)
    keys(t, "pwd")
    out = enter(t)

    expect_in("/", out, "the line was still editable after a fruitless tab")


@test("tab does not run the line", needs=("pwd",))
def tab_holds_the_line(t):
    settle(t)

    keys(t, "pwd")
    seen = keys(t, TAB, settle=0.6)

    # the fix for this: tab used to fall through and execute
    if PROMPT.search(seen):
        raise AssertionError("tab ran the line instead of holding it:\n%s" % seen)

    out = enter(t)
    expect_in("/", out, "and enter still runs it")


@test("a command waiting for input takes the next line", needs=("su", "whoami"), su=True)
def waiting_command(t):
    settle(t)

    t.shell.answer("su", "user: ", t.timeout)
    t.shell.answer("nosuchuser", "Pass : ", t.timeout)
    t.shell.send_line("nosuchpass")

    # a command that was answered interactively ends the same way a one-line
    # one does: the error, and then a prompt to type the next thing at
    out = t.shell.expect(PROMPT, t.timeout)
    expect_in("CmdErr", out, "the credentials were refused")

    expect_in(t.username, t.run("whoami"), "the failed su changed nothing")


@test("ctrl c abandons a command that is waiting", needs=("su", "whoami"), su=True)
def interrupt_waiting_command(t):
    settle(t)

    t.shell.answer("su", "user: ", t.timeout)

    # the target discards pending input shortly after printing a prompt, so an
    # interrupt sent immediately is thrown away rather than delivered; see the
    # destructive flush noted in the plan
    t.shell.drain(0.8, 5.0)

    t.shell.send_raw(CTRL_C)
    out = t.shell.drain(0.6, 8.0)

    # CMD_RESULT_ABORTED — the interrupt reached the command rather than the
    # line editor, at the first of its prompts as well as the masked one
    expect_in("CmdErr : 10", out, "the waiting command was aborted")
    expect_in(t.username, t.run("whoami"), "the shell came back to the prompt")


@test("a masked prompt does not echo what is typed", needs=("su",), su=True)
def masked_prompt(t):
    settle(t)

    t.shell.answer("su", "user: ", t.timeout)
    t.shell.answer(t.username, "Pass : ", t.timeout)

    secret = "visiblesecret"
    echoed = keys(t, secret, settle=0.5)
    expect_not_in(secret, echoed, "the password was echoed back")

    t.shell.send_raw("\n")
    t.shell.expect(PROMPT, t.timeout)


@test("cls clears the screen and the shell stays usable", needs=("cls", "pwd"))
def cls_clears_screen(t):
    settle(t)
    t.run("cls")
    expect_in("/", t.run("pwd"), "the shell is usable after cls")
