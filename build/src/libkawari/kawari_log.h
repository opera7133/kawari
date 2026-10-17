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
#include <cstdio>
#include "misc/charset.h"
using namespace std;

namespace kawari_log {
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

public:
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
	errlevel=level;
}
//---------------------------------------------------------------------------
//extern TKawariLogger Logger;
//---------------------------------------------------------------------------
#endif // KAWARI_LOG_H
