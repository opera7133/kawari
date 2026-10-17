//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// SAORI インターフェース(SAORI over DirectSSTP)
//
//   DLLを読まず、SAORI/1.0の要求を本体へDirect SSTPで送り、
//   本体に内蔵されたモジュール (internal_crypto など) をSAORIとして呼ぶ。
//   仕様 : ukadoc の spec_saori_ex.html
//
//  2026.10.18  Phase 9.0.0   新設
//
//---------------------------------------------------------------------------
#ifndef SAORI_DSSTP_H
#define SAORI_DSSTP_H
//---------------------------------------------------------------------------
#include "config.h"
#include "saori/saori_module.h"
#include "misc/dsstp.h"
#include "misc/phttp.h"
#include "libkawari/kawari_log.h"
//---------------------------------------------------------------------------
#include <string>
//---------------------------------------------------------------------------
namespace saori{
//---------------------------------------------------------------------------
// パスの先頭にこれを付けると、必ず本体の内蔵モジュールとして扱う (L"directsstp:")
extern const wchar_t DIRECTSSTP_PREFIX[];

// モジュール名を小文字にする
std::wstring LowerModuleName(const std::wstring &name);

// 内蔵モジュール (internal_*) の名前として使える形か
// (internal_で始まり、英数字と_だけ。大文字小文字は問わない)
bool IsInternalModuleName(const std::wstring &name);

// pathが本体の内蔵モジュールを指すか。指すときはモジュール名 (小文字、.dllなし) を返す
// - "directsstp:名前" は常に該当
// - 拡張子が.dllで、名前が internal_ で始まるか互換モジュールの名前で、
//   DLLの実体が無いもの。実体があれば従来どおりDLLとして読む
bool ResolveDirectSSTPModule(const std::wstring &path, std::wstring &name);

// 本体の内蔵モジュールを呼ぶ (EXECUTE SAORI/1.0 を送る)
// request : Argument<n> を含むSAORI/1.0のリクエスト
// response : 応答。送れなかったときや本体が対応していないときは、
//            500 とErrorDescriptionを持つ応答を作る
// rcs : 応答のCharset (無い/不明ならKC_NULL)
// timeout_ms : 本体が応答するまで待つ時間
void CallDirectSSTPModule(const dsstp::TBasewareLink &link, TKawariLogger &logger,
	const std::wstring &module, const TPHMessage &request,
	TPHMessage &response, TKawariCharset &rcs, unsigned int timeout_ms);
//---------------------------------------------------------------------------
// 本体の内蔵モジュール
class TModuleDirectSSTP : public TModule{
public:
	virtual bool Initialize(void) { return true; }
	virtual bool Load(void) { return true; }
	virtual bool Unload(void) { return true; }

	// メッセージを経由しない呼び出し (使われない。念のため)
	virtual std::string Request(const std::string &reqstr);

	// SAORI/1.0 Request
	virtual bool RequestMessage(const TPHMessage &request, TPHMessage &response, TKawariCharset &rcs);

	TModuleDirectSSTP(IModuleFactory &fac, const std::wstring &p,
		const std::wstring &n, const dsstp::TBasewareLink *l)
		: TModule(fac, p, 0), name(n), link(l) {}

private:
	std::wstring name;
	const dsstp::TBasewareLink *link;
};
//---------------------------------------------------------------------------
class TModuleFactoryDirectSSTP : public IModuleFactory{
public:
	TModuleFactoryDirectSSTP(TKawariLogger &lgr, const dsstp::TBasewareLink *l)
		: IModuleFactory(lgr), link(l) {}

	// pathが内蔵モジュールを指すなら生成する。指さなければNULL
	virtual TModule *CreateModule(const std::wstring &path);

	virtual void DeleteModule(TModule *module);

private:
	const dsstp::TBasewareLink *link;
};
//---------------------------------------------------------------------------
} // namespace saori
//---------------------------------------------------------------------------
#endif // SAORI_DSSTP_H
