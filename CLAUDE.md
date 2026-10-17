# CLAUDE.md

華和梨 (KAWARI) — 伺か用の SHIORI (擬似AI) モジュール。C++ 製、Phase 8.2.8 (Core 3.2.8)。
CI は無い。最低限のテストは `build/src/test/` (下記「テスト」)。

## 文字コード (最重要)

- `build/src` 配下の `.cpp` / `.h` / `.rb` は **EUC-JP**、改行は LF。コメントやリテラルに日本語を含む。
- 標準の Read / Edit / Write は UTF-8 前提のため EUC-JP を壊す。`.claude/euc-tools/hook.py` (PreToolUse フック) が弾くので、**`euc_read` / `euc_edit` / `euc_write` / `euc_sed` / `euc_grep` を使う** (`.mcp.json` で MCP サーバとして登録済み)。
  - `sed` や PowerShell の `-replace`、`od` / `xxd` 等はフックで禁止。
  - 純 ASCII の `.cpp` / `.h` に日本語を足す場合も `euc_edit` を使う (標準 Edit だと UTF-8 が混入する)。
- 例外: `gcc.mak` / `files.mak` / `kawari_rc_sjis_encoded.h` などは UTF-8 (実質 ASCII)。`document/*.html` も UTF-8 (フックの記述による)。
- `build/BUILD.TXT` と `README.jp.md` は **CP932 (Shift-JIS)**。`euc_*` ツールでは編集できず、標準 Edit / Write も拒否される。変更するなら cp932 を明示して書き換えること。
- `libkawari/kawari_rc.cpp` が include する `kawari_rc_sjis_encoded.h` は、`kawari_rc.sjis` を `sjis2ascii.rb` で八進エスケープ化した生成物と思われる。直接手編集せず、元の `.sjis` 側を直す。

## ビルド

ビルド手順の一次資料は `build/BUILD.TXT` (CP932)。**作業ディレクトリは `build/src`**。

```
cd build/src
make -f gcc.mak depend    # ruby で depend.mak を再生成 (先に clean も走る)
make -f gcc.mak
```

- ターゲット機種は `gcc.mak` 冒頭の `MACH_TYPE` (`cygwin|mingw|linux|freebsd|darwin|js`、デフォルト `mingw`) で決まる。機種ごとの差分は `gcc-mach.mak`。FreeBSD は `gmake`。
- 成果物は `build/mach/<MACH_TYPE>/` に出る (`shiori` 共有ライブラリ、`kosui`、`kawari_encode` / `kawari_encode2` / `kawari_decode2`)。リポジトリ直下の `bin/` はコミット済みのリリース用バイナリで、ビルドしても更新されない。
- `all` は最後に `upx` ターゲットを実行するが、`UPX` は既定で空 (圧縮しない)。UPX 圧縮はマルウェア誤検知の元になるため外した。使うなら `make -f gcc.mak UPX=upx`。
- `CXX` は `ifndef CXX` で決まるため、**環境変数 `CXX` が設定されているとそれが優先される**。存在しないパスが入っていると全コンパイルが失敗するので、`CXX=g++` を明示するのが安全。
- Windows では `make` ではなく `mingw32-make` (または `gmake`) を使う。MinGW に同梱されている。
- **64bit の MinGW-w64 ではビルドが通らない** (g++ 13.2 / x86_64-w64-mingw32 で確認)。`saori/saori_native.cpp` と `tool/kosui_dsstp.cpp` が 32bit 前提のポインタ⇔整数キャストでエラーになり、`shiori.dll` と `kosui.exe` が作れない (`kawari_encode` / `encode2` / `decode2` だけは出来る)。`SAORI_NATIVE=no` でも `saori_module.cpp` が native 実装を参照していてリンクに失敗する。リリース版 `bin/shiori.dll` も i386 (32bit)。
- **32bit 版 MinGW-w64 (i686) ならビルドが通る** (g++ 16.1.0 i686-posix-dwarf で `shiori.dll` / `kosui.exe` / `kawari_*.exe` の全ターゲットが i386 で生成されることを確認。`UPX=` で upx 無効)。choco なら管理者シェルで `choco install mingw -y --x86`、`C:\ProgramData\mingw64\mingw32\bin` に入る。別の (64bit 版等の) g++ が PATH にある場合は、こちらを先頭に置く:
  ```
  export PATH=/c/ProgramData/mingw64/mingw32/bin:$PATH
  cd build/src && gmake -f gcc.mak CXX=g++ UPX=
  ```
  ビルドは `build/src` に `.o` と `build/mach/mingw` に成果物を作るので、作業ツリーを汚したくなければ `build/` をコピーして実行する。`"SHIORI/"SHIORIVER" ..."` の文字列連結が C++11 以降で警告になるが無害。
- 最適化は `-O2`。かつては `-O1` 固定だった (README.jp.md の 8.2.5 の項: 辞書に半角中点があると異常動作。原因は UPX か gcc 最適化か不明のまま) が、UPX を外した上で `-O2` でも既存テスト 19 件と半角中点入り辞書の動作を確認した。
- ソースファイルを追加・削除したら `build/src/files.mak` を編集し、`make depend` で `depend.mak` を作り直す。VC++ 用の `vc_kawari/*.sln|*.vcproj|*.dsp` は手動メンテで、gcc.mak と同期していない可能性が高い。
- 任意機能は `gcc.mak` のフラグで切り替える: `SAORI_NATIVE` (デフォルト yes) / `SAORI_PYTHON` / `SAORI_JAVA` / `SHIORI_PYTHON` / `STLport`。Python 関連の記述は Python 2 構文 (`print sys.version[:3]`)。
- mingw / cygwin で gcc3 系の `wstring` が通らない場合は STLport を使う (`STLport = yes`)。
- `MACH_TYPE = js` は `em++` (Emscripten) でビルドし、`em-pre.js` / `em-post.js` で包んだ `Kawari` コンストラクタを `module.exports` する。公開する関数は `gcc-mach.mak` の `EXPORTED_FUNCTIONS`、JS 側へ露出するオブジェクトは `em-post.js` で管理している。
- bcc32 / VC++ でビルドするにはソースを SJIS に変換する必要がある (`build/conv.sh -c sjis src`、Ruby の `kconv` 前提で古い)。**ファイルをその場で書き換える**ので、作業ツリーではなくコピーに対して実行すること。

## テスト

ビルド後に `powershell -NoProfile -ExecutionPolicy Bypass -File build/src/test/run.ps1` (詳細は `build/src/test/README.md`)。`kosui.exe` に 1 行ずつ評価させる kosui スイートと、`shiori.dll` を直接 `load` / `request` / `unload` する shiori スイートがあり、ケースは `kosui/cases.txt` / `shiori/cases.txt` に足す。

- shiori スイートは C のハーネスをその場でビルドするので、**DLL と同じ 32bit の gcc** が `PATH` の先頭に必要 (ビット数が違うとランナーが止まる)。
- テスト用のソース・辞書・ケースは ASCII のみ (本体の EUC-JP と混ぜない)。
- kosui を手で使うときの注意: 入力の最後に `exit` が無いと EOF で無限ループする。非 ASCII を含む出力は未検証。KIS の要素数は `size <エントリ>`。辞書のカンマは単語の区切りで、複数単語のエントリはランダムに 1 つが選ばれる (期待値が揺れる原因)。
- .NET Framework の `Process` は標準入力の先頭に UTF-8 BOM を付ける。kosui のような stdin を読むツールには、`cmd /c "exe < file"` で渡すこと (`run.ps1` はそうしている)。

## 華和梨スクリプト (KIS) の勘所

テストやデバッグで辞書を書くときの記憶の鍵。文法の詳細は `document/kis_reference.html`、構文 BNF は `document/kiu_specbnf.txt`。以下は `kosui` で動作確認したもの。

```
# 辞書 (kawarirc.kis など)。"エントリ名 : 単語"。コメントは行頭の #
greet : Hello
nest  : ${greet} world!        # ${エントリ} で参照。入れ子も可: ${${key}}
list  : a,b,c                  # カンマで単語を区切る。参照すると 1 つがランダムに選ばれる

# スクリプト領域 (=kis ～ =end) の外の行は辞書、内側は KIS の文の並び
=kis
function twice $(
    echo $@arg[1]$@arg[1];     # $@arg[n] が引数。twice ab -> abab
);
=end
```

```
${greet}                       -> Hello
$[ ${x} + 1 ]                  -> 式の評価 (x が 1 なら 2)
$(echo hi)                     -> hi         インラインスクリプトは $(コマンド 引数...)
$(size list)                   -> 3          エントリの単語数
$(set A "TEST")                -> (空)       代入。次の行の ${A} は TEST。文の間の空白は出力に残る
$(if $[ 1 < 2 ] $(echo yes) else $(echo no))   -> yes
$(adddict list "d")            -> 単語の追加 (size は 4 になる)
```

- SHIORI/3.0 の入口は `System.Callback.OnGET` (GET) と `System.Callback.OnNOTIFY`。リクエストヘッダは `System.Request.<ヘッダ名>` (例: `${System.Request.ID}`, `${System.Request.Reference0}`) に入り、`System.Response.<ヘッダ名>` を設定すると応答ヘッダになる。評価結果が空なら 204、あれば 200 で Value に入る。
- `version` / `craftman` / `shioriid` / `name` はモジュールが自動で答える (辞書で上書き可)。最小の動くハンドラは `build/src/test/shiori/kawarirc.kis`。
- 未定義のエントリを参照すると空文字になりエラーにならない。タイプミスを見つけるには kosui で `loglevel error warning decl` を指定する。

## その他

- 仕様の一次資料は `document/` (`kis_reference.html` が KIS コマンド、`kiu_specbnf.txt` が構文 BNF)。更新履歴は `document/changelog.txt`、バージョンは `libkawari/kawari_version.h`。
- `build/contrib/` は過去に取り込んだパッチ (Python モジュール化、darwin、SAORI フォールバック等) の保管場所。
- `config.h` が OS / コンパイラ判定 (`KAWARI_WIN32` / `KAWARI_POSIX` 等) を担う。`js` ターゲットは `-U_WIN32` 等で Windows 判定を打ち消している。
