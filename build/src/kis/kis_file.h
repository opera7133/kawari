//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// KawariInlineScript -- file attribute --
//
//      Programed by NAKAUE.T (Meister)
//
//  2001.05.14   Phase 0.50    First version
//  2001.06.17   Phase6.0にあわせて修正 (NAKAUE.T)
//               load
//  2001.12.09  Phase 7.1.2   kis_textfileと統合
//  2002.03.17  Phase 7.9.0   Phase7.9に合わせて変更 (NAKAUE.T)
//                            textload仕様変更/textsave追加
//                            readdirで.と..を除外
//  2002.07.10  Phase 8.1.0   isdir/isexist/isfile追加
//
//---------------------------------------------------------------------------
// 関数テーブルへの登録
#ifdef INLINE_SCRIPT_REGIST
INLINE_SCRIPT_REGIST(KIS_save);
INLINE_SCRIPT_REGIST(KIS_savecrypt);
INLINE_SCRIPT_REGIST(KIS_load);
INLINE_SCRIPT_REGIST(KIS_textload);
INLINE_SCRIPT_REGIST(KIS_textsave);
INLINE_SCRIPT_REGIST(KIS_textappend);
INLINE_SCRIPT_REGIST(KIS_readdir);
INLINE_SCRIPT_REGIST(KIS_cncpath);
INLINE_SCRIPT_REGIST(KIS_dirname);
INLINE_SCRIPT_REGIST(KIS_filename);
INLINE_SCRIPT_REGIST(KIS_isdir);
INLINE_SCRIPT_REGIST(KIS_isfile);
INLINE_SCRIPT_REGIST(KIS_isexist);
#else
//---------------------------------------------------------------------------
#ifndef KIS_FILE_H
#define KIS_FILE_H
//---------------------------------------------------------------------------
#include "kis/kis_base.h"
//---------------------------------------------------------------------------
class KIS_save : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"save";
		Format_=L"save File1 Entry1 [Entry2...]";
		Returnval_=L"(NULL)";
		Information_=L"save entries to File1";

		return(true);
	}

	// インタープリタ
    virtual wstring Function(const vector<wstring>& args){
        Run(args, false);
        return L"";
    }
protected:
    virtual void Run(const vector<wstring>& args, bool crypt);
};
//---------------------------------------------------------------------------
class KIS_savecrypt : public KIS_save {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"savecrypt";
		Format_=L"savecrypt File1 Entry1 [Entry2...]";
		Returnval_=L"(NULL)";
		Information_=L"save entries to File1 by encrypt expression";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args)
	{
		Run(args, true);
		return L"";
	}
};
//---------------------------------------------------------------------------
class KIS_load : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"load";
		Format_=L"load File1";
		Returnval_=L"(NULL)";
		Information_=L"load dictionary file";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);

};
//---------------------------------------------------------------------------
class KIS_textload : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"textload";
		Format_=L"textload Entry1 File1";
		Returnval_=L"(NULL)";
		Information_=L"load textfile to Entry1";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
class KIS_textsave : public TKisFunction_base {
public:

	// インタープリタの実体
	wstring Function_(const vector<wstring>& args,bool flag);

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"textsave";
		Format_=L"textsave File1 Entry1 [Entry2...]";
		Returnval_=L"(NULL)";
		Information_=L"save textfile by Entries";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args)
	{
		return(Function_(args,true));
	}
};
//---------------------------------------------------------------------------
class KIS_textappend : public KIS_textsave {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"textappend";
		Format_=L"textappend File1 Entry1 [Entry2...]";
		Returnval_=L"(NULL)";
		Information_=L"append textfile by Entries";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args)
	{
		return(Function_(args,false));
	}
};
//---------------------------------------------------------------------------
class KIS_readdir : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"readdir";
		Format_=L"readdir Entry1 Dir1";
		Returnval_=L"(NULL)";
		Information_=L"read directory entries and set to Entry1";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
class KIS_cncpath : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"cncpath";
		Format_=L"cncpath path [ expath ]";
		Returnval_=L"path expression in canonical form";
		Information_=L"create canonical path expression";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
class KIS_dirname : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"dirname";
		Format_=L"dirname path";
		Returnval_=L"base directory";
		Information_=L"get directory part of path";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
class KIS_filename : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"filename";
		Format_=L"filename path";
		Returnval_=L"filename";
		Information_=L"get filename part of path";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
class KIS_isdir : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"isdir";
		Format_=L"isdir path";
		Returnval_=L"true or false";
		Information_=L"answer if path is directory or not";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
class KIS_isfile : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"isfile";
		Format_=L"isfile path";
		Returnval_=L"true or false";
		Information_=L"answer if path is file or not";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
class KIS_isexist : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"isexist";
		Format_=L"isexist path";
		Returnval_=L"true or false";
		Information_=L"answer if path is exist or not";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
#endif	// KIS_FILE_H
//---------------------------------------------------------------------------
#endif	// INLINE_SCRIPT_REGIST

