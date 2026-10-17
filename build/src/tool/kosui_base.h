//---------------------------------------------------------------------------
//
// 抽象化華和梨インターフェース
//
//      Programed by NAKAUE.T (Meister)
//
//  2001.08.12  Phase 6.2.1   FirstVersion
//  2026.10.18  Phase 9.0.0   wchar_t化
//
//---------------------------------------------------------------------------
#ifndef KOSUI_BASE_H
#define KOSUI_BASE_H
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include <string>
#include <iostream>
using namespace std;
//---------------------------------------------------------------------------
class TKawariInterface_base {
public:
	// デストラクタ
	virtual ~TKawariInterface_base() {}

	// 情報を得る
	virtual wstring GetInformation(void)=0;

	// 与えられたスクリプトを解釈・実行する
	virtual wstring Parse(const wstring& script)=0;
};
//---------------------------------------------------------------------------
#endif

