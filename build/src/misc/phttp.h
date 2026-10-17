//---------------------------------------------------------------------------
//
// TPHMessage - 偽HTTPメッセージ -
//
//      Programed by Suikyo.
//
//  2026.10.18  Phase 9.0.0   wchar_t化
//
//---------------------------------------------------------------------------
#ifndef PHTTP_H
#define PHTTP_H
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "misc/mmap.h"
#include "misc/charset.h"
//---------------------------------------------------------------------------
#include <iostream>
#include <string>
//---------------------------------------------------------------------------
class TPHMessage : public TMMap<std::wstring, std::wstring> {
private:
	std::wstring startline;
public:
	// スタートラインの設定
	void SetStartline(const std::wstring &line) { startline=line; }

	// スタートラインを得る
	const std::wstring &GetStartline(void) const { return startline; }

	// シリアライズ (wide文字列)
	std::wstring SerializeW(void) const;

	// シリアライズ (指定文字コードのバイト列)
	std::string Serialize(TKawariCharset cs) const;

	// デシリアライズ (wide文字列)
	void DeserializeW(const std::wstring &mes);

	// デシリアライズ (指定文字コードのバイト列。KC_NULLなら自動判定)
	void Deserialize(const std::string &mes, TKawariCharset cs);

	// 生バイト列のヘッダから"Charset:"を探して文字コードを返す
	// (ASCIIで探す。大文字小文字無視。無い/不明な場合はKC_NULL)
	static TKawariCharset ScanCharset(const std::string &mes);

	// ダンプ
	void Dump(std::wostream &os) const;
};
//---------------------------------------------------------------------------
#endif // PHTTP_H
