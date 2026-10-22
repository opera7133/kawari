//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// ロガー
//
//      Programed by Suikyo
//
//  2002.04.12  Phase 8.0.0   Engineから分離
//  2026.10.18  Phase 9.0.0   wchar_t化 (std::wostream + バイト列変換streambuf)
//
//---------------------------------------------------------------------------
#ifndef KAWARI_LOG_H
#define KAWARI_LOG_H
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <cstdio>
#include "misc/charset.h"
using namespace std;

namespace kawari_log {
	// 診断コードの番号部分 (K-E<番号> / K-W<番号>)。
	// 追記専用。公開後は番号を変えず、使わなくなっても欠番にする。
	// 0001-0999 : コンパイラ・VM・辞書、1000- : KIS コマンド
	enum DiagCode {
		// エラー (K-Exxxx)
		E_QUOTE_EOL = 1,
		E_QUOTE_EOF = 2,
		E_UNKNOWN_MODE = 3,
		E_NO_ENTRYNAMES = 4,
		E_MLENTRYDEF_NOT_CLOSED = 5,
		E_INVALID_ENTRYDEF = 6,
		E_ILLCHAR_IN_SCRIPT = 7,
		E_ENTRYID_NOT_FOUND = 8,
		E_FIRST_STATEMENT_NOT_FOUND = 9,
		E_STATEMENTLIST_SEPARATOR_NOT_FOUND = 10,
		E_INTERNAL_SUBST = 11,
		E_INTERNAL_INLINE_SCRIPT = 12,
		E_SCRIPT_SUBST_NOT_CLOSED = 13,
		E_INTERNAL_BLOCK = 14,
		E_BLOCK_NOT_CLOSED = 15,
		E_INTERNAL_INDEX = 16,
		E_INDEX_BRACKET_NOT_FOUND = 17,
		E_INDEX_EXPR_NOT_FOUND = 18,
		E_INTERNAL_EXPR = 19,
		E_EXPR_NOT_CLOSED = 20,
		E_PARSE_ERROR_AFTER = 21,
		E_EXPR_BLOCK_NOT_CLOSED = 22,
		E_ILLCHAR_IN_EXPRESSION = 23,
		E_INTERNAL_ENTRYCALL = 24,
		E_ENTRYCALL_NOT_CLOSED = 25,
		E_SETEXPR_BLOCK_NOT_CLOSED = 26,
		E_DIVIDED_BY_ZERO = 27,
		E_UNDEFINED_FUNCTION = 28,
		E_ENGINE_UNKNOWN_MODE = 29,
		E_PROTECTED_ENTRY = 30,
		E_CAN_NOT_DELETE_WORD = 31,
		E_KIS_INVALID_INDEX = 32,
		E_KIS_ILLEGAL_COPY_OPERATION = 33,
		E_KIS_FILE_SAVE_FAILED = 34,
		E_KIS_FILE_LOAD_FAILED = 35,
		E_KIS_SAORI_CALL_FAILED = 36,
		E_BREAK_OUTOF_LOOP = 37,
		E_CONTINUE_OUTOF_LOOP = 38,
		E_KIS_ERROR = 1000,			// 個別の番号を持たない KIS コマンドのエラー
		E_KIS_REGEX = 1100,			// 正規表現
		E_KIS_REGEX_OPTION = 1101,
		E_KIS_REGEX_NESTED = 1102,
		E_SAORI = 1200,				// SAORI (個別の番号を持たないもの)
		// 警告 (K-Wxxxx)
		W_BLANK_DEFINITION = 1,
		W_ENTRYIDLIST_ENDS_WITH_COMMA = 2,
		W_READ_EMPTY_ENTRY = 3,		// loglevel decl
		W_KIS_WARNING = 1000,		// 個別の番号を持たない KIS コマンドの警告
		W_KIS_TOO_FEW_ARGUMENTS = 1001,
		W_KIS_TOO_MANY_ARGUMENTS = 1002,
		W_SAORI = 1200
	};

	enum LOGLEVEL {
		LOG_ERROR		=0x0001,
		LOG_WARNING		=0x0002,
		LOG_INFO		=0x0004,
		LOG_DECL		=0x0008,
		LOG_DUMP		=0x0010,
		LOG_BASEEVENTS	=0x0100,
		LOG_RSCEVENTS	=0x0200,
		LOG_MOUSEEVENTS	=0x0400,
		LOG_TIMEEVENTS	=0x0800
	};
}

//---------------------------------------------------------------------------
// wide文字列をバイト列(指定文字コード)に変換して、std::ostreamに書き出すstreambuf
// 改行・sync・一定量たまった時点で変換して書き出す
class TWideToByteStreamBuf : public std::wstreambuf {
private:
	std::ostream *dest;
	FILE *fdest;		// FILE*出力先 (destより優先。閉じるのは呼び出し側)
	TKawariCharset charset;
	bool crlf;			// trueなら LF を CRLF に変換して出す
	std::wstring pending;

	void Flush(bool final);
protected:
	virtual int_type overflow(int_type c=traits_type::eof());
	virtual std::streamsize xsputn(const wchar_t *s, std::streamsize n);
	virtual int sync(void);
public:
	TWideToByteStreamBuf(std::ostream *os, TKawariCharset cs, bool crlf_=false);
	TWideToByteStreamBuf(FILE *fp, TKawariCharset cs, bool crlf_=false);
	virtual ~TWideToByteStreamBuf();
	// 出力先の変更
	void SetDestination(std::ostream *os, TKawariCharset cs, bool crlf_=false);
	void SetDestination(FILE *fp, TKawariCharset cs, bool crlf_=false);
};
//---------------------------------------------------------------------------
// TWideToByteStreamBufを内蔵したstd::wostream
// 例: ofstream ofs(...,ios::binary); TWideOStream wos(&ofs, KC_UTF8); wos << L"..." << endl;
class TWideOStream : public std::wostream {
private:
	TWideToByteStreamBuf buf;
public:
	TWideOStream(std::ostream *os, TKawariCharset cs, bool crlf=false);
	TWideOStream(FILE *fp, TKawariCharset cs, bool crlf=false);
	virtual ~TWideOStream();
};
//---------------------------------------------------------------------------
// 診断の位置情報を提供するインターフェース (実体は仮想機械)
class IDiagContext {
public:
	virtual ~IDiagContext() {}
	// 現在実行中の位置。分からなければ false。line が 0 以下なら行は不明
	virtual bool GetLocation(std::wstring &file, int &line) const=0;
	// 呼び出し連鎖を内側から外側へ 1 フレーム 1 行で返す (最大 maxdepth 行)
	virtual void GetCallChain(std::vector<std::wstring> &lines, unsigned int maxdepth) const=0;
};
//---------------------------------------------------------------------------
class TKawariLogger;
//---------------------------------------------------------------------------
// 出力された診断 1 件の記録 (kosui --check が集計・JSON 出力に使う)
struct TDiagRecord {
	bool iserror;					// true : error, false : warning
	unsigned int code;				// 番号部分 (kawari_log::DiagCode)
	bool hasloc;
	std::wstring file;
	int line;						// 0 以下なら不明
	std::wstring message;
	std::vector<std::wstring> chain;	// 呼び出し連鎖 ("in ..." の行)
	TDiagRecord(void) : iserror(false), code(0), hasloc(false), line(0) {}
};
//---------------------------------------------------------------------------
// 出力された診断の集計。TKawariLogger に取り付ける (持ち主は呼び出し側)
struct TDiagCollector {
	unsigned int errors;
	unsigned int warnings;
	bool keep;						// true なら records に 1 件ずつ残す
	std::vector<TDiagRecord> records;
	TDiagCollector(void) : errors(0), warnings(0), keep(false) {}
};
//---------------------------------------------------------------------------
// 診断メッセージを溜めておき、破棄時に「位置: 種別 コード: 本文」の形で出力する
// 一時オブジェクト。TKawariLogger::Diag() が返す。
//   logger.Diag(LOG_ERROR, E_XXX) << L"..." << x << endl;
// loglevel で無効な種別なら何も溜めず何も出さない。
class TDiagStream {
private:
	TKawariLogger *logger;
	kawari_log::LOGLEVEL lvl;
	unsigned int code;
	mutable bool active;
	bool explicitloc;
	std::wstring file;
	int line;
	unsigned int depth;
	std::wstring buf;
public:
	TDiagStream(TKawariLogger *lg, kawari_log::LOGLEVEL l, unsigned int c, bool act, unsigned int dep)
		: logger(lg), lvl(l), code(c), active(act), explicitloc(false), line(0), depth(dep) {}
	// 一時オブジェクトの受け渡し用 (出力の責任は新しい方に移る)
	TDiagStream(const TDiagStream &o)
		: logger(o.logger), lvl(o.lvl), code(o.code), active(o.active), explicitloc(o.explicitloc),
		  file(o.file), line(o.line), depth(o.depth), buf(o.buf) { o.active=false; }
	~TDiagStream();

	// 位置を明示する (字句解析など、実行位置とは別の場所を指すとき)
	TDiagStream &At(const std::wstring &f, int ln) { file=f; line=ln; explicitloc=true; return *this; }

	template<class T> TDiagStream &operator<<(const T &v) {
		if (active) { std::wostringstream o; o << v; buf+=o.str(); }
		return *this;
	}
	TDiagStream &operator<<(std::wostream &(*pf)(std::wostream &)) {
		if (active) { std::wostringstream o; pf(o); buf+=o.str(); }
		return *this;
	}
};
//---------------------------------------------------------------------------
class TKawariLogger {
private:
	// ログストリーム
	std::wostream *LogStream;

	// どこにも入出力しないストリーム
	std::wostream *NullStream;

	// 標準出力用 (SetStreamStdOutで初めて作られる。cout へKC_ACPで出力)
	std::wostream *StdOutStream;

	// エラーレベル (論理和)
	unsigned int errlevel;

	// 診断の位置情報の提供元 (仮想機械。無ければ位置なしで出力)
	IDiagContext *diagctx;

	// 診断だけの出力先。NULL なら通常のログストリームへ出す
	std::wostream *diagsink;

	// 診断の集計 (無ければ数えない)
	TDiagCollector *collector;

	// true ならエラーレベルを変えさせない (loglevel 文が診断を消さないようにする)
	bool levellocked;

public:
	// 診断の出力先を通常のログとは別にする (NULL で元に戻す)
	void SetDiagSink(std::wostream *sink) { diagsink=sink; }

	// 診断を集計する (NULL で止める)
	void SetDiagCollector(TDiagCollector *c) { collector=c; }

	// エラーレベルを固定する (固定中の SetErrLevel は無視される)
	void LockLevel(bool lock) { levellocked=lock; }

	// 診断メッセージの出力 (位置は実行中の位置。呼び出し連鎖は depth 行まで)
	TDiagStream Diag(kawari_log::LOGLEVEL lvl, unsigned int code, unsigned int depth=8) const {
		return TDiagStream(const_cast<TKawariLogger *>(this), lvl, code, Check(lvl), depth);
	}

	// 診断メッセージの実出力 (TDiagStream から呼ばれる)
	void EmitDiag(kawari_log::LOGLEVEL lvl, unsigned int code, bool explicitloc,
				  const std::wstring &file, int line, unsigned int depth, const std::wstring &msg) const;

	// 位置情報の提供元の設定
	void SetDiagContext(IDiagContext *ctx) { diagctx=ctx; }
	IDiagContext *GetDiagContext(void) const { return diagctx; }

	// 出力ストリーム取得
	std::wostream& GetStream(void) const;

	// 出力ストリーム取得
	std::wostream& GetStream(kawari_log::LOGLEVEL lvl) const;

	// 出力ストリームの設定 (NULLならどこにも出力しない)
	void SetStream(std::wostream *outstream);

	// 出力ストリームを標準出力(コンソール。KC_ACP)に設定
	void SetStreamStdOut(void);

	// エラーレベルを得る
	unsigned int ErrLevel(void) const { return errlevel; }

	// エラーレベルの設定
	void SetErrLevel(unsigned int level);

	// エラーレベルのチェック
	bool Check(unsigned int lvl) const{
		return ((errlevel&lvl)!=0);
	}
	TKawariLogger (void);
	~TKawariLogger ();
};
//---------------------------------------------------------------------------
// 出力ストリーム取得
inline std::wostream& TKawariLogger::GetStream(void) const
{
	return(*LogStream);
}
//---------------------------------------------------------------------------
// 出力ストリーム取得
inline std::wostream& TKawariLogger::GetStream(kawari_log::LOGLEVEL lvl) const{
	if ((errlevel&lvl)!=0)
		return(*LogStream);
	else
		return(*NullStream);
}
//---------------------------------------------------------------------------
// 出力ストリームの設定
inline void TKawariLogger::SetStream(std::wostream *outstream)
{
	if (outstream)
		LogStream=outstream;
	else
		LogStream=NullStream;
}
//---------------------------------------------------------------------------
// エラーレベルの設定
inline void TKawariLogger::SetErrLevel(unsigned int level){
	if (!levellocked) errlevel=level;
}
//---------------------------------------------------------------------------
//extern TKawariLogger Logger;
//---------------------------------------------------------------------------
#endif // KAWARI_LOG_H
