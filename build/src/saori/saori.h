//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// SAORI インターフェース
//
//      Programed by Suikyo.
//
//  2002.04.15  Phase 8.0.0   えびさわさんバージョンを参考に導入
//  2026.10.18  Phase 9.0.0   wchar_t化 (SAORIとの送受信はCharsetに従う)
//                            SAORI over DirectSSTP対応 (本体の内蔵モジュール)
//
//---------------------------------------------------------------------------
#ifndef SAORI_H
#define SAORI_H
//---------------------------------------------------------------------------
#include "config.h"
#include "misc/phttp.h"
#include "misc/charset.h"
#include "misc/dsstp.h"
#include "libkawari/kawari_log.h"
//---------------------------------------------------------------------------
#include <string>
#include <map>
#include <set>
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
	// 登録されていないinternal_で始まる名前は、本体の内蔵モジュールにあれば登録してから返す
	TBind *const GetModule(const std::wstring &alias);

	// 登録済みモジュール名のリストを得る
	int ListModule(std::vector<std::wstring> &list);

	// コンストラクタ
	// link : 本体への接続 (SAORI over DirectSSTPで使う)
	TSaoriPark(TKawariLogger &logger, dsstp::TBasewareLink *link);

	// 全モジュールのアンロード
	~TSaoriPark();

private:
	class TModuleFactoryMaster *factory;
	TKawariLogger &logger;
	dsstp::TBasewareLink *link;

	// alias -> Module
	std::map<std::wstring, TBind *> aliasmap;

	// 本体の内蔵モジュールの一覧 (internal_systemのListModulesで得る)
	enum TInternalModules {
		IM_UNKNOWN,	// まだ問い合わせていない (本体のウィンドウが分からない場合を含む)
		IM_LISTED,	// 一覧を得た
		IM_ANY,		// 一覧は得られないが、SAORI over DirectSSTPには対応している
		IM_NONE		// 対応していない
	} internalstate;
	std::set<std::wstring> internalmodules;

	// 本体に内蔵モジュールの一覧を問い合わせる
	void FetchInternalModules(void);

	// aliasが本体の内蔵モジュール(internal_*)の名前なら、登録してtrueを返す
	bool RegisterInternalModule(const std::wstring &alias);

	friend class TBind;
};
//---------------------------------------------------------------------------
} // namespace saori
//---------------------------------------------------------------------------
#endif // SAORI_H
