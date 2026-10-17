//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// KawariInlineScript -- toupper, tolower --
//
//      Programed by Chikara.H (MDR)
//
//  2001.09.04  created(9割はkis_escape.cppから拝借）
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "kis/kis_substitute.h"
//---------------------------------------------------------------------------
#include <ctype.h>
#include <cstdlib>
using namespace std;
//---------------------------------------------------------------------------
wstring KIS_toupper::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 2)) return (L"");

	wstring retstr;
	for(unsigned int i=1;i<args.size();i++) {
		if(i>1) retstr+=wstring(L" ");

		for(unsigned int j=0;j<args[i].size();j++) {
			if( L'a' <= args[i][j] && args[i][j] <= L'z') {
				retstr+= (wchar_t)(args[i][j]-L'a'+L'A');
			} else {
				retstr+=args[i][j];
			}
		}
	}


	return(retstr);
}
//---------------------------------------------------------------------------
wstring KIS_tolower::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 2)) return (L"");

	wstring retstr;
	for(unsigned int i=1;i<args.size();i++) {
		if(i>1) retstr+=wstring(L" ");

		for(unsigned int j=0;j<args[i].size();j++) {
			if( args[i][j] >= L'A'  && args[i][j] <= L'Z') {
				retstr+= (wchar_t)(args[i][j]-L'A'+L'a');
			} else {
				retstr+=args[i][j];
			}
		}
	}


	return(retstr);
}
//

//---------------------------------------------------------------------------

