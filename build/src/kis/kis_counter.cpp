//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
//  2001.05.30  Phase 5.1     First version
//
//      Programed by NAKAUE.T (Meister)
//
//  2001.05.25  Phase 5.1     First version
//  2005.06.21  Phase 8.2.3   負の添え字に対応
//  2005.06.28  Phase 8.2.3   TEntryRange対応
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "kis/kis_counter.h"
#include "libkawari/kawari_engine.h"
#include "libkawari/kawari_rc.h"
#include "misc/misc.h"
using namespace kawari::resource;
using namespace kawari_log;
//---------------------------------------------------------------------------
#include <cstdlib>
using namespace std;
//---------------------------------------------------------------------------
wstring KIS_inc::Function_(const vector<wstring>& args,bool flag_dec)
{
	if(!AssertArgument(args, 2, 4)) return(L"");

	int diff=(args.size()<3)? 1: StringToInt(args[2]);
	if(flag_dec) diff=-diff;
#if 0
	wstring entryname;
	int st,end;
	unsigned int size;

	int range=TKawariEngine::DecodeEntryName(args[1],entryname,st,end);

	switch(range) {
	case 0:		// 範囲指定無し
		st=end=0;
		break;
	case 1:		// 単独要素指定
		size=Engine->EntrySize(entryname);
		if(st<0) st=size+st;
		if(st<0) {
			// エラー
			GetLogger().GetStream(kawari_log::LOG_ERROR) << args[0] << RC.S(ERR_KIS_DICT_INVALID_INDEX) << endl;
			return(L"");
		}
		break;
	case 2:		// 範囲指定
		// エラー
		GetLogger().GetStream(kawari_log::LOG_ERROR) << args[0] << RC.S(ERR_KIS_DICT_INVALID_INDEX) << endl;
		return(L"");
	}

	int counter=StringToInt(Engine->IndexParse(entryname,st))+diff;

	if(args.size()>=4) {
		int limit=StringToInt(args[3]);
		if((counter>limit)&&!flag) counter=limit;
		if((counter<limit)&&flag)  counter=limit;
	}

	TEntry entry=Engine->CreateEntry(entryname);
	TWordID id=Engine->CreateStrWord(IntToString(counter));
	TWordID nullstr=Engine->CreateStrWord(L"");

	entry.Replace2(st,id,nullstr);
#else
	TEntryRange range=Engine->GetEntryRange(args[1]);
	if(range.Start==TKawariEngine::NPos){
		GetLogger().GetStream(LOG_ERROR) << args[0]
			<< RC.S(ERR_KIS_DICT_INVALID_INDEX) << endl;
		return(L"");
	}

	int limit=(args.size()>=4)?
		StringToInt(args[3]):((flag_dec)? INT_MIN: INT_MAX);
	TWordID nullstr=Engine->CreateStrWord(L"");

	if(!range.Range)
		range.Start=range.End=0;

	//MEMO: 範囲指定した場合、その全てをinc/decするように仕様追加。
	for(unsigned int i=range.Start;i<=range.End;i++){
		// atoi()は""に対して0を返すことが保証されている。
		int counter=StringToInt(Engine->IndexParse(range.Entry,i))+diff;
		if(((!flag_dec)&&(counter>limit))
		   ||(flag_dec&&(counter<limit))) counter=limit;
		TWordID wid=Engine->CreateStrWord(IntToString(counter));
		range.Entry.Replace2(i,wid,nullstr);
	}
#endif
	return(L"");
}
//---------------------------------------------------------------------------

