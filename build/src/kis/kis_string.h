//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// KawariInlineScript -- 文字列操作 --
//
//      Programed by Sky
//
//  2002.03.25  Phase 8        First version
//                             インデックスは全て0オリジンとなった。
//
//---------------------------------------------------------------------------
// 関数テーブルへの登録
#ifdef INLINE_SCRIPT_REGIST
INLINE_SCRIPT_REGIST(KIS_substr);
INLINE_SCRIPT_REGIST(KIS_length);
INLINE_SCRIPT_REGIST(KIS_match);
INLINE_SCRIPT_REGIST(KIS_match_at);
INLINE_SCRIPT_REGIST(KIS_char_at);
INLINE_SCRIPT_REGIST(KIS_rmatch);
INLINE_SCRIPT_REGIST(KIS_sub);
INLINE_SCRIPT_REGIST(KIS_rsub);
INLINE_SCRIPT_REGIST(KIS_gsub);
INLINE_SCRIPT_REGIST(KIS_reverse);
INLINE_SCRIPT_REGIST(KIS_tr);
INLINE_SCRIPT_REGIST(KIS_compare);
#else
//---------------------------------------------------------------------------
#ifndef KIS_STRING_H
#define KIS_STRING_H
//---------------------------------------------------------------------------
#include "kis/kis_base.h"
//---------------------------------------------------------------------------
// 旧expr系
//---------------------------------------------------------------------------
class KIS_substr : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"substr";
		Format_=L"substr TARGET START [ LENGTH ]";
		Returnval_=L"substring";
		Information_=L"cut TARGET out from START by LENGTH (or end of TARGET)";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
class KIS_length : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"length";
		Format_=L"length TARGET";
		Returnval_=L"length of argument string.";
		Information_=L"returns length of TARGET.";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
class KIS_match : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"match";
		Format_=L"match STRING PATTERN [ INDEX ]";
		Returnval_=L"position of PATTERN in STRING or -1 if not found";
		Information_=L"search PATTERN in STRING";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
// 新規
//---------------------------------------------------------------------------
class KIS_match_at : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"match_at";
		Format_=L"match_at STRING PATTERN [ INDEX ]";
		Returnval_=L"\"1\" if match, \"\" if not match.";
		Information_=L"returns if PATTERN matches with STRING at INDEX";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
class KIS_rmatch : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"rmatch";
		Format_=L"rmatch STRING PATTERN [ INDEX ]";
		Returnval_=L"last position of PATTERN in STRING or -1 if not found";
		Information_=L"search last PATTERN in STRING";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
class KIS_char_at : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"char_at";
		Format_=L"char_at STRING INDEX";
		Returnval_=L"character in STRING at specified INDEX";
		Information_=L"get character at specified index of string";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
class KIS_sub : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"sub";
		Format_=L"sub STRING PATTERN REPLACE";
		Returnval_=L"replaced STRING";
		Information_=L"substitute PATTERN found first in STRING with REPLACE";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
class KIS_rsub : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"rsub";
		Format_=L"rsub STRING PATTERN REPLACE";
		Returnval_=L"replaced STRING";
		Information_=L"substitute PATTERN found last in STRING with REPLACE";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
class KIS_gsub : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"gsub";
		Format_=L"gsub STRING PATTERN REPLACE";
		Returnval_=L"replaced STRING";
		Information_=L"substitute all PATTERNs found in STRING with REPLACE";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
class KIS_reverse : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"reverse";
		Format_=L"reverse STRING";
		Returnval_=L"reversed STRING";
		Information_=L"reverse order of characters in STRING";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
class KIS_tr : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"tr";
		Format_=L"tr STRING SEARCH REPLACE";
		Returnval_=L"transliterated STRING";
		Information_=L"do perl tr///d action";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
class KIS_compare : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"compare";
		Format_=L"compare STRING1 STRING2";
		Returnval_=L"1 if STRING1 is greater than STRING2. 0 if equqals. -1 if STRING2 is greater than STRING1";
		Information_=L"compare two STRINGs";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
#endif // KIS_STRING_H
//---------------------------------------------------------------------------
#endif // INLINE_SCRIPT_REGIST
