//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// SAORI インターフェース
//
//      Programed by Suikyo.
//
//  2002.04.15  Phase 8.0.0   えびさわさんバージョンを参考に導入
//  2026.10.18  Phase 9.0.0   wchar_t化 (SAORIとの送受信はCharsetに従う)
//
//---------------------------------------------------------------------------
#ifndef SAORI_H
#define SAORI_H
//---------------------------------------------------------------------------
#include "config.h"
#include "misc/phttp.h"
#include "misc/charset.h"
#include "libkawari/kawari_log.h"
//---------------------------------------------------------------------------
#include <string>
#include <map>
#include <vector>
//---------------------------------------------------------------------------
namespace saori{
enum LOADTYPE {
	PRELOAD,
	LOADONCALL,
	NORESIDENT
};
class SAORIInfo{
public:
	std::wstring path;
	int bindcount;
	int loadcount;

	SAORIInfo(const std::wstring &p, int bc, int lc): path(p), bindcount(bc), loadcount(lc) {}
};
//---------------------------------------------------------------------------
// SAORIたん縛り
class TBind{
public:
	// SAORI/1.0 リクエストを発行
	// ライブラリのロード＋リンクは必要に応じて行われる。
	// 戻り値 : request/responseプロトコルが成功すればTRUE
	bool Request(const TPHMessage &request, TPHMessage &response);

	TBind(class IModuleFactory &fac, TKawariLogger &lgr, const std::wstring &path, const saori::LOADTYPE t);

	~TBind();

private:
	saori::LOADTYPE type;
	std::wstring libpath;
	class TModule *module;
	class IModuleFactory &factory;
	TKawariLogger &logger;

	// 次回以降の送信に使う文字コード (初期値UTF-8。GET Versionの応答で決まる)
	TKawariCharset sendcs;

	bool Query(const TPHMessage &request, TPHMessage &response, TKawariCharset *respcs=NULL);
	void Attach(void);
	void Detach(void);

};
//---------------------------------------------------------------------------
// Facadeクラス
class TSaoriPark {
public:
	// モジュールの登録
	void RegisterModule(const std::wstring &alias, const std::wstring &path, const LOADTYPE type);

	// モジュール登録の削除
	void EraseModule(const std::wstring &alias);

	// モジュールを得る
	TBind *const GetModule(const std::wstring &alias);

	// 登録済みモジュール名のリストを得る
	int ListModule(std::vector<std::wstring> &list);

	// コンストラクタ
	TSaoriPark(TKawariLogger &logger);

	// 全モジュールのアンロード
	~TSaoriPark();

private:
	class TModuleFactoryMaster *factory;
	TKawariLogger &logger;

	// alias -> Module
	std::map<std::wstring, TBind *> aliasmap;

	friend class TBind;
};
//---------------------------------------------------------------------------
} // namespace saori
//---------------------------------------------------------------------------
#endif // SAORI_H
