//---------------------------------------------------------------------------
//
// コマンドライン引数をwstringで得る
//
//  2026.10.18  Phase 9.0.0   新規 (wchar_t化)
//
//---------------------------------------------------------------------------
#ifndef WARGS_H
#define WARGS_H
//---------------------------------------------------------------------------
#include "config.h"
#include "misc/charset.h"
//---------------------------------------------------------------------------
#include <string>
#include <vector>
#ifdef KAWARI_WIN32
#	include <windows.h>
#	include <shellapi.h>
#endif
//---------------------------------------------------------------------------
// Windows : CommandLineToArgvW(GetCommandLineW())
// POSIX   : argvをUTF-8として変換
// args[0]はプログラム名
inline std::vector<std::wstring> GetWideArgs(int argc, char *argv[])
{
	std::vector<std::wstring> args;
#ifdef KAWARI_WIN32
	int wargc=0;
	wchar_t **wargv=::CommandLineToArgvW(::GetCommandLineW(), &wargc);
	if (wargv){
		for (int i=0; i<wargc; i++) args.push_back(std::wstring(wargv[i]));
		::LocalFree(wargv);
		return args;
	}
#endif
	for (int i=0; i<argc; i++) args.push_back(UTF8toW(std::string(argv[i])));
	return args;
}
//---------------------------------------------------------------------------
#endif
