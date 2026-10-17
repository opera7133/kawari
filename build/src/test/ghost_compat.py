#!/usr/bin/env python3
# Compatibility test of two SHIORI DLLs (for example KAWARI 8.x and 9.x) on a real ghost.
#
#   python build/src/test/ghost_compat.py --ghost <ghost>/ghost/master [--old-dll <dll>] [--new-dll <dll>]
#
# The ghost is never touched: it is copied for every check. In the copies
#   * every program (*.exe, *.bat ...) is deleted, and
#   * every DLL except shiori.dll (SAORI and so on) is replaced by harness/stub_saori.c,
#     which answers "Result: 0" and records the requests it gets,
# so the events of the ghost cannot start programs or open windows. Use --real-saori to keep
# a harmless SAORI (a comma separated list of file names) when a ghost needs its answers.
#
# Checks (the DLLs are loaded one at a time by harness/shiori_harness.c in interactive mode):
#   replay    the old DLL walks through the ghost (SSP-like events, then the \q[] / \_a[] links of
#             the answers, depth first); the new DLL gets the same requests. The answers must be
#             the same. The random seed is fixed ("srand"), so the answers are reproducible.
#   saori     the requests that reached the SAORI must be the same (legacy SAORI: no Charset in
#             its answer, so Shift_JIS; only the Charset of "GET Version" differs on purpose)
#   utf8      the new DLL gets the requests in UTF-8: the answers must be the same text
#   utf8saori the SAORI answers with its Charset (a SAORI that knows UTF-8): the SHIORI answers
#             must stay the same and the SAORI requests must be the same text (UTF-8)
#   upgrade   the new DLL reads the save data written by the old DLL
#   reload    (--reload N) load / unload N times in one process, then the same answers
#   paths     (--paths) the ghost in folders with Japanese / 0x5C / non-ANSI names, load and loadu
#   downgrade (information only) the old DLL reads the save data written by the new DLL
#
# The ghost's own dictionaries decide how deep the walk goes. A ghost may need a request that
# SSP would send in a special situation: give it with --start, and put the entries it needs into
# the copy with --append-dict (see build/src/test/README.md).
#
# Needs Windows and a C compiler of the same bitness as the DLLs (32bit MinGW).
# Exit codes: 0 = all checks passed, 1 = a check failed, 2 = the test could not run.
# This file is ASCII only.

import argparse
import collections
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import threading

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, '..', '..', '..'))
HARNESS_C = os.path.join(HERE, 'harness', 'shiori_harness.c')
STUB_C = os.path.join(HERE, 'harness', 'stub_saori.c')

PROGRAM_EXT = ('.exe', '.com', '.bat', '.cmd', '.scr', '.msi', '.lnk', '.vbs', '.ps1', '.jar')
HEAVY_EXT = ('.png', '.jpg', '.gif', '.bmp', '.pna', '.wav', '.mp3', '.ogg', '.wma', '.mid', '.mp4', '.avi', '.pdf')
CHARSETS = {'Shift_JIS': 'cp932', 'UTF-8': 'utf-8'}


def stop_setup(message):
    print('ghost_compat: CANNOT RUN - ' + message)
    sys.exit(2)


# ---- tools ----------------------------------------------------------------------------------

def pe_machine(path):
    with open(path, 'rb') as f:
        b = f.read(4096)
    pe = struct.unpack_from('<I', b, 0x3C)[0]
    return {0x14c: 'i386', 0x8664: 'x64'}.get(struct.unpack_from('<H', b, pe + 4)[0], 'other')


def build_tools(cc, work, dll_machine):
    try:
        target = subprocess.run([cc, '-dumpmachine'], capture_output=True, text=True).stdout.strip()
    except OSError:
        stop_setup("cannot run the C compiler '%s' (use --cc to give a 32bit MinGW gcc)" % cc)
    machine = 'i386' if re.match(r'i[3-6]86-', target) else ('x64' if target.startswith('x86_64-') else 'other')
    if machine != dll_machine:
        stop_setup("the DLL is %s but '%s' builds for %s. Put a matching MinGW first in PATH or use --cc"
                   % (dll_machine, cc, target))
    harness = os.path.join(work, 'shiori_harness.exe')
    stub = os.path.join(work, 'stub_saori.dll')
    for cmd in ([cc, '-O1', '-Wall', '-o', harness, HARNESS_C],
                [cc, '-O1', '-Wall', '-shared', '-o', stub, STUB_C]):
        r = subprocess.run(cmd, capture_output=True, text=True)
        if r.returncode != 0:
            stop_setup('building %s failed:\n%s' % (os.path.basename(cmd[-1]), r.stderr))
    return harness, stub


# ---- ghost copies ---------------------------------------------------------------------------

class Ghosts:
    def __init__(self, src, stub, real_saori, seed, append_dict):
        self.src, self.stub, self.real = src, stub, set(real_saori)
        self.seed, self.append = seed, append_dict

    def prepare(self, dest, dll, log=True):
        """a sanitized copy of the ghost with the DLL under test, a fixed seed and a log"""
        if os.path.exists(dest):
            shutil.rmtree(dest)
        shutil.copytree(self.src, dest, ignore=shutil.ignore_patterns(*['*' + e for e in HEAVY_EXT]))
        for root, _, files in os.walk(dest):
            for f in files:
                low = f.lower()
                p = os.path.join(root, f)
                if low.endswith(PROGRAM_EXT):
                    os.remove(p)
                elif low.endswith('.dll') and low not in self.real:
                    shutil.copy(self.stub, p)
        shutil.copy(dll, os.path.join(dest, 'shiori.dll'))
        inject = b''
        if log:
            os.makedirs(os.path.join(dest, 'logs'), exist_ok=True)
            inject += b'loglevel error warning info baseevents;\r\nlogfile logs/kawari.log;\r\n'
        if self.seed is not None:
            inject += b'srand %d;\r\n' % self.seed
        if self.append:
            with open(os.path.join(dest, 'zz_compat.kis'), 'wb') as f:
                f.write('\r\n'.join(self.append).encode('cp932') + b'\r\n')
            inject += b'load zz_compat.kis;\r\n'
        rc = os.path.join(dest, 'kawarirc.kis')
        data = open(rc, 'rb').read() if os.path.exists(rc) else b''
        m = re.search(rb'(?m)^=kis[ \t]*\r?$', data)
        if m:
            data = data[:m.end()] + b'\r\n' + inject + data[m.end():]
        else:
            data = b'=kis\r\n' + inject + b'=end\r\n' + data
        open(rc, 'wb').write(data)

    @staticmethod
    def clone(src, dest, dll):
        """the state left by a session, with another DLL"""
        if os.path.exists(dest):
            shutil.rmtree(dest)
        shutil.copytree(src, dest)
        shutil.rmtree(os.path.join(dest, 'logs'), ignore_errors=True)
        os.makedirs(os.path.join(dest, 'logs'))
        shutil.copy(dll, os.path.join(dest, 'shiori.dll'))


# ---- SHIORI sessions ------------------------------------------------------------------------

GET_VERSION_26 = '@GetVersion26'


def build_request(verb, id_, refs, cs):
    if id_ == GET_VERSION_26:
        return ('GET Version SHIORI/2.6\r\nSender: SSP\r\nCharset: %s\r\n\r\n' % cs).encode('ascii')
    h = ['%s SHIORI/3.0' % verb, 'Charset: %s' % cs, 'Sender: SSP', 'SenderType: internal,raise',
         'SecurityLevel: local', 'ID: %s' % id_]
    for i, r in enumerate(refs):
        h.append('Reference%d: %s' % (i, r))
    return ('\r\n'.join(h) + '\r\n\r\n').encode(CHARSETS[cs], 'replace')


class Session:
    """shiori_harness.exe in interactive mode"""

    def __init__(self, harness, ghost, timeout, env=None):
        e = dict(os.environ)
        e.update(env or {})
        self.p = subprocess.Popen([harness, os.path.join(ghost, 'shiori.dll'), ghost, '-'],
                                  stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                  bufsize=0, env=e)
        self.timer = threading.Timer(timeout, self.p.kill)
        self.timer.start()

    def send(self, data):
        self.p.stdin.write(struct.pack('<I', len(data)) + data)
        self.p.stdin.flush()
        h = self.p.stdout.read(4)
        if len(h) < 4:
            raise RuntimeError('the harness stopped: ' + self.p.stderr.read().decode('cp932', 'replace').strip())
        n = struct.unpack('<I', h)[0]
        buf = b''
        while len(buf) < n:
            c = self.p.stdout.read(n - len(buf))
            if not c:
                break
            buf += c
        return buf

    def close(self):
        self.p.stdin.close()
        self.p.wait()
        self.timer.cancel()
        err = self.p.stderr.read().decode('cp932', 'replace').strip()
        return self.p.returncode, err


def value_of(resp, cs):
    m = re.search(r'\r\nValue: (.*?)\r\n', resp.decode(CHARSETS[cs], 'replace'))
    return m.group(1) if m else None


# links in a script: \q[title,ID] \q[title,OnEvent,ref...] \__q[ID] \_a[ID] \![raise,OnEvent,ref...] \![open,inputbox,ID,...]
LINKS = [(re.compile(r'\\_?q\[([^\],]*),([^\]]*)\]'), 'q'), (re.compile(r'\\__q\[([^\]]*)\]'), 'q2'),
         (re.compile(r'\\_a\[([^\]]*)\]'), 'a'), (re.compile(r'\\!\[(?:raise|notify),([^\]]*)\]'), 'raise'),
         (re.compile(r'\\!\[open,(?:inputbox|dateinput|timeinput|sliderinput|ipinput|passwordinput),([^,\]]+)'), 'input')]


def followups(script):
    out = []
    for rx, kind in LINKS:
        for m in rx.findall(script):
            if kind == 'q':
                title, rest = m
                parts = rest.split(',')
                if re.match(r'^On\w+$', parts[0]):
                    out.append(('GET', parts[0], parts[1:]))
                else:
                    out.append(('GET', 'OnChoiceSelect', [parts[0], title]))
            elif kind == 'q2':
                out.append(('GET', 'OnChoiceSelect', [m, m]))
            elif kind == 'a':
                out.append(('GET', 'OnAnchorSelect', [m]))
            elif kind == 'raise':
                parts = m.split(',')
                out.append(('GET', parts[0], parts[1:]))
            else:
                out.append(('GET', 'OnUserInput', [m, 'test input']))
    return out


def default_starts(first_boot):
    """what SSP sends around a start; the 4th item (False) = do not follow the links of the answer"""
    nf = lambda t: t + (False,)
    s = []
    if first_boot:
        s.append(nf(('GET', 'OnFirstBoot', ['0', '', '', '', '', '', '', 'master'])))
    s.append(nf(('GET', 'OnBoot', ['master', '', '', '', '', '', 'master'])))
    for i in range(3):
        s.append(nf(('GET', 'OnSecondChange', ['1', '0', '0', '1', str(i)])))
    s.append(nf(('GET', 'OnMinuteChange', ['1', '0', '0', '1'])))
    s.append(nf(('GET', 'OnMouseMove', ['10', '10', '0', '0', 'Head', '0'])))
    return s


MORE_EVENTS = [
    ('GET', 'OnMouseDoubleClick', ['10', '10', '0', '0', 'Head', '0']),
    ('GET', 'OnMouseClick', ['10', '10', '0', '0', 'Head', '0']),
    ('GET', 'OnMouseWheel', ['10', '10', '1', '0', 'Head', '0']),
    ('GET', 'OnSurfaceChange', ['0', '10', '', '']),
    ('GET', 'OnTranslate', ['\\0\\s[0]Hello\\e', '', '', '']),
    ('GET', 'OnKeyPress', ['a', '65']),
    ('GET', 'OnChoiceTimeout', ['x']),
    ('GET', 'OnBalloonClose', ['0']),
    ('NOTIFY', 'ownerghostname', ['ghost']),
    ('NOTIFY', 'basewareversion', ['2.6.0', 'SSP']),
    ('NOTIFY', 'installedghostname', ['ghost', 'x']),
]
END = [('GET', 'OnClose', ['user'])]


def parse_start(text):
    """"GET OnMenu a,b" -> ('GET', 'OnMenu', ['a', 'b'])"""
    parts = text.split(None, 2)
    if len(parts) < 2 or parts[0] not in ('GET', 'NOTIFY'):
        stop_setup("bad --start '%s' (expected: GET|NOTIFY <ID> [<Reference0>,<Reference1>,...])" % text)
    return (parts[0], parts[1], parts[2].split(',') if len(parts) > 2 else [])


def crawl(harness, ghost, starts, limit, timeout):
    """walk through the ghost; returns the requests, the answers, the exit code and the stderr"""
    s = Session(harness, ghost, timeout)
    reqs, resps, seen = [], [], set()
    queue = collections.deque([('GET', GET_VERSION_26, [], False)] + starts)
    while queue and len(reqs) < limit:
        item = queue.popleft()
        key = (item[0], item[1], tuple(item[2]))
        if key in seen:
            continue
        seen.add(key)
        r = s.send(build_request(key[0], key[1], key[2], 'Shift_JIS'))
        reqs.append(key)
        resps.append(r)
        v = value_of(r, 'Shift_JIS')
        if v and len(item) == 3:
            queue.extendleft(reversed(followups(v)))
    for k in END:
        reqs.append((k[0], k[1], tuple(k[2])))
        resps.append(s.send(build_request(k[0], k[1], k[2], 'Shift_JIS')))
    rc, err = s.close()
    return reqs, resps, rc, err


def replay(harness, ghost, reqs, timeout, cs='Shift_JIS', env=None):
    s = Session(harness, ghost, timeout, env)
    try:
        resps = [s.send(build_request(v, i, list(r), cs)) for v, i, r in reqs]
    finally:
        rc, err = s.close()
    return resps, rc, err


# ---- comparison -----------------------------------------------------------------------------

class Normalizer:
    """the version strings of the two DLLs are not a difference"""

    def __init__(self, ver_a, ver_b):
        self.vers = [v for v in {ver_a, ver_b} if v]

    def text(self, s):
        for v in self.vers:
            s = s.replace(v, 'VER')
        return s

    def raw(self, b):
        for v in self.vers:
            b = b.replace(v.encode('ascii'), b'VER')
        return b


def version_of(resp):
    m = re.search(rb'\r\nVersion: (\S+)', resp)
    return m.group(1).decode('ascii') if m else None


def show(b, cs='Shift_JIS', n=200):
    return b[:n].decode(CHARSETS[cs], 'replace').replace('\r\n', ' | ')


def compare_answers(reqs, ra, rb, norm, cs_a='Shift_JIS', cs_b='Shift_JIS'):
    """decoded comparison (the answers may be in different charsets); returns the differences"""
    diffs = []
    for k, a, b in zip(reqs, ra, rb):
        sa = norm.text(a.decode(CHARSETS[cs_a], 'replace'))
        sb = norm.text(b.decode(CHARSETS[cs_b], 'replace'))
        sa = sa.replace('Charset: ' + cs_a, 'Charset: X')
        sb = sb.replace('Charset: ' + cs_b, 'Charset: X')
        if k[1] == GET_VERSION_26:
            # 9.0.0 answers every request with a Charset header, 8.x did not for SHIORI/2.x
            sa = sa.replace('Charset: X\r\n', '')
            sb = sb.replace('Charset: X\r\n', '')
        if sa != sb:
            diffs.append((k, a, b))
    return diffs


def saori_logs(ghost):
    out = {}
    for root, _, files in os.walk(ghost):
        for f in files:
            if f.endswith('.dll.log'):
                t = open(os.path.join(root, f), 'rb').read()
                out[os.path.relpath(os.path.join(root, f), ghost)] = [x for x in t.split(b'\n-----\n') if x]
    return out


def saori_entries(ghost):
    """SAORI requests as text, decoded by their own Charset, without the Charset line"""
    out = {}
    for m, lst in saori_logs(ghost).items():
        o = []
        for x in lst:
            cs = re.search(rb'Charset: (\S+)', x)
            enc = 'utf-8' if cs and cs.group(1) == b'UTF-8' else 'cp932'
            o.append(re.sub(r'Charset: \S+\r\n', '', x.decode(enc, 'replace')))
        out[m] = o
    return out


class Report:
    def __init__(self):
        self.results = []

    def add(self, name, ok, detail='', info=False):
        self.results.append((name, ok, detail, info))
        status = 'info' if info else ('ok  ' if ok else 'NG  ')
        print('%s %s%s' % (status, name, ('  - ' + detail) if detail else ''))

    def diffs(self, name, reqs, diffs, cs_a='Shift_JIS', cs_b='Shift_JIS', info=False, limit=5):
        self.add(name, not diffs, '%d requests, %d differ' % (len(reqs), len(diffs)), info)
        for k, a, b in diffs[:limit]:
            print('      %s %s %s' % (k[0], k[1], list(k[2])[:3]))
            print('        A: %s' % show(a, cs_a))
            print('        B: %s' % show(b, cs_b))


def main():
    ap = argparse.ArgumentParser(description='Compare two SHIORI DLLs on a real ghost (see the top of the source).')
    ap.add_argument('--ghost', required=True, help='the folder with shiori.dll and kawarirc.kis (ghost/master)')
    ap.add_argument('--old-dll', help='baseline DLL (default: shiori.dll of the ghost)')
    ap.add_argument('--new-dll', help='DLL under test (default: bin/shiori.dll of this repository)')
    ap.add_argument('--cc', default='gcc', help='C compiler of the same bitness as the DLLs (default: gcc)')
    ap.add_argument('--limit', type=int, default=1500, help='maximum number of requests of the walk (default 1500)')
    ap.add_argument('--seed', type=int, default=12345, help='random seed put into the copies (default 12345)')
    ap.add_argument('--first-boot', action='store_true', help='start with OnFirstBoot')
    ap.add_argument('--start', action='append', default=[], metavar='"GET ID ref0,ref1"',
                    help='an extra request after the start events; its answer is followed (repeatable)')
    ap.add_argument('--append-dict', action='append', default=[], metavar='LINE',
                    help='a dictionary line put into the copy, for example "event.OnMenu : ${Mainmenu}" (repeatable)')
    ap.add_argument('--real-saori', default='', help='file names of SAORI DLLs that are kept as they are (comma separated)')
    ap.add_argument('--reload', type=int, default=0, metavar='N', help='also check N load / unload cycles')
    ap.add_argument('--paths', action='store_true', help='also check folders with Japanese / non-ANSI names')
    ap.add_argument('--timeout', type=int, default=600, help='seconds per harness session (default 600)')
    ap.add_argument('--work', help='work folder (default: a new temporary folder)')
    ap.add_argument('--keep', action='store_true', help='keep the work folder')
    args = ap.parse_args()

    if os.name != 'nt':
        stop_setup('Windows only (the harness loads the DLL with the Win32 API)')
    ghost = os.path.abspath(args.ghost)
    if not os.path.isfile(os.path.join(ghost, 'kawarirc.kis')):
        stop_setup('kawarirc.kis was not found in ' + ghost)
    old_dll = os.path.abspath(args.old_dll or os.path.join(ghost, 'shiori.dll'))
    new_dll = os.path.abspath(args.new_dll or os.path.join(REPO, 'bin', 'shiori.dll'))
    for p in (old_dll, new_dll):
        if not os.path.isfile(p):
            stop_setup('not found: ' + p)
    if pe_machine(old_dll) != pe_machine(new_dll):
        stop_setup('the two DLLs have different bitness')

    work = os.path.abspath(args.work) if args.work else tempfile.mkdtemp(prefix='kawari-ghost-compat-')
    os.makedirs(work, exist_ok=True)
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')
    try:
        return run(args, ghost, old_dll, new_dll, work)
    finally:
        if args.keep:
            print('work folder kept: ' + work)
        else:
            shutil.rmtree(work, ignore_errors=True)


def run(args, ghost, old_dll, new_dll, work):
    harness, stub = build_tools(args.cc, work, pe_machine(old_dll))
    g = Ghosts(ghost, stub, [x.strip().lower() for x in args.real_saori.split(',') if x.strip()],
               args.seed, args.append_dict)
    T = args.timeout
    rep = Report()
    starts = default_starts(args.first_boot) + [t + () for t in MORE_EVENTS] + [parse_start(s) for s in args.start]
    # the extra --start requests come after the start events and the fixed events
    d = lambda name: os.path.join(work, name)

    print('ghost : ' + ghost)
    print('old   : ' + old_dll)
    print('new   : ' + new_dll)

    # ---- baseline: the old DLL walks through the ghost, twice
    g.prepare(d('old'), old_dll)
    reqs, ra, rc, err = crawl(harness, d('old'), starts, args.limit, T)
    if rc != 0:
        stop_setup('the harness failed with the old DLL (exit code %s): %s' % (rc, err))
    answered = sum(1 for r in ra if r.startswith(b'SHIORI/3.0 200'))
    print('walk  : %d requests, %d answered with 200' % (len(reqs), answered))
    g.prepare(d('old2'), old_dll)
    rb, rc, err = replay(harness, d('old2'), reqs, T)
    again = [1 for k, a, b in zip(reqs, ra, rb) if a != b]
    if again:
        stop_setup('the old DLL does not answer the same twice (%d of %d differ); the ghost uses time or '
                   'randomness that --seed does not fix, so versions cannot be compared' % (len(again), len(reqs)))

    # ---- replay with the new DLL
    g.prepare(d('new'), new_dll)
    rn, rc, err = replay(harness, d('new'), reqs, T)
    if rc != 0:
        stop_setup('the harness failed with the new DLL (exit code %s): %s' % (rc, err))
    norm = Normalizer(version_of(ra[0]), version_of(rn[0]))
    print('version: %s -> %s' % (version_of(ra[0]), version_of(rn[0])))
    diffs = compare_answers(reqs, ra, rn, norm)
    rep.diffs('replay', reqs, diffs)

    # ---- SAORI requests (legacy SAORI)
    ea, en = saori_entries(d('old')), saori_entries(d('new'))
    bad = []
    n = 0
    for m in sorted(set(ea) | set(en)):
        a, b = ea.get(m, []), en.get(m, [])
        n += len(a)
        if len(a) != len(b):
            bad.append('%s: %d vs %d requests' % (m, len(a), len(b)))
        for x, y in zip(a, b):
            if x != y:
                bad.append('%s: %r vs %r' % (m, x[:120], y[:120]))
    la, ln = saori_logs(d('old')), saori_logs(d('new'))
    gv = sum(1 for m in la for x in la[m] if x.startswith(b'GET Version') and b'Charset: Shift_JIS' in x)
    gv_new = sum(1 for m in ln for x in ln[m] if x.startswith(b'GET Version') and b'Charset: UTF-8' in x)
    rep.add('saori', not bad, '%d requests to %d stubbed modules, %d differ' % (n, len(ea), len(bad)))
    for b_ in bad[:5]:
        print('      ' + b_)
    if n:
        print('      note: "GET Version" is sent in Shift_JIS by the old DLL (%d) and in UTF-8 by the new DLL (%d), on purpose' % (gv, gv_new))

    # ---- UTF-8 requests
    g.prepare(d('utf8'), new_dll)
    r8, rc, err = replay(harness, d('utf8'), reqs, T, cs='UTF-8')
    rep.diffs('utf8', reqs, compare_answers(reqs, ra, r8, norm, 'Shift_JIS', 'UTF-8'), 'Shift_JIS', 'UTF-8')

    # ---- a SAORI that knows UTF-8
    g.prepare(d('echo'), new_dll)
    re_, rc, err = replay(harness, d('echo'), reqs, T, env={'KAWARI_STUB_CHARSET': 'echo'})
    ok1 = compare_answers(reqs, ra, re_, norm)
    ee = saori_entries(d('echo'))
    bad = []
    for m in sorted(set(ea) | set(ee)):
        a, b = ea.get(m, []), ee.get(m, [])
        if len(a) != len(b):
            bad.append('%s: %d vs %d requests' % (m, len(a), len(b)))
        for x, y in zip(a, b):
            if x != y and not x.startswith('GET Version'):
                bad.append('%s: %r vs %r' % (m, x[:120], y[:120]))
    utf8_exec = sum(1 for m, lst in saori_logs(d('echo')).items() for x in lst
                    if b'Charset: UTF-8' in x and not x.startswith(b'GET Version'))
    rep.diffs('utf8saori (SHIORI answers)', reqs, ok1)
    rep.add('utf8saori (SAORI requests)', not bad, '%d requests sent in UTF-8, %d differ' % (utf8_exec, len(bad)))
    for b_ in bad[:5]:
        print('      ' + b_)

    # ---- save data: a second session on the state left by the first one
    def second_session(tag, src_state, first_dll, second_dll, info):
        a, b = d(tag + '_a'), d(tag + '_b')
        Ghosts.clone(src_state, a, first_dll)
        Ghosts.clone(src_state, b, second_dll)
        r2, ra2, rc2, err2 = crawl(harness, a, default_starts(False) + MORE_EVENTS + [parse_start(s) for s in args.start],
                                   args.limit, T)
        rb2, rc3, err3 = replay(harness, b, r2, T)
        rep.diffs(tag, r2, compare_answers(r2, ra2, rb2, norm), info=info)

    second_session('upgrade', d('old'), old_dll, new_dll, False)
    second_session('downgrade', d('new'), old_dll, new_dll, True)

    # ---- load / unload cycles
    if args.reload:
        # the probe request of every round moves the state of the ghost, so the answers differ from the
        # ones without reload; the old DLL after the same cycles is the baseline
        env = {'HARNESS_RELOAD': str(args.reload)}
        g.prepare(d('reload_old'), old_dll)
        g.prepare(d('reload_new'), new_dll)
        rr_old, rc, err = replay(harness, d('reload_old'), reqs, T, env=env)
        rr_new, rc, err = replay(harness, d('reload_new'), reqs, T, env=env)
        rep.diffs('reload x%d' % args.reload, reqs, compare_answers(reqs, rr_old, rr_new, norm))

    # ---- folder names
    if args.paths:
        part = reqs[:60]
        for name in ('\u30c6\u30b9\u30c8\u8868\u80fd\u30bd', 'with space', '\u9ad9\ufa11\u2460\u3231', '\U0001F600 emoji',
                     '\u4e2d\u6587\ud55c\uae00'):
            ansi = True
            try:
                name.encode('cp932')
            except UnicodeEncodeError:
                ansi = False
            for label, dll, env in (('old load', old_dll, {}), ('new load', new_dll, {}),
                                    ('new loadu', new_dll, {'HARNESS_LOADU': '1'})):
                if not ansi and label != 'new loadu':
                    continue
                dest = os.path.join(work, 'paths', name + ' ' + label.replace(' ', '_'))
                g.prepare(dest, dll)
                e = dict(env)
                if not ansi:
                    e['HARNESS_DIR_W'] = dest
                rp, rc, err = replay(harness, dest, part, T, env=e)
                rep.diffs('paths %s [%s]' % (label, name.encode('ascii', 'backslashreplace').decode('ascii')), part,
                          compare_answers(part, ra[:len(part)], rp, norm))

    failed = [r for r in rep.results if not r[1] and not r[3]]
    print('\nghost_compat: %d checks, %d failed' % (len([r for r in rep.results if not r[3]]), len(failed)))
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
