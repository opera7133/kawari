# test

ビルド済みの `kosui.exe` と `shiori.dll` を確かめる、最低限のテスト。リポジトリには何も書き込まず、作業ファイルは一時フォルダに作る。

```
powershell -NoProfile -ExecutionPolicy Bypass -File build/src/test/run.ps1 [-Suite kosui|shiori] [-BinDir <dir>] [-CC <gcc>] [-KeepWork]
```

- 先に `build/src` で `gmake -f gcc.mak` を済ませておく。`-BinDir` の既定は `build/mach/mingw`。
- 終了コード: 0 = 全件成功、1 = 失敗あり、2 = テストを実行できない (バイナリが無い、gcc のビット数が DLL と違う等)。

## kosui スイート (`kosui/`)

辞書・KIS エンジンのテスト。`kosui/cases.txt` の 1 ケースごとに `kosui.exe` を起動し、`exit` まで 1 行を評価させて出力を完全一致で比べる。

```
echo ${greet}      # echo モードで評価する
= Hello            # 期待する出力 (複数行なら "=" の行を並べる)

cmd size npw       # command モード ($(size npw) として評価する)
= 3
```

- ケースごとに `kosui/kawarirc.kis` を新しいフォルダへコピーして実行するので、ケース同士は影響しない。
- 辞書には**カンマを書かない**。カンマは単語の区切りで、複数単語のエントリはランダムに 1 つが選ばれる。
- 期待値は ASCII のみ。
- kosui は入力が尽きると無限ループするため、ランナーが必ず `exit` を付け、`-TimeoutSeconds` (既定 30 秒) で打ち切る。
- 動作中のゴースト (SSP など) があると kosui が `Attach :` を聞いてくる。ランナーはそれを検出して `0` (インタプリタモード) と答え、そのケースからやり直す (`note:` の行が出る)。

## shiori スイート (`shiori/`, `harness/`)

`shiori.dll` の `load` / `request` / `unload` と SHIORI/3.0 の処理のテスト。ランナーが `harness/shiori_harness.c` を `-CC` (既定 `gcc`) でビルドし、`shiori/cases.txt` のリクエストを 1 つのロード済みモジュールへ順に送る。

```
GET OnRef a,b                  # GET <ID> [<Reference0>,<Reference1>,...]
~ SHIORI/3.0 200 OK            # 応答に含まれるべき文字列 (部分一致、複数可)
~ Value: r0=a r1=b

NOTIFY ownerghostname testghost
~ SHIORI/3.0 204 No Content
```

- `shiori/kawarirc.kis` が試験用のゴースト辞書。ファイルごと一時フォルダへコピーして読ませる。
- 同じファイルのリクエストは同じセッションで順に送られるので、前のケースの結果を後のケースが使える。
- **ハーネスは DLL と同じビット数でビルドする** (DLL は 32bit)。`PATH` の先頭に 32bit 版 MinGW を置くか `-CC` で指定する。合っていないとランナーが理由を示して止まる。

ハーネス単体でも使える (`-KeepWork` で作業フォルダを残すと、リクエストと応答のファイルが見られる)。

```
shiori_harness <dll> <ゴーストのフォルダ> <出力ファイル> <リクエストファイル> [...]
```

リクエストファイルは CRLF 区切りで空行終わりの生のバイト列。応答は `\n=====\n` で区切って出力ファイルに書かれる。文字コードの変換はしない。

環境変数でふるまいを変えられる。

| 変数 | 内容 |
|---|---|
| `HARNESS_LOADU` | `load` の代わりに `loadu` (UTF-8 パス) を呼ぶ |
| `HARNESS_DIR_W` | UTF-16 のフォルダ名。`HARNESS_LOADU` と併用すると ACP 外の文字を含むフォルダも試せる。DLL もこのフォルダから `LoadLibraryW` で読む |
| `HARNESS_RELOAD=<n>` | リクエストの前に `load` / `unload` を n 回繰り返す (各周回で `version` を 1 つ送る)。SSP のゴースト切替に相当 |

対話モード: `<出力ファイル>` に `-` を渡し、リクエストファイルは渡さない。標準入力から「4 バイト (リトルエンディアン) の長さ + 生のバイト列」のフレームでリクエストを読み、応答を同じ形式で標準出力へ返す。入力が尽きると `unload` して終わる。応答を見て次のリクエストを決めるドライバ (下の `ghost_compat.py`) が使う。

## スタブ SAORI (`harness/stub_saori.c`)

実在のゴーストを動かすと、SAORI (`exec.dll`、`process.dll`、`bln.dll` など) が本物のプログラムを起動したり窓を開いたりする。このスタブはその代わりに置く DLL で、どんなリクエストにも `SAORI/1.0 200 OK` と `Result: 0` だけを返し (例えば `process exist <exe>` は「動いていない」になる)、受け取ったリクエストを `<DLL名>.log` に記録する。

```
gcc -O1 -Wall -shared -o stub_saori.dll stub_saori.c      # DLL と同じビット数 (32bit) で
```

- 既定では応答に `Charset` を付けない (昔の SAORI と同じ。呼び出し側は Shift_JIS を使い続ける)。環境変数 `KAWARI_STUB_CHARSET=echo` なら、リクエストの `Charset` をそのまま返す (UTF-8 対応 SAORI の再現)。
- ログは SAORI へ送られたバイト列そのもの。版ごとの送信内容 (ヘッダ、文字コード、引数) の比較に使う。

## ゴースト互換テスト (`ghost_compat.py`)

実在のゴーストを 2 つの `shiori.dll` (例: 8.2.x と 9.x) で動かし、応答とセーブデータの扱いが同じかを比べる。Windows と、DLL と同じビット数の gcc が要る。

```
python build/src/test/ghost_compat.py --ghost <ゴースト>/ghost/master [--old-dll <基準のDLL>] [--new-dll <試すDLL>] [--cc <gcc>] [--reload 5] [--paths]
```

- 基準は既定でゴースト同梱の `shiori.dll`、試す側は既定で `bin/shiori.dll`。
- ゴーストには触れない。チェックのたびにコピーし、コピーの中で、プログラム (`*.exe`、`*.bat` など) を消し、`shiori.dll` 以外の DLL をスタブ SAORI に差し替え、乱数の種 (`srand`) とログを `kawarirc.kis` の先頭に足す。実物の SAORI が要るときは `--real-saori a.dll,b.dll`。
- 基準側の DLL が、SSP が起動時に送るイベントと、応答の中の `\q[]` / `\_a[]` / `\![raise,...]` のリンクを深さ優先でたどる。同じリクエスト列を試す側に流して比べる。
- チェック: `replay` (応答が同じ)、`saori` (SAORI へ届いたリクエストが同じ。`GET Version` の Charset は仕様で変わる)、`utf8` (UTF-8 のリクエスト)、`utf8saori` (UTF-8 対応 SAORI)、`upgrade` (旧版のセーブを新版で読む)、`reload` (`--reload N`、N 回の `load`/`unload` の後)、`paths` (`--paths`、日本語・0x5C・ACP 外のフォルダ名、`load` と `loadu`)。`downgrade` (新版のセーブを旧版で読む) は参考表示で、失敗にしない。
- 終了コード: 0 = 全件成功、1 = 失敗あり、2 = 実行できない。基準側が 2 回続けて同じ応答を返さないときは、比べられないので 2 で止まる。
- ゴーストが特別な状況でしか出さない画面 (メニューなど) へは、`--start "GET <ID> <Reference0>,<Reference1>"` で入る。呼べるイベントが無ければ `--append-dict "<辞書の1行>"` でコピーに足す。例えば kakera (OpenKEEPS) のメニューは:

```
python build/src/test/ghost_compat.py --ghost <kakera>/ghost/master ^
  --append-dict "event.OnCompatMenu : ${Mainmenu}" --start "GET OnCompatMenu"
```
