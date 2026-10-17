#!/usr/bin/env python3
#
# このツール群が扱う EUC-JP は、Python のコーデックとしては "euc_jis_2004" を用いる。
# euc_jis_2004 は EUC-JP (JIS X 0208) の上位互換で、JIS X 0213 の文字
# (0xAB 始まりなどの機種依存文字) も扱える。素の "euc_jp" では、そうした文字を含む既存の
# ソースが strict デコードに失敗して編集不能になるため、コーデックは常に euc_jis_2004 に固定する。
# 本ツール群は EUC-JP 専用で、他の文字コードのファイルは編集せず拒否する。
import json
import os
import re
import sys
import tempfile
import time
from pathlib import Path

# Windows の stdout/stdin を UTF-8 に固定する（既定のコードページだと日本語や \ufffd を出力できずクラッシュする）
sys.stdout.reconfigure(encoding="utf-8")
sys.stdin.reconfigure(encoding="utf-8")

# ──────────────────────────────────────────────
# Tool implementations
# ──────────────────────────────────────────────

ENCODING = "euc_jis_2004"

DEFAULT_READ_LIMIT = 2000
LINE_TRUNCATE_CHARS = 2000

# 出力肥大化防止用のデフォルト上限
DEFAULT_GLOB_LIMIT = 500
DEFAULT_GREP_FILE_LIMIT = 200      # files_with_matches / count の最大ファイル数
DEFAULT_GREP_CONTENT_LIMIT = 50    # content モードの最大ファイル数
DEFAULT_GREP_TOTAL_LINES = 2000    # content モードの合計出力行数上限
DEFAULT_TIMEOUT = 30.0
BINARY_CHECK_SIZE = 8192           # バイナリ判定で読むバイト数
DEFAULT_HEXDUMP_LENGTH = 512       # hexdump のデフォルト表示バイト数
MAX_HEXDUMP_LENGTH = 8192          # hexdump の1回あたり上限バイト数
DEFAULT_SED_PREVIEW_LINES = 50     # dry run で表示する変更行数の上限
DEFAULT_EOL_LIMIT = 200            # euc_eol が列挙する問題ファイルの上限

def read_euc(path: str, offset: int = 0, limit=None) -> str:
    _check_euc(path)
    p = Path(path)
    text = p.read_text(encoding=ENCODING, errors="replace")
    all_lines = text.splitlines(keepends=True)
    total_lines = len(all_lines)

    effective_limit = limit if limit is not None else DEFAULT_READ_LIMIT
    window = all_lines[offset:offset + effective_limit]

    def fmt_line(i, line):
        # 長すぎる行を切り詰める（改行を除いた本文部分が対象）
        nl = "\n" if line.endswith("\n") else ""
        body = line.rstrip("\n")
        if len(body) > LINE_TRUNCATE_CHARS:
            body = body[:LINE_TRUNCATE_CHARS] + "... [truncated]"
        return f"{i + offset + 1:>6}\t{body}{nl}"

    result = "".join(fmt_line(i, ln) for i, ln in enumerate(window))

    remaining = total_lines - offset - len(window)
    if remaining > 0 and limit is None:
        result += (
            f"\n[File truncated: showing lines {offset + 1}–{offset + len(window)} of {total_lines}. "
            f"Use offset and limit parameters to read more.]"
        )

    return result

def _atomic_write_bytes(path: Path, data: bytes) -> None:
    """バイト列を一時ファイルに書き込んでからリネームする。テキストモードを一切使わないため
    改行変換（\\n→\\r\\n）が発生せず、内容が意図通りにそのまま保存される。
    書き込み後に読み戻して検証し、実際にディスクへ反映されたことを保証する。"""
    parent = path.parent
    fd, tmp_path = tempfile.mkstemp(dir=parent, suffix=".tmp")
    try:
        with os.fdopen(fd, "wb") as f:
            f.write(data)
            f.flush()
            os.fsync(f.fileno())
        # Windows では上書き先が既存の場合 os.rename が失敗するので os.replace を使う
        os.replace(tmp_path, path)
    except Exception:
        # 書き込みやリネームに失敗した場合、一時ファイルを掃除して例外を再送出
        try:
            os.unlink(tmp_path)
        except OSError:
            pass
        raise
    # 検証: 書き込んだ内容がディスク上のファイルと一致することを確認する。
    # 一致しなければ「成功」と報告しない（成功報告の信頼性を担保する）。
    if path.read_bytes() != data:
        raise IOError(f"write verification failed: on-disk content of {path} does not match what was written")

def _atomic_write(path: Path, content: str, encoding: str = ENCODING) -> None:
    """テキストを指定エンコーディングでバイト列化してから書き込む。改行変換は行わない。"""
    try:
        data = content.encode(encoding)
    except UnicodeEncodeError as e:
        bad = content[e.start:e.end]
        raise ValueError(
            f"Content contains character(s) not representable in {encoding}: {bad!r} (at position {e.start})"
        )
    _atomic_write_bytes(path, data)

def _dominant_eol(raw: bytes) -> str:
    """ファイルの主たる改行コードを返す（\\r\\n か \\n）。編集後もこれを保つ。"""
    crlf = raw.count(b"\r\n")
    lf = raw.count(b"\n") - crlf
    return "\r\n" if crlf > 0 and crlf >= lf else "\n"

def _reject_if_mixed_eol(path: str, raw: bytes, verb: str) -> None:
    """改行コードが混在するファイルへの編集を拒否する。

    edit/sed は照合のため一旦 LF 正規化し、書き戻す際に主改行コードへ復元する。
    この復元は「ファイル全体を主改行コードへ統一する」ことと等価であり、混在ファイルに対しては
    1行の変更が全行の差分になる（svn diff が壊滅する）。無言で統一するくらいなら拒否して、
    euc_eol による明示的な修復を先に行わせる。
    """
    crlf, lf_only, cr_only = _eol_stats(raw)
    kinds = sum(1 for n in (crlf, lf_only, cr_only) if n)
    if kinds < 2:
        return
    raise ValueError(
        f"[euc-tools] '{path}' は改行コードが混在しています"
        f"（CRLF={crlf}, LF only={lf_only}, CR only={cr_only}）。"
        f" このまま{verb}すると全行が主改行コードへ統一され、差分が全行に膨らみます。"
        " 先に euc_eol (mode='fix') で改行コードを統一してください。"
    )

def write_euc(path: str, content: str) -> str:
    _check_euc(path, require_euc=True)  # 既存ファイルが EUC-JP 以外なら上書き拒否
    _atomic_write(Path(path), content)
    return f"Written: {path}"

def edit_euc(path: str, old_string: str, new_string: str, replace_all: bool = False) -> str:
    _check_euc(path)
    p = Path(path)
    raw = p.read_bytes()

    # 編集用は strict デコード。不正 EUC-JP バイトを含むファイルは errors='replace' で
    # 黙って欠損させるのではなく、明示的に編集を拒否して破壊を防ぐ。
    try:
        text = raw.decode(ENCODING)
    except UnicodeDecodeError as e:
        raise ValueError(
            f"[euc-tools] '{path}' は EUC-JP として不正なバイト(0x{raw[e.start]:02x} @ offset {e.start})を含みます。"
            " 破壊を避けるため編集を拒否します。euc_detect でエンコーディングを確認してください。"
        )

    if old_string == new_string:
        raise ValueError("No changes to make: old_string and new_string are exactly the same.")

    _reject_if_mixed_eol(path, raw, "編集")

    # 改行コードを正規化して照合する（old_string が CRLF/LF どちらでも一致させる）。
    eol = _dominant_eol(raw)
    text_n = text.replace("\r\n", "\n")
    old_n = old_string.replace("\r\n", "\n")
    new_n = new_string.replace("\r\n", "\n")

    count = text_n.count(old_n)
    if count == 0:
        raise ValueError("old_string not found in file")
    if not replace_all and count > 1:
        raise ValueError(
            f"old_string is not unique ({count} matches). Add more context or use replace_all=true."
        )

    if replace_all:
        new_text_n = text_n.replace(old_n, new_n)
    else:
        new_text_n = text_n.replace(old_n, new_n, 1)

    # 元ファイルの改行コードに戻してから書き込む（LF→CRLF の無言変換を防ぐ）。
    new_text = new_text_n.replace("\n", eol)
    try:
        out = new_text.encode(ENCODING)
    except UnicodeEncodeError as e:
        bad = new_text[e.start:e.end]
        raise ValueError(f"new_string contains character(s) not representable in EUC-JP: {bad!r}")

    _atomic_write_bytes(p, out)

    n = count if replace_all else 1
    msg = f"Edited: {path} ({n} replacement{'s' if n != 1 else ''})"
    # 整合性の開示: 元ファイルが非正規の EUC-JP バイト列を含み、再エンコードで
    # 未編集箇所のバイトが変わった可能性がある場合は正直に警告する。
    if text.encode(ENCODING) != raw:
        msg += (
            "\n[Warning: file contained non-canonical EUC-JP byte sequences that were "
            "normalized to their canonical form on save.]"
        )
    return msg

def _split_top_level_commas(s: str):
    """ブレース深さ0のカンマで分割する。ネストした {..} 内のカンマは分割対象にしない。"""
    parts = []
    depth = 0
    cur = []
    for ch in s:
        if ch == "{":
            depth += 1
            cur.append(ch)
        elif ch == "}":
            depth -= 1
            cur.append(ch)
        elif ch == "," and depth == 0:
            parts.append("".join(cur))
            cur = []
        else:
            cur.append(ch)
    parts.append("".join(cur))
    return parts


def _expand_braces(pattern: str):
    """glob パターンの {a,b,c} 展開を行いパターンのリストを返す（ripgrep 互換）。
    pathlib.Path.glob はブレース展開に非対応なため、'*.{h,cpp}' のようなパターンが
    サイレントに0件になる問題を防ぐ。ネストにも対応。カンマを含まない '{}' や '{abc}'、
    対応する '}' が無い '{' はリテラルとしてそのまま残す。"""
    start = pattern.find("{")
    if start == -1:
        return [pattern]

    depth = 0
    end = start
    while end < len(pattern):
        ch = pattern[end]
        if ch == "{":
            depth += 1
        elif ch == "}":
            depth -= 1
            if depth == 0:
                break
        end += 1
    else:
        # 対応する閉じ括弧が無い → 展開しない
        return [pattern]

    prefix = pattern[:start]
    inner = pattern[start + 1:end]
    suffix = pattern[end + 1:]
    options = _split_top_level_commas(inner)

    if len(options) <= 1:
        # カンマ無し（'{}' や '{abc}'）は展開対象外。後続だけ再帰的に展開する。
        return [prefix + "{" + inner + "}" + s for s in _expand_braces(suffix)]

    out = []
    seen = set()
    for opt in options:
        # opt / suffix 内に残るブレースも展開するため結合して再帰する。
        for rest in _expand_braces(opt + suffix):
            expanded = prefix + rest
            if expanded not in seen:
                seen.add(expanded)
                out.append(expanded)
    return out


def glob_files(pattern: str, path: str = ".", head_limit: int = 0, timeout: float = DEFAULT_TIMEOUT) -> str:
    base = Path(path)
    deadline = time.monotonic() + timeout
    collected = []
    seen = set()
    for pat in _expand_braces(pattern):
        for p in base.glob(pat):
            if time.monotonic() >= deadline:
                collected.sort(key=lambda x: x.stat().st_mtime, reverse=True)
                limit = head_limit if head_limit else DEFAULT_GLOB_LIMIT
                collected = collected[:limit]
                result = "\n".join(str(m) for m in collected)
                return result + f"\n\n[TIMEOUT: glob aborted after {timeout:.0f}s — {len(collected)} results shown (partial)]"
            if p not in seen:
                seen.add(p)
                collected.append(p)

    if not collected:
        return "(no matches)"

    collected.sort(key=lambda x: x.stat().st_mtime, reverse=True)
    effective_limit = head_limit if head_limit else DEFAULT_GLOB_LIMIT
    truncated = len(collected) > effective_limit
    collected = collected[:effective_limit]
    result = "\n".join(str(m) for m in collected)
    if truncated:
        result += f"\n\n[Output truncated: showing {effective_limit} of many matches. Use head_limit to adjust.]"
    return result

# ファイルタイプ → glob パターンのマッピング（rg --type 互換）
TYPE_GLOBS = {
    "c":    ["*.c", "*.h"],
    "cpp":  ["*.cpp", "*.cc", "*.cxx", "*.h", "*.hpp", "*.hxx"],
    "py":   ["*.py", "*.pyi"],
    "js":   ["*.js", "*.mjs", "*.cjs"],
    "ts":   ["*.ts", "*.tsx", "*.mts"],
    "rust": ["*.rs"],
    "go":   ["*.go"],
    "java": ["*.java"],
    "html": ["*.html", "*.htm"],
    "css":  ["*.css", "*.scss", "*.sass"],
    "json": ["*.json"],
    "xml":  ["*.xml"],
    "md":   ["*.md", "*.markdown"],
    "txt":  ["*.txt"],
    "sh":   ["*.sh", "*.bash"],
    "bat":  ["*.bat", "*.cmd"],
    "make": ["Makefile", "GNUmakefile", "*.mk", "*.mak"],
}

def _is_binary(path: Path) -> bool:
    """先頭数KBにヌルバイトがあればバイナリとみなす。"""
    try:
        with open(path, "rb") as f:
            chunk = f.read(BINARY_CHECK_SIZE)
        return b"\x00" in chunk
    except Exception:
        return True


def _check_euc(path: str, require_euc: bool = False) -> None:
    """既存ファイルが EUC-JP として扱ってよいか検査し、不適切なら ValueError を送出する。
    対象:
      - UTF-8 BOM 付きファイル
      - UTF-8 strict デコード成功 かつ EUC-JP strict デコード失敗のファイル
      - require_euc=True のとき、EUC-JP としても UTF-8 としても不正なファイル (CP932 など)
    ASCII のみのファイル（両方成功）は判断不能なので通す。
    """
    p = Path(path)
    if not p.exists() or not p.is_file():
        return
    try:
        raw = p.read_bytes()
    except Exception:
        return

    if raw.startswith(b"\xef\xbb\xbf"):
        raise ValueError(
            f"[euc-tools] ERROR: '{path}' は UTF-8 (BOM付き) ファイルです。"
            " euc_* ツールではなく通常の Read / Write / Edit ツールを使用してください。"
        )

    try:
        raw.decode(ENCODING)
        return  # EUC-JP として有効 (ASCII のみの場合を含む)
    except UnicodeDecodeError:
        pass

    try:
        raw.decode("utf-8")
    except UnicodeDecodeError:
        # UTF-8 でも EUC-JP でもない。読み取り (errors='replace') は許すが、書き込み系は拒否する。
        if require_euc:
            raise ValueError(
                f"[euc-tools] ERROR: '{path}' は EUC-JP としても UTF-8 としても不正なバイト列を含みます"
                " (CP932 などの可能性)。破壊を避けるため書き込みを拒否します。"
                " euc_detect / euc_hexdump でエンコーディングを確認してください。"
            )
        return

    raise ValueError(
        f"[euc-tools] ERROR: '{path}' は UTF-8 ファイルです（EUC-JP として無効なバイト列を含む）。"
        " euc_* ツールではなく通常の Read / Write / Edit ツールを使用してください。"
    )


def _iter_candidates(base: Path, glob_pattern, file_type, deadline: float):
    """ファイル候補をイテレータとして返す。事前に全件リスト化しないことで巨大ディレクトリでのメモリ・時間浪費を防ぐ。"""
    if base.is_file():
        yield base
        return

    if file_type and not glob_pattern:
        ext_pats = TYPE_GLOBS.get(file_type, [])
        if not ext_pats:
            ext_pats = ["*"]
        seen = set()
        for ext_pat in ext_pats:
            for p in base.glob(f"**/{ext_pat}"):
                if time.monotonic() >= deadline:
                    return
                if p.is_file() and p not in seen:
                    seen.add(p)
                    yield p
    else:
        g = glob_pattern or "**/*"
        seen = set()
        for pat in _expand_braces(g):
            for p in base.glob(pat):
                if time.monotonic() >= deadline:
                    return
                if p.is_file() and p not in seen:
                    seen.add(p)
                    yield p


def grep_euc(
    pattern: str,
    path: str = ".",
    glob_pattern=None,
    output_mode: str = "files_with_matches",
    lines_before: int = 0,
    lines_after: int = 0,
    case_insensitive: bool = False,
    show_line_numbers: bool = True,
    only_matching: bool = False,
    head_limit: int = 0,
    offset: int = 0,
    multiline: bool = False,
    file_type: str = None,
    timeout: float = DEFAULT_TIMEOUT,
) -> str:
    flags = re.IGNORECASE if case_insensitive else 0
    if multiline:
        flags |= re.MULTILINE | re.DOTALL

    rx = re.compile(pattern, flags)

    before = lines_before
    after = lines_after

    base = Path(path)
    deadline = time.monotonic() + timeout

    results = []
    file_matches = []
    count_map = {}
    timed_out = False
    total_content_lines = 0

    # output_mode に応じたデフォルト上限
    if output_mode == "content":
        auto_limit = DEFAULT_GREP_CONTENT_LIMIT
    else:
        auto_limit = DEFAULT_GREP_FILE_LIMIT
    effective_limit = head_limit if head_limit else auto_limit
    hit_limit = False

    for fp in _iter_candidates(base, glob_pattern, file_type, deadline):
        if time.monotonic() >= deadline:
            timed_out = True
            break

        if _is_binary(fp):
            continue

        try:
            text = fp.read_text(encoding=ENCODING, errors="replace")
        except Exception:
            continue

        lines = text.splitlines()
        matched_indices = [i for i, ln in enumerate(lines) if rx.search(ln)]
        if not matched_indices:
            continue

        if output_mode == "files_with_matches":
            file_matches.append(str(fp))
            if len(file_matches) >= offset + effective_limit:
                hit_limit = True
                break
        elif output_mode == "count":
            count_map[str(fp)] = len(matched_indices)
            if len(count_map) >= offset + effective_limit:
                hit_limit = True
                break
        else:  # content
            def trunc(s):
                if len(s) > LINE_TRUNCATE_CHARS:
                    return s[:LINE_TRUNCATE_CHARS] + "... [truncated]"
                return s

            block_lines = []
            if only_matching:
                # -o: マッチした部分文字列のみを 1 行ずつ出力する（ripgrep --only-matching 相当）
                for idx in matched_indices:
                    for m in rx.finditer(lines[idx]):
                        seg = trunc(m.group(0))
                        if show_line_numbers:
                            block_lines.append(f"{fp}:{idx + 1}:{seg}")
                        else:
                            block_lines.append(f"{fp}:{seg}")
            else:
                shown = set()
                for idx in matched_indices:
                    start = max(0, idx - before)
                    end = min(len(lines) - 1, idx + after)
                    for i in range(start, end + 1):
                        shown.add(i)
                shown = sorted(shown)

                prev = None
                for i in shown:
                    if prev is not None and i > prev + 1:
                        block_lines.append("--")
                    # ripgrep 形式: マッチ行は path:line:text、コンテキスト行は path-line-text
                    sep = ":" if rx.search(lines[i]) else "-"
                    line_text = trunc(lines[i])
                    if show_line_numbers:
                        block_lines.append(f"{fp}{sep}{i + 1}{sep}{line_text}")
                    else:
                        block_lines.append(f"{fp}{sep}{line_text}")
                    prev = i

            if not block_lines:
                continue
            results.append("\n".join(block_lines))
            total_content_lines += len(block_lines)

            if len(results) >= offset + effective_limit or total_content_lines >= DEFAULT_GREP_TOTAL_LINES:
                hit_limit = True
                break

    notices = []
    if timed_out:
        notices.append(f"[TIMEOUT: search aborted after {timeout:.0f}s — results are partial]")
    if hit_limit:
        notices.append(f"[Output truncated at limit. Use offset/head_limit to paginate.]")
    notice_str = "\n\n" + "\n".join(notices) if notices else ""

    if output_mode == "files_with_matches":
        out = file_matches
        if offset:
            out = out[offset:]
        out = out[:effective_limit]
        body = "\n".join(out) if out else "(no matches)"
        return body + notice_str
    elif output_mode == "count":
        items = list(count_map.items())
        if offset:
            items = items[offset:]
        items = items[:effective_limit]
        # ripgrep --count 形式: path:count
        body = "\n".join(f"{k}:{v}" for k, v in items) if items else "(no matches)"
        return body + notice_str
    else:
        if offset:
            results = results[offset:]
        results = results[:effective_limit]
        body = "\n".join(results) if results else "(no matches)"
        return body + notice_str

def _invalid_euc_offsets(raw: bytes, max_report: int = 20):
    """EUC-JP として不正なバイトのオフセット一覧を返す。エラー位置を1バイト飛ばして走査を継続する。"""
    offsets = []
    pos = 0
    total = len(raw)
    while pos < total:
        try:
            raw[pos:].decode(ENCODING)
            break
        except UnicodeDecodeError as e:
            offsets.append(pos + e.start)
            if len(offsets) >= max_report:
                offsets.append(-1)  # 打ち切りマーカー
                break
            pos = pos + e.start + 1
    return offsets


def _eol_stats(raw: bytes):
    """CRLF / 単独LF / 単独CR の出現数を返す。"""
    crlf = raw.count(b"\r\n")
    lf_only = raw.count(b"\n") - crlf
    cr_only = raw.count(b"\r") - crlf
    return crlf, lf_only, cr_only


def hexdump_euc(path: str, offset: int = 0, length: int = DEFAULT_HEXDUMP_LENGTH, summary_only: bool = False) -> str:
    """バイナリ読みの代替。文字コード破損と改行コードの健全性を1発で確認する。

    `cmd /c type` や PowerShell の Get-Content -Encoding Byte のような外部コマンドを使うと
    パイプ経由で EUC-JP が化けたり改行が正規化されたりするため、生バイトを直接読んで整形する。
    """
    p = Path(path)
    raw = p.read_bytes()
    total = len(raw)

    # ── サマリ ──
    lines = [f"File: {path}", f"Size: {total} bytes"]

    if raw.startswith(b"\xef\xbb\xbf"):
        bom = "UTF-8 BOM (EF BB BF)"
    elif raw.startswith(b"\xff\xfe"):
        bom = "UTF-16 LE BOM (FF FE)"
    elif raw.startswith(b"\xfe\xff"):
        bom = "UTF-16 BE BOM (FE FF)"
    else:
        bom = "none"
    lines.append(f"BOM: {bom}")
    lines.append(f"Detected encoding: {detect_encoding(path)}")

    crlf, lf_only, cr_only = _eol_stats(raw)
    if crlf and not lf_only and not cr_only:
        eol_desc = "CRLF (consistent)"
    elif lf_only and not crlf and not cr_only:
        eol_desc = "LF (consistent)"
    elif cr_only and not crlf and not lf_only:
        eol_desc = "CR (consistent)"
    elif crlf or lf_only or cr_only:
        eol_desc = "MIXED — 要確認"
    else:
        eol_desc = "none (no line breaks)"
    lines.append(f"Line endings: {eol_desc}  [CRLF={crlf}, LF only={lf_only}, CR only={cr_only}]")

    if total == 0:
        lines.append("Final newline: n/a (empty file)")
    elif raw.endswith(b"\n") or raw.endswith(b"\r"):
        lines.append("Final newline: yes")
    else:
        lines.append("Final newline: NO — 末尾に改行がありません")

    bad = _invalid_euc_offsets(raw)
    if not bad:
        lines.append("Invalid EUC-JP bytes: none")
    else:
        truncated = bad and bad[-1] == -1
        shown = [o for o in bad if o >= 0]
        desc = ", ".join(f"0x{o:X}({o})" for o in shown)
        lines.append(f"Invalid EUC-JP bytes: {len(shown)}{'+' if truncated else ''} at offset {desc}"
                     + (" ... (打ち切り)" if truncated else ""))

    nul = raw.count(b"\x00")
    if nul:
        lines.append(f"NUL bytes: {nul} — バイナリの可能性")

    if summary_only:
        return "\n".join(lines)

    # ── ダンプ本体 ──
    if offset < 0:
        offset = 0
    if length <= 0:
        length = DEFAULT_HEXDUMP_LENGTH
    length = min(length, MAX_HEXDUMP_LENGTH)
    chunk = raw[offset:offset + length]

    lines.append("")
    if not chunk:
        lines.append(f"[No bytes at offset {offset} (file size {total})]")
        return "\n".join(lines)

    lines.append(f"Dump: offset 0x{offset:08X} ({offset}) .. 0x{offset + len(chunk):08X} ({offset + len(chunk)}) of {total}")
    # テキスト欄は16バイト境界で行を切るため、境界をまたぐ2バイト文字は化けて見える。
    # 破損の権威ある判定は上の "Invalid EUC-JP bytes" 行であり、テキスト欄は目安。
    lines.append("Offset    +0 +1 +2 +3 +4 +5 +6 +7  +8 +9 +A +B +C +D +E +F  | text (EUC-JP, 16B境界で分断)")

    for i in range(0, len(chunk), 16):
        row = chunk[i:i + 16]
        hex1 = " ".join(f"{b:02X}" for b in row[:8])
        hex2 = " ".join(f"{b:02X}" for b in row[8:])
        hexpart = f"{hex1:<23} {hex2:<23}"
        # 表示用テキスト: 制御文字は記号化し、非ASCIIは EUC-JP として復号を試みる
        text = row.decode(ENCODING, errors="replace")
        disp = []
        for ch in text:
            if ch == "\r":
                disp.append("␍")
            elif ch == "\n":
                disp.append("␊")
            elif ch == "\t":
                disp.append("→")
            elif ord(ch) < 0x20 or ord(ch) == 0x7F:
                disp.append("·")
            else:
                disp.append(ch)
        lines.append(f"{offset + i:08X}  {hexpart} | {''.join(disp)}")

    remaining = total - (offset + len(chunk))
    if remaining > 0:
        lines.append(f"\n[{remaining} more bytes. Use offset/length to continue.]")

    return "\n".join(lines)


def sed_euc(path: str, pattern: str, replacement: str, count: int = 0,
             case_insensitive: bool = False, multiline: bool = False,
             dry_run: bool = False) -> str:
    """EUC-JP 固定・改行コード維持の正規表現置換。sed / PowerShell -replace の代替。

    行の照合は改行を LF に正規化した状態で行い、書き戻す際に元ファイルの主改行コードへ戻す。
    これにより「LF が紛れ込む」「CRLF が二重になる」事故を構造的に防ぐ。
    """
    _check_euc(path)
    p = Path(path)
    raw = p.read_bytes()

    try:
        text = raw.decode(ENCODING)
    except UnicodeDecodeError as e:
        raise ValueError(
            f"[euc-tools] '{path}' は EUC-JP として不正なバイト(0x{raw[e.start]:02x} @ offset {e.start})を含みます。"
            " 破壊を避けるため置換を拒否します。euc_hexdump で破損箇所を確認してください。"
        )

    flags = re.MULTILINE
    if case_insensitive:
        flags |= re.IGNORECASE
    if multiline:
        flags |= re.DOTALL
    rx = re.compile(pattern, flags)

    _reject_if_mixed_eol(path, raw, "置換")

    eol = _dominant_eol(raw)
    text_n = text.replace("\r\n", "\n")
    repl_n = replacement.replace("\r\n", "\n")

    n_matches = len(rx.findall(text_n))
    if n_matches == 0:
        raise ValueError(f"pattern not found in file: {pattern!r}")

    new_text_n, n_done = rx.subn(repl_n, text_n, count=count if count > 0 else 0)
    if new_text_n == text_n:
        raise ValueError("No changes to make: replacement produced identical content.")

    if dry_run:
        # 変化した行だけを before/after で提示する
        old_lines = text_n.split("\n")
        new_lines = new_text_n.split("\n")
        out = [f"[dry run] {path}: {n_done} replacement(s), line endings preserved as "
               f"{'CRLF' if eol == '\r\n' else 'LF'}"]
        shown = 0
        for i in range(max(len(old_lines), len(new_lines))):
            o = old_lines[i] if i < len(old_lines) else None
            n = new_lines[i] if i < len(new_lines) else None
            if o != n:
                if shown >= DEFAULT_SED_PREVIEW_LINES:
                    out.append("... (以降省略)")
                    break
                if o is not None:
                    out.append(f"-{i + 1}: {o}")
                if n is not None:
                    out.append(f"+{i + 1}: {n}")
                shown += 1
        return "\n".join(out)

    new_text = new_text_n.replace("\n", eol)
    try:
        data = new_text.encode(ENCODING)
    except UnicodeEncodeError as e:
        bad = new_text[e.start:e.end]
        raise ValueError(f"replacement contains character(s) not representable in EUC-JP: {bad!r}")

    _atomic_write_bytes(p, data)

    msg = f"Replaced: {path} ({n_done} replacement{'s' if n_done != 1 else ''}, {'CRLF' if eol == '\r\n' else 'LF'} preserved)"
    if text.encode(ENCODING) != raw:
        msg += ("\n[Warning: file contained non-canonical EUC-JP byte sequences that were "
                "normalized to their canonical form on save.]")
    return msg


def _eol_kind(raw: bytes) -> str:
    """ファイルの改行コード種別を返す: 'CRLF' / 'LF' / 'CR' / 'mixed' / 'none'。"""
    crlf, lf_only, cr_only = _eol_stats(raw)
    kinds = [k for k, n in (("CRLF", crlf), ("LF", lf_only), ("CR", cr_only)) if n]
    if not kinds:
        return "none"
    return kinds[0] if len(kinds) == 1 else "mixed"

def _normalize_eol_bytes(raw: bytes, target: bytes) -> bytes:
    """バイト列の改行コードを target に統一する。

    バイト単位で処理するためエンコーディングに依存しない。EUC-JP / UTF-8 いずれも、
    多バイト文字の後続バイトに 0x0D / 0x0A は現れないため誤爆しない。
    （UTF-16 系は該当しないので、呼び出し側で BOM により除外している。）
    """
    lf = raw.replace(b"\r\n", b"\n").replace(b"\r", b"\n")
    return lf.replace(b"\n", target) if target != b"\n" else lf

def eol_euc(path: str = ".", glob_pattern=None, file_type=None,
             mode: str = "check", to: str = "lf", dry_run: bool = True,
             head_limit: int = 0, timeout: float = DEFAULT_TIMEOUT) -> str:
    """改行コードの一括検査・修復。エンコーディング非依存（バイト単位）。

    mode='check' は問題のあるファイルだけを列挙し、mode='fix' は to へ統一する。
    fix は既定で dry_run=True。実際に書き換えるには dry_run=False を明示すること。
    """
    if mode not in ("check", "fix"):
        raise ValueError("mode must be 'check' or 'fix'")
    if to not in ("crlf", "lf"):
        raise ValueError("to must be 'crlf' or 'lf'")

    target = b"\r\n" if to == "crlf" else b"\n"
    want = "CRLF" if to == "crlf" else "LF"
    limit = head_limit if head_limit > 0 else DEFAULT_EOL_LIMIT

    base = Path(path)
    if not base.exists():
        raise ValueError(f"Path not found: {path}")
    deadline = time.monotonic() + timeout

    scanned = 0
    ok = 0
    skipped = 0
    issues = []      # (表示パス, kind, (crlf, lf, cr))
    truncated = False
    timed_out = False

    for f in _iter_candidates(base, glob_pattern, file_type, deadline):
        if time.monotonic() >= deadline:
            timed_out = True
            break
        try:
            raw = f.read_bytes()
        except Exception:
            skipped += 1
            continue
        # UTF-16 はバイト単位の改行置換が破壊的なので触らない
        if raw.startswith(b"\xff\xfe") or raw.startswith(b"\xfe\xff") or _is_binary(f):
            skipped += 1
            continue

        scanned += 1
        kind = _eol_kind(raw)
        if kind in ("none", want):
            ok += 1
            continue
        if len(issues) >= limit:
            truncated = True
            continue
        issues.append((str(f), kind, _eol_stats(raw)))

    header = f"Scanned {scanned} file(s) under {base}"
    if glob_pattern:
        header += f" [glob={glob_pattern}]"
    if file_type:
        header += f" [type={file_type}]"
    summary = f"OK ({want}): {ok}   Issues: {len(issues)}{'+' if truncated else ''}   Skipped (binary/UTF-16/unreadable): {skipped}"
    lines = [header, summary]

    if not issues:
        if timed_out:
            lines.append(f"[timeout: scan stopped after {timeout}s — results are partial]")
        return "\n".join(lines)

    width = max(len(p) for p, _, _ in issues)
    for p, kind, (crlf, lf_only, cr_only) in issues:
        lines.append(f"  {p.ljust(width)}  {kind:<5}  [CRLF={crlf}, LF only={lf_only}, CR only={cr_only}]")
    if truncated:
        lines.append(f"  ... (以降省略: 上限 {limit} 件。head_limit で拡張可)")

    if mode == "check":
        lines.append(f"\nRun with mode='fix', dry_run=false to convert these to {want}.")
        if timed_out:
            lines.append(f"[timeout: scan stopped after {timeout}s — results are partial]")
        return "\n".join(lines)

    # ── fix ──
    if dry_run:
        lines.insert(0, f"[dry run] Would convert {len(issues)} file(s) to {want}. Pass dry_run=false to apply.")
        if timed_out:
            lines.append(f"[timeout: scan stopped after {timeout}s — results are partial]")
        return "\n".join(lines)

    converted = 0
    failed = []
    for p, _, _ in issues:
        fp = Path(p)
        try:
            data = _normalize_eol_bytes(fp.read_bytes(), target)
            _atomic_write_bytes(fp, data)
            converted += 1
        except Exception as e:
            failed.append(f"  {p}: {e}")

    # 一覧は「変換前の状態」なので、残存する問題と読み違えないよう見出しを差し替える。
    lines[1] = (f"Converted {converted} of {len(issues)} file(s) to {want}"
                f" (already {want}: {ok}, skipped: {skipped}). Listed below with their previous state:")
    if failed:
        lines.append("Failed:")
        lines.extend(failed)
    if timed_out:
        lines.append(f"[timeout: scan stopped after {timeout}s — results are partial]")
    return "\n".join(lines)


def detect_encoding(path: str) -> str:
    """ファイルのエンコーディングを推定する。外部ライブラリ不要。"""
    p = Path(path)
    raw = p.read_bytes()

    # BOM チェック
    if raw.startswith(b"\xef\xbb\xbf"):
        return "UTF-8 (with BOM)"
    if raw.startswith(b"\xff\xfe"):
        return "UTF-16 LE"
    if raw.startswith(b"\xfe\xff"):
        return "UTF-16 BE"

    # UTF-8 strict decode を試みる
    try:
        raw.decode("utf-8")
        return "UTF-8"
    except UnicodeDecodeError:
        pass

    # EUC-JP (euc_jis_2004) を優先する。本ツール群は EUC-JP 専用のため、
    # CP932 としても有効な場合は注記だけ付けて EUC-JP と判定する。
    try:
        raw.decode(ENCODING)
        if all(b < 0x80 for b in raw):
            return "ASCII (EUC-JP / UTF-8 / CP932 のいずれとしても有効)"
        try:
            raw.decode("cp932")
            return "EUC-JP (CP932 としても有効: 判別が曖昧です)"
        except UnicodeDecodeError:
            return "EUC-JP"
    except UnicodeDecodeError:
        pass

    # CP932 (MS Shift-JIS) strict decode (参考情報。本ツール群では編集できない)
    try:
        raw.decode("cp932")
        return "CP932 (MS Shift-JIS) — euc_* ツールでは編集できません"
    except UnicodeDecodeError:
        pass

    return "Unknown (binary or unrecognized encoding)"

def convert_encoding(path: str, from_encoding: str, to_encoding: str, output_path=None) -> str:
    """ファイルのエンコーディングを変換する。output_path 省略時は上書き。"""
    # EUC-JP 系の指定は、素の euc_jp ではなく常に euc_jis_2004 へ寄せる (JIS X 0213 文字を扱うため)。
    # "sjis"/"shift_jis" は MS 拡張を欠く shift_jis コーデックではなく cp932 へ寄せる。
    ENC_ALIASES = {
        "eucjp": ENCODING, "euc-jp": ENCODING, "euc_jp": ENCODING, "euc": ENCODING,
        "euc_jis_2004": ENCODING, "euc-jis-2004": ENCODING,
        "sjis": "cp932", "s-jis": "cp932", "shift_jis": "cp932",
        "cp932": "cp932", "ms932": "cp932", "windows-31j": "cp932",
        "utf8": "utf-8", "utf-8": "utf-8",
    }
    src_enc = ENC_ALIASES.get(from_encoding.lower(), from_encoding)
    dst_enc = ENC_ALIASES.get(to_encoding.lower(), to_encoding)

    src = Path(path)
    # バイト列から復号することで universal-newline による改行正規化を避け、改行コードを保持する。
    text = src.read_bytes().decode(src_enc, errors="replace")
    dst = Path(output_path) if output_path else src
    _atomic_write(dst, text, encoding=dst_enc)
    return f"Converted: {path} ({from_encoding} -> {to_encoding}) -> {dst}"

# ──────────────────────────────────────────────
# Tool definitions
# ──────────────────────────────────────────────

TOOLS = [
    {
        "name": "euc_read",
        "description": "Read an EUC-JP encoded file. Returns content with line numbers.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "file_path": {"type": "string", "description": "Absolute or relative path to the file"},
                "offset": {"type": "integer", "description": "Line number to start reading from (0-indexed)", "default": 0},
                "limit": {"type": "integer", "description": "Maximum number of lines to read (default: 2000). Provide offset+limit to page through large files."}
            },
            "required": ["file_path"]
        }
    },
    {
        "name": "euc_write",
        "description": "Write content to a file encoded in EUC-JP. Overwrites existing content.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "file_path": {"type": "string", "description": "Absolute or relative path to the file"},
                "content": {"type": "string", "description": "Content to write"}
            },
            "required": ["file_path", "content"]
        }
    },
    {
        "name": "euc_edit",
        "description": "Exact string replacement in an EUC-JP encoded file. old_string must be unique in the file unless replace_all is true.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "file_path": {"type": "string", "description": "Absolute or relative path to the file"},
                "old_string": {"type": "string", "description": "The exact string to replace"},
                "new_string": {"type": "string", "description": "The replacement string"},
                "replace_all": {"type": "boolean", "description": "Replace all occurrences (default: false)", "default": False}
            },
            "required": ["file_path", "old_string", "new_string"]
        }
    },
    {
        "name": "euc_glob",
        "description": "Find files matching a glob pattern. Returns matching paths sorted by modification time (newest first).",
        "inputSchema": {
            "type": "object",
            "properties": {
                "pattern": {"type": "string", "description": "Glob pattern, e.g. '**/*.txt' or 'src/**/*.dic'"},
                "path": {"type": "string", "description": "Base directory to search in (default: current directory)", "default": "."},
                "head_limit": {"type": "integer", "description": "Limit output to first N results", "default": 0},
                "timeout": {"type": "number", "description": "Timeout in seconds (default: 30)", "default": 30}
            },
            "required": ["pattern"]
        }
    },
    {
        "name": "euc_grep",
        "description": (
            "Search file contents using a regex pattern, decoding files as EUC-JP. "
            "output_mode: 'files_with_matches' (default) returns file paths; "
            "'content' returns matching lines with optional context; "
            "'count' returns match counts per file."
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "pattern": {"type": "string", "description": "Regular expression pattern to search for"},
                "path": {"type": "string", "description": "File or directory to search (default: current directory)", "default": "."},
                "glob": {"type": "string", "description": "Glob pattern to filter files, e.g. '**/*.txt'"},
                "type": {"type": "string", "description": "File type to search (e.g. cpp, py, js, rust). Common types: c, cpp, py, js, ts, rust, go, java, html, css, json, xml, md"},
                "output_mode": {
                    "type": "string",
                    "enum": ["files_with_matches", "content", "count"],
                    "description": "Output format (default: files_with_matches)",
                    "default": "files_with_matches"
                },
                "-A": {"type": "integer", "description": "Number of lines to show after each match (requires output_mode: content)", "default": 0},
                "-B": {"type": "integer", "description": "Number of lines to show before each match (requires output_mode: content)", "default": 0},
                "-C": {"type": "integer", "description": "Number of lines to show before and after each match (requires output_mode: content)", "default": 0},
                "-i": {"type": "boolean", "description": "Case-insensitive matching", "default": False},
                "-n": {"type": "boolean", "description": "Show line numbers in output (requires output_mode: content). Defaults to true.", "default": True},
                "-o": {"type": "boolean", "description": "Show only the matched parts of a line, one per line (requires output_mode: content)", "default": False},
                "head_limit": {"type": "integer", "description": "Limit output to first N results/files", "default": 0},
                "offset": {"type": "integer", "description": "Skip first N results before applying head_limit", "default": 0},
                "multiline": {"type": "boolean", "description": "Enable multiline mode where . matches newlines and patterns can span lines", "default": False},
                "timeout": {"type": "number", "description": "Timeout in seconds. Search stops after this time and returns partial results (default: 30)", "default": 30}
            },
            "required": ["pattern"]
        }
    },
    {
        "name": "euc_detect",
        "description": "Detect the character encoding of a file (UTF-8, EUC-JP, CP932, etc.) without external libraries.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "file_path": {"type": "string", "description": "Absolute or relative path to the file"}
            },
            "required": ["file_path"]
        }
    },
    {
        "name": "euc_convert",
        "description": "Convert a file's character encoding (e.g. UTF-8 to EUC-JP, or vice versa). EUC-JP is handled as euc_jis_2004.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "file_path": {"type": "string", "description": "Path to the source file"},
                "from_encoding": {"type": "string", "description": "Source encoding: euc-jp, utf-8, cp932, etc. ('euc-jp' is handled as euc_jis_2004; 'sjis'/'shift_jis' are aliases for cp932)"},
                "to_encoding": {"type": "string", "description": "Target encoding: euc-jp, utf-8, cp932, etc. ('euc-jp' is handled as euc_jis_2004; 'sjis'/'shift_jis' are aliases for cp932)"},
                "output_path": {"type": "string", "description": "Output file path. Omit to overwrite the source file."}
            },
            "required": ["file_path", "from_encoding", "to_encoding"]
        }
    },
    {
        "name": "euc_hexdump",
        "description": (
            "Inspect a file's raw bytes: encoding/BOM detection, line-ending breakdown (CRLF/LF/CR), "
            "invalid EUC-JP byte offsets, final-newline check, plus a hex + EUC-JP dump. "
            "Use this INSTEAD of shell commands (type, cat, Format-Hex, od) when checking for "
            "character-encoding corruption or line-ending mistakes — piping through a shell mangles EUC-JP."
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "file_path": {"type": "string", "description": "Absolute or relative path to the file"},
                "offset": {"type": "integer", "description": "Byte offset to start the dump from (default: 0)", "default": 0},
                "length": {"type": "integer", "description": "Number of bytes to dump (default: 512, max: 8192)", "default": 512},
                "summary_only": {"type": "boolean", "description": "Show only the header summary without the hex dump", "default": False}
            },
            "required": ["file_path"]
        }
    },
    {
        "name": "euc_sed",
        "description": (
            "Regex search-and-replace across an EUC-JP file. Decoding is fixed to EUC-JP and "
            "the file's dominant line ending (CRLF/LF) is preserved, so this is the safe replacement for "
            "sed / PowerShell -replace, which corrupt EUC-JP and silently rewrite line endings. "
            "Use dry_run to preview changed lines first."
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "file_path": {"type": "string", "description": "Absolute or relative path to the file"},
                "pattern": {"type": "string", "description": "Python regular expression to match (re.MULTILINE is always on, so ^ and $ match line boundaries)"},
                "replacement": {"type": "string", "description": "Replacement string. Backreferences use \\1, \\g<name>."},
                "count": {"type": "integer", "description": "Maximum number of replacements. 0 (default) replaces all occurrences.", "default": 0},
                "-i": {"type": "boolean", "description": "Case-insensitive matching", "default": False},
                "multiline": {"type": "boolean", "description": "Let . match newlines (re.DOTALL) so patterns can span lines", "default": False},
                "dry_run": {"type": "boolean", "description": "Preview the changed lines without writing the file", "default": False}
            },
            "required": ["file_path", "pattern", "replacement"]
        }
    },
    {
        "name": "euc_eol",
        "description": (
            "Audit and repair line endings (CRLF/LF/CR) across a file or a whole directory tree. "
            "mode='check' (default) lists only the files that are mixed or not the expected ending; "
            "mode='fix' with dry_run=false rewrites them. Works on raw bytes, so it is encoding-independent "
            "(EUC-JP, CP932 and UTF-8 all safe); binary and UTF-16 files are skipped automatically. "
            "Use this when euc_edit / euc_sed refuse a file for having mixed line endings, or to sweep a "
            "tree for stray CRLF in an LF codebase."
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "path": {"type": "string", "description": "File or directory to scan (default: current directory)", "default": "."},
                "glob": {"type": "string", "description": "Glob pattern to filter files, e.g. '**/*.cpp'"},
                "type": {"type": "string", "description": "File type to scan (e.g. cpp, py, js). Same type names as euc_grep."},
                "mode": {
                    "type": "string",
                    "enum": ["check", "fix"],
                    "description": "'check' reports only; 'fix' converts (still requires dry_run=false to write)",
                    "default": "check"
                },
                "to": {
                    "type": "string",
                    "enum": ["crlf", "lf"],
                    "description": "Target line ending (default: lf)",
                    "default": "lf"
                },
                "dry_run": {"type": "boolean", "description": "For mode='fix': preview without writing. Defaults to true — pass false to actually convert.", "default": True},
                "head_limit": {"type": "integer", "description": "Maximum number of problem files to list (default: 200)", "default": 0},
                "timeout": {"type": "number", "description": "Timeout in seconds. Scan stops after this time and returns partial results (default: 30)", "default": 30}
            },
            "required": []
        }
    }
]

# ──────────────────────────────────────────────
# JSON-RPC handler
# ──────────────────────────────────────────────

def handle(req: dict):
    method = req.get("method")
    id_ = req.get("id")

    if method == "initialize":
        return {"jsonrpc": "2.0", "id": id_, "result": {
            "protocolVersion": "2024-11-05",
            "capabilities": {"tools": {}},
            "serverInfo": {"name": "Euc-tools", "version": "1.0.0"}
        }}
    elif method == "tools/list":
        return {"jsonrpc": "2.0", "id": id_, "result": {"tools": TOOLS}}
    elif method == "tools/call":
        name = req["params"]["name"]
        args = req["params"].get("arguments", {})
        try:
            if name == "euc_read":
                result = read_euc(args["file_path"], args.get("offset", 0), args.get("limit"))
            elif name == "euc_write":
                result = write_euc(args["file_path"], args["content"])
            elif name == "euc_edit":
                result = edit_euc(
                    args["file_path"],
                    args["old_string"],
                    args["new_string"],
                    args.get("replace_all", False),
                )
            elif name == "euc_glob":
                result = glob_files(args["pattern"], args.get("path", "."), args.get("head_limit", 0), args.get("timeout", DEFAULT_TIMEOUT))
            elif name == "euc_grep":
                # -C は -A/-B の両方を兼ねる。旧パラメータ名(context/case_insensitive)も互換で受け付ける。
                ctx = args.get("-C", args.get("context", 0))
                before = max(args.get("-B", 0), ctx)
                after = max(args.get("-A", 0), ctx)
                case_i = args.get("-i", args.get("case_insensitive", False))
                result = grep_euc(
                    args["pattern"],
                    args.get("path", "."),
                    args.get("glob"),
                    args.get("output_mode", "files_with_matches"),
                    before,
                    after,
                    case_i,
                    args.get("-n", True),
                    args.get("-o", False),
                    args.get("head_limit", 0),
                    args.get("offset", 0),
                    args.get("multiline", False),
                    args.get("type"),
                    args.get("timeout", 30.0),
                )
            elif name == "euc_detect":
                result = detect_encoding(args["file_path"])
            elif name == "euc_convert":
                result = convert_encoding(
                    args["file_path"],
                    args["from_encoding"],
                    args["to_encoding"],
                    args.get("output_path"),
                )
            elif name == "euc_hexdump":
                result = hexdump_euc(
                    args["file_path"],
                    args.get("offset", 0),
                    args.get("length", DEFAULT_HEXDUMP_LENGTH),
                    args.get("summary_only", False),
                )
            elif name == "euc_sed":
                result = sed_euc(
                    args["file_path"],
                    args["pattern"],
                    args["replacement"],
                    args.get("count", 0),
                    args.get("-i", args.get("case_insensitive", False)),
                    args.get("multiline", False),
                    args.get("dry_run", False),
                )
            elif name == "euc_eol":
                result = eol_euc(
                    args.get("path", "."),
                    args.get("glob"),
                    args.get("type"),
                    args.get("mode", "check"),
                    args.get("to", "lf"),
                    args.get("dry_run", True),
                    args.get("head_limit", 0),
                    args.get("timeout", DEFAULT_TIMEOUT),
                )
            else:
                raise ValueError(f"Unknown tool: {name}")
            return {"jsonrpc": "2.0", "id": id_, "result": {
                "content": [{"type": "text", "text": result}]
            }}
        except Exception as e:
            return {"jsonrpc": "2.0", "id": id_, "result": {
                "content": [{"type": "text", "text": str(e)}],
                "isError": True
            }}
    elif method in ("notifications/initialized", "notifications/cancelled"):
        return None
    else:
        return {"jsonrpc": "2.0", "id": id_, "error": {"code": -32601, "message": "Method not found"}}

if __name__ == "__main__":
    for line in sys.stdin:
        line = line.strip()
        if not line:
            continue
        try:
            req = json.loads(line)
        except json.JSONDecodeError:
            continue
        resp = handle(req)
        if resp is not None:
            print(json.dumps(resp, ensure_ascii=False), flush=True)
