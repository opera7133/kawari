//---------------------------------------------------------------------------
//
// 文字コード変換 (std::string <-> std::wstring)
//
//  2026.10.18  Phase 9.0.0   新設 (wchar_t化)
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "misc/charset.h"
//---------------------------------------------------------------------------
#include <string>
#include <cstring>
#include <cwchar>
//---------------------------------------------------------------------------
#ifdef KAWARI_WIN32
#	include <windows.h>
#else
#	include <iconv.h>
#	include <cerrno>
#endif
//---------------------------------------------------------------------------
using namespace std;
//---------------------------------------------------------------------------
// ASCII限定の簡易変換
std::wstring AtoW(const std::string &s)
{
	wstring ret;
	ret.reserve(s.size());
	for(string::size_type i=0; i<s.size(); i++) {
		unsigned char c=(unsigned char)s[i];
		ret+=(wchar_t)((c<0x80)?c:L'?');
	}
	return ret;
}
//---------------------------------------------------------------------------
std::string WtoA(const std::wstring &ws)
{
	string ret;
	ret.reserve(ws.size());
	for(wstring::size_type i=0; i<ws.size(); i++) {
		unsigned long c=(unsigned long)ws[i];
		ret+=(char)((c<0x80)?c:'?');
	}
	return ret;
}
//---------------------------------------------------------------------------
// UTF-8 (自前実装)
// wchar_tが16bitの環境ではサロゲートペアを使う
//---------------------------------------------------------------------------
std::wstring UTF8toW(const std::string &s)
{
	wstring ret;
	const string::size_type len=s.size();
	string::size_type i=0;
	ret.reserve(len);

	while(i<len) {
		unsigned long c=(unsigned char)s[i];
		unsigned long cp=0xfffd;
		int n=0;	// 後続バイト数
		unsigned long minv=0;

		if(c<0x80) {
			ret+=(wchar_t)c;
			i++;
			continue;
		} else if((c>=0xc2)&&(c<=0xdf)) {
			n=1; cp=c&0x1f; minv=0x80;
		} else if((c>=0xe0)&&(c<=0xef)) {
			n=2; cp=c&0x0f; minv=0x800;
		} else if((c>=0xf0)&&(c<=0xf4)) {
			n=3; cp=c&0x07; minv=0x10000;
		} else {
			// 不正な先頭バイト
			ret+=(wchar_t)0xfffd;
			i++;
			continue;
		}

		// 後続バイトが足りなければ不正
		bool ok=(i+n<len);
		if(ok) {
			for(int k=1; k<=n; k++) {
				unsigned long cc=(unsigned char)s[i+k];
				if((cc&0xc0)!=0x80) { ok=false; break; }
				cp=(cp<<6)|(cc&0x3f);
			}
		}
		if(ok&&((cp<minv)||(cp>0x10ffff)||((cp>=0xd800)&&(cp<=0xdfff)))) ok=false;

		if(!ok) {
			ret+=(wchar_t)0xfffd;
			i++;
			continue;
		}
		i+=n+1;

		if(cp>=0x10000) {
			if(sizeof(wchar_t)>=4) {
				ret+=(wchar_t)cp;
			} else {
				cp-=0x10000;
				ret+=(wchar_t)(0xd800|(cp>>10));
				ret+=(wchar_t)(0xdc00|(cp&0x3ff));
			}
		} else {
			ret+=(wchar_t)cp;
		}
	}
	return ret;
}
//---------------------------------------------------------------------------
std::string WtoUTF8(const std::wstring &ws)
{
	string ret;
	const wstring::size_type len=ws.size();
	ret.reserve(len*3/2+1);

	for(wstring::size_type i=0; i<len; i++) {
		unsigned long cp=(unsigned long)ws[i]&0xffffffffUL;
		if(sizeof(wchar_t)<4) cp&=0xffff;

		if((cp>=0xd800)&&(cp<=0xdbff)) {
			// 上位サロゲート
			if((i+1<len)&&(((unsigned long)ws[i+1]&0xfc00)==0xdc00)) {
				cp=0x10000+((cp-0xd800)<<10)+(((unsigned long)ws[i+1]&0xffff)-0xdc00);
				i++;
			} else {
				cp=0xfffd;
			}
		} else if((cp>=0xdc00)&&(cp<=0xdfff)) {
			cp=0xfffd;
		} else if(cp>0x10ffff) {
			cp=0xfffd;
		}

		if(cp<0x80) {
			ret+=(char)cp;
		} else if(cp<0x800) {
			ret+=(char)(0xc0|(cp>>6));
			ret+=(char)(0x80|(cp&0x3f));
		} else if(cp<0x10000) {
			ret+=(char)(0xe0|(cp>>12));
			ret+=(char)(0x80|((cp>>6)&0x3f));
			ret+=(char)(0x80|(cp&0x3f));
		} else {
			ret+=(char)(0xf0|(cp>>18));
			ret+=(char)(0x80|((cp>>12)&0x3f));
			ret+=(char)(0x80|((cp>>6)&0x3f));
			ret+=(char)(0x80|(cp&0x3f));
		}
	}
	return ret;
}
//---------------------------------------------------------------------------
bool HasUTF8BOM(const std::string &s)
{
	return (s.size()>=3)
		&&((unsigned char)s[0]==0xef)
		&&((unsigned char)s[1]==0xbb)
		&&((unsigned char)s[2]==0xbf);
}
//---------------------------------------------------------------------------
bool IsValidUTF8(const std::string &s)
{
	const string::size_type len=s.size();
	string::size_type i=0;

	while(i<len) {
		unsigned long c=(unsigned char)s[i];
		if(c<0x80) { i++; continue; }

		int n;
		unsigned long cp, minv;
		if((c>=0xc2)&&(c<=0xdf)) { n=1; cp=c&0x1f; minv=0x80; }
		else if((c>=0xe0)&&(c<=0xef)) { n=2; cp=c&0x0f; minv=0x800; }
		else if((c>=0xf0)&&(c<=0xf4)) { n=3; cp=c&0x07; minv=0x10000; }
		else return false;

		if(i+n>=len) return false;
		for(int k=1; k<=n; k++) {
			unsigned long cc=(unsigned char)s[i+k];
			if((cc&0xc0)!=0x80) return false;
			cp=(cp<<6)|(cc&0x3f);
		}
		if((cp<minv)||(cp>0x10ffff)||((cp>=0xd800)&&(cp<=0xdfff))) return false;
		i+=n+1;
	}
	return true;
}
//---------------------------------------------------------------------------
TKawariCharset DetectCharset(const std::string &s)
{
	if(HasUTF8BOM(s)) return KC_UTF8;
	return IsValidUTF8(s) ? KC_UTF8 : KC_SJIS;
}
//---------------------------------------------------------------------------
// Shift_JIS / ACP
//---------------------------------------------------------------------------
#ifdef KAWARI_WIN32
//---------------------------------------------------------------------------
static std::wstring MBtoW_win(const std::string &s, UINT cp)
{
	if(s.empty()) return wstring();
	int n=MultiByteToWideChar(cp, 0, s.data(), (int)s.size(), NULL, 0);
	if(n<=0) return wstring();
	wstring ret((wstring::size_type)n, L'\0');
	n=MultiByteToWideChar(cp, 0, s.data(), (int)s.size(), &ret[0], n);
	if(n<=0) return wstring();
	ret.resize((wstring::size_type)n);
	return ret;
}
//---------------------------------------------------------------------------
static std::string WtoMB_win(const std::wstring &ws, UINT cp)
{
	if(ws.empty()) return string();
	int n=WideCharToMultiByte(cp, 0, ws.data(), (int)ws.size(), NULL, 0, NULL, NULL);
	if(n<=0) return string();
	string ret((string::size_type)n, '\0');
	n=WideCharToMultiByte(cp, 0, ws.data(), (int)ws.size(), &ret[0], n, NULL, NULL);
	if(n<=0) return string();
	ret.resize((string::size_type)n);
	return ret;
}
//---------------------------------------------------------------------------
std::wstring SJIStoW(const std::string &s) { return MBtoW_win(s, 932); }
std::string WtoSJIS(const std::wstring &ws) { return WtoMB_win(ws, 932); }
std::wstring ACPtoW(const std::string &s) { return MBtoW_win(s, CP_ACP); }
std::string WtoACP(const std::wstring &ws) { return WtoMB_win(ws, CP_ACP); }
//---------------------------------------------------------------------------
#else
//---------------------------------------------------------------------------
// POSIX: iconv
static iconv_t OpenSJIS(bool toSJIS)
{
	static const char* const names[]={"CP932", "SHIFT_JIS", "SJIS", NULL};
	for(int i=0; names[i]!=NULL; i++) {
		// macOS libiconv does not expose WCHAR_T; select the native UTF-32 encoding.
#if defined(__APPLE__)
        const wchar_t endian=1;
        const char *wide=*(const unsigned char*)&endian ? "UTF-32LE" : "UTF-32BE";
#else
        const char *wide="WCHAR_T";
#endif
        iconv_t cd=toSJIS ? iconv_open(names[i], wide) : iconv_open(wide, names[i]);
		if(cd!=(iconv_t)-1) return cd;
	}
	return (iconv_t)-1;
}
//---------------------------------------------------------------------------
// inunit: 入力の1要素のバイト数 (変換不能時にこれだけ読み飛ばす)
// repl:   変換不能時に出力するバイト列
static std::string IconvRun(iconv_t cd, const char *in, size_t inlen, size_t inunit, const std::string &repl)
{
	string ret;
	char buf[1024];
	while(inlen>0) {
		char *op=buf;
		size_t oleft=sizeof(buf);
		char *ip=const_cast<char*>(in);
		size_t r=iconv(cd, &ip, &inlen, &op, &oleft);
		in=ip;
		ret.append(buf, sizeof(buf)-oleft);
		if(r==(size_t)-1) {
			if(errno==E2BIG) continue;
			// EILSEQ / EINVAL : 1要素読み飛ばして代替文字
			size_t skip=(inlen<inunit)?inlen:inunit;
			in+=skip;
			inlen-=skip;
			ret+=repl;
			iconv(cd, NULL, NULL, NULL, NULL);
		}
	}
	// フラッシュ
	{
		char *op=buf;
		size_t oleft=sizeof(buf);
		iconv(cd, NULL, NULL, &op, &oleft);
		ret.append(buf, sizeof(buf)-oleft);
	}
	return ret;
}
//---------------------------------------------------------------------------
std::wstring SJIStoW(const std::string &s)
{
	if(s.empty()) return wstring();
	iconv_t cd=OpenSJIS(false);
	if(cd==(iconv_t)-1) return AtoW(s);
	wchar_t q=L'?';
	string repl((const char*)&q, sizeof(wchar_t));
	string bytes=IconvRun(cd, s.data(), s.size(), 1, repl);
	iconv_close(cd);
	return wstring((const wchar_t*)bytes.data(), bytes.size()/sizeof(wchar_t));
}
//---------------------------------------------------------------------------
std::string WtoSJIS(const std::wstring &ws)
{
	if(ws.empty()) return string();
	iconv_t cd=OpenSJIS(true);
	if(cd==(iconv_t)-1) return WtoA(ws);
	string ret=IconvRun(cd, (const char*)ws.data(), ws.size()*sizeof(wchar_t), sizeof(wchar_t), string("?"));
	iconv_close(cd);
	return ret;
}
//---------------------------------------------------------------------------
std::wstring ACPtoW(const std::string &s) { return UTF8toW(s); }
std::string WtoACP(const std::wstring &ws) { return WtoUTF8(ws); }
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
std::wstring MBtoW(const std::string &s, TKawariCharset cs)
{
	if(cs==KC_NULL) cs=DetectCharset(s);
	switch(cs) {
	case KC_UTF8: return UTF8toW(HasUTF8BOM(s) ? s.substr(3) : s);
	case KC_ACP:  return ACPtoW(s);
	case KC_SJIS:
	default:      return SJIStoW(s);
	}
}
//---------------------------------------------------------------------------
std::string WtoMB(const std::wstring &ws, TKawariCharset cs)
{
	switch(cs) {
	case KC_UTF8: return WtoUTF8(ws);
	case KC_ACP:  return WtoACP(ws);
	case KC_SJIS:
	case KC_NULL:
	default:      return WtoSJIS(ws);
	}
}
//---------------------------------------------------------------------------
TKawariCharset CharsetFromName(const std::wstring &name)
{
	// 前後の空白を除き、小文字化して比較
	wstring::size_type b=0, e=name.size();
	while((b<e)&&((name[b]==L' ')||(name[b]==L'\t')||(name[b]==L'\r')||(name[b]==L'\n'))) b++;
	while((e>b)&&((name[e-1]==L' ')||(name[e-1]==L'\t')||(name[e-1]==L'\r')||(name[e-1]==L'\n'))) e--;

	string n;
	for(wstring::size_type i=b; i<e; i++) {
		wchar_t c=name[i];
		if((c>=L'A')&&(c<=L'Z')) c=(wchar_t)(c-L'A'+L'a');
		if(c>=0x80) return KC_NULL;
		n+=(char)c;
	}

	if((n=="shift_jis")||(n=="shift-jis")||(n=="x-sjis")||(n=="sjis")||(n=="windows-31j")||(n=="cp932")||(n=="ms932")||(n=="ms_kanji")||(n=="csshiftjis"))
		return KC_SJIS;
	if((n=="utf-8")||(n=="utf8"))
		return KC_UTF8;
	return KC_NULL;
}
//---------------------------------------------------------------------------
const wchar_t* CharsetName(TKawariCharset cs)
{
	switch(cs) {
	case KC_SJIS: return L"Shift_JIS";
	case KC_UTF8: return L"UTF-8";
	default:      return L"";
	}
}
//---------------------------------------------------------------------------
