//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// KawariInlineScript -- 本体との連携 (EXECUTE SSTP) --
//
//  2026.10.18  Phase 9.0.0   getproperty / setproperty / executesstp / executesstpx 追加
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "kis/kis_sstp.h"
//---------------------------------------------------------------------------
#include "libkawari/kawari_engine.h"
#include "libkawari/kawari_log.h"
#include "misc/dsstp.h"
#include "misc/misc.h"
using namespace kawari_log;
//---------------------------------------------------------------------------
#include <string>
#include <vector>
using namespace std;
//---------------------------------------------------------------------------
// EXECUTE SSTP を送る
int KIS_executesstp::Execute(const wstring& name, const wstring& command,
	const vector<wstring>& refs, vector<wstring>& lines)
{
	lines.clear();
	int code=0;
	dsstp::TBasewareLink &link=Engine->GetBaseware();

	// 改行や\0はヘッダの区切りになるので、コマンドや引数には含められない
	bool valid=(!command.empty())&&(!dsstp::HasForbiddenChar(command));
	for (unsigned int i=0; valid&&(i<refs.size()); i++)
		valid=!dsstp::HasForbiddenChar(refs[i]);

	if (!valid){
		GetLogger().Diag(LOG_ERROR, kawari_log::E_KIS_ERROR) << L"KIS[" << name
			<< L"] error : the command is empty, or the command or a reference contains a line break.";
	}else if (!link.IsAllowed()){
		GetLogger().Diag(LOG_WARNING, kawari_log::W_KIS_WARNING) << L"KIS[" << name
			<< L"] refused : the request is not from local.";
	}else if (!link.IsReady()){
		GetLogger().Diag(LOG_WARNING, kawari_log::W_KIS_WARNING) << L"KIS[" << name
			<< L"] the window of the baseware is unknown yet (NOTIFY hwnd is not received).";
	}else{
		string response;
		if (!link.Send(dsstp::BuildExecuteRequest(command, refs), response, 5000)){
			GetLogger().Diag(LOG_WARNING, kawari_log::W_KIS_WARNING) << L"KIS[" << name
				<< L"] no response from baseware.";
		}else{
			code=dsstp::ParseExecuteResponse(response, lines);
		}
	}

	Engine->PushStrAfterClear(L"System.SSTP.ResponseCode", IntToString(code));
	Engine->ClearEntry(L"System.SSTP.Data");
	if (!lines.empty()){
		TEntry entry=Engine->CreateEntry(L"System.SSTP.Data");
		for (unsigned int i=0; i<lines.size(); i++)
			entry.Push(Engine->CreateStrWord(lines[i]));
	}
	return code;
}
//---------------------------------------------------------------------------
wstring KIS_executesstp::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 2)) return (L"");

	vector<wstring> refs(args.begin()+2, args.end());
	vector<wstring> lines;
	int code=Execute(args[0], args[1], refs, lines);
	if ((code<200)||(code>=300)||lines.empty()) return (L"");
	return lines[0];
}
//---------------------------------------------------------------------------
wstring KIS_executesstpx::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 3)) return (L"");

	vector<wstring> refs(args.begin()+3, args.end());
	vector<wstring> lines;
	int code=Execute(args[0], args[2], refs, lines);

	const wstring stem=args[1];
	if (stem.size()){
		Engine->ClearEntry(stem);
		if (!lines.empty()){
			TEntry entry=Engine->CreateEntry(stem);
			for (unsigned int i=0; i<lines.size(); i++)
				entry.Push(Engine->CreateStrWord(lines[i]));
		}
	}
	if ((code<200)||(code>=300)||lines.empty()) return (L"");
	return lines[0];
}
//---------------------------------------------------------------------------
wstring KIS_getproperty::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 2, 3)) return (L"");

	vector<wstring> refs(1, args[1]);
	vector<wstring> lines;
	int code=Execute(args[0], L"GetProperty", refs, lines);
	if (code==200) return (lines.empty())?wstring():lines[0];
	// 存在しないプロパティ(204)や、送れなかったとき(0)は既定値
	return (args.size()>=3)?args[2]:wstring();
}
//---------------------------------------------------------------------------
wstring KIS_setproperty::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 3, 3)) return (L"");

	vector<wstring> refs;
	refs.push_back(args[1]);
	refs.push_back(args[2]);
	vector<wstring> lines;
	Execute(args[0], L"SetProperty", refs, lines);
	return (L"");
}
//---------------------------------------------------------------------------
