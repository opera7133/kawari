//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// KawariInlineScript -- エスケープ --
//
//      Programed by NAKAUE.T (Meister)
//
//  2001.07.14  Phase 6.1     First version
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "kis/kis_escape.h"
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
wstring KIS_escape::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 2)) return (L"");

	wstring retstr;
	for(unsigned int i=1;i<args.size();i++) {
		if(i>1) retstr+=wstring(L" ");

		for(unsigned int j=0;j<args[i].size();j++) {
			if((args[i][j]==L'\\')||(args[i][j]==L'%')) retstr+=L'\\';
			retstr+=args[i][j];
		}
	}


	return(retstr);
}
//---------------------------------------------------------------------------

