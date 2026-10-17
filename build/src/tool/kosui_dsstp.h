//---------------------------------------------------------------------------
//
// KOSUI - Direct SSTP Adapter
//
//      Programed by Suikyo.
//
//  2002.05.04  Phase 8.0.0   FirstVersion
//  2026.10.18  Phase 9.0.0   wchar_t化
//
//---------------------------------------------------------------------------
#ifndef KOSUI_DSSTP_H
#define KOSUI_DSSTP_H
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include <windows.h>
#include <string>
//---------------------------------------------------------------------------
class TKosuiDSSTPInterface : public TKawariInterface_base {
public:
	// コンストラクタ
	// hwnd : DAのウィンドウハンドル
	// event : 使用するイベント名
	TKosuiDSSTPInterface (HWND hwnd, const std::wstring& evt);

	// デストラクタ
	virtual ~TKosuiDSSTPInterface();

	// 情報を得る
	virtual std::wstring GetInformation(void);

	// 与えられたソースを解釈・実行する
	virtual std::wstring Parse(const std::wstring& script);

private:
	// リクエストタイプ
	std::wstring event;

	// DAのウィンドウハンドル
	HWND dahwnd;

	// DSSTPに使うこちら側のダミーウィンドウハンドル
	HWND dumhwnd;
};
//---------------------------------------------------------------------------
#endif // KOSUI_DSSTP_H
