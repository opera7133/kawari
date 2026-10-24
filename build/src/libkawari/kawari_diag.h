//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// 診断 (定義位置の記録と診断コード)
//
//  2026.10.22  Phase 9.2.0   作成
//
//---------------------------------------------------------------------------
#ifndef KAWARI_DIAG_H
#define KAWARI_DIAG_H
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "libkawari/kawari_rc.h"
//---------------------------------------------------------------------------
#include <string>
#include <vector>
#include <map>
#include <set>
//---------------------------------------------------------------------------
// 辞書ファイルから読み込んだ単語・エントリの定義位置を覚えておく
//
// 単語は内容が同じなら 1 つに共有される (TWordID が同じになる) ので、
// 位置は中間コードのノードではなく (エントリ, 単語) の組に付ける。
// 単語が削除されると ID が再利用されるので、削除時に必ず EraseWord() すること。
// 辞書から読み込んだもの以外 (実行時に set / adddict などで作ったもの) は記録しない。
class TSourceMap {
public:
	struct Loc {
		unsigned int file;	// FileName() の添字。0 なら不明
		unsigned int line;	// 1 オリジン。0 なら不明
		Loc(void) : file(0), line(0) {}
	};

	TSourceMap(void);

	// ファイル名の添字を得る (無ければ登録する)
	unsigned int FileIndex(const std::wstring &name);

	// 添字からファイル名を得る (範囲外・0 は空)
	const std::wstring &FileName(unsigned int index) const;

	// エントリ entry に単語 word を定義した位置を記録する
	// 同じ組が既にあれば最初の記録を残す
	void RecordWord(unsigned int entry, unsigned int word, unsigned int file, unsigned int line);

	// 単語が削除されたときに、その単語の記録を全て消す
	void EraseWord(unsigned int word);

	// 単語の定義位置を探す
	// entry が 0 でなければ、そのエントリでの定義だけを探す。
	// 0 なら最初に記録された定義を返す (どのエントリから呼ばれたか分からないとき)。
	bool FindWord(unsigned int entry, unsigned int word, Loc &loc) const;

	// エントリが最初に定義された位置を探す
	bool FindEntry(unsigned int entry, Loc &loc) const;

private:
	struct Rec {
		unsigned int entry;
		unsigned int file;
		unsigned int line;
		Rec(void) : entry(0), file(0), line(0) {}
	};

	std::vector<std::wstring> files;
	std::map<std::wstring, unsigned int> fileindex;
	// 単語 ID で引く。最初の定義
	std::vector<Rec> primary;
	// 同じ単語が別のエントリでも定義されているときの 2 番目以降
	std::multimap<unsigned int, Rec> extra;
	// エントリ ID で引く。最初の定義
	std::vector<Rec> entryfirst;
};
//---------------------------------------------------------------------------
// 静的検査 (kosui --check) のために、読み込み中に集めておく情報
//  - コンパイラだけが気づける問題 (木には残らない記述の癖) の所見
//  - 「# lint:disable-next-line=K-W2001,K-W2003」の抑制コメント
// 検査用のときだけエンジンが持ち、コンパイラ・字句解析器に渡す。通常は NULL で何もしない。
class TLintNotes {
public:
	struct Note {
		std::wstring file;
		unsigned int line;
		unsigned int code;		// 警告の番号部分 (kawari_log::DiagCode)
		std::wstring name;		// 問題のあるコマンド名など
		Note(void) : line(0), code(0) {}
	};
	std::vector<Note> notes;

	// 抑制コメントを覚える。comment はコメント行の「#」より後ろ (前後の空白は問わない)。
	// line はコメントのある行で、効くのはその次の行。抑制コメントでなければ何もしない。
	void ReadComment(const std::wstring &file, unsigned int line, const std::wstring &comment);

	// file の line 行を code について抑制しているか
	bool IsSuppressed(const std::wstring &file, unsigned int line, unsigned int code) const;

private:
	std::map<std::wstring, std::map<unsigned int, std::set<unsigned int> > > suppress;
};
//---------------------------------------------------------------------------
namespace kawari {
namespace diag {
	// リソース ID に対応する診断コードの番号 (kawari_log::DiagCode)
	// 診断コードを持たない ID なら 0
	unsigned int NumberOf(kawari::resource::ID id);
}
}
//---------------------------------------------------------------------------
#endif // KAWARI_DIAG_H
