//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// SAORI インターフェース(SAORI over DirectSSTP)
//
//  2026.10.18  Phase 9.0.0   新設
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "saori/saori_dsstp.h"
#include "libkawari/kawari_log.h"
#include "misc/misc.h"
#include "misc/charset.h"
//---------------------------------------------------------------------------
#include <cwctype>
#include <map>
#include <vector>
using namespace std;
using namespace kawari_log;
//---------------------------------------------------------------------------
namespace saori{
//---------------------------------------------------------------------------
const wchar_t DIRECTSSTP_PREFIX[]=L"directsstp:";
//---------------------------------------------------------------------------
// DLLの代わりに本体の内蔵機能を呼べる、internal_以外の名前 (互換モジュール)
static const wchar_t * const compat_modules[]={
	L"houchi", L"clipcopy", L"textcopy", L"saori_ip", L"saori_cpuid",
	L"mciaudio", L"cpuusage", L"httpc", L"string", L"hanayu",
};
//---------------------------------------------------------------------------
wstring LowerModuleName(const wstring &name)
{
	wstring ret=name;
	for (wstring::iterator it=ret.begin(); it!=ret.end(); it++)
		*it=(wchar_t)towlower(*it);
	return ret;
}
//---------------------------------------------------------------------------
// internal_で始まり、英数字と_だけの名前か
bool IsInternalModuleName(const wstring &name)
{
	static const wchar_t head[]=L"internal_";
	const wstring::size_type headlen=(sizeof(head)/sizeof(head[0]))-1;
	if (name.size()<=headlen) return false;
	if (LowerModuleName(name.substr(0, headlen))!=head) return false;
	for (wstring::const_iterator it=name.begin(); it!=name.end(); it++){
		wchar_t c=*it;
		if (!(((c>=L'A')&&(c<=L'Z'))||((c>=L'a')&&(c<=L'z'))||((c>=L'0')&&(c<=L'9'))||(c==L'_')))
			return false;
	}
	return true;
}
//---------------------------------------------------------------------------
// pathが本体の内蔵モジュールを指すか
bool ResolveDirectSSTPModule(const wstring &path, wstring &name)
{
	const wstring::size_type prefixlen=(sizeof(DIRECTSSTP_PREFIX)/sizeof(DIRECTSSTP_PREFIX[0]))-1;
	if (path.compare(0, prefixlen, DIRECTSSTP_PREFIX)==0){
		name=LowerModuleName(path.substr(prefixlen));
		return !name.empty();
	}

	// 拡張子が.dllのものだけ
	wstring fname=LowerModuleName(PathToFileName(path));
	static const wchar_t ext[]=L".dll";
	const wstring::size_type extlen=(sizeof(ext)/sizeof(ext[0]))-1;
	if ((fname.size()<=extlen)||(fname.compare(fname.size()-extlen, extlen, ext)!=0))
		return false;
	fname.erase(fname.size()-extlen);

	bool is_module=IsInternalModuleName(fname);
	for (unsigned int i=0; (!is_module)&&(i<sizeof(compat_modules)/sizeof(compat_modules[0])); i++)
		is_module=(fname==compat_modules[i]);
	if (!is_module) return false;

	// 実体があるときは、これまでどおりDLLとして読む
	bool isdir=false, isfile=false;
	if (PathStat(path, &isdir, &isfile)&&isfile) return false;

	name=fname;
	return true;
}
//---------------------------------------------------------------------------
// エラー応答を作る
static void MakeError(TPHMessage &response, int code, const wstring &description)
{
	response=TPHMessage();
	response.SetStartline((code==400)?L"SAORI/1.0 400 Bad Request":L"SAORI/1.0 500 Internal Server Error");
	response.Add(L"Charset", L"UTF-8");
	response.Add(L"ErrorDescription", description);
}
//---------------------------------------------------------------------------
// 本体の内蔵モジュールを呼ぶ
void CallDirectSSTPModule(const dsstp::TBasewareLink &link, TKawariLogger &logger,
	const wstring &module, const TPHMessage &request,
	TPHMessage &response, TKawariCharset &rcs, unsigned int timeout_ms)
{
	response=TPHMessage();
	rcs=KC_NULL;

	// Direct SSTPは呼び出し元によらずLocalとして扱われるので、externalなリクエストからは呼ばない
	if (!link.IsAllowed()){
		MakeError(response, 400, L"external request is not allowed");
		logger.GetStream(LOG_WARNING) << L"[SAORI DirectSSTP] (" << module
			<< L") refused: the request is not from local." << endl;
		return;
	}
	if (dsstp::HasForbiddenChar(module)){
		MakeError(response, 400, L"invalid module name");
		return;
	}

	// 引数を番号順に並べる (空の引数も、その位置の引数として数える)
	map<unsigned int, wstring> argmap;
	unsigned int argcount=0;
	for (TPHMessage::const_iterator it=request.begin(); it!=request.end(); it++){
		const wstring &key=it->first;
		static const wchar_t head[]=L"Argument";
		const wstring::size_type headlen=(sizeof(head)/sizeof(head[0]))-1;
		if ((key.size()<=headlen)||(key.compare(0, headlen, head)!=0)) continue;
		if (key.find_first_not_of(L"0123456789", headlen)!=wstring::npos) continue;
		int idx=StringToInt(key.substr(headlen));
		if ((idx<0)||(idx>65535)) continue;
		argmap[(unsigned int)idx]=it->second;
		if ((unsigned int)idx+1>argcount) argcount=(unsigned int)idx+1;
	}

	wstring text=L"EXECUTE SAORI/1.0\r\nModule: "+module
		+L"\r\nCharset: UTF-8\r\nSender: kawari\r\nSecurityLevel: Local\r\n";
	for (unsigned int i=0; i<argcount; i++){
		wstring value;
		map<unsigned int, wstring>::const_iterator ai=argmap.find(i);
		if (ai!=argmap.end()) value=ai->second;
		if (dsstp::HasForbiddenChar(value)){
			// 送ると後ろにヘッダを足せてしまう
			MakeError(response, 400, L"Argument"+IntToString((int)i)+L" contains a line break");
			logger.GetStream(LOG_WARNING) << L"[SAORI DirectSSTP] (" << module
				<< L") Argument" << i << L" contains a line break. not sent." << endl;
			return;
		}
		text+=L"Argument"+IntToString((int)i)+L": "+value+L"\r\n";
	}
	text+=L"\r\n";

	string resbytes;
	if (!link.Send(WtoUTF8(text), resbytes, timeout_ms)){
		MakeError(response, 500, L"no response from baseware");
		logger.GetStream(LOG_ERROR) << L"[SAORI DirectSSTP] (" << module
			<< L") no response from baseware"
			<< (link.IsReady()?L".":L" (the window of the baseware is unknown yet).") << endl;
		return;
	}

	// 末尾に\0が付いてくることがある
	string::size_type nul=resbytes.find('\0');
	if (nul!=string::npos) resbytes.erase(nul);

	// 応答は要求と同じ文字コードで返る。Charsetが無ければUTF-8とみなす
	rcs=TPHMessage::ScanCharset(resbytes);
	response.Deserialize(resbytes, (rcs==KC_NULL)?KC_UTF8:rcs);

	// 対応していない本体は、SAORI/1.0の要求をSSTPの要求として読み、SSTP/1.xのエラーを返す
	if (response.GetStartline().compare(0, 6, L"SAORI/")!=0){
		MakeError(response, 500, L"baseware does not support SAORI over DirectSSTP");
		rcs=KC_NULL;
		logger.GetStream(LOG_ERROR) << L"[SAORI DirectSSTP] (" << module
			<< L") the baseware does not support SAORI over DirectSSTP." << endl;
		return;
	}

	// 失敗はErrorDescriptionを添えてログに出す
	const wstring &startline=response.GetStartline();
	wstring::size_type space=startline.find(L' ');
	int code=(space==wstring::npos)?0:StringToInt(startline.substr(space+1));
	if (code>=400){
		wstring description;
		TPHMessage::const_iterator ei=response.find(L"ErrorDescription");
		if (ei!=response.end()) description=ei->second;
		logger.GetStream(LOG_WARNING) << L"[SAORI DirectSSTP] (" << module << L") "
			<< startline << (description.empty()?L"":L" / ") << description << endl;
	}
}
//---------------------------------------------------------------------------
// メッセージを経由しない呼び出し
string TModuleDirectSSTP::Request(const string &reqstr)
{
	TPHMessage request, response;
	TKawariCharset rcs=KC_NULL;
	request.Deserialize(reqstr, TPHMessage::ScanCharset(reqstr));
	RequestMessage(request, response, rcs);
	return response.Serialize(KC_UTF8);
}
//---------------------------------------------------------------------------
// SAORI/1.0 Request
bool TModuleDirectSSTP::RequestMessage(const TPHMessage &request, TPHMessage &response, TKawariCharset &rcs)
{
	response=TPHMessage();
	rcs=KC_NULL;

	const wstring &startline=request.GetStartline();
	if (startline.compare(0, 11, L"GET Version")==0){
		// 本体に問い合わせず、使えるものとして答える (使えるかどうかは呼び出したときに分かる)
		response.SetStartline(L"SAORI/1.0 200 OK");
		response.Add(L"Charset", L"UTF-8");
		response.Add(L"Result", L"SAORI over DirectSSTP");
		rcs=KC_UTF8;
		return true;
	}
	if (startline.compare(0, 7, L"EXECUTE")!=0){
		MakeError(response, 400, L"invalid request");
		return true;
	}

	// 本体のモジュールは、結果が返るまで待つ。時間のかかる処理は、モジュール側の--asyncで
	// 非同期にする仕様なので、待つ上限は長めにしてある
	CallDirectSSTPModule(*link, GetFactory().GetLogger(), name, request, response, rcs, 30000);
	return true;
}
//---------------------------------------------------------------------------
TModule *TModuleFactoryDirectSSTP::CreateModule(const wstring &path)
{
	wstring name;
	if (!ResolveDirectSSTPModule(path, name)) return NULL;
	GetLogger().GetStream(LOG_INFO) << L"[SAORI DirectSSTP] CreateModule (" << name << L")" << endl;
	return new TModuleDirectSSTP(*this, path, name, link);
}
//---------------------------------------------------------------------------
void TModuleFactoryDirectSSTP::DeleteModule(TModule *module)
{
	delete module;
}
//---------------------------------------------------------------------------
} // namespace saori
//---------------------------------------------------------------------------
