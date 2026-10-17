//---------------------------------------------------------------------------
//
//  Kawari - KOSUI Adapter
//
//      Programed by Suikyo
//
//  2002.04.12  Phase 8.0.0   インターフェース切り直し
//  2026.10.18  Phase 9.0.0   wchar_t化
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#ifndef KAWARI_KOSUI_H
#define KAWARI_KOSUI_H
//---------------------------------------------------------------------------
#include <string>
#include <iostream>
#include <cstdlib>
#include <ctime>
using namespace std;
//---------------------------------------------------------------------------
#include "tool/kosui_base.h"
#include "libkawari/kawari_engine.h"
#include "libkawari/kawari_log.h"
#include "libkawari/kawari_version.h"
#include "misc/misc.h"
//---------------------------------------------------------------------------
// 華和梨＝幸水アダプタ
class TKawariKosuiAdapter: public TKawariInterface_base {
private:

	// 華和梨エンジン
	TKawariEngine engine;

public:

	TKawariKosuiAdapter(const wstring& datapath,const wstring& inifile)
	{
		// 乱数初期化
		SRandom((unsigned int)time(NULL));

		engine.CreateEntry(L"System.DataPath").Push(engine.CreateStrWord(datapath));
		engine.WriteProtect(L"System.DataPath");

		// セットアップ
		engine.GetLogger().SetStreamStdOut();
		engine.SetDataPath(datapath);

		if (inifile.size())
			engine.LoadKawariDict(inifile);
	}

	~TKawariKosuiAdapter()
	{
		TEntry entry=engine.GetEntry(L"System.Callback.OnUnload");
		unsigned int size=entry.Size();
		for(unsigned int i=0;i<size;i++) engine.Parse(entry.Index(i));
	}

	// 情報を得る (バージョン情報はASCIIのみ)
	virtual wstring GetInformation(void)
	{
		return AtoW(string(
			"Version : " KAWARI_NAME "/" KAWARI_MAJOR "." KAWARI_MINOR "." KAWARI_SUBMINOR "\n"
			"Kernel Version : " KAWARI_CORE_VERSION "  / " KAWARI_CORE_CODENAME "\n"
			KAWARI_CORE_CREDITS));
	}

	// 与えられたスクリプトを解釈・実行する
	virtual wstring Parse(const wstring& script)
	{
		return(engine.Parse(script));
	}
};
//---------------------------------------------------------------------------
#endif // KAWARI_KOSUI_H
