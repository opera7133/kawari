//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// KawariInlineScript -- file attribute --
//
//      Programed by Kouji.U (sky)
//
//  2001.06.03  Created
//  2001.06.17  Phase 6.0     Phase6.0にあわせて修正 (NAKAUE.T)
//  2001.07.14  Phase 6.1     loadで絶対パス指定可能に (NAKAUE.T)
//  2001.07.19  Phase 6.2     Mingw対応 (NAKAUE.T)
//  2001.12.09  Phase 7.1.2   kis_textfileと統合
//                            readdir追加
//  2001.12.18  Phase 7.2     VC向けにFindFirstFileA、FindNextFileAを使った
//                            opendir/readdir/closedir作成(Thanks: えびさわ)
//  2002.03.17  Phase 7.9.0   Phase7.9に合わせて変更 (NAKAUE.T)
//                            textload仕様変更/textsave追加
//                            readdirで.と..を除外
//  2002.04.16  Phase 8.0.0   パス文字列操作をmisc/misc.cppへ移動
//  2002.07.10  Phase 8.1.0   isdir/isexist/isfile追加
//  2008.01.23  Phase 8.2.5   isexist(ルートディレクトリ誤認対策)
//  2008.03.30  Phase 8.2.8   textsave修正(複数のエントリを保存できない)
//  2026.10.18  Phase 9.0.0   wchar_t化 (textload自動判定/textsave UTF-8, ListDirectory/PathStat使用)
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "kis/kis_file.h"
#include "libkawari/kawari_engine.h"
#include "libkawari/kawari_rc.h"
#include "misc/misc.h"
using namespace kawari::resource;
using namespace kawari_log;
//---------------------------------------------------------------------------
#include <string>
#include <vector>
#include "misc/wfile.h"
using namespace std;
//---------------------------------------------------------------------------
void KIS_save::Run(const vector<wstring>& args, bool crypt)
{
	if(!AssertArgument(args, 3)) return;

	vector<wstring> entry;
	entry.insert(entry.end(),args.begin()+2,args.end());

	// saveは相対パスのみ
	wstring filename;
	if (IsAbsolutePath(CanonicalPath(args[1])))
		filename=PathToFileName(args[1]);
	else
		filename=CanonicalPath(Engine->GetDataPath(), args[1]);

	if(!Engine->SaveKawariDict(filename,entry,crypt)) {
		// エラー
		GetLogger().GetStream(LOG_ERROR) << args[0] << RC.S(ERR_KIS_FILE_SAVE_FAILED) << filename << endl;
	}

	return;
}
//---------------------------------------------------------------------------
wstring KIS_load::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 2, 2)) return (L"");

	wstring filename=CanonicalPath(Engine->GetDataPath(), args[1]);

	if(!Engine->LoadKawariDict(filename)) {
		// エラー
		GetLogger().GetStream(LOG_ERROR) << args[0] << RC.S(ERR_KIS_FILE_LOAD_FAILED) << filename << endl;
	}

	return(L"");
}
//---------------------------------------------------------------------------
wstring KIS_textload::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 3, 3)) return (L"");

	wstring filename=CanonicalPath(Engine->GetDataPath(), args[2]);

	wstring text;
	if(!ReadTextFile(filename, text)) {
		// エラー
		GetLogger().GetStream(LOG_ERROR) << args[0] << RC.S(ERR_KIS_FILE_LOAD_FAILED) << filename << endl;
		return(L"");
	}

	TEntry entry=Engine->CreateEntry(args[1]);

	// 1行ずつ分割 (CRLF/LF対応。最後の改行の後ろは空行にしない)
	wstring::size_type pos=0;
	while(pos<text.size()) {
		wstring::size_type nl=text.find(L'\n', pos);
		wstring buff;
		if(nl==wstring::npos) {
			buff=text.substr(pos);
			pos=text.size();
		} else {
			buff=text.substr(pos, nl-pos);
			pos=nl+1;
		}
		if((buff.size()>0)&&(buff[buff.size()-1]==L'\r')) buff.erase(buff.size()-1);
		entry.Push(Engine->CreateStrWord(buff));
	}

	return(L"");
}
//---------------------------------------------------------------------------
wstring KIS_textsave::Function_(const vector<wstring>& args,bool flag)
{
	if(!AssertArgument(args, 3)) return (L"");

	// saveは相対パスのみ
	wstring filename;
	if (IsAbsolutePath(CanonicalPath(args[1])))
		filename=PathToFileName(args[1]);
	else
		filename=CanonicalPath(Engine->GetDataPath(), args[1]);

#ifdef KAWARI_MS
	const wchar_t *eol=L"\r\n";
#else
	const wchar_t *eol=L"\n";
#endif
	wstring text;
	for(unsigned int i=2;i<args.size();i++) {
		TEntry entry=Engine->CreateEntry(args[i]);
		unsigned int size=entry.Size();

		for(unsigned int l=0;l<size;l++) {
			text+=Engine->IndexParse(entry,l);
			text+=eol;
		}
	}

	// UTF-8 (BOMなし)で書き出す
	if(!WriteTextFile(filename, text, KC_UTF8, false, !flag)) {
		// エラー
		GetLogger().GetStream(LOG_ERROR) << args[0] << RC.S(ERR_KIS_FILE_LOAD_FAILED) << filename << endl;
		return(L"");
	}

	return(L"");
}
//---------------------------------------------------------------------------
wstring KIS_readdir::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 3, 3)) return (L"");

	wstring dirname=CanonicalPath(Engine->GetDataPath(), args[2]);

	vector<wstring> names;
	if(!ListDirectory(dirname, names)) return(L"");

	Engine->ClearEntry(args[1]);

	for(unsigned int i=0;i<names.size();i++) {
		const wstring &entry=names[i];
		if((entry!=L".")&&(entry!=L"..")) Engine->CreateEntry(args[1]).Push(Engine->CreateStrWord(entry));
	}

	return(L"");
}
//---------------------------------------------------------------------------
wstring KIS_cncpath::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 2, 3)) return (L"");

	if (args.size()==2){
		return CanonicalPath(args[1]);
	}else{
		return CanonicalPath(args[1], args[2]);
	}
}
//---------------------------------------------------------------------------
wstring KIS_dirname::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 2, 2)) return (L"");

	return PathToBaseDir(CanonicalPath(args[1]));
}
//---------------------------------------------------------------------------
wstring KIS_filename::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 2, 2)) return (L"");

	return PathToFileName(CanonicalPath(args[1]));
}
//---------------------------------------------------------------------------
wstring KIS_isdir::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 2, 2)) return (L"");

	wstring dirname=CanonicalPath(Engine->GetDataPath(), args[1]);
	bool isdir=false, isfile=false;

	if(!PathStat(dirname, &isdir, &isfile)) return(L"");

	if(isdir) {
		return(L"1");
	}else{
		return(L"0");
	}
}
//---------------------------------------------------------------------------
wstring KIS_isfile::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 2, 2)) return (L"");

	wstring pathname=CanonicalPath(Engine->GetDataPath(), args[1]);
	bool isdir=false, isfile=false;

	if(!PathStat(pathname, &isdir, &isfile)) return(L"");

	if(isfile) {
		return(L"1");
	}else{
		return(L"0");
	}
}
//---------------------------------------------------------------------------
wstring KIS_isexist::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 2, 2)) return (L"");

	wstring dirname=PathToBaseDir(CanonicalPath(Engine->GetDataPath(), args[1]));
	wstring filename=PathToFileName(CanonicalPath(Engine->GetDataPath(), args[1]));

	unsigned int pos=(dirname).rfind(FILE_SEPARATOR);
	if(pos==wstring::npos) {
		// フルパスなのにdirnameにFILE_SEPARATORがない場合、推定でルート
		// FILE_SEPARATORを追加しカレントディレクトリとの誤認を防ぐ
		dirname+=FILE_SEPARATOR;
	}

	vector<wstring> names;
	if(!ListDirectory(dirname, names)) return(L"");

	wstring str=L"0";
	for(unsigned int i=0;i<names.size();i++) {
		const wstring &entry=names[i];
		if((entry!=L".")&&(entry!=L"..")&&(entry==filename)) {
			str=L"1";
			break;
		}
	}

	return(str);
}
//---------------------------------------------------------------------------
