#!/usr/bin/env python3

"""
find, du, stat, sort, tee and diff, each from several sides, and each against
the commands it is used with: a listing piped into sort or wc, a size checked
against stat and wc, a sorted copy diffed against its source, a pipeline split
by tee. Every result is read back through the shell, the way an operator sees it.
"""

from .registry import test, expect_in, expect_not_in

W = "cu_"
OK = "RC_OK"
NO = "RC_NO"


def rows(out):
    """The non-blank lines of an answer, trimmed."""
    return [line.strip() for line in out.splitlines() if line.strip()]


def status(t, command):
    """Whether a command succeeded, read through && and || rather than a raw code."""
    out = t.run("%s && echo %s || echo %s" % (command, OK, NO))
    if OK in out:
        return True
    if NO in out:
        return False
    raise AssertionError("no status came back from %r:\n%s" % (command, out))


def write_lines(t, path, lines):
    """A file holding exactly these lines, built with echo."""
    t.run("echo %s > %s" % (lines[0], path))
    for line in lines[1:]:
        t.run("echo %s >> %s" % (line, path))


def tree(t, name):
    """
    A workspace holding a.txt, notes.md, sub/b.txt and sub/deep/c.log, with the
    byte count of each file as echo writes it.
    """
    path = t.workspace(W + name)
    t.run("mkdir sub")
    t.run("mkdir sub/deep")
    t.run("echo alpha > a.txt")
    t.run("echo bravo charlie > notes.md")
    t.run("echo delta > sub/b.txt")
    t.run("echo echo-foxtrot > sub/deep/c.log")
    sizes = {"a.txt": 6, "notes.md": 14, "sub/b.txt": 6, "sub/deep/c.log": 13}
    return path, sizes


def stat_size(t, path):
    for line in rows(t.run("stat %s" % path)):
        if line.startswith("Size:"):
            return int(line.split()[1])
    raise AssertionError("stat %s gave no size" % path)


def wc_bytes(t, path):
    counts = t.run("wc %s" % path).split()
    if len(counts) < 3 or not counts[2].isdigit():
        raise AssertionError("wc %s gave no counts" % path)
    return int(counts[2])


# ---- find ---------------------------------------------------------------------

@test("find with no path lists the tree under the working directory, parents first",
      needs=("find", "mkdir", "echo"))
def find_lists_tree(t):
    tree(t, "find")
    got = rows(t.run("find"))
    for entry in (".", "./a.txt", "./notes.md", "./sub", "./sub/b.txt", "./sub/deep", "./sub/deep/c.log"):
        if entry not in got:
            raise AssertionError("find left out %s:\n%s" % (entry, "\n".join(got)))
    if got.index("./sub") > got.index("./sub/b.txt") or got.index("./sub/deep") > got.index("./sub/deep/c.log"):
        raise AssertionError("find listed a child before its directory:\n%s" % "\n".join(got))
    if got[0] != ".":
        raise AssertionError("find did not start with the directory it was given:\n%s" % "\n".join(got))


@test("find given an absolute path prints absolute paths", needs=("find", "mkdir", "echo"))
def find_absolute(t):
    path, _ = tree(t, "findabs")
    got = rows(t.run("find %s" % path))
    expect_in(path + "/sub/deep/c.log", "\n".join(got), "find with an absolute path")
    for line in got:
        if not line.startswith(path):
            raise AssertionError("find printed %r outside the path it was given" % line)


@test("find -name matches the last part of each path at every depth", needs=("find", "mkdir", "echo"))
def find_name_glob(t):
    tree(t, "findname")
    got = rows(t.run("find . -name '*.txt'"))
    if sorted(got) != ["./a.txt", "./sub/b.txt"]:
        raise AssertionError("find -name '*.txt' gave:\n%s" % "\n".join(got))
    if rows(t.run("find . -name 'c.?og'")) != ["./sub/deep/c.log"]:
        raise AssertionError("find -name with ? did not find c.log")
    if sorted(rows(t.run("find . -name '[ab].txt'"))) != ["./a.txt", "./sub/b.txt"]:
        raise AssertionError("find -name with a set did not find both")
    if rows(t.run("find . -name deep")) != ["./sub/deep"]:
        raise AssertionError("find -name did not match a directory by name")
    if rows(t.run("find . -name 'nothing*'")):
        raise AssertionError("find -name matched something that is not there")


@test("find -type splits the tree into files and directories that add up to it",
      needs=("find", "mkdir", "echo"))
def find_type(t):
    tree(t, "findtype")
    files = rows(t.run("find . -type f"))
    dirs = rows(t.run("find . -type d"))
    whole = rows(t.run("find ."))
    if sorted(dirs) != [".", "./sub", "./sub/deep"]:
        raise AssertionError("find -type d gave:\n%s" % "\n".join(dirs))
    if len(files) != 4 or set(files) & set(dirs):
        raise AssertionError("find -type f gave:\n%s" % "\n".join(files))
    if sorted(files + dirs) != sorted(whole):
        raise AssertionError("files and directories do not add up to the whole listing")
    if rows(t.run("find . -type f -name '*.log'")) != ["./sub/deep/c.log"]:
        raise AssertionError("find with both -type and -name did not narrow to c.log")
    if rows(t.run("find . -name sub -type f")):
        raise AssertionError("find -type f matched the directory sub")


@test("find given one file prints that file alone", needs=("find", "mkdir", "echo"))
def find_one_file(t):
    tree(t, "findone")
    if rows(t.run("find a.txt")) != ["a.txt"]:
        raise AssertionError("find on a file did not print just the file")


@test("find refuses a missing path, an unknown predicate and a bad type",
      needs=("find", "mkdir", "echo"))
def find_refusals(t):
    tree(t, "findbad")
    if status(t, "find nowhere"):
        raise AssertionError("find on a missing path succeeded")
    if status(t, "find . -size 3"):
        raise AssertionError("find took an unknown predicate")
    if status(t, "find . -type x"):
        raise AssertionError("find took an unknown type")
    if status(t, "find . -name"):
        raise AssertionError("find took -name without a pattern")
    if status(t, "find . sub"):
        raise AssertionError("find took a path after the expression")
    if not status(t, "find . -name nothing"):
        raise AssertionError("find with no match did not succeed")


@test("find piped into wc counts the entries and into sort orders them",
      needs=("find", "wc", "sort", "mkdir", "echo"))
def find_into_wc_and_sort(t):
    tree(t, "findpipe")
    counts = t.run("find . -type f | wc").split()
    if not counts or counts[0] != "4":
        raise AssertionError("find | wc counted %r lines, not 4" % counts[:1])
    ordered = rows(t.run("find . | sort"))
    if ordered != sorted(rows(t.run("find ."))):
        raise AssertionError("find | sort is not the sorted listing:\n%s" % "\n".join(ordered))


@test("find written to a file reads back the same and diffs clean against a second run",
      needs=("find", "diff", "cat", "mkdir", "echo"))
def find_into_file(t):
    tree(t, "findfile")
    t.run("find sub > ../%slist1" % W)
    t.run("find sub > ../%slist2" % W)
    try:
        if rows(t.run("cat ../%slist1" % W)) != rows(t.run("find sub")):
            raise AssertionError("find written to a file did not read back the same")
        if not status(t, "diff ../%slist1 ../%slist2" % (W, W)):
            raise AssertionError("two listings of the same tree differ")
    finally:
        t.run("rm ../%slist1" % W)
        t.run("rm ../%slist2" % W)


# ---- du -----------------------------------------------------------------------

def du_rows(t, command):
    found = {}
    order = []
    for line in rows(t.run(command)):
        parts = line.split()
        if len(parts) == 2 and parts[0].isdigit():
            found[parts[1]] = int(parts[0])
            order.append(parts[1])
    return found, order


@test("du -b adds the files under each directory into it, children listed first",
      needs=("du", "mkdir", "echo"))
def du_bytes_tree(t):
    _, sizes = tree(t, "du")
    found, order = du_rows(t, "du -b")
    deep = sizes["sub/deep/c.log"]
    sub = deep + sizes["sub/b.txt"]
    whole = sub + sizes["a.txt"] + sizes["notes.md"]
    want = {"./sub/deep": deep, "./sub": sub, ".": whole}
    if found != want:
        raise AssertionError("du -b gave %r, wanted %r" % (found, want))
    if order != ["./sub/deep", "./sub", "."]:
        raise AssertionError("du did not list children before their parent: %r" % order)


@test("du -s prints only the total, the same as the last line of du",
      needs=("du", "mkdir", "echo"))
def du_summary(t):
    tree(t, "dus")
    full, order = du_rows(t, "du -b")
    summary, sorder = du_rows(t, "du -sb")
    if sorder != ["."] or summary["."] != full["."]:
        raise AssertionError("du -sb gave %r against du -b %r" % (summary, full))
    combined, _ = du_rows(t, "du -s -b")
    if combined != summary:
        raise AssertionError("du -s -b and du -sb disagree: %r %r" % (combined, summary))


@test("du counts each file by the blocks it takes unless asked for bytes", needs=("du", "mkdir", "echo"))
def du_kib(t):
    t.workspace(W + "dukib")
    t.run("echo z > one")
    found, _ = du_rows(t, "du")
    block = found.get(".")
    if not block or block & (block - 1):
        raise AssertionError("du of a 2 byte file reported %r KiB, not one whole block" % found)
    t.run("echo y > two")
    found, _ = du_rows(t, "du")
    if found.get(".") != 2 * block:
        raise AssertionError("two 2 byte files reported %r KiB, not two blocks of %d" % (found, block))
    found, _ = du_rows(t, "du -b")
    if found.get(".") != 4:
        raise AssertionError("du -b of two 2 byte files reported %r" % found)


@test("du of one file agrees with stat and wc", needs=("du", "stat", "wc", "mkdir", "echo"))
def du_file_agrees(t):
    tree(t, "dufile")
    found, _ = du_rows(t, "du -b notes.md")
    if found.get("notes.md") != stat_size(t, "notes.md") or found.get("notes.md") != wc_bytes(t, "notes.md"):
        raise AssertionError("du -b, stat and wc disagree on notes.md: %r" % found)


@test("du goes down when a file is removed and up when one grows",
      needs=("du", "rm", "mkdir", "echo"))
def du_follows_changes(t):
    tree(t, "duchg")
    before, _ = du_rows(t, "du -sb")
    t.run("rm a.txt")
    after, _ = du_rows(t, "du -sb")
    if after["."] != before["."] - 6:
        raise AssertionError("removing a 6 byte file moved du from %d to %d" % (before["."], after["."]))
    t.run("echo grown >> notes.md")
    grown, _ = du_rows(t, "du -sb")
    if grown["."] != after["."] + 6:
        raise AssertionError("appending 6 bytes moved du from %d to %d" % (after["."], grown["."]))


@test("du refuses a missing path, a bad option and a second path", needs=("du", "mkdir", "echo"))
def du_refusals(t):
    tree(t, "dubad")
    if status(t, "du nowhere"):
        raise AssertionError("du on a missing path succeeded")
    if status(t, "du -x"):
        raise AssertionError("du took an unknown option")
    if status(t, "du a.txt sub"):
        raise AssertionError("du took a second path")


# ---- stat ---------------------------------------------------------------------

@test("stat names a file, its size and type, and the mode a new file gets",
      needs=("stat", "wc", "touch", "echo", "mkdir"))
def stat_file(t):
    t.workspace(W + "stat")
    t.run("echo four > f")
    out = t.run("stat f")
    expect_in("File: f", out, "stat")
    expect_in("regular file", out, "stat")
    expect_in("-rw-", out, "stat mode")
    if stat_size(t, "f") != wc_bytes(t, "f"):
        raise AssertionError("stat and wc disagree on the size")
    expect_in("Modify:", out, "stat")


@test("stat follows a chmod", needs=("stat", "chmod", "echo", "mkdir"))
def stat_after_chmod(t):
    t.workspace(W + "statmod")
    t.run("echo x > f")
    t.run("chmod 600 f")
    out = t.run("stat f")
    expect_in("(0600/-rw-------)", out, "stat after chmod 600")
    t.run("chmod 644 f")
    expect_in("(0644/-rw-r--r--)", t.run("stat f"), "stat after chmod 644")


@test("stat calls a directory one and names the owner whoami gives",
      needs=("stat", "whoami", "mkdir"))
def stat_directory_owner(t):
    t.workspace(W + "statdir")
    t.run("mkdir inner")
    out = t.run("stat inner")
    expect_in("directory", out, "stat on a directory")
    expect_in("drwx", out, "stat directory mode")
    me = [l for l in rows(t.run("whoami")) if l][-1]
    expect_in("/%s)" % me, out, "stat owner")


@test("stat answers every path it can and fails for the one it cannot",
      needs=("stat", "echo", "mkdir"))
def stat_several(t):
    t.workspace(W + "statmany")
    t.run("echo a > one")
    t.run("echo bb > two")
    out = t.run("stat one nowhere two")
    expect_in("File: one", out, "stat of several")
    expect_in("File: two", out, "stat of several")
    expect_in("nowhere", out, "stat names the missing path")
    if status(t, "stat one nowhere"):
        raise AssertionError("stat with a missing path succeeded")
    if not status(t, "stat one two"):
        raise AssertionError("stat of two files failed")
    if status(t, "stat"):
        raise AssertionError("stat with no path succeeded")


# ---- sort ---------------------------------------------------------------------

FRUIT = ["pear", "apple", "10", "9", "apple", "Banana"]


@test("sort orders by bytes, reverses with -r and drops repeats with -u",
      needs=("sort", "echo", "mkdir"))
def sort_plain(t):
    t.workspace(W + "sort")
    write_lines(t, "f", FRUIT)
    if rows(t.run("sort f")) != sorted(FRUIT):
        raise AssertionError("sort gave:\n%s" % t.run("sort f"))
    if rows(t.run("sort -r f")) != sorted(FRUIT, reverse=True):
        raise AssertionError("sort -r gave:\n%s" % t.run("sort -r f"))
    if rows(t.run("sort -u f")) != sorted(set(FRUIT)):
        raise AssertionError("sort -u gave:\n%s" % t.run("sort -u f"))
    if rows(t.run("sort -ru f")) != sorted(set(FRUIT), reverse=True):
        raise AssertionError("sort -ru gave:\n%s" % t.run("sort -ru f"))


@test("sort -n orders by the leading number, negatives and fractions included",
      needs=("sort", "echo", "mkdir"))
def sort_numeric(t):
    t.workspace(W + "sortn")
    write_lines(t, "n", ["10", "-3", "2.5", "100", "2", "-10", "0"])
    if rows(t.run("sort -n n")) != ["-10", "-3", "0", "2", "2.5", "10", "100"]:
        raise AssertionError("sort -n gave:\n%s" % t.run("sort -n n"))
    if rows(t.run("sort -rn n")) != ["100", "10", "2.5", "2", "0", "-3", "-10"]:
        raise AssertionError("sort -rn gave:\n%s" % t.run("sort -rn n"))
    if rows(t.run("sort n"))[:2] != ["-10", "-3"]:
        raise AssertionError("sort without -n did not order by bytes")


@test("sort -n puts lines with no number first and keeps equal numbers apart by text",
      needs=("sort", "echo", "mkdir"))
def sort_numeric_ties(t):
    t.workspace(W + "sortties")
    write_lines(t, "m", ["5 b", "x", "5 a", "1"])
    if rows(t.run("sort -n m")) != ["x", "1", "5 a", "5 b"]:
        raise AssertionError("sort -n gave:\n%s" % t.run("sort -n m"))
    if rows(t.run("sort -nu m")) != ["x", "1", "5 b"]:
        raise AssertionError("sort -nu did not keep the first of the equal numbers:\n%s" % t.run("sort -nu m"))


@test("sort reads a pipe the same as a file and feeds head", needs=("sort", "cat", "head", "echo", "mkdir"))
def sort_pipes(t):
    t.workspace(W + "sortpipe")
    write_lines(t, "f", FRUIT)
    if rows(t.run("cat f | sort")) != rows(t.run("sort f")):
        raise AssertionError("cat f | sort differs from sort f")
    if rows(t.run("sort f | head 2")) != sorted(FRUIT)[:2]:
        raise AssertionError("sort | head 2 gave:\n%s" % t.run("sort f | head 2"))
    if rows(t.run("cat f | sort -r | sort")) != sorted(FRUIT):
        raise AssertionError("sorting a reversed sort did not give the sorted file")


@test("sort written to a file is the sorted file, and sort of an empty file is empty",
      needs=("sort", "cat", "touch", "echo", "mkdir"))
def sort_into_file(t):
    t.workspace(W + "sortfile")
    write_lines(t, "f", FRUIT)
    t.run("sort f > s")
    if rows(t.run("cat s")) != sorted(FRUIT):
        raise AssertionError("sort > file did not hold the sorted lines")
    t.run("touch empty")
    if rows(t.run("sort empty")):
        raise AssertionError("sort of an empty file printed something")
    if not status(t, "sort empty"):
        raise AssertionError("sort of an empty file failed")


@test("sort refuses a bad option, a missing file and a second file", needs=("sort", "echo", "mkdir"))
def sort_refusals(t):
    t.workspace(W + "sortbad")
    t.run("echo a > f")
    if status(t, "sort -z f"):
        raise AssertionError("sort took an unknown option")
    if status(t, "sort nowhere"):
        raise AssertionError("sort of a missing file succeeded")
    if status(t, "sort f f"):
        raise AssertionError("sort took a second file")
    if status(t, "sort"):
        raise AssertionError("sort with no input succeeded")


# ---- tee ----------------------------------------------------------------------

@test("tee passes its input on and writes it to the file", needs=("tee", "cat", "echo", "mkdir"))
def tee_writes(t):
    t.workspace(W + "tee")
    out = t.run("echo hello there | tee f")
    expect_in("hello there", out, "tee passes the input on")
    if rows(t.run("cat f")) != ["hello there"]:
        raise AssertionError("tee did not write the input to the file")


@test("tee truncates by default and appends with -a", needs=("tee", "cat", "echo", "mkdir"))
def tee_append(t):
    t.workspace(W + "teea")
    t.run("echo old > f")
    t.run("echo new | tee f")
    if rows(t.run("cat f")) != ["new"]:
        raise AssertionError("tee without -a did not replace the file")
    t.run("echo more | tee -a f")
    if rows(t.run("cat f")) != ["new", "more"]:
        raise AssertionError("tee -a did not append:\n%s" % t.run("cat f"))


@test("tee into two files leaves two copies that diff clean", needs=("tee", "diff", "cat", "echo", "mkdir"))
def tee_two_files(t):
    t.workspace(W + "tee2")
    write_lines(t, "src", FRUIT)
    t.run("cat src | tee one two")
    if not status(t, "diff one two"):
        raise AssertionError("the two copies tee wrote differ")
    if not status(t, "diff src one"):
        raise AssertionError("the copy tee wrote differs from its input")


@test("tee splits a pipeline: the file holds what passed and the next stage counts it",
      needs=("tee", "sort", "wc", "cat", "echo", "mkdir"))
def tee_mid_pipeline(t):
    t.workspace(W + "teepipe")
    write_lines(t, "src", FRUIT)
    counts = t.run("cat src | sort | tee sorted | wc").split()
    if not counts or counts[0] != str(len(FRUIT)):
        raise AssertionError("wc after tee counted %r lines" % counts[:1])
    if rows(t.run("cat sorted")) != sorted(FRUIT):
        raise AssertionError("the file tee wrote mid pipeline is not the sorted lines")


@test("tee with no file passes through and with a bad path still passes the input on",
      needs=("tee", "echo", "mkdir"))
def tee_edges(t):
    t.workspace(W + "teeedge")
    expect_in("through", t.run("echo through | tee"), "tee with no file")
    out = t.run("echo kept | tee nodir/f")
    expect_in("kept", out, "tee with a bad path")
    if status(t, "echo kept | tee nodir/f"):
        raise AssertionError("tee into a missing directory succeeded")
    if status(t, "echo x | tee -q f"):
        raise AssertionError("tee took an unknown option")


# ---- diff ---------------------------------------------------------------------

@test("diff of identical files prints nothing and succeeds", needs=("diff", "cp", "echo", "mkdir"))
def diff_same(t):
    t.workspace(W + "diffsame")
    write_lines(t, "a", FRUIT)
    t.run("cp a b")
    if rows(t.run("diff a b")):
        raise AssertionError("diff of a file and its copy printed something")
    if not status(t, "diff a b"):
        raise AssertionError("diff of identical files did not succeed")


@test("diff writes change, add and delete hunks in the normal format", needs=("diff", "echo", "mkdir"))
def diff_hunks(t):
    t.workspace(W + "diffhunk")
    write_lines(t, "f", ["pear", "apple", "10", "9", "apple"])
    write_lines(t, "g", ["banana", "apple", "10", "7"])
    if rows(t.run("diff f g")) != ["1c1", "< pear", "---", "> banana",
                                   "4,5c4", "< 9", "< apple", "---", "> 7"]:
        raise AssertionError("diff f g gave:\n%s" % t.run("diff f g"))
    if status(t, "diff f g"):
        raise AssertionError("diff of differing files succeeded")
    write_lines(t, "h", ["pear", "apple", "10", "9", "apple", "extra"])
    if rows(t.run("diff f h")) != ["5a6", "> extra"]:
        raise AssertionError("diff of an added line gave:\n%s" % t.run("diff f h"))
    if rows(t.run("diff h f")) != ["6d5", "< extra"]:
        raise AssertionError("diff of a removed line gave:\n%s" % t.run("diff h f"))


@test("diff against an empty file adds or deletes every line", needs=("diff", "touch", "echo", "mkdir"))
def diff_empty(t):
    t.workspace(W + "diffempty")
    t.run("touch e")
    write_lines(t, "f", ["one", "two"])
    if rows(t.run("diff e f")) != ["0a1,2", "> one", "> two"]:
        raise AssertionError("diff empty file gave:\n%s" % t.run("diff e f"))
    if rows(t.run("diff f e")) != ["1,2d0", "< one", "< two"]:
        raise AssertionError("diff to empty file gave:\n%s" % t.run("diff f e"))
    if not status(t, "diff e e"):
        raise AssertionError("diff of an empty file with itself did not succeed")


@test("diff of a file and its sorted copy shows the moves, and sorting both makes them equal",
      needs=("diff", "sort", "echo", "mkdir"))
def diff_with_sort(t):
    t.workspace(W + "diffsort")
    write_lines(t, "f", ["c", "a", "b"])
    t.run("sort f > s")
    if status(t, "diff f s"):
        raise AssertionError("an unsorted file and its sorted copy did not differ")
    t.run("sort s > s2")
    if not status(t, "diff s s2"):
        raise AssertionError("sorting a sorted file changed it")


@test("diff decides && and || in a script line", needs=("diff", "cp", "echo", "mkdir"))
def diff_in_shell(t):
    t.workspace(W + "diffsh")
    t.run("echo a > x")
    t.run("cp x y")
    expect_in("SAME", t.run("diff x y && echo SAME || echo DIFFER"), "diff of copies")
    t.run("echo b >> y")
    out = t.run("diff x y && echo SAME || echo DIFFER")
    expect_in("DIFFER", out, "diff after an append")
    expect_in("1a2", out, "diff after an append")
    expect_not_in("SAME", out, "diff after an append")


@test("diff refuses a missing file, a directory and a missing argument", needs=("diff", "echo", "mkdir"))
def diff_refusals(t):
    t.workspace(W + "diffbad")
    t.run("echo a > x")
    t.run("mkdir d")
    if status(t, "diff x nowhere"):
        raise AssertionError("diff with a missing file succeeded")
    if status(t, "diff x d"):
        raise AssertionError("diff with a directory succeeded")
    if status(t, "diff x"):
        raise AssertionError("diff with one file succeeded")


# ---- across all of them ---------------------------------------------------------

@test("find, du, stat and wc agree on every file in a tree", needs=("find", "du", "stat", "wc", "mkdir", "echo"))
def all_agree_on_sizes(t):
    _, sizes = tree(t, "agree")
    total = 0
    for path in rows(t.run("find . -type f")):
        size = stat_size(t, path)
        if size != wc_bytes(t, path):
            raise AssertionError("stat and wc disagree on %s" % path)
        total += size
    found, _ = du_rows(t, "du -sb")
    if found.get(".") != total or total != sum(sizes.values()):
        raise AssertionError("du -sb %r against the files' own sizes %d" % (found, total))


@test("a tree listed, sorted, saved by tee and diffed against a fresh sorted listing matches",
      needs=("find", "sort", "tee", "diff", "wc", "mkdir", "echo"))
def all_in_one_pipeline(t):
    tree(t, "pipeall")
    counts = t.run("find . -type f | sort | tee ../%sl1 | wc" % W).split()
    try:
        if not counts or counts[0] != "4":
            raise AssertionError("the pipeline counted %r files" % counts[:1])
        t.run("find . -type f | sort > ../%sl2" % W)
        if not status(t, "diff ../%sl1 ../%sl2" % (W, W)):
            raise AssertionError("the tee copy and a fresh sorted listing differ:\n%s"
                                 % t.run("diff ../%sl1 ../%sl2" % (W, W)))
    finally:
        t.run("rm ../%sl1" % W)
        t.run("rm ../%sl2" % W)


@test("an empty name given to find, du, stat, sort and diff is a path that is not there, and stat reads the names after it",
      needs=("find", "du", "stat", "sort", "diff", "echo", "mkdir"))
def empty_name_is_not_absent(t):
    t.workspace(W + "empty")
    t.run("echo alpha > a.txt")
    for command in ('find ""', 'du ""', 'sort ""', 'diff "" a.txt', 'diff a.txt ""', 'stat ""'):
        if status(t, command):
            raise AssertionError("%s succeeded, as if no name had been given" % command)
    expect_in("Size:", t.run('stat "" a.txt'), "stat of the name after an empty one")
    expect_in("alpha", t.run("sort a.txt"), "sort of a file after the empty-name checks")
