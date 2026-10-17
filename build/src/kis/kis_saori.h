//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// KawariInlineScript -- SAORI --
//
//  2002.03.28  created
//  2002.03.29  add callsaorix
//
//---------------------------------------------------------------------------
// 関数テーブルへの登録
#ifdef INLINE_SCRIPT_REGIST
INLINE_SCRIPT_REGIST(KIS_saoriregist);
INLINE_SCRIPT_REGIST(KIS_saorierase);
INLINE_SCRIPT_REGIST(KIS_saorilist);
INLINE_SCRIPT_REGIST(KIS_callsaori);
INLINE_SCRIPT_REGIST(KIS_callsaorix);
#else
//---------------------------------------------------------------------------
#ifndef KIS_SAORI_H
#define KIS_SAORI_H
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "kis/kis_base.h"
#include "misc/phttp.h"
//---------------------------------------------------------------------------
#include <vector>
#include <map>
using namespace std;
//---------------------------------------------------------------------------
class KIS_saoriregist : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"saoriregist";
		Format_=L"saoriregist dllname aliasname [ loadoption ]";
		Returnval_=L"(NULL)";
		Information_=L"register saorimodule";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
class KIS_saorierase : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"saorierase";
		Format_=L"saorierase aliasname";
		Returnval_=L"(NULL)";
		Information_=L"unregister saorimodule";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
class KIS_saorilist : public TKisFunction_base {
public:
	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"saorilist";
		Format_=L"saorilist stem";
		Returnval_=L"list of registered aliases";
		Information_=L"returns a list of registered aliases, ether loaded or not";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
class KIS_callsaori : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"callsaori";
		Format_=L"callsaori saorimodulename [param0] ...";
		Returnval_=L"(dll specific)";
		Information_=L"invoke request to SAORI/1.0 module";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);

protected:
	virtual bool CallSaori(
		const wstring& saoriname, const vector<wstring>& args,
		TPHMessage &response);
};
//---------------------------------------------------------------------------
class KIS_callsaorix : public KIS_callsaori {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"callsaorix";
		Format_=L"callsaorix stem saorimodulename [param0] ...";
		Returnval_=L"(dll specific)";
		Information_=L"invoke request to SAORI/1.0 module, and store values to stem";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
#endif
