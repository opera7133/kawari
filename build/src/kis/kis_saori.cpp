//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// KawariInlineScript -- SAORI --
//
//  2001.03.28 initial
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "kis/kis_saori.h"
//---------------------------------------------------------------------------
#include "libkawari/kawari_engine.h"
#include "libkawari/kawari_log.h"
#include "libkawari/kawari_rc.h"
#include "misc/misc.h"
#include "misc/phttp.h"
using namespace kawari::resource;
using namespace kawari_log;
//---------------------------------------------------------------------------
#include <cstdlib>
#include <map>
using namespace std;
//---------------------------------------------------------------------------
wstring KIS_saoriregist::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 3, 4)) return (L"");

	TKawariEngine::SAORILOADTYPE loadopt=TKawariEngine::LOADONCALL;
	if (args.size()>=4){
		if (args[3]==L"preload")
			loadopt=TKawariEngine::PRELOAD;
		else if (args[3]==L"noresident")
			loadopt=TKawariEngine::NORESIDENT;
	}
	Engine->RegisterSAORIModule(args[2], CanonicalPath(Engine->GetDataPath(), args[1]), loadopt);

	return (L"");
}
//---------------------------------------------------------------------------
wstring KIS_saorierase::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 2, 2)) return (L"");
	Engine->EraseSAORIModule(args[1]);
	return (L"");
}
//---------------------------------------------------------------------------
wstring KIS_saorilist::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 2, 2)) return (L"");

	const wstring stem=args[1];
	if (stem.size()==0) return (L"");

	vector<wstring> list;
	if (Engine->ListSAORIModule(list)){
		for (vector<wstring>::iterator it=list.begin(); it!=list.end(); it++){
			Engine->Push(stem, (*it));
		}
	}
	return (L"");
}
//---------------------------------------------------------------------------
bool KIS_callsaori::CallSaori(
	const wstring& saoriname, const vector<wstring>& args, TPHMessage &response)
{
	// リクエスト生成
	TPHMessage request;
	request.SetStartline(L"EXECUTE SAORI/1.0");
	request[L"Sender"]=L"kawari";
	wstring slevel=Engine->IndexParse(L"System.Sender.Path");
	// shiori.dllから動かした場合、Sender.Pathは必ず設定される。
	// そうでないのは幸水。だからLocalで良い。
	request[L"SecurityLevel"]=(slevel.size())?
		((slevel==L"local")? wstring(L"Local") : slevel): wstring(L"Local");
	for (unsigned int i=0; i<args.size(); i++)
		request[L"Argument"+IntToString(i)]=args[i];

	// リクエスト
	if ((!Engine->RequestToSAORIModule(saoriname, request, response))
		||(!response.GetStartline().size())){
		// Failed
		GetLogger().GetStream(LOG_ERROR) << RC.S(ERR_KIS_SAORI_CALL_FAILED1) << saoriname << RC.S(ERR_KIS_SAORI_CALL_FAILED2) << endl;
		return false;
	}

	// ステータスラインの分解
	const wstring &statusline=response.GetStartline();
	unsigned int pos=statusline.find(L' ');
	if (pos==wstring::npos) return false;
	unsigned int npos=statusline.find(L' ', pos+1);
	wstring statuscode=statusline.substr(pos+1, npos-pos-1);

	if (!(statuscode[0]==L'2')) return false;
	return true;
}
//---------------------------------------------------------------------------
wstring KIS_callsaori::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 2)) return (L"");

	const wstring saoriname=args[1];
	vector<wstring> saori_args;
	for (unsigned int i=2; i<args.size(); i++)
		saori_args.push_back(args[i]);

	TPHMessage response;

	if (CallSaori(saoriname, saori_args, response)){
		if (response.count(L"Result"))
			return response[L"Result"];
		else
			return (L"");
	}else{
		return (L"");
	}
}

//---------------------------------------------------------------------------
wstring KIS_callsaorix::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 3)) return (L"");

	const wstring saoriname=args[1];
	const wstring stem=args[2];
	vector<wstring> saori_args;
	for (unsigned int i=3; i<args.size(); i++)
		saori_args.push_back(args[i]);

	TPHMessage response;

	if (CallSaori(saoriname, saori_args, response)){
		if (stem.size()){
			wstring stemp=stem+L'.';
			int value_max=0;
			for (TPHMessage::iterator it=response.begin(); it!=response.end(); it++){
				const wstring key=it->first;
				if (key.find(L"Value")==0){
					int num=StringToInt(key.substr(5))+1;
					if (num>value_max) value_max=num;
				}
				Engine->PushStrAfterClear(stemp+key, it->second);
			}
			Engine->PushStrAfterClear(stemp+L"size", IntToString(value_max));
			Engine->PushStrAfterClear(stem, response.GetStartline());
		}
		if (response.count(L"Result"))
			return response[L"Result"];
		else
			return (L"");
	}else{
		return (L"");
	}
}
//---------------------------------------------------------------------------
