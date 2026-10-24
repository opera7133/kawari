//--------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// 字句解析
//
//      Programed by Suikyo
//
//  2002.04.18  Phase 8.0.0   書き直し
//  2026.10.18  Phase 9.0.0   wchar_t化 (std::wistreamを読む)
//
//--------------------------------------------------------------------------
#ifndef LEXER_H__
#define LEXER_H__
//--------------------------------------------------------------------------
#include "config.h"
//--------------------------------------------------------------------------
#include "libkawari/kawari_log.h"
#include "libkawari/kawari_rc.h"
#include "libkawari/kawari_diag.h"
#include "misc/charset.h"
//--------------------------------------------------------------------------
#include <iostream>
#include <string>
#include <vector>
//--------------------------------------------------------------------------
class Token;
class TKawariLexer;
class TKawariPreProcessor;
//--------------------------------------------------------------------------
class Token {
public:
	// 特殊文字は、全て文字コードがそのままintになって返る。
	enum Type {
		T_ERROR = 0x100,	//
		T_LITERAL,		//
		T_QLITERAL,		//
		T_SPACE,		// ' ', '\t'
		T_EOL,			// CR, LF, CRLF
		T_SPECIAL,		// special marks (in peek() only)
		T_MODESWITCH,	// '=xxxxx'
		T_EOF,
		T_DUMMY
	} type;
	std::wstring str;
	Token (Type t, const std::wstring &s) : type(t), str(s) {}
	Token (void) : type(T_DUMMY), str(L"") {}
	Token &set (Type t) { type=t; return *this; }
	Token &set (const std::wstring &s) { str=s; return *this; }
	Token &set (Type t, const std::wstring &s)
		{ type=t; str=s; return *this; }
	Token &set (Type t, const wchar_t &ch)
		{ type=t; str+=ch; return *this; }
};

//-------------------------------------------------------------------------
// class TKawariLexer;
//
class TKawariLexer{
public:
	// コンストラクタ
	TKawariLexer (std::wistream &input, TKawariLogger &lgr,
				  std::wstring filename=L"<unknown>",
				  bool preprocess=true, int lineno=0,
				  TKawariCharset cryptcharset=KC_SJIS);

	// Lexerのモード
	enum Mode {
		ID_MODE = 0,			// エントリ名モード
		LITERAL_MODE = 1,		// 文字列モード(エントリ定義中)
		LITERAL_MODE2 = 2,		// 文字列モード(インラインスクリプト中)
		LITERAL_MODE3 = 3		// 文字列モード(ブロック中)
	};

	// 次のトークンがあるか
	bool hasNext(void);

	// 次のトークンのタイプを返す。
	// 読むだけでポインタを進めない
	// mode : Lexerのモード指定(LITERAL_MODE, ID_MODE)
	Token::Type peek(Mode mode=LITERAL_MODE);

	// 次のトークンを返す
	// mode : Lexerのモード指定(LITERAL_MODE, ID_MODE)
	Token next(Mode mode=LITERAL_MODE);

	// !!! check type before use this !!!
	// 次のリテラルトークンを返す
	// 次がベアリテラルでなかったら、""が返る。予め分かっている場合にだけ使うこと。
	// mode : Lexerのモード指定(LITERAL_MODE, ID_MODE)
	std::wstring getLiteral(Mode mode=LITERAL_MODE);

	// !!! check type, before use this !!!
	// 次のクォートリテラルトークン(非デコード)を返す
	// 次がクォートリテラルでなかったら""が返る。
	std::wstring getQuotedLiteral(void);

	// !!! check type before use this !!!
	// [0-9]+
	// 整数文字列
	std::wstring getDecimalLiteral(void);

	// 1文字スキップ
	int skip(void);

	// ある文字の手前までスキップ (文法エラーからの復旧用)
	// noret : 改行を許さない場合true
	bool simpleSkipTo(wchar_t ch, bool noret=true);

	// ファイル名を返す
	const std::wstring &getFileName(void) const;

	// 現在の行番号を返す
	int getLineNo(void) const;

	// 現在行の残りを全て返す
	std::wstring getRestOfLine(void);

	// モードスイッチのリセット
	inline void ResetModeSwitch(void);

	// サービスメソッド
	// 空白(空白文字と改行文字)のトークンを読み飛ばす。
	void skipWS(void);

	// サービスメソッド
	// 空白(空白文字と改行文字)のトークンを読み飛ばし、
	// その次のトークンのタイプを返す
	Token::Type skipWS(Mode m);

	// サービスメソッド
	// 空白文字のトークンを読み飛ばし、その次のトークンのタイプを返す
	// 改行文字は飛ばさない。
	Token::Type skipS(Mode m=LITERAL_MODE);

	// 指定長さだけリードポインタを戻す
	bool UngetChars(unsigned int length);

	// サービスメソッド
	// クォート文字列をデコード
	static std::wstring DecodeQuotedString(const std::wstring &orgsen);

	// サービスメソッド
	// 文字列をエントリ名で使用可能な文字列にエンコードする
	static std::wstring EncodeEntryName(const std::wstring &orgsen);

	~TKawariLexer ();

	// コンパイルエラー (メッセージはリソース ID。suffix は末尾に続ける)
	void error(kawari::resource::ID id, const std::wstring &suffix=L"");

	// コンパイル警告
	void warning(kawari::resource::ID id, const std::wstring &suffix=L"");

	// 静的検査用の情報の受け取り先 (NULL で何もしない)。コメント行の抑制指定を拾う。
	void SetLintNotes(TLintNotes *notes);

private:
	// 文字からトークンタイプを得る
	Token::Type checkType(Mode m, wchar_t ch) const;

	class TKawariPreProcessor *pp;
	std::wstring fn;

	TKawariLogger &logger;
};
//---------------------------------------------------------------------------
// プリプロセッサ
//
class TKawariPreProcessor {
public:
	TKawariPreProcessor(std::wistream &input, int lineno=0, bool preprocess=true,
						TKawariCharset cryptcharset=KC_SJIS)
		: is(input), pp(preprocess), mc(false), isMS(false), ln(lineno), pos(0), cryptcs(cryptcharset),
	  lintnotes(NULL) {}
	bool peek(wchar_t &ch);
	bool getch(wchar_t &ch);
	bool eof(void);
	std::wstring getline(void);
	bool unget(void);
	int getLineNo(void){ return ln; }

	int getColumn(void){ return pos; }

	// バッファの一部を切り出す
	std::wstring substring(int index, int length);

	virtual ~TKawariPreProcessor() {}

	// 現在、モード切替待ちか
	bool IsWaitingModeSwitch(void){ return isMS; }

	// モードフラグのリセット
	void ResetModeSwitch(void){ isMS=false; }

	// 静的検査用 : コメント行の抑制指定を notes に渡す (file はその所属ファイル名)
	void SetLintNotes(TLintNotes *notes, const std::wstring &file){ lintnotes=notes; lintfile=file; }

private:
	bool processNextLine(void);
	// 入力ストリーム
	std::wistream &is;
	// pre-processするか否かのフラグ
	bool pp;
	// 複数行コメントのフラグ
	bool mc;

	// Mode Switch
	bool isMS;
	// 現在行の行数
	unsigned int ln;
	// 現在カラム
	unsigned int pos;
	// 暗号化行を復号した後の文字コード
	TKawariCharset cryptcs;
	// 出力バッファ
	std::wstring linebuf;
	// 静的検査用の受け取り先 (無ければ NULL)
	TLintNotes *lintnotes;
	std::wstring lintfile;
};
//---------------------------------------------------------------------------
inline void TKawariLexer::SetLintNotes(TLintNotes *notes){
	pp->SetLintNotes(notes, fn);
}
//---------------------------------------------------------------------------
// 次のトークンがあるか
inline bool TKawariLexer::hasNext(void){
	return !pp->eof();
}
//---------------------------------------------------------------------------
inline void TKawariLexer::ResetModeSwitch(void){
	pp->ResetModeSwitch();
}
//---------------------------------------------------------------------------
// ファイル名が無い (実行時に文字列をコンパイルしている) ときは、
// 字句解析器の行ではなく実行中の位置を使う。
inline void TKawariLexer::error(kawari::resource::ID id, const std::wstring &suffix){
	TDiagStream ds=logger.Diag(kawari_log::LOG_ERROR, kawari::diag::NumberOf(id));
	if (getFileName()!=L"<unknown>") ds.At(getFileName(), getLineNo());
	ds << RC.S(id) << suffix;
}
//---------------------------------------------------------------------------
inline void TKawariLexer::warning(kawari::resource::ID id, const std::wstring &suffix){
	TDiagStream ds=logger.Diag(kawari_log::LOG_WARNING, kawari::diag::NumberOf(id));
	if (getFileName()!=L"<unknown>") ds.At(getFileName(), getLineNo());
	ds << RC.S(id) << suffix;
}
//---------------------------------------------------------------------------
inline bool TKawariPreProcessor::peek(wchar_t &ch){
	// EOFで読めなかったときに戻すと、最後の文字がまた読めてしまう
	if (!getch(ch)) return false;
	unget();
	return true;
}
//---------------------------------------------------------------------------
inline bool TKawariPreProcessor::getch(wchar_t &ch){
	if (pos>=linebuf.size())
		if (!processNextLine())
			return false;
	ch=linebuf[pos++];
	return true;
}
//---------------------------------------------------------------------------
inline bool TKawariPreProcessor::unget (void){
	if (pos==0) return false;
	pos--;
	return true;
}
//---------------------------------------------------------------------------
inline bool TKawariPreProcessor::eof (void){
	return ((pos>=linebuf.size())&&(is.eof()));
}
//---------------------------------------------------------------------------
#endif  // LEXER_H__
