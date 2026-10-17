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
- 動作中のゴーストがあると kosui が `Attach :` を聞いてくるので、テスト前にゴーストを終了しておく。

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
