//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// KawariInlineScript -- 正規表現 --
//
//      uka_regexp (build/src/uka_regexp, git submodule) を使う
//
//  2026.10.18  Phase 9.0.0   First version
//
//---------------------------------------------------------------------------
// 関数テーブルへの登録
#ifdef INLINE_SCRIPT_REGIST
INLINE_SCRIPT_REGIST(KIS_regex_match);
INLINE_SCRIPT_REGIST(KIS_regex_find);
INLINE_SCRIPT_REGIST(KIS_regex_findall);
INLINE_SCRIPT_REGIST(KIS_regex_count);
INLINE_SCRIPT_REGIST(KIS_regex_replace);
INLINE_SCRIPT_REGIST(KIS_regex_replace_first);
INLINE_SCRIPT_REGIST(KIS_regex_erase);
INLINE_SCRIPT_REGIST(KIS_regex_erase_first);
INLINE_SCRIPT_REGIST(KIS_regex_split);
INLINE_SCRIPT_REGIST(KIS_regex_escape);
#else
//---------------------------------------------------------------------------
#ifndef KIS_REGEX_H
#define KIS_REGEX_H
//---------------------------------------------------------------------------
#include "kis/kis_base.h"
//---------------------------------------------------------------------------
class KIS_regex_match : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"regex_match";
		Format_=L"regex_match STRING PATTERN [ OPTIONS [ ENTRY ] ]";
		Returnval_=L"\"1\" if PATTERN matches somewhere in STRING, \"0\" if not, \"\" if error";
		Information_=L"regular expression match. the matched text and the groups are stored to ENTRY";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
class KIS_regex_find : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"regex_find";
		Format_=L"regex_find STRING PATTERN [ OPTIONS [ INDEX [ ENTRY ] ] ]";
		Returnval_=L"position of the first match in STRING or -1 if not found, \"\" if error";
		Information_=L"regular expression search from INDEX. the matched text and the groups are stored to ENTRY";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
class KIS_regex_findall : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"regex_findall";
		Format_=L"regex_findall ENTRY STRING PATTERN [ OPTIONS ]";
		Returnval_=L"number of matches, \"\" if error";
		Information_=L"store all texts matched with PATTERN in STRING to ENTRY";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
class KIS_regex_count : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"regex_count";
		Format_=L"regex_count STRING PATTERN [ OPTIONS ]";
		Returnval_=L"number of matches, \"\" if error";
		Information_=L"count the matches of PATTERN in STRING";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
class KIS_regex_replace : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"regex_replace";
		Format_=L"regex_replace STRING PATTERN REPLACE [ OPTIONS ]";
		Returnval_=L"replaced STRING, \"\" if error";
		Information_=L"substitute all matches of PATTERN in STRING with REPLACE ($1 ${name} $& $$ are available)";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
class KIS_regex_replace_first : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"regex_replace_first";
		Format_=L"regex_replace_first STRING PATTERN REPLACE [ OPTIONS ]";
		Returnval_=L"replaced STRING, \"\" if error";
		Information_=L"substitute the first match of PATTERN in STRING with REPLACE";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
class KIS_regex_erase : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"regex_erase";
		Format_=L"regex_erase STRING PATTERN [ OPTIONS ]";
		Returnval_=L"STRING without the matches, \"\" if error";
		Information_=L"delete all matches of PATTERN from STRING";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
class KIS_regex_erase_first : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"regex_erase_first";
		Format_=L"regex_erase_first STRING PATTERN [ OPTIONS ]";
		Returnval_=L"STRING without the first match, \"\" if error";
		Information_=L"delete the first match of PATTERN from STRING";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
class KIS_regex_split : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"regex_split";
		Format_=L"regex_split ENTRY STRING PATTERN [ OPTIONS [ MAX ] ]";
		Returnval_=L"number of pieces, \"\" if error";
		Information_=L"split STRING at the matches of PATTERN and store the pieces to ENTRY";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
class KIS_regex_escape : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"regex_escape";
		Format_=L"regex_escape STRING";
		Returnval_=L"STRING escaped for a regular expression";
		Information_=L"put \\ before the special characters so that STRING matches itself";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
#endif // KIS_REGEX_H
//---------------------------------------------------------------------------
#endif // INLINE_SCRIPT_REGIST
