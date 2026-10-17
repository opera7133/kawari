//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// KawariInlineScript -- オンラインヘルプ --
//
//      Programed by NAKAUE.T (Meister)
//
//  2001.07.21  Phase 6.2     First version
//  2001.08.06  Phase 6.2     ver追加
//  2001.08.08  Phase 6.2     helpに関数一覧追加
//
//---------------------------------------------------------------------------
// 関数テーブルへの登録
#ifdef INLINE_SCRIPT_REGIST
INLINE_SCRIPT_REGIST(KIS_help);
INLINE_SCRIPT_REGIST(KIS_ver);
#else
//---------------------------------------------------------------------------
#ifndef KIS_HELP_H
#define KIS_HELP_H
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "kis/kis_base.h"
//---------------------------------------------------------------------------
class KIS_help : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"help";
		Format_=L"help Command1";
		Returnval_=L"help message";
		Information_=L"print online help of KIS command (for Kosui use)";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
class KIS_ver : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"ver";
		Format_=L"ver";
		Returnval_=L"version info";
		Information_=L"return KAWARI version info formatted by \"basename.subname/verNo.\"";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
#endif

