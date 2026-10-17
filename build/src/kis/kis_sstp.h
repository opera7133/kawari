//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// KawariInlineScript -- 本体との連携 (EXECUTE SSTP) --
//
//  2026.10.18  Phase 9.0.0   getproperty / setproperty / executesstp / executesstpx 追加
//
//---------------------------------------------------------------------------
// 関数テーブルへの登録
#ifdef INLINE_SCRIPT_REGIST
INLINE_SCRIPT_REGIST(KIS_executesstp);
INLINE_SCRIPT_REGIST(KIS_executesstpx);
INLINE_SCRIPT_REGIST(KIS_getproperty);
INLINE_SCRIPT_REGIST(KIS_setproperty);
#else
//---------------------------------------------------------------------------
#ifndef KIS_SSTP_H
#define KIS_SSTP_H
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "kis/kis_base.h"
//---------------------------------------------------------------------------
#include <vector>
using namespace std;
//---------------------------------------------------------------------------
class KIS_executesstp : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"executesstp";
		Format_=L"executesstp command [reference0] ...";
		Returnval_=L"first line of the additional data";
		Information_=L"send EXECUTE SSTP to the baseware. "
			L"System.SSTP.ResponseCode and System.SSTP.Data are set";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);

protected:
	// EXECUTE SSTP を送る
	// 応答コード(送れなかったら0)をSystem.SSTP.ResponseCodeに、
	// 追加データ(1行1単語)をSystem.SSTP.Dataに入れる
	// 戻り値 : 応答コード
	int Execute(const wstring& name, const wstring& command,
		const vector<wstring>& refs, vector<wstring>& lines);
};
//---------------------------------------------------------------------------
class KIS_executesstpx : public KIS_executesstp {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"executesstpx";
		Format_=L"executesstpx stem command [reference0] ...";
		Returnval_=L"first line of the additional data";
		Information_=L"send EXECUTE SSTP to the baseware, "
			L"and store the additional data to stem (one line per word)";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
class KIS_getproperty : public KIS_executesstp {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"getproperty";
		Format_=L"getproperty propertyname [default]";
		Returnval_=L"value of the property";
		Information_=L"get a value from the property system of the baseware";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
class KIS_setproperty : public KIS_executesstp {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"setproperty";
		Format_=L"setproperty propertyname value";
		Returnval_=L"(NULL)";
		Information_=L"set a value to the property system of the baseware";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
#endif
