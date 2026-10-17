//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// ロガー
//
//      Programed by Suikyo
//
//  2002.04.12  Phase 8.0.0   作成
//  2008.01.23  Phase 8.2.5   include <cstdio> to define [EOF] (PaulLiu)
//  2026.10.18  Phase 9.0.0   wchar_t化
//
//---------------------------------------------------------------------------
#include "libkawari/kawari_log.h"
#include <cstdio>
//---------------------------------------------------------------------------
class nullstreambuf : public wstreambuf {
public:
	nullstreambuf (): wstreambuf() {}
	virtual int_type overflow(int_type p_iChar=traits_type::eof()){
		return traits_type::not_eof(p_iChar);
	}
	virtual std::streamsize xsputn(const wchar_t *, std::streamsize n){
		return n;
	}
	virtual int_type uflow(void){
		return traits_type::eof();
	}
	virtual int_type pbackfail(int_type p_iChar=traits_type::eof()){
		return p_iChar;
	}
	virtual int_type underflow(void){
		return traits_type::eof();
	}
	virtual int sync(){
		return 0;
	}
};
//---------------------------------------------------------------------------
TWideToByteStreamBuf::TWideToByteStreamBuf(std::ostream *os, TKawariCharset cs, bool crlf_)
	: dest(os), fdest(NULL), charset(cs), crlf(crlf_)
{
}
//---------------------------------------------------------------------------
TWideToByteStreamBuf::TWideToByteStreamBuf(FILE *fp, TKawariCharset cs, bool crlf_)
	: dest(NULL), fdest(fp), charset(cs), crlf(crlf_)
{
}
//---------------------------------------------------------------------------
TWideToByteStreamBuf::~TWideToByteStreamBuf()
{
	Flush(true);
}
//---------------------------------------------------------------------------
void TWideToByteStreamBuf::SetDestination(std::ostream *os, TKawariCharset cs, bool crlf_)
{
	Flush(true);
	dest=os;
	fdest=NULL;
	charset=cs;
	crlf=crlf_;
}
//---------------------------------------------------------------------------
void TWideToByteStreamBuf::SetDestination(FILE *fp, TKawariCharset cs, bool crlf_)
{
	Flush(true);
	dest=NULL;
	fdest=fp;
	charset=cs;
	crlf=crlf_;
}
//---------------------------------------------------------------------------
void TWideToByteStreamBuf::Flush(bool final)
{
	if(pending.empty()) return;

	wstring::size_type len=pending.size();
	// 16bit wchar_tでは、末尾の上位サロゲートは次回に回す
	if((!final)&&(sizeof(wchar_t)<4)) {
		unsigned long last=(unsigned long)pending[len-1]&0xffff;
		if((last>=0xd800)&&(last<=0xdbff)) len--;
	}
	if(len==0) return;

	if(dest||fdest) {
		wstring w=pending.substr(0, len);
		if(crlf) {
			wstring c;
			c.reserve(w.size()+16);
			for(wstring::size_type i=0; i<w.size(); i++) {
				if(w[i]==0x0a) c+=(wchar_t)0x0d;
				c+=w[i];
			}
			w.swap(c);
		}
		string bytes=WtoMB(w, charset);
		if(fdest) {
			if(!bytes.empty()) fwrite(bytes.data(), 1, bytes.size(), fdest);
			if(final) fflush(fdest);
		} else {
			if(!bytes.empty()) dest->write(bytes.data(), (std::streamsize)bytes.size());
			if(final) dest->flush();
		}
	}
	pending.erase(0, len);
}
//---------------------------------------------------------------------------
TWideToByteStreamBuf::int_type TWideToByteStreamBuf::overflow(int_type c)
{
	if(!traits_type::eq_int_type(c, traits_type::eof())) {
		wchar_t wc=traits_type::to_char_type(c);
		pending+=wc;
		if((wc==L'\n')||(pending.size()>=4096)) Flush(false);
	}
	return traits_type::not_eof(c);
}
//---------------------------------------------------------------------------
std::streamsize TWideToByteStreamBuf::xsputn(const wchar_t *s, std::streamsize n)
{
	if(n<=0) return 0;
	pending.append(s, (wstring::size_type)n);
	if((pending.size()>=4096)||(wmemchr(s, L'\n', (size_t)n)!=NULL)) Flush(false);
	return n;
}
//---------------------------------------------------------------------------
int TWideToByteStreamBuf::sync(void)
{
	Flush(true);
	return 0;
}
//---------------------------------------------------------------------------
TWideOStream::TWideOStream(std::ostream *os, TKawariCharset cs, bool crlf)
	: std::wostream(NULL), buf(os, cs, crlf)
{
	rdbuf(&buf);
}
//---------------------------------------------------------------------------
TWideOStream::TWideOStream(FILE *fp, TKawariCharset cs, bool crlf)
	: std::wostream(NULL), buf(fp, cs, crlf)
{
	rdbuf(&buf);
}
//---------------------------------------------------------------------------
TWideOStream::~TWideOStream()
{
	flush();
}
//---------------------------------------------------------------------------
TKawariLogger::TKawariLogger(void){
	errlevel=0;
	StdOutStream=NULL;
	LogStream=NullStream=new wostream(new nullstreambuf);
}
//---------------------------------------------------------------------------
TKawariLogger::~TKawariLogger(void){
	if(StdOutStream) {
		StdOutStream->flush();
		delete StdOutStream;
	}
	delete NullStream->rdbuf();
	delete NullStream;
}
//---------------------------------------------------------------------------
// 出力ストリームを標準出力に設定
void TKawariLogger::SetStreamStdOut(void){
	if(!StdOutStream) StdOutStream=new TWideOStream(&cout, KC_ACP);
	LogStream=StdOutStream;
}
//---------------------------------------------------------------------------
//TKawariLogger Logger;
//---------------------------------------------------------------------------
