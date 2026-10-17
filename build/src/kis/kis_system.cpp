//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// KawariInlineScript -- システム操作 --
//
//      Programed by Suikyo.
//
//  2002.04.20  Phase 8.0.0   従来kawari.iniにあったコマンドの一部
//  2002.06.23  Phase 8.0.0   KIS_rccharset 追加
//  2002.11.20  Phase 8.1.0   ログストリームをエンジン単位で管理
//  2003.11.18  Phase 8.2.0   getenv追加
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "kis/kis_system.h"
#include "libkawari/kawari_engine.h"
#include "libkawari/kawari_log.h"
#include "misc/misc.h"
using namespace kawari_log;
//---------------------------------------------------------------------------
#include <fstream>
#include <cstdlib>
using namespace std;
//---------------------------------------------------------------------------
wstring KIS_securitylevel::Function(const vector<wstring>& args){
	if(!AssertArgument(args, 2, 2)) return (L"");
	if (initialized){
		GetLogger().GetStream(LOG_ERROR) << L"SecurityLevel is already fixed." << endl;
		return (L"");
	}

	unsigned int lv=2;
	if (IsInteger(args[1])){
		lv=(unsigned int)StringToInt(args[1]);
	}else if(args[1]==L"low"){
		lv=0;
	}else if(args[1]==L"middle"){
		lv=1;
	}else if(args[1]==L"high"){
		lv=2;
	}else if(args[1]==L"ultrahigh"){
		lv=3;
	}
	Engine->PushStrAfterClear(L"System.SecurityLevel", IntToString(lv));
	Engine->WriteProtect(L"System.SecurityLevel");
	initialized=true;
	if (GetLogger().Check(LOG_INFO)){
		switch (lv){
		case 0:
			GetLogger().GetStream() << L"SecurityLevel: low" << endl; break;
		case 1:
			GetLogger().GetStream() << L"SecurityLevel: middle" << endl; break;
		case 2:
			GetLogger().GetStream() << L"SecurityLevel: high" << endl; break;
		case 3:
			GetLogger().GetStream() << L"SecurityLevel: ultrahigh" << endl; break;
		}
	}
	return (L"");
}
//---------------------------------------------------------------------------
wstring KIS_logfile::Function(const vector<wstring>& args)
{
	if (args.size()==1){
		// ロギング中止
		GetLogger().SetStream(NULL);
		CloseStream();
	}else if (args.size()>=2){
		GetLogger().SetStream(NULL);
		CloseStream();
		if (args[1]==L"-"){
			GetLogger().SetStreamStdOut();
		}else{
			wstring path=CanonicalPath(Engine->GetDataPath(), args[1]);
			// Unicodeパスで開く (ACP外の文字を含むパスでも可)
			filestream=kawari_wfopen(path, L"wb");
			if (filestream){
				// UTF-8 (BOMなし)で書き出す。Windowsでは従来のテキストモードと同じく改行をCRLFにする
#ifdef KAWARI_MS
				widestream=new TWideOStream(filestream, KC_UTF8, true);
#else
				widestream=new TWideOStream(filestream, KC_UTF8, false);
#endif
				GetLogger().SetStream(widestream);
			}
		}
	}
	return (L"");
}
//---------------------------------------------------------------------------
void KIS_logfile::CloseStream(void)
{
	// widestreamのデストラクタが残りをfilestreamへ書き出す
	if (widestream){
		widestream->flush();
		delete widestream;
		widestream=NULL;
	}
	if (filestream){
		fclose(filestream);
		filestream=NULL;
	}
}
//---------------------------------------------------------------------------
KIS_logfile::~KIS_logfile(){
	// ロギング中止
	GetLogger().SetStream(NULL);
	CloseStream();
}
//---------------------------------------------------------------------------
wstring KIS_loglevel::Function(const vector<wstring>& args)
{
	if (args.size()==1)
		return IntToString(GetLogger().ErrLevel());

	unsigned int level=0;
	if (IsInteger(args[1])){
		level=StringToInt(args[1]);
	}else{
		for (unsigned int i=1; i<args.size(); i++){
			if (args[i]==L"error")
				level|=LOG_ERROR;
			else if (args[i]==L"warning")
				level|=LOG_WARNING;
			else if (args[i]==L"info")
				level|=LOG_INFO;
			else if (args[i]==L"decl")
				level|=LOG_DECL;
			else if (args[i]==L"paranoia")
				level|=LOG_ERROR|LOG_WARNING|LOG_INFO|LOG_DECL|LOG_DUMP;
			else if (args[i]==L"baseevents")
				level|=LOG_BASEEVENTS;
			else if (args[i]==L"mouseevents")
				level|=LOG_MOUSEEVENTS;
			else if (args[i]==L"rscevents")
				level|=LOG_RSCEVENTS;
			else if (args[i]==L"timeevents")
				level|=LOG_TIMEEVENTS;
			else if (args[i]==L"quiet")
				level=0;
		}
	}
	GetLogger().SetErrLevel(level);
	return (L"");
}
//---------------------------------------------------------------------------
wstring KIS_debugger::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 2, 2)) return (L"");
	if(args[1]==L"on"){
		Engine->PushStrAfterClear(L"System.Debugger", L"on");
		GetLogger().GetStream(LOG_INFO) << L"Debugger: on" << endl;
	}else if(args[1]==L"off"){
		Engine->ClearEntry(L"System.Debugger");
		GetLogger().GetStream(LOG_INFO) << L"Debugger: off" << endl;
	}
	return (L"");
}
//---------------------------------------------------------------------------
wstring KIS_rccharset::Function(const vector<wstring>& args){
	if(!AssertArgument(args, 2, 2)) return (L"");

	RC.SwitchTo(args[1]);
	return (L"");
}
//---------------------------------------------------------------------------
wstring KIS_getenv::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 2, 2)) return (L"");

	wstring str;
	if(!GetEnvironmentW(args[1], str)) return(L"");
	return(str);
}
//---------------------------------------------------------------------------
