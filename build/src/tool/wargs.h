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
// 拡張子を除いた部分の長さ (フォルダ名の中の'.'は拡張子とみなさない)
inline std::wstring::size_type OutputBaseLength(const std::wstring &path)
{
	std::wstring::size_type sep=path.find_last_of(L"\\/");
	std::wstring::size_type pos=path.find_last_of(L'.');
	if (pos==std::wstring::npos) return path.size();
	if ((sep!=std::wstring::npos)&&(pos<sep)) return path.size();
	return pos;
}
//---------------------------------------------------------------------------
// path の pos 以降が拡張子 ext か (ASCIIの大文字小文字は区別しない)
inline bool HasExtension(const std::wstring &path, std::wstring::size_type pos, const std::wstring &ext)
{
	if (path.size()-pos!=ext.size()) return false;
	for (std::wstring::size_type i=0; i<ext.size(); i++){
		wchar_t c=path[pos+i];
		if ((c>=L'A')&&(c<=L'Z')) c=(wchar_t)(c-L'A'+L'a');
		if (c!=ext[i]) return false;
	}
	return true;
}
//---------------------------------------------------------------------------
#endif
