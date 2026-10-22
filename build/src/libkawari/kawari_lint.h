//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// 静的検査 (kosui --check の Lint)
//
//  2026.10.23  Phase 9.2.0   作成
//
//---------------------------------------------------------------------------
#ifndef KAWARI_LINT_H
#define KAWARI_LINT_H
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include <string>
#include <vector>
#include <set>
//---------------------------------------------------------------------------
class TNS_KawariDictionary;
class TKawariVM;
class TKawariLogger;
class TKVMCode_base;
class TKVMCodeScriptStatement;
class TLintNotes;
//---------------------------------------------------------------------------
// 読み込み済みの辞書と =kis ゾーンの中間コードを歩いて、実行しなくても分かる
// 疑わしい記述を警告する。実行には一切関わらない (--check からだけ呼ぶ)。
//
//  K-W2001 : どこにも定義されていないエントリの参照
//  K-W2002 : 組み込みにも定義済みの関数にもないコマンド名
//
// 偽陽性を減らすため、次のものは「定義されている」と見なす。
//  - 辞書に存在するエントリ
//  - KIS の文の引数に、リテラルとして現れた名前 (set / adddict / foreach の変数など)
//  - System. で始まる名前 (実行時に本体や SAORI が作る)、@ で始まるローカル名
//  - 名前の一部が ${...} などで組み立てられるもの (静的には分からない)
class TKawariLint {
public:
	// 読み込み時に保持した =kis ゾーンの中間コード (所有はしない)
	struct Zone {
		unsigned int file;		// TSourceMap のファイル名の添字
		const TKVMCode_base *code;
	};

	// 検査を実行する。戻り値は出した警告の数
	// (呼び出し側がこのクラスの大きさに依存しないよう、実体は .cpp の中で作る)
	// notes : 読み込み中にコンパイラが集めた所見と抑制コメント (NULL でも良い)
	static unsigned int Check(TNS_KawariDictionary &d, TKawariVM &v, TKawariLogger &l,
							  const std::vector<Zone> &zones, const TLintNotes *notes);

	TKawariLint(TNS_KawariDictionary &d, TKawariVM &v, TKawariLogger &l);

	unsigned int Run(const std::vector<Zone> &zones, const TLintNotes *notes);

private:
	// 今歩いている場所
	struct Ctx {
		unsigned int file;		// 0 なら不明
		int line;				// 0 以下なら不明
		const std::wstring *entry;	// 単語を持つエントリ名 (ゾーンなら NULL)
		Ctx(void) : file(0), line(0), entry(NULL) {}
	};

	// 歩き始める場所 (辞書の単語 1 つ、または =kis ゾーン 1 つ)
	struct Root {
		const TKVMCode_base *code;
		Ctx ctx;
		std::wstring entryname;
		Root(void) : code(NULL) {}
	};

	// 見つけた問題。最後に (ファイル, 行) の順にそろえて出力する
	struct Finding {
		unsigned int file;
		int line;
		unsigned int seq;		// 見つけた順 (同じ行の並びを安定させる)
		unsigned int code;		// 警告の番号部分 (kawari_log::DiagCode)
		std::wstring name;
		std::wstring entry;		// 位置が分からないときに添えるエントリ名
		Finding(void) : file(0), line(0), seq(0), code(0) {}
	};
	static bool FindingLess(const Finding &a, const Finding &b);

	TNS_KawariDictionary &dict;
	TKawariVM &vm;
	TKawariLogger &logger;

	// 1 周目 : 定義の収集 / 2 周目 : 検査
	bool collecting;
	// 文の引数にリテラルで現れた名前 ("名前[添字]" は添字を除く)
	std::set<std::wstring> mentioned;
	// function 文で定義される関数名
	std::set<std::wstring> funcs;
	// 文の引数で、名前の一部が組み立てられていたものの先頭のリテラル (launch_a_"${n}" の launch_a_)
	std::set<std::wstring> dynprefix;
	// 同じ場所での重複報告を避ける
	std::set<std::wstring> reported;
	std::vector<Finding> findings;

	void Walk(const TKVMCode_base *code, const Ctx &ctx);
	void Statement(const TKVMCodeScriptStatement *st, const Ctx &ctx);
	void Reference(const std::wstring &name, const Ctx &ctx);
	bool IsKnownCommand(const std::wstring &name);
	static bool LooksLikeEntryName(const std::wstring &s);
	void CollectEmbedded(const std::wstring &s);
	// code : kawari_log::W_LINT_*
	void Report(unsigned int code, const std::wstring &name, const Ctx &ctx);
};
//---------------------------------------------------------------------------
#endif // KAWARI_LINT_H
