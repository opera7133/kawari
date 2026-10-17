//---------------------------------------------------------------------------
//
// KOSUI -- Kawari cOnSole Use Interpritor --
// 幸水 -- コンソール版華和梨インタープリタ --
//
//      Programed by NAKAUE.T (Meister)
//
//  2001.05.30  Phase 5.1     コンソールインタープリタ
//  2001.08.06  Phase 6.2     KawariDebuggerに合わせてデフォルトをecho-modeに
//  2001.08.12  Phase 6.2.1   KawariDebuggerと統合
//  2002.01.08  番外          NothingIsDone向け
//  2002.03.12  Phase 7.9.0   Attach入力の所のちんまいバグ修正
//  2002.04.12  Phase 8.0.0   インターフェース切り直し
//  2008.01.23  Phase 8.2.5   quietモード追加
//  2026.10.18  Phase 9.0.0   wchar_t化 (コンソール入出力はACP)
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#ifdef __BORLANDC__
#	define _POSIX_
#	include <limits.h>
#endif
//---------------------------------------------------------------------------
#include <string>
#include <iostream>
#include <cstdlib>
using namespace std;
//---------------------------------------------------------------------------
#include "tool/kosui_base.h"
#include "tool/kawari_kosui.h"
#include "misc/misc.h"
#include "misc/charset.h"
#include "tool/wargs.h"
#ifdef USEKDB
#	include "tool/kdb.h"
#	include "tool/kosui_dsstp.h"
#endif
#if defined(KAWARI_POSIX)
#	include <unistd.h>
#	include <limits.h>
#else
#	include <direct.h>
#endif
#ifdef _MSC_VER
#define PATH_MAX _MAX_PATH
#endif
//---------------------------------------------------------------------------
// 接続するゴーストを選ぶ
TKawariInterface_base* AttachTarget(const wstring& datapath,const wstring& inifile,const wstring& event)
{
	cout << "[" << 0 << "] : " << " Interpreter mode." << endl;

	#ifdef USEKDB
		// 華和梨デバッガを組み込む場合
		map<HWND,wstring> ghostname;
		GetGhostList(ghostname);

		if(ghostname.size()>0) {
			vector<HWND> ghostlist;
			map<HWND,wstring>::iterator it;
			for(it=ghostname.begin();it!=ghostname.end();it++) {
				ghostlist.push_back(it->first);
				cout << "[" << ghostlist.size() << "] : " << WtoACP(it->second) << endl;
			}

			unsigned int no;
			while(true) {
				cout << "Attach : ";
				string buff;
				getline(cin,buff);
				no=atoi(buff.c_str());
				if(no<=ghostlist.size()) break;
			}

			if(no>0) return(new TKosuiDSSTPInterface(ghostlist[no-1],event));
		}
	#endif

	return(new TKawariKosuiAdapter(datapath,inifile));
}
//---------------------------------------------------------------------------
int main(int argc,char *argv[])
{
	vector<wstring> args=GetWideArgs(argc, argv);

	wstring datapath;
#ifdef KAWARI_WIN32
	wchar_t cwd[PATH_MAX+1];
	if(NULL==_wgetcwd(cwd, PATH_MAX+1)){
#else
	char cwd[PATH_MAX+1];
	if(NULL==getcwd(cwd, PATH_MAX+1)){
#endif
		datapath=CanonicalPath(L"./");
	}else{
#ifdef KAWARI_WIN32
		datapath=cwd;
#else
		datapath=UTF8toW(string(cwd));
#endif
		if(datapath.length()&&(datapath[datapath.length()-1]!=FILE_SEPARATOR))
			datapath+=FILE_SEPARATOR;
	}
	wstring inifile=L"kawarirc.kis";
	wstring event=L"ShioriEcho";
	bool quietmode=false;

	if(args.size()>1) {
		for (int i=1; i<(int)args.size(); i++){
			wstring param=args[i];
			if ((param==L"--event")||(param==L"-e")){
				if (++i<(int)args.size()){
					param=args[i];
					if (param.size())
						event=param;
				}
			}else if (param==L"--norc"){
				inifile=L"";
			}else if (param==L"--quiet"){
				quietmode=true;
			}else{
				inifile=param;
				wstring::size_type pos=inifile.rfind(L'/');
				if(pos==wstring::npos) pos=inifile.rfind(L'\\');
				if(pos!=wstring::npos)
					datapath=CanonicalPath(inifile.substr(0,pos+1));
			}
		}
		if(quietmode){
			// quietモード(inifileを実行したらすぐに終了)
			TKawariInterface_base *Kawari=new TKawariKosuiAdapter(datapath,inifile);
			delete Kawari;
			return(0);
		}
	}

	cout << "K O S U I -- Kawari cOnSole Use Interpritor --" << endl;
	cout << "2001 Programed by NAKAUE.T (Meister)" << endl << endl;

	TKawariInterface_base *Kawari=AttachTarget(datapath,inifile,event);

	cout << endl;
	cout << WtoACP(Kawari->GetInformation()) << endl << endl;

	cout << "'.' is mode change command." << endl;

	bool commode=false;
	while(true) {
		if(commode) cout << "command-mode > " << flush;
		 else cout << "echo-mode > " << flush;

		string bytes;
		getline(cin,bytes);
		// 標準入力はACP
		wstring buff=ACPtoW(bytes);

		if(buff.size()==0) continue;

		if(buff==L"exit") break;
		if(buff==L".") {
			commode=!commode;
			continue;
		}

		if(commode) cout << WtoACP(Kawari->Parse(wstring(L"$(")+buff+L")")) << endl;
		 else cout << WtoACP(Kawari->Parse(buff)) << endl;
	}

	delete Kawari;

	return(0);
}
//---------------------------------------------------------------------------

