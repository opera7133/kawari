//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// KawariInlineScript -- 引数展開実行 --
//
//      Programed by NAKAUE.T (Meister)
//
//  2002.03.18  Phase 7.9.0   新コミュニケート機構補助
//  2005.06.28  Phase 8.2.3   TEntryRange対応
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "kis/kis_xargs.h"
#include "libkawari/kawari_rc.h"
using namespace kawari::resource;
using namespace kawari_log;
//---------------------------------------------------------------------------
#include "libkawari/kawari_engine.h"
//---------------------------------------------------------------------------
wstring KIS_xargs::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 3)) return (L"");

#if 0
	wstring entryname;
	int st,end;

	TKawariEngine::DecodeEntryName(args[1],entryname,st,end);

	TEntry entry=Engine->GetEntry(entryname);
	unsigned int size=entry.Size();
	if (size) {
		if(st<0) st=size+st;
		if(end<0) end=size+end;
		if((st<0)||(end<0)||(st>end)) {
			// エラー
			GetLogger().Diag(kawari_log::LOG_ERROR, kawari_log::E_KIS_INVALID_INDEX) << args[0] << L" : Invalid index";
			return(L"");
		}
	}else{
		st=end=0;
	}

	vector<wstring> args2;
	for(unsigned int i=2;i<args.size();i++) {
		args2.push_back(args[i]);
	}
	for(unsigned int i=(unsigned int)st;i<=(unsigned int)end;i++) {
		args2.push_back(Engine->IndexParse(entry,i));
	}
#else
	TEntryRange range=Engine->GetEntryRange(args[1]);
	if(range.Start==TKawariEngine::NPos){
		GetLogger().Diag(LOG_ERROR, kawari_log::E_KIS_INVALID_INDEX) << args[0]
			<< RC.S(ERR_KIS_DICT_INVALID_INDEX);
		return(L"");
	}

	vector<wstring> args2;
	for(unsigned int i=2;i<args.size();i++)
		args2.push_back(args[i]);

	for(unsigned int i=range.Start; i<=range.End; i++)
		args2.push_back(Engine->IndexParse(range.Entry,i));
#endif
	return(Engine->FunctionCall(args2));
}
//---------------------------------------------------------------------------

