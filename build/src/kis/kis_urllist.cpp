//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// KawariInlineScript -- SHIORI/2.5 URLリスト --
//
//      Programed by NAKAUE.T (Meister)
//
//  2001.08.08  Phase 6.2     First version
//  2001.08.12  Phase 6.2.1   末尾に[2]追加
//                            仕様書バグにやられた(;_;)
//  2001.08.25  Phase 6.3     chr追加・・・華和梨らしい？
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "kis/kis_urllist.h"
//---------------------------------------------------------------------------
#include "misc/misc.h"
#include <cstdlib>
using namespace std;
//---------------------------------------------------------------------------
wstring KIS_urllist::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 4)) return (L"");
	if((args.size()%3)!=1) return(L"");

	wstring retstr;
	for(unsigned int i=1;i<args.size();i+=3) {
		if(args[i]!=L"-") retstr=retstr+args[i]+L"\x1"+args[i+1]+L"\x1"+args[i+2]+L"\x2";
		 else retstr+=L"-\x2";
	}

	return(retstr);
}
//---------------------------------------------------------------------------
wstring KIS_chr::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 2, 2)) return (L"");

	unsigned int code=(unsigned int)StringToInt(args[1]);

	// 旧版はバイト列として出力していたため、0x80以上はSJISコードとして解釈する
	if(code<0x80) {
		return(wstring(1, (wchar_t)code));
	} else if(code<=255) {
		char buff[1];
		buff[0]=(char)(code&0xff);
		return(SJIStoW(string(buff,1)));
	} else {
		char buff[2];
		buff[0]=(char)((code>>8)&0xff);
		buff[1]=(char)(code&0xff);
		return(SJIStoW(string(buff,2)));
	}
}
//---------------------------------------------------------------------------

