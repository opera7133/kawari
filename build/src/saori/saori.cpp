//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// SAORI インターフェース(汎用)
//
//      Programed by Suikyo.
//
//  2002.04.15  Phase 8.0.0   えびさわさんバージョンを参考に導入
//  2026.10.18  Phase 9.0.0   wchar_t化 (Charset対応)
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "saori/saori.h"
#include "saori/saori_module.h"
#include "libkawari/kawari_log.h"
//---------------------------------------------------------------------------
using namespace kawari_log;
using namespace std;
using namespace saori;
//---------------------------------------------------------------------------
TBind::TBind(IModuleFactory &fac, TKawariLogger &lgr, const wstring &path, const LOADTYPE t)
	 : type(t), libpath(path), module(NULL), factory(fac), logger(lgr), sendcs(KC_UTF8)
{
	if (type==PRELOAD)
		Attach();
}
//---------------------------------------------------------------------------
// SAORI/1.0 request
bool TBind::Request(const TPHMessage &request, TPHMessage &response)
{
	Attach();
	if (!module)
		return false;

	bool ret=Query(request, response);

	if (type==NORESIDENT)
		Detach();

	return ret;
}
//---------------------------------------------------------------------------
// respcs : 応答のCharsetヘッダ (無い/不明ならKC_NULL)
bool TBind::Query(const TPHMessage &request, TPHMessage &response, TKawariCharset *respcs){

	// 送信するCharsetを要求ヘッダに反映する
	TPHMessage req(request);
	req.erase(L"Charset");
	req.Add(L"Charset", CharsetName(sendcs));

	if (logger.Check(LOG_INFO)){
		logger.GetStream() << L"[SAORI] Query to ("+libpath+L")" << endl
			<< L"---------------------- REQUEST" << endl;
		req.Dump(logger.GetStream());
	}

	// シリアライズ
	string reqstr=req.Serialize(sendcs);

	// Request
	string resstr=module->Request(reqstr);

	// デシリアライズ
	// 応答のCharsetに従う。無ければ、送ったのがSJISならSJIS、そうでなければ自動判定
	TKawariCharset rcs=TPHMessage::ScanCharset(resstr);
	TKawariCharset dcs=rcs;
	if (dcs==KC_NULL)
		dcs=(sendcs==KC_SJIS)?KC_SJIS:KC_NULL;
	response=TPHMessage();
	response.Deserialize(resstr, dcs);

	// 応答に有効なCharsetがあれば、次回以降の送信に使う
	if (rcs!=KC_NULL)
		sendcs=rcs;
	if (respcs) *respcs=rcs;

	if (logger.Check(LOG_INFO)){
		logger.GetStream() << L"----------------------RESPONSE" << endl;
		response.Dump(logger.GetStream());
		logger.GetStream() << L"[SAORI] Query end." << endl;
	}

	return true;
}
//---------------------------------------------------------------------------
void TBind::Attach(void){
	if (module) return;
	module=factory.CreateModule(libpath);
	if (!module){
		logger.GetStream(LOG_ERROR) << L"[SAORI] module attach failed" << endl;
		return;
	}

	TPHMessage request, response;

	// GET Version チェック
	// 最初はUTF-8で送り、応答のCharsetがあればそれを以後使う。
	// 無ければ以後はShift_JISで送る。
	sendcs=KC_UTF8;
	request.SetStartline(L"GET Version SAORI/1.0");
	request[L"Sender"]=L"kawari";

	TKawariCharset rcs=KC_NULL;
	Query(request, response, &rcs);
	if (rcs==KC_NULL)
		sendcs=KC_SJIS;

	if (response.GetStartline().find(L"SAORI/1.")!=0){
		logger.GetStream(LOG_ERROR) << L"[SAORI] SAORI version mismatch." << endl;
		Detach();
		return;
	}else{
		logger.GetStream(LOG_INFO) << L"[SAORI] (" << libpath << L") attached." << endl;
	}
}
//---------------------------------------------------------------------------
void TBind::Detach(void){
	if (module){
		factory.DeleteModule(module);
		module=NULL;
	}
	logger.GetStream(LOG_INFO) << L"[SAORI] (" << libpath << L") detached." << endl;
}
//---------------------------------------------------------------------------
TBind::~TBind(){
	Detach();
}
//---------------------------------------------------------------------------
// モジュールの登録
void TSaoriPark::RegisterModule(const wstring &aliasname, const wstring &path, const saori::LOADTYPE type){
	// 2002/09/02 多重ロード対策 suikyo@yk.rim.or.jp
	if (aliasmap.count(aliasname))
		EraseModule(aliasname);
	TBind *module=new TBind((*factory), logger, path, type);
	aliasmap[aliasname]=module;

	logger.GetStream(LOG_INFO) << L"[SAORI] Registered \"" << aliasname << L"\" = (" << path << L")" << endl;
}
//---------------------------------------------------------------------------
// モジュール登録の削除
void TSaoriPark::EraseModule(const wstring &alias){
	if (aliasmap.count(alias)){
		TBind *module=aliasmap[alias];
		delete module;
		aliasmap.erase(alias);
		logger.GetStream(LOG_INFO) << L"[SAORI] Unregistered (" << alias << L")" << endl;
	}else{
		logger.GetStream(LOG_WARNING) << L"[SAORI] Can not unregister (" << alias << L"). not found." << endl;
	}
}
//---------------------------------------------------------------------------
// モジュールを得る
TBind * const TSaoriPark::GetModule(const wstring &alias) {
	if (aliasmap.count(alias)){
		return aliasmap[alias];
	}else{
		logger.GetStream(LOG_ERROR) << L"[SAORI] module (" << alias << L") not found." << endl;
		return NULL;
	}
}
//---------------------------------------------------------------------------
// 登録済みモジュール名のリストを得る
int TSaoriPark::ListModule(vector<wstring> &list)
{
	logger.GetStream(LOG_INFO) << L"listmodule" << endl;
	int ret=0;
	map<wstring, TBind *>::const_iterator it=aliasmap.begin();
	for(;it!=aliasmap.end(); it++){
		logger.GetStream(LOG_INFO) << L"[SAORI] found(" << it->first << L")" << endl;
		list.push_back(it->first);
		ret++;
	}
	return ret;
}
//---------------------------------------------------------------------------
TSaoriPark::TSaoriPark(TKawariLogger &lgr) : logger(lgr) {
	factory=new TModuleFactoryMaster(logger);
}
//---------------------------------------------------------------------------
// 全モジュールのアンロード
TSaoriPark::~TSaoriPark(){
	for(map<wstring, TBind *>::iterator it=aliasmap.begin(); it!=aliasmap.end(); it++){
		if (it->second){
			delete it->second;
		}
	}
	delete factory;
}
//---------------------------------------------------------------------------
