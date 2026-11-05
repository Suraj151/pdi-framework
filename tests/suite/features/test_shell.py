#!/usr/bin/env python3

"""
The shell grammar itself, over whatever transport the target is reached by.

Redirection and pipelines belong to the shell, not to any one command: the
command being redirected does not know it is being redirected, and the command
downstream of a pipe does not know its input was not a file. These check that
on a real board, where the output an operator sees is the only evidence.

These mirror tests/host/system/test_shell_redirect.cpp.
"""

import re
import time

from .registry import test, expect_in, expect_not_in, Skip, meminfo

W = "wt_"



def counts(text):
    """The three numbers wc prints, or None when it printed something else."""
    for line in text.splitlines():
        found = re.findall(r"\d+", line)
        if len(found) >= 3:
            return [int(n) for n in found[:3]]
    return None


def free_heap(t):
    """What the target says it has left, or a skip when it does not say."""
    found = meminfo(t)
    if found is None:
        raise Skip("this target does not report free heap")
    return found["MemFree"]


def heap_band(t, seconds=20.0, interval=2.0):
    """
    The top of the board's free heap and the spread it covers on its own.

    The heap does not sit still. It visits discrete levels as services take and
    return blocks, and it holds each one for tens of seconds: measured on esp8266
    across five bursts it sat at +240, 0, -776 and -1016 relative to its start,
    and on esp32 the step is nearer 4 KB. Samples taken close together therefore
    land wholly inside whichever level is current, which is how a board whose
    heap ended *above* where it started twice reported a leak. Watching for long
    enough to see the spread is what lets a threshold be about leaks rather than
    about which level the sampling happened to catch.
    """
    seen = [free_heap(t)]
    started = time.time()
    while time.time() - started < seconds:
        time.sleep(interval)
        seen.append(free_heap(t))
    return max(seen), max(seen) - min(seen)


def assert_no_leak(t, top, band, rounds, what, budget=512):
    """
    Fail on a drop the board's own movement cannot account for.

    Waiting does not separate the two — the levels recur, so a settle is a second
    throw of the same dice. Their sizes do: a held block is bounded by the spread
    measured before the work started, while a leak keeps going past it.
    """
    after, after_band = heap_band(t)
    moves = max(band, after_band)
    allowed = moves + budget

    if top - after <= allowed:
        return

    raise AssertionError("heap fell %d bytes over %d %s (%d -> %d), past the %d "
                         "this board moves on its own plus %d"
                         % (top - after, rounds, what, top, after, moves, budget))


@test("a command with no redirect handling of its own is captured", needs=("pwd", "cat"))
def redirect_captures_any_command(t):
    t.workspace(W + "shredir")

    # pwd contains no redirect code at all; the shell supplies every bit of it
    out = t.run("pwd > where.txt")
    expect_not_in("shredir", out, "the terminal saw the output")

    expect_in("shredir", t.run("cat where.txt"), "the file holds it instead")


@test("a redirected file ends its lines the way a file should", needs=("echo", "wc"))
def redirect_line_ending(t):
    t.workspace(W + "shend")
    t.run("echo abc > ended.txt")

    # three characters and one terminator. a carriage return is what a terminal
    # needs, not what a file holds, so a fourth byte here and no more
    expect_in("4", t.run("wc ended.txt"), "byte count of a redirected line")


@test("the terminal is given back after a redirect", needs=("echo", "cat"))
def redirect_hands_back(t):
    t.workspace(W + "shback")
    t.run("echo away > gone.txt")

    expect_in("still here", t.run("echo still here"), "echo after a redirect")


@test("a redirect to an unwritable path is refused", needs=("echo",))
def redirect_bad_target(t):
    t.workspace(W + "shbad")

    out = t.run("echo hi > /nosuchmount/nope.txt")
    expect_not_in("hi", out, "nothing was printed as if it had worked")


@test("a dangling redirect is a syntax error", needs=("echo",))
def redirect_dangling(t):
    expect_in("syntax error", t.run("echo hi >"), "a target that was never named")


@test("output of one command becomes input of the next", needs=("echo", "wc"))
def pipeline_two_stages(t):
    t.workspace(W + "shpipe")

    # wc counts what echo produced, not a file it was handed
    out = t.run("echo hello world | wc")
    expect_in("1", out, "line count through a pipe")
    expect_in("2", out, "word count through a pipe")


@test("a pipeline carries through three stages", needs=("echo", "grep", "wc"))
def pipeline_three_stages(t):
    t.workspace(W + "shpipe3")

    expect_in("1", t.run("echo alpha | grep alpha | wc"), "count after a filter")


@test("grep filters what it is piped", needs=("echo", "grep"))
def pipeline_grep_filters(t):
    t.workspace(W + "shgrep")

    expect_in("needle", t.run("echo needle | grep needle"), "a match through a pipe")
    expect_not_in("needle", t.run("echo needle | grep absent"), "a miss through a pipe")


@test("a file can be piped into a filter", needs=("echo", "cat", "grep"))
def pipeline_from_a_file(t):
    t.workspace(W + "shcatpipe")
    t.run("echo findme here > src.txt")

    expect_in("findme", t.run("cat src.txt | grep findme"), "a file through a pipe")


@test("the last stage of a pipeline can still be redirected", needs=("echo", "wc", "cat"))
def pipeline_then_redirect(t):
    t.workspace(W + "shboth")

    out = t.run("echo hello world | wc > counted.txt")
    expect_not_in("hello world", out, "the terminal saw the payload")

    expect_in("2", t.run("cat counted.txt"), "the count landed in the file")


@test("an empty pipeline stage is a syntax error", needs=("echo", "wc"))
def pipeline_empty_stage(t):
    expect_in("syntax error", t.run("echo hi |"), "a pipe with nothing after it")


@test("cat passes its input straight through", needs=("echo", "cat"))
def pipeline_cat_passthrough(t):
    t.workspace(W + "shcat")

    expect_in("through", t.run("echo through | cat"), "cat as a filter")


@test("head stops at the count it was piped", needs=("echo", "cat", "head"))
def pipeline_head_count(t):
    path = t.workspace(W + "shhead") + "/lines.txt"
    t.run("echo one > %s" % path)
    t.run("echo two >> %s" % path)
    t.run("echo three >> %s" % path)

    # with a pipe there is no file to name, so the lone argument is the count
    out = t.run("cat %s | head 2" % path)
    expect_in("one", out, "the first line")
    expect_not_in("three", out, "head stopped before the last line")


@test("tail keeps the last lines it was piped", needs=("echo", "cat", "tail"))
def pipeline_tail_count(t):
    path = t.workspace(W + "shtail") + "/lines.txt"
    t.run("echo one > %s" % path)
    t.run("echo two >> %s" % path)
    t.run("echo three >> %s" % path)

    out = t.run("cat %s | tail 2" % path)
    expect_in("three", out, "the last line")
    expect_not_in("one", out, "tail started after the first line")


@test("a file argument still wins when nothing is piped", needs=("echo", "head"))
def head_file_argument_unchanged(t):
    path = t.workspace(W + "shfarg") + "/lines.txt"
    t.run("echo one > %s" % path)
    t.run("echo two >> %s" % path)
    t.run("echo three >> %s" % path)

    # no pipe, so the first positional is the path it has always been
    out = t.run("head %s 1" % path)
    expect_in("one", out, "the first line")
    expect_not_in("three", out, "head stopped before the last line")


@test("a command given neither a file nor a pipe is unaffected", needs=("wc",))
def pipeline_no_input(t):
    t.workspace(W + "shnoin")

    # wc still wants an argument when nothing has claimed its input
    expect_not_in("0 0 0", t.run("wc"), "wc invented a count from nowhere")


@test("an appending redirect keeps what was already there", needs=("echo", "cat", "wc"))
def redirect_append_through_the_shell(t):
    t.workspace(W + "shapp")

    t.run("echo first > note.txt")
    t.run("echo second >> note.txt")

    out = t.run("cat note.txt")
    expect_in("first", out, "the original line survived")
    expect_in("second", out, "the appended line is there")

    got = counts(t.run("wc note.txt"))
    if got is not None and got[0] < 2:
        raise AssertionError("an append left %d line(s), not 2" % got[0])


@test("output larger than the write buffer keeps every byte", needs=("help", "wc", "cat"))
def redirect_spans_the_write_buffer(t):
    """
    A file stream gathers bytes and commits a block at a time, so anything
    longer than one buffer exercises the boundary between blocks. `help` lists
    every registered command, which is far more than one buffer's worth.
    """
    t.workspace(W + "shbig")

    printed = t.run("help", timeout=max(t.timeout, 30))
    t.run("help > big.txt", timeout=max(t.timeout, 30))

    got = counts(t.run("wc big.txt", timeout=max(t.timeout, 30)))
    if got is None:
        raise Skip("wc did not report a count for the redirected file")

    # every command name that reached the terminal must be in the file too
    body = t.run("cat big.txt", timeout=max(t.timeout, 30))
    for token in ("help", "echo", "cat"):
        expect_in(token, body, "%s survived a multi-block redirect" % token)

    if got[2] < 200:
        raise AssertionError("a multi-block redirect wrote only %d bytes:\n%s"
                             % (got[2], printed[:200]))


@test("a pipeline bigger than the pipe does not wedge the shell", needs=("help", "wc"))
def pipeline_beyond_capacity_is_survivable(t):
    """
    A pipe is fixed capacity, as a linux one is, so a stage can outrun it. What
    must never happen is the shell being left unusable by it.
    """
    out = t.run("help | wc", timeout=max(t.timeout, 30))

    if counts(out) is None:
        raise AssertionError("a large pipeline printed no counts:\n%s" % out[:200])

    expect_in("/", t.run("pwd"), "the shell still answers after a large pipeline")


@test("two sessions can redirect at the same time", needs=("echo", "cat"))
def redirect_is_per_session(t):
    """
    Descriptors belong to a session, not to the board. Two sessions redirecting
    at once must not land in each other's file or each other's terminal.
    """
    peer = t.peer()
    try:
        t.run("echo fromfirst > /shared_a.txt")
        peer.run("echo fromsecond > /shared_b.txt", t.timeout)

        expect_in("fromfirst", t.run("cat /shared_a.txt"), "the first session's file")
        expect_not_in("fromsecond", t.run("cat /shared_a.txt"), "the other session's text")

        expect_in("fromsecond", peer.run("cat /shared_b.txt", t.timeout),
                  "the second session's file")
    finally:
        peer.close()
        t.run("rm /shared_a.txt")
        t.run("rm /shared_b.txt")


@test("redirecting many times does not leak", needs=("echo", "ps", "rm"), slow=True)
def redirect_releases_everything(t):
    """
    Every redirect allocates a descriptor table, an adapter and a file stream,
    and must give all three back. A leak shows as heap that never returns.
    """
    t.workspace(W + "shleak")

    # settle first: the first redirect of a session allocates what later ones reuse
    for _ in range(3):
        t.run("echo settle > leak.txt")

    top, band = heap_band(t)

    for _ in range(20):
        t.run("echo repeated > leak.txt")

    assert_no_leak(t, top, band, 20, "redirects")


@test("piping many times does not leak", needs=("echo", "wc", "ps"), slow=True)
def pipeline_releases_everything(t):
    """A pipe is allocated per stage and freed once the next stage has read it."""
    for _ in range(3):
        t.run("echo settle | wc")

    top, band = heap_band(t)

    for _ in range(15):
        t.run("echo repeated here | wc")

    assert_no_leak(t, top, band, 15, "pipelines")


@test("a redirect works on a memory filesystem too", needs=("echo", "cat"), mounts=("/tmp",))
def redirect_into_tmpfs(t):
    t.run("echo intmpfs > /tmp/redir.txt")

    expect_in("intmpfs", t.run("cat /tmp/redir.txt"), "a redirect into tmpfs")
    t.run("rm /tmp/redir.txt")


@test("a redirect operator typed into a waiting command is not a redirect",
      needs=("fedit", "cat", "rm"), mounts=("/",), slow=True)
def operator_inside_an_interactive_command(t):
    """
    The shell parses a line only when it is starting a command. A command that
    is already waiting for input gets the line raw, so an editor can hold a '>'
    without the shell stealing it into a file.
    """
    from ..driver.shell import PROMPT

    path = "/wt_shedit.txt"
    body = "a > b | c"
    t.run("rm %s" % path)

    t.shell.send_line("fedit %s" % path)
    t.shell.expect("ESC", t.timeout)
    t.shell.drain(0.5)

    t.shell.send_raw(body)
    t.shell.drain(0.5)
    t.shell.send_raw("\r")
    t.shell.drain(0.5)

    t.shell.send_raw("\x03")
    t.shell.expect("save", t.timeout)
    t.shell.send_line("!w")
    t.shell.expect("saved", t.timeout)
    t.shell.expect(PROMPT, t.timeout)

    try:
        # the operators are text the editor held, not a redirect the shell took
        expect_in(">", t.run("cat %s" % path), "the editor kept the operator")
        expect_in("|", t.run("cat %s" % path), "the editor kept the pipe")
    finally:
        t.run("rm %s" % path)


@test("a command reads the file it is given as a source", needs=("echo", "wc"))
def source_feeds_a_command(t):
    path = t.workspace(W + "shsrc") + "/in.txt"
    t.run("echo one > %s" % path)
    t.run("echo two >> %s" % path)

    got = counts(t.run("wc < %s" % path))
    if got is None:
        raise AssertionError("wc printed no counts when given a source")
    if got[0] != 2:
        raise AssertionError("wc counted %d lines from a source, not 2" % got[0])


@test("a source feeds the first stage of a pipeline", needs=("echo", "cat", "grep"))
def source_feeds_a_pipeline(t):
    path = t.workspace(W + "shsrcpipe") + "/in.txt"
    t.run("echo alpha > %s" % path)
    t.run("echo beta >> %s" % path)

    out = t.run("cat < %s | grep alpha" % path)
    expect_in("alpha", out, "the matching line came through the pipe")
    expect_not_in("beta", out, "the other line was filtered out")


@test("a source and a target work on one line", needs=("echo", "wc", "cat"))
def source_and_target_together(t):
    base = t.workspace(W + "shboth2")
    t.run("echo one > in.txt")
    t.run("echo two >> in.txt")

    out = t.run("wc < in.txt > out.txt")
    expect_not_in("2", out, "the terminal saw the count")

    expect_in("2", t.run("cat out.txt"), "the count landed in the target")


@test("a missing source is refused and the terminal comes back", needs=("wc", "echo"))
def source_missing_is_refused(t):
    t.workspace(W + "shsrcmiss")

    expect_in("cannot open", t.run("wc < absent.txt"), "a source that is not there")
    expect_in("still here", t.run("echo still here"), "the shell after a refused source")


@test("a dangling source operator is a syntax error", needs=("wc",))
def source_dangling(t):
    expect_in("syntax error", t.run("wc <"), "a source that was never named")


@test("a semicolon runs both commands", needs=("echo", "cat"))
def sequence_semicolon(t):
    t.workspace(W + "shseq")

    # the typed line is echoed back over every transport, so the markers are
    # read out of files whose names carry none of them
    t.run("echo aa > one.txt ; echo bb > two.txt")

    expect_in("aa", t.run("cat one.txt"), "the first segment ran")
    expect_in("bb", t.run("cat two.txt"), "the second segment ran")


@test("a semicolon runs the second command even after a failure", needs=("echo", "cat"))
def sequence_semicolon_after_failure(t):
    t.workspace(W + "shseqf")

    t.run("nosuchcommand ; echo cc > after.txt")

    expect_in("cc", t.run("cat after.txt"), "the segment after a failure")


@test("and-and skips the second command after a failure", needs=("echo", "ls"))
def sequence_and_skips(t):
    t.workspace(W + "shand")

    t.run("nosuchcommand && echo dd > guard.txt")

    expect_not_in("guard.txt", t.run("ls"), "the guarded segment must not have run")


@test("and-and runs the second command after success", needs=("echo", "cat", "pwd"))
def sequence_and_runs(t):
    t.workspace(W + "shand2")

    t.run("pwd && echo ee > ran.txt")

    expect_in("ee", t.run("cat ran.txt"), "the segment after a success")


@test("or-or runs the second command only after a failure", needs=("echo", "cat", "ls"))
def sequence_or(t):
    t.workspace(W + "shor")

    t.run("nosuchcommand || echo ff > fallback.txt")
    expect_in("ff", t.run("cat fallback.txt"), "the fallback after a failure")

    t.run("pwd || echo gg > unused.txt")
    expect_not_in("unused.txt", t.run("ls"), "the fallback after a success")


@test("the last exit status reads back through the question mark",
      needs=("echo", "cat", "pwd"))
def sequence_exit_status(t):
    t.workspace(W + "shstat")

    t.run("pwd ; echo $? > ok.txt")
    expect_in("0", t.run("cat ok.txt"), "the status of a command that worked")

    t.run("nosuchcommand ; echo $? > bad.txt")
    # CMD_ERROR_NOENT, the whole cmd band being -36xx
    expect_in("-36", t.run("cat bad.txt"), "the status of a command that failed")


@test("single quotes suppress the status expansion", needs=("echo", "cat", "pwd"))
def sequence_quotes_suppress(t):
    t.workspace(W + "shquote")

    t.run("pwd")
    t.run("echo '$?' > quoted.txt")

    expect_in("$?", t.run("cat quoted.txt"), "a single quoted word is literal")


@test("double quotes still expand the status", needs=("echo", "cat", "pwd"))
def sequence_double_quotes_expand(t):
    t.workspace(W + "shdq")

    t.run("pwd")
    t.run('echo "$?" > dq.txt')

    out = t.run("cat dq.txt")
    expect_in("0", out, "a double quoted word still expands")
    expect_not_in("$?", out, "the operator itself must not survive")


@test("a chain stops at the first failure", needs=("echo", "ls", "pwd"))
def sequence_chain_stops(t):
    t.workspace(W + "shchain")

    t.run("pwd && nosuchcommand && echo hh > third.txt")

    expect_not_in("third.txt", t.run("ls"), "the segment past the failure")


@test("a skipped segment leaves the status of the one that ran",
      needs=("echo", "cat", "pwd"))
def sequence_skipped_keeps_status(t):
    t.workspace(W + "shskip")

    # the second segment never runs, so the status is still nosuchcommand's
    t.run("nosuchcommand && pwd")
    t.run("echo $? > kept.txt")

    expect_in("-36", t.run("cat kept.txt"), "the status of the segment that ran")


@test("a pipeline still works inside a segment", needs=("echo", "wc", "cat", "pwd"))
def sequence_pipeline_inside(t):
    t.workspace(W + "shseqpipe")

    t.run("pwd && echo iii | wc > piped.txt")

    counted = counts(t.run("cat piped.txt"))
    if counted is None or counted[2] != 4:
        raise AssertionError("a pipeline inside a segment: %r" % counted)


@test("a redirect binds to its own segment, not to the line",
      needs=("echo", "cat", "ls"))
def sequence_redirect_binds_to_segment(t):
    t.workspace(W + "shseqredir")

    t.run("echo jj > left.txt ; echo kk")

    # the redirect belongs to the first segment alone
    expect_in("jj", t.run("cat left.txt"), "the first segment was redirected")
    expect_not_in("kk", t.run("cat left.txt"), "the second segment was not")


@test("a trailing separator is not a syntax error", needs=("echo", "cat"))
def sequence_trailing_separator(t):
    t.workspace(W + "shtrail")

    out = t.run("echo ll > trail.txt ;")
    expect_not_in("syntax error", out, "a line may end on a separator")

    expect_in("ll", t.run("cat trail.txt"), "the segment before it still ran")


@test("a dangling sequence operator is a syntax error", needs=("echo",))
def sequence_dangling_operator(t):
    expect_in("syntax error", t.run("echo mm &&"), "and-and with nothing after it")
    expect_in("syntax error", t.run("echo nn ||"), "or-or with nothing after it")


@test("a quoted operator is text, not an operator", needs=("echo", "cat", "ls"))
def sequence_quoted_operator_is_text(t):
    base = t.workspace(W + "shquoteop")

    # each of these used to be taken as the operator it names
    t.run('echo "a && b" > q1.txt')
    expect_in("&&", t.run("cat q1.txt"), "a quoted and-and is text")

    t.run('echo "a | b" > q2.txt')
    expect_in("|", t.run("cat q2.txt"), "a quoted pipe is text")

    t.run('echo "a ; b" > q3.txt')
    expect_in(";", t.run("cat q3.txt"), "a quoted separator is text")


@test("a quoted redirect operator does not redirect", needs=("echo", "ls"))
def sequence_quoted_redirect_is_text(t):
    t.workspace(W + "shquoteredir")

    out = t.run('echo "a > b"')

    # the shell must not have made a file out of the quoted word
    expect_not_in("b", t.run("ls"), "a quoted redirect must not create a file")
    expect_in(">", out, "the quoted operator is printed instead")


@test("a redirect on a middle pipeline stage takes that stage's output",
      needs=("echo", "wc", "cat"))
def pipeline_stage_redirect(t):
    t.workspace(W + "shstage")

    # the redirect is applied after the pipe, so the file takes the output and
    # the next stage reads an empty pipe
    out = t.run("echo oo > mid.txt | wc")

    counted = counts(out)
    if counted is None or counted != [0, 0, 0]:
        raise AssertionError("the downstream stage should read nothing: %r" % counted)

    expect_in("oo", t.run("cat mid.txt"), "the redirected stage wrote its file")


@test("and-or chains associate to the left", needs=("echo", "cat", "pwd", "ls"))
def sequence_left_association(t):
    t.workspace(W + "shassoc")

    # (nosuchcommand || pwd) && echo, so the last segment runs
    t.run("nosuchcommand || pwd && echo rr > assoc.txt")
    expect_in("rr", t.run("cat assoc.txt"), "and-and after a recovered failure")

    # (pwd || nosuchcommand) && echo, the or-or is skipped and the and-and runs
    t.run("pwd || nosuchcommand && echo ss > assoc2.txt")
    expect_in("ss", t.run("cat assoc2.txt"), "and-and after a skipped or-or")


@test("a line may not begin with an operator", needs=("echo", "wc"))
def sequence_leading_operator(t):
    expect_in("syntax error", t.run("&& echo tt"), "a leading and-and")
    expect_in("syntax error", t.run("|| echo uu"), "a leading or-or")
    expect_in("syntax error", t.run("; echo vv"), "a leading separator")
    expect_in("syntax error", t.run("| wc"), "a leading pipe")


@test("a doubled separator is a syntax error", needs=("echo",))
def sequence_doubled_separator(t):
    expect_in("syntax error", t.run("echo ww ;; echo xx"), "two separators in a row")


@test("an unterminated quote is a syntax error", needs=("echo",))
def sequence_unterminated_quote(t):
    expect_in("syntax error", t.run('echo "yy'), "a double quote never closed")
    expect_in("syntax error", t.run("echo 'zz"), "a single quote never closed")


@test("a line carries more than two segments", needs=("echo", "cat"))
def sequence_three_segments(t):
    t.workspace(W + "shthree")

    t.run("echo s1 > a1.txt ; echo s2 > a2.txt ; echo s3 > a3.txt")

    expect_in("s1", t.run("cat a1.txt"), "the first segment")
    expect_in("s2", t.run("cat a2.txt"), "the second segment")
    expect_in("s3", t.run("cat a3.txt"), "the third segment")


@test("an append inside a segment adds to what the first one wrote",
      needs=("echo", "wc"))
def sequence_append_inside(t):
    t.workspace(W + "shappseq")

    t.run("echo b1 > both.txt ; echo b2 >> both.txt")

    counted = counts(t.run("wc both.txt"))
    if counted is None or counted[0] != 2:
        raise AssertionError("the append should have added a second line: %r" % counted)


@test("the status expands inside a longer word", needs=("echo", "cat", "pwd"))
def sequence_status_inside_word(t):
    t.workspace(W + "shembed")

    t.run("pwd ; echo code=$? > embedded.txt")

    expect_in("code=0", t.run("cat embedded.txt"), "the status inside a word")


@test("the status after a pipeline is the last stage's", needs=("echo", "wc", "cat"))
def sequence_status_after_pipeline(t):
    t.workspace(W + "shpipestat")

    t.run("echo cc | wc ; echo $? > pstat.txt")

    expect_in("0", t.run("cat pstat.txt"), "a pipeline that worked")


@test("quotes are removed from what a command is given", needs=("echo", "wc", "cat"))
def quote_removal(t):
    t.workspace(W + "shqrem")

    # the quotes are the shell's own: they keep the spacing together and then
    # go, so the file holds four characters and a terminator, not six
    t.run('echo "a  b" > sp.txt')

    counted = counts(t.run("wc sp.txt"))
    if counted is None or counted[2] != 5:
        raise AssertionError("a quoted word should reach the file without its "
                             "quotes: %r" % counted)

    expect_in("a  b", t.run("cat sp.txt"), "the spacing inside the quotes is kept")


@test("an empty quoted word is empty, not two quote characters",
      needs=("echo", "wc"))
def empty_quoted_word(t):
    t.workspace(W + "shqempty")

    t.run('echo "" > empty.txt')

    counted = counts(t.run("wc empty.txt"))
    if counted is None or counted[2] != 1:
        raise AssertionError("an empty quoted word should write only the line "
                             "ending: %r" % counted)


@test("quotes are removed from the middle of a word too", needs=("echo", "cat"))
def quote_removal_interior(t):
    t.workspace(W + "shqmid")

    # the quoting is the shell's wherever it appears, not only around the whole
    t.run('echo a"b"c > mid.txt')
    expect_in("abc", t.run("cat mid.txt"), "an interior quoted run")

    t.run('echo x" "y > gap.txt')
    expect_in("x y", t.run("cat gap.txt"), "a quoted run holding a space")


@test("a backslash escapes a quote", needs=("echo", "cat"))
def quote_escape(t):
    t.workspace(W + "shqesc")

    t.run('echo a\\"b > esc.txt')
    expect_in('a"b', t.run("cat esc.txt"), "an escaped quote is the character")


@test("single quotes suppress expansion and double quotes do not",
      needs=("echo", "cat", "pwd"))
def quote_expansion_rules(t):
    t.workspace(W + "shqexp")

    t.run("pwd")
    t.run("echo 'lit $? here' > lit.txt")
    expect_in("$?", t.run("cat lit.txt"), "a single quoted run is literal")

    t.run('echo "exp $? here" > exp.txt')
    out = t.run("cat exp.txt")
    expect_in("exp 0 here", out, "a double quoted run still expands")
    expect_not_in("$?", out, "the operator itself does not survive")


@test("a redirect target may be quoted", needs=("echo", "cat"))
def quoted_redirect_target(t):
    t.workspace(W + "shqtgt")

    # the quotes belong to the shell, so the file is named by what they enclose
    t.run('echo hi > "my file.txt"')
    expect_in("hi", t.run('cat "my file.txt"'), "a quoted target names the file")
