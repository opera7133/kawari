//---------------------------------------------------------------------------
//
// 本体(SSP, ninixなど)との通信 (Direct SSTP)
//
//   EXECUTE SSTP (getproperty / setproperty / executesstp) と
//   SAORI over DirectSSTP (本体に内蔵されたモジュールの呼び出し) の
//   送受信を受け持つ。
//   Windows : WM_COPYDATA (dwData=9801)。本体のウィンドウは NOTIFY hwnd で知る
//   POSIX   : ninix のUNIXソケット (FMOから探す)
//   それ以外 (js など) : 送れない (Sendは常にfalse)
//
//  2026.10.18  Phase 9.0.0   新設
//
//---------------------------------------------------------------------------
#ifndef DSSTP_H
#define DSSTP_H
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include <string>
#include <vector>
//---------------------------------------------------------------------------
namespace dsstp {
//---------------------------------------------------------------------------
// 本体への接続情報。エンジン1つにつき1つ持つ
class TBasewareLink {
private:
	// 本体のウィンドウ (NOTIFY hwnd のスコープ0の値)。分からなければ0
	unsigned long handle;

	// 今処理しているリクエストの送り主が、本体へ送ってよいか
	// (Direct SSTP は呼び出し元によらず常にLocalとして扱われるので、
	//  externalなリクエストからは送らない)
	bool allowed;

public:
	TBasewareLink(void) : handle(0), allowed(true) {}

	// ウィンドウを設定する
	void SetHandle(unsigned long h) { handle=h; }

	// NOTIFY hwnd の Reference0 ("123" または "123\x01" 区切りで各スコープ) から設定する
	// 戻り値 : 設定できたらtrue
	bool SetHandleFromReference(const std::wstring &ref);

	// 送り先が分かっているか
	bool IsReady(void) const { return handle!=0; }

	// 送ってよいリクエストか
	void SetAllowed(bool a) { allowed=a; }
	bool IsAllowed(void) const { return allowed; }

	// 要求(バイト列)を送り、応答(バイト列)を受け取る
	// 本体が応答するまで最大timeout_msミリ秒待つ
	// 戻り値 : 応答を受け取ったらtrue。送れなかった・応答が無かったときfalse
	bool Send(const std::string &request, std::string &response, unsigned int timeout_ms) const;
};
//---------------------------------------------------------------------------
// ヘッダの値に書けない文字 (CR, LF, NUL) を含むか
// (含んだまま送ると、後ろにヘッダを足せてしまう)
bool HasForbiddenChar(const std::wstring &s);
//---------------------------------------------------------------------------
// EXECUTE SSTP の要求 (UTF-8のバイト列) を作る
// refs : Reference0, Reference1, ... の値
std::string BuildExecuteRequest(const std::wstring &command, const std::vector<std::wstring> &refs);
//---------------------------------------------------------------------------
// EXECUTE SSTP の応答 (バイト列) を解析する
// 応答は「ステータス行、ヘッダ、空行、追加データ」。ヘッダがあるときは Content-Length で
// 長さが示され、無いときは追加データの後に空行が付く。
// lines : 追加データを1行ずつ入れる
// 戻り値 : 応答コード (解析できなければ0)
int ParseExecuteResponse(const std::string &response, std::vector<std::wstring> &lines);
//---------------------------------------------------------------------------
} // namespace dsstp
//---------------------------------------------------------------------------
#endif // DSSTP_H
