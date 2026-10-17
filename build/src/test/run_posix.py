#!/usr/bin/env python3
# Minimal tests for the KAWARI binaries built on POSIX (kosui and libshiori.so / .bundle).
#
# The same case files as run.ps1 are used:
#   kosui   feeds one line to kosui and compares its output (test/kosui/cases.txt)
#   shiori  loads the shared library with ctypes, sends the SHIORI/3.0 requests of
#           test/shiori/cases.txt in order and checks the responses
#   utf8    a few requests with Charset: UTF-8 and non-ASCII references
#
#   python3 build/src/test/run_posix.py [--bin-dir build/mach/linux] [--suite all|kosui|shiori|utf8]
#
# Exit codes: 0 = all passed, 1 = a case failed, 2 = the tests could not run.

import argparse
import ctypes
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
passed = 0
failed = 0


def stop_setup(message):
    print("test: CANNOT RUN - " + message)
    sys.exit(2)


def result(ok, name, detail=()):
    global passed, failed
    if ok:
        passed += 1
        print("ok  " + name)
    else:
        failed += 1
        print("NG  " + name)
        for d in detail:
            print("      " + d)


def show(text):
    return text.replace("\r", "\\r").replace("\n", "\\n")


# A line that starts with expect_mark adds an expected line to the current case,
# any other line (not blank, not "#") starts a new case ("<verb> <rest>").
def read_cases(path, expect_mark, verbs):
    cases = []
    current = None
    with open(path, encoding="latin-1") as f:
        for line_no, line in enumerate(f.read().splitlines(), 1):
            if line.strip() == "" or line.startswith("#"):
                continue
            if line.startswith(expect_mark):
                if current is None:
                    stop_setup("%s:%d: expected text before any case" % (path, line_no))
                text = line[1:]
                if text.startswith(" "):
                    text = text[1:]
                current["expected"].append(text)
                continue
            parts = line.split(None, 1)
            if parts[0] not in verbs:
                stop_setup("%s:%d: unknown verb '%s'" % (path, line_no, parts[0]))
            current = {"line": line_no, "verb": parts[0], "rest": parts[1] if len(parts) > 1 else "", "expected": []}
            cases.append(current)
    for case in cases:
        if not case["expected"]:
            stop_setup("%s:%d: the case has no expected text" % (path, case["line"]))
    return cases


# ---- kosui --------------------------------------------------------------------------------

def test_kosui(bin_dir, work, timeout):
    exe = os.path.join(bin_dir, "kosui")
    if not os.path.isfile(exe):
        stop_setup("kosui was not found in " + bin_dir)
    fixture = os.path.join(HERE, "kosui")
    cases = read_cases(os.path.join(fixture, "cases.txt"), "=", ("echo", "cmd"))

    print("== kosui (%d cases)" % len(cases))
    banner = "'.' is mode change command.\n"
    for index, case in enumerate(cases, 1):
        name = "kosui:%d %s %s" % (case["line"], case["verb"], case["rest"])
        d = os.path.join(work, "kosui%d" % index)
        os.mkdir(d)
        shutil.copy(os.path.join(fixture, "kawarirc.kis"), d)
        if case["verb"] == "cmd":
            session = ".\n%s\nexit\n" % case["rest"]
        else:
            session = "%s\nexit\n" % case["rest"]
        try:
            run = subprocess.run([exe], cwd=d, input=session.encode("latin-1"),
                                 stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=timeout)
        except subprocess.TimeoutExpired:
            result(False, name, ["kosui did not finish within %d seconds" % timeout])
            continue
        out = run.stdout.decode("latin-1").replace("\r\n", "\n")
        expected = "\n".join(case["expected"])
        pos = out.find(banner)
        if pos < 0:
            result(False, name, ["kosui did not reach its prompt", "output: " + show(out)])
            continue
        rest = out[pos + len(banner):]
        m = re.match(r"(?s)^(?:(?:echo|command)-mode > )+(.*?)\n(?:echo|command)-mode > $", rest)
        if not m:
            result(False, name, ["could not find the result in the output", "output: " + show(rest)])
            continue
        actual = m.group(1)
        result(actual == expected, name, ["expected: " + show(expected), "actual:   " + show(actual)])


# ---- shiori -------------------------------------------------------------------------------

class Shiori:
    def __init__(self, lib_path, ghost_dir):
        self.libc = ctypes.CDLL(None)
        self.libc.malloc.restype = ctypes.c_void_p
        self.libc.malloc.argtypes = [ctypes.c_size_t]
        self.libc.free.argtypes = [ctypes.c_void_p]
        self.lib = ctypes.CDLL(lib_path)
        self.lib.load.restype = ctypes.c_int
        self.lib.load.argtypes = [ctypes.c_void_p, ctypes.c_long]
        self.lib.request.restype = ctypes.c_void_p
        self.lib.request.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_long)]
        self.lib.unload.restype = ctypes.c_int
        path = ghost_dir.rstrip("/") + "/"
        if not self.lib.load(self._alloc(path.encode("utf-8")), len(path.encode("utf-8"))):
            stop_setup("load() returned FALSE")

    def _alloc(self, data):
        # The module frees the memory of load() and request() with free().
        p = self.libc.malloc(len(data) + 1)
        ctypes.memmove(p, data + b"\0", len(data) + 1)
        return p

    def request(self, data):
        length = ctypes.c_long(len(data))
        p = self.lib.request(self._alloc(data), ctypes.byref(length))
        if not p:
            return b""
        res = ctypes.string_at(p, length.value)
        self.libc.free(p)
        return res

    def unload(self):
        self.lib.unload()


def make_request(verb, ident, refs, charset):
    headers = ["%s SHIORI/3.0" % verb, "Charset: " + charset, "Sender: SSP",
               "SecurityLevel: local", "ID: " + ident]
    for n, ref in enumerate(refs):
        headers.append("Reference%d: %s" % (n, ref))
    return "\r\n".join(headers) + "\r\n\r\n"


def find_library(bin_dir):
    for name in ("libshiori.so", "libshiori.bundle", "libshiori.dylib"):
        p = os.path.join(bin_dir, name)
        if os.path.isfile(p):
            return p
    stop_setup("libshiori.so / libshiori.bundle was not found in " + bin_dir)


def make_ghost(work, name):
    ghost = os.path.join(work, name)
    os.mkdir(ghost)
    shutil.copy(os.path.join(HERE, "shiori", "kawarirc.kis"), ghost)
    return ghost


def test_shiori(bin_dir, work):
    cases = read_cases(os.path.join(HERE, "shiori", "cases.txt"), "~", ("GET", "NOTIFY"))
    shiori = Shiori(find_library(bin_dir), make_ghost(work, "ghost"))
    print("== shiori (%d cases)" % len(cases))
    for case in cases:
        parts = case["rest"].split(None, 1)
        refs = [r.strip() for r in parts[1].split(",")] if len(parts) > 1 else []
        req = make_request(case["verb"], parts[0], refs, "Shift_JIS")
        response = shiori.request(req.encode("latin-1")).decode("latin-1")
        name = "shiori:%d %s %s" % (case["line"], case["verb"], case["rest"])
        missing = [t for t in case["expected"] if t not in response]
        detail = ["missing: " + t for t in missing]
        if missing:
            detail.append("response: " + show(response))
        result(not missing, name, detail)
    shiori.unload()


# Non-ASCII text through the SHIORI layer: decoded by the Charset of the request and
# answered in the same charset.
def test_utf8(bin_dir, work):
    # The ghost folder name is not ASCII either (the path is given to load() as UTF-8).
    shiori = Shiori(find_library(bin_dir), make_ghost(work, "ghost-\u3042\u8868"))
    print("== utf8")
    r0 = "\u3042\u8868\u2600\u00e9\ud55c"
    for charset, encoding, text in (("UTF-8", "utf-8", r0), ("Shift_JIS", "cp932", "\u3042\u8868\u30bd")):
        req = make_request("GET", "OnRef", [text, "x"], charset)
        response = shiori.request(req.encode(encoding))
        try:
            decoded = response.decode(encoding)
        except UnicodeDecodeError:
            decoded = response.decode("latin-1")
        name = "utf8: GET OnRef (Charset: %s)" % charset
        want = ["SHIORI/3.0 200 OK", "Charset: " + charset, "Value: r0=%s r1=x" % text]
        missing = [t for t in want if t not in decoded]
        detail = ["missing: " + t.encode("unicode_escape").decode("ascii") for t in missing]
        if missing:
            detail.append("response: " + show(decoded.encode("unicode_escape").decode("ascii")))
        result(not missing, name, detail)
    shiori.unload()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--bin-dir", default=None)
    ap.add_argument("--suite", default="all", choices=("all", "kosui", "shiori", "utf8"))
    ap.add_argument("--timeout", type=int, default=30)
    args = ap.parse_args()

    bin_dir = args.bin_dir
    if not bin_dir:
        mach = "darwin" if sys.platform == "darwin" else "linux"
        bin_dir = os.path.join(HERE, "..", "..", "mach", mach)
    if not os.path.isdir(bin_dir):
        stop_setup("bin dir does not exist: " + bin_dir)
    bin_dir = os.path.abspath(bin_dir)

    work = tempfile.mkdtemp(prefix="kawari-test-")
    try:
        if args.suite in ("all", "kosui"):
            test_kosui(bin_dir, work, args.timeout)
        # Each suite loads the library once; the module keeps one instance per process,
        # so the SHIORI suites run in child processes.
        for suite in ("shiori", "utf8"):
            if args.suite in ("all", suite):
                if args.suite == "all":
                    run = subprocess.run([sys.executable, os.path.abspath(__file__), "--bin-dir", bin_dir, "--suite", suite])
                    if run.returncode == 2:
                        sys.exit(2)
                    count_child(run.returncode)
                else:
                    (test_shiori if suite == "shiori" else test_utf8)(bin_dir, work)
    finally:
        shutil.rmtree(work, ignore_errors=True)

    if args.suite == "all":
        print("")
        print("test: %s" % ("all passed" if not child_failed and failed == 0 else "some cases failed"))
        print("kosui: %d passed, %d failed" % (passed, failed))
        sys.exit(1 if (failed or child_failed) else 0)
    print("")
    print("test: %d passed, %d failed" % (passed, failed))
    sys.exit(1 if failed else 0)


child_failed = False


def count_child(returncode):
    global child_failed
    if returncode != 0:
        child_failed = True


if __name__ == "__main__":
    main()
