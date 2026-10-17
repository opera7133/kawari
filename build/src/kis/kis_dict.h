//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// KawariInlineScript -- 辞書操作 --
//
//      Programed by NAKAUE.T (Meister)
//
//  2001.05.14  Phase 0.50    First version
//  2001.05.30  Phase 5.1     
//  2001.07.08  Phase 6.0     evalバグフィックス
//  2001.07.14  Phase 6.1     clear追加
//  2001.08.08  Phase 6.2     entry追加
//  2001.08.25  Phase 6.3     entry仕様追加
//                            getこっそり追加
//                            size追加
//  2001.12.18  Phase 7.2     arrayさらにこっそり追加
//  2002.03.10  Phase 7.9.0   辞書の直接アクセス禁止
//                            enumerate,array廃止
//                            get仕様変更,exec,enumexec導入
//  2002.03.13                エントリ名に添え字概念導入
//  2002.03.17                辞書操作関数整理
//                            list操作関数追加
//  2002.04.27  Phase 8.0.0   writeprotect追加
//  2003.04.27  Phase 8.1.0   shiftcodeバグフィックス
//  2008.02.16  Phase 8.2.7   insertstrバグフィックス
//
//---------------------------------------------------------------------------
// 関数テーブルへの登録
#ifdef INLINE_SCRIPT_REGIST
INLINE_SCRIPT_REGIST(KIS_encode_entryname);
INLINE_SCRIPT_REGIST(KIS_eval);
INLINE_SCRIPT_REGIST(KIS_size);
INLINE_SCRIPT_REGIST(KIS_set);
INLINE_SCRIPT_REGIST(KIS_setstr);
INLINE_SCRIPT_REGIST(KIS_adddict);
INLINE_SCRIPT_REGIST(KIS_adddictstr);
INLINE_SCRIPT_REGIST(KIS_push);
INLINE_SCRIPT_REGIST(KIS_pushstr);
INLINE_SCRIPT_REGIST(KIS_unshift);
INLINE_SCRIPT_REGIST(KIS_unshiftstr);
INLINE_SCRIPT_REGIST(KIS_pop);
INLINE_SCRIPT_REGIST(KIS_shift);
INLINE_SCRIPT_REGIST(KIS_popcode);
INLINE_SCRIPT_REGIST(KIS_shiftcode);
INLINE_SCRIPT_REGIST(KIS_entry);
INLINE_SCRIPT_REGIST(KIS_get);
INLINE_SCRIPT_REGIST(KIS_getrandom);
INLINE_SCRIPT_REGIST(KIS_getcode);
INLINE_SCRIPT_REGIST(KIS_clear);
INLINE_SCRIPT_REGIST(KIS_insert);
INLINE_SCRIPT_REGIST(KIS_insertstr);
INLINE_SCRIPT_REGIST(KIS_writeprotect);
INLINE_SCRIPT_REGIST(KIS_wordcount);
INLINE_SCRIPT_REGIST(KIS_entrycount);
INLINE_SCRIPT_REGIST(KIS_find);
INLINE_SCRIPT_REGIST(KIS_rfind);
INLINE_SCRIPT_REGIST(KIS_cleartree);
INLINE_SCRIPT_REGIST(KIS_listsub);
INLINE_SCRIPT_REGIST(KIS_listtree);
INLINE_SCRIPT_REGIST(KIS_copy);
INLINE_SCRIPT_REGIST(KIS_copytree);
INLINE_SCRIPT_REGIST(KIS_move);
INLINE_SCRIPT_REGIST(KIS_movetree);
#else
//---------------------------------------------------------------------------
#ifndef KIS_DICT_H
#define KIS_DICT_H
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "kis/kis_base.h"
//---------------------------------------------------------------------------
class KIS_encode_entryname : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"encode_entryname";
		Format_=L"encode_entryname Word1";
		Returnval_=L"encoded entry name";
		Information_=L"encode Word1 to entry name";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
class KIS_eval : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"eval";
		Format_=L"eval Word1 ...";
		Returnval_=L"decode result";
		Information_=L"parse all arguments and return its decode result";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
class KIS_size : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"size";
		Format_=L"size Entry1";
		Returnval_=L"a word number in Entry1";
		Information_=L"return a word number in Entry1";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
class KIS_set : public TKisFunction_base {
protected:

	// インタープリタの実体
	wstring Function_(const vector<wstring>& args,bool flag);

public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"set";
		Format_=L"set Index1 Word1";
		Returnval_=L"(NULL)";
		Information_=L"set Word1 to Index1";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args)
	{
		return(Function_(args,false));
	}
};
//---------------------------------------------------------------------------
class KIS_setstr : public KIS_set {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"setstr";
		Format_=L"setstr Index1 Str1";
		Returnval_=L"(NULL)";
		Information_=L"set string Str1 to Index1 (equals to 'set')";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args)
	{
		return(Function_(args,true));
	}
};
//---------------------------------------------------------------------------
class KIS_adddict : public TKisFunction_base {
protected:

	// インタープリタの実体
	wstring Function_(const vector<wstring>& args,bool flag);

public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"adddict";
		Format_=L"adddict Entry1 Word1";
		Returnval_=L"(NULL)";
		Information_=L"append Word1 to Entry1";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args)
	{
		return(Function_(args,false));
	}
};
//---------------------------------------------------------------------------
class KIS_adddictstr : public KIS_adddict {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"adddictstr";
		Format_=L"adddictstr Entry1 Str1";
		Returnval_=L"(NULL)";
		Information_=L"append string Str1 to Entry1";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args)
	{
		return(Function_(args,true));
	}
};
//---------------------------------------------------------------------------
class KIS_push : public KIS_adddict {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"push";
		Format_=L"push Entry1 Word1";
		Returnval_=L"(NULL)";
		Information_=L"append Word1 to Entry1(equals to 'adddict')";

		return(true);
	}
};
//---------------------------------------------------------------------------
class KIS_pushstr : public KIS_adddictstr {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"pushstr";
		Format_=L"pushstr Entry1 Str1";
		Returnval_=L"(NULL)";
		Information_=L"append string Str1 to Entry1(equals to 'adddictstr')";

		return(true);
	}
};
//---------------------------------------------------------------------------
class KIS_unshift : public TKisFunction_base {
protected:

	// インタープリタの実体
	wstring Function_(const vector<wstring>& args,bool flag);

public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"unshift";
		Format_=L"unshift Entry1 Word1";
		Returnval_=L"(NULL)";
		Information_=L"insert Word1 to the top of Entry1";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args)
	{
		return(Function_(args,false));
	}
};
//---------------------------------------------------------------------------
class KIS_unshiftstr : public KIS_unshift {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"unshiftstr";
		Format_=L"unshiftstr Entry1 Str1";
		Returnval_=L"(NULL)";
		Information_=L"insert string Str1 to the top of Entry1";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args)
	{
		return(Function_(args,true));
	}
};
//---------------------------------------------------------------------------
class KIS_pop : public TKisFunction_base {
protected:

	// インタープリタの実体
	wstring Function_(const vector<wstring>& args,bool flag,bool flag2);

public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"pop";
		Format_=L"pop Entry1";
		Returnval_=L"word";
		Information_=L"remove last word of Entry1 and returns it";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args)
	{
		return(Function_(args,false,false));
	}
};
//---------------------------------------------------------------------------
class KIS_shift : public KIS_pop {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"shift";
		Format_=L"shift Entry1";
		Returnval_=L"word";
		Information_=L"remove first word of Entry1 and returns it";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args)
	{
		return(Function_(args,true,false));
	}
};
//---------------------------------------------------------------------------
class KIS_popcode : public KIS_pop {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"popcode";
		Format_=L"popcode Entry1";
		Returnval_=L"word";
		Information_=L"remove last word of Entry1 and return it";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args)
	{
		return(Function_(args,false,true));
	}
};
//---------------------------------------------------------------------------
class KIS_shiftcode : public KIS_pop {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"shiftcode";
		Format_=L"shiftcode Entry1";
		Returnval_=L"word";
		Information_=L"remove first word of Entry1 and return it";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args)
	{
		return(Function_(args,true,true));
	}
};
//---------------------------------------------------------------------------
class KIS_getrandom : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"getrandom";
		Format_=L"getrandom Entry1 [Word1]";
		Returnval_=L"${Entry1} or Word1";
		Information_=L"equivalent to \"${Entry1}\"";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
class KIS_entry : public KIS_getrandom {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"entry";
		Format_=L"entry Entry1 [Word1]";
		Returnval_=L"${Entry1} or Word1";
		Information_=L"equivalent to \"${Entry1}\"";

		return(true);
	}
};
//---------------------------------------------------------------------------
class KIS_get : public TKisFunction_base {
protected:

	// インタープリタの実体
	wstring Function_(const vector<wstring>& args,bool flag);

public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"get";
		Format_=L"get Index1";
		Returnval_=L"word";
		Information_=L"enumerate and join all words in Entry1 from start to end";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args)
	{
		return(Function_(args,false));
	}
};
//---------------------------------------------------------------------------
class KIS_getcode : public KIS_get {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"getcode";
		Format_=L"get Index1";
		Returnval_=L"word";
		Information_=L"enumerate and join all words in Entry1 from start to end";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args)
	{
		return(Function_(args,true));
	}
};
//---------------------------------------------------------------------------
class KIS_clear : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"clear";
		Format_=L"clear Index1";
		Returnval_=L"(NULL)";
		Information_=L"clear all content of Index1";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
class KIS_insert : public TKisFunction_base {
protected:
	// インタープリタの実体
	wstring Function_(const vector<wstring>& args,bool flag);

public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"insert";
		Format_=L"insert Index1 Word1";
		Returnval_=L"(NULL)";
		Information_=L"Insert Word1 into before Index1";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args)
	{
		return(Function_(args,false));
	}
};
//---------------------------------------------------------------------------
class KIS_insertstr : public KIS_insert {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"insertstr";
		Format_=L"insertstr Index1 Str1";
		Returnval_=L"(NULL)";
		Information_=L"Insert Str1 into before Index1";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args)
	{
		return(Function_(args,true));
	}
};
//---------------------------------------------------------------------------
class KIS_writeprotect : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"writeprotect";
		Format_=L"writeprotect Entry";
		Returnval_=L"(NULL)";
		Information_=L"Let Entry be protected. Contents of Entry can't change anymore";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
class KIS_wordcount : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"wordcount";
		Format_=L"wordcount";
		Returnval_=L"number of words in dictionary";
		Information_=L"Count number of all words in dictionary";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
class KIS_entrycount : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"entrycount";
		Format_=L"entrycount";
		Returnval_=L"number of entries in dictionary";
		Information_=L"Count number of all entries in dictionary";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
class KIS_find : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"find";
		Format_=L"find Entry Word [ StartIndex ]";
		Returnval_=L"Index(0+) if found, or -1";
		Information_=L"Search upword for specified Word in Entry, from StartIndex(default:0).";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
class KIS_rfind : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"rfind";
		Format_=L"rfind Entry Word [ StartIndex ] ";
		Returnval_=L"Index(0+) if found, or -1";
		Information_=L"Search downword for specified Word in Entry, from StartIndex(default:size of Entry).";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
class KIS_cleartree : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"cleartree";
		Format_=L"cleartree Entry1";
		Returnval_=L"(NULL)";
		Information_=L"Clear all entries whose name starts with \"Entry1\"";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args);
};
//---------------------------------------------------------------------------
class KIS_listsub : public TKisFunction_base {
protected:
	// subflag : trueの時, ent_list動作。 falseの時allentry動作。
	void _Function(const vector<wstring>& args, bool subflag);
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"listsub";
		Format_=L"listsub Entry1 Entry2";
		Returnval_=L"(NULL)";
		Information_=L"List all sub entries of Entry2 to Entry1";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args){
		_Function(args, true);
		return (L"");
	}
};
//---------------------------------------------------------------------------
class KIS_listtree : public KIS_listsub {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"listtree";
		Format_=L"listtree Entry1 Entry2";
		Returnval_=L"(NULL)";
		Information_=L"List all entries whose name starts with \"Entry2\" to Entry1";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args){
		_Function(args, false);
		return (L"");
	}
};
//---------------------------------------------------------------------------
class KIS_copy : public TKisFunction_base {
protected:
	// rmflag : trueの時, move動作
	void _Function(const vector<wstring>& args, bool rmflag);
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"copy";
		Format_=L"copy Entry1 Entry2";
		Returnval_=L"(NULL)";
		Information_=L"copy words of Entry1 to Entry2";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args){
		_Function(args, false);
		return (L"");
	}
};
//---------------------------------------------------------------------------
class KIS_copytree : public TKisFunction_base {
protected:
	// rmflag : trueの時, move動作
	void _Function(const vector<wstring>& args, bool rmflag);
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"copytree";
		Format_=L"copytree Entry1 Entry2";
		Returnval_=L"(NULL)";
		Information_=L"copy Entry1 to Entry2 recursively";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args){
		_Function(args, false);
		return (L"");
	}
};
//---------------------------------------------------------------------------
class KIS_move : public KIS_copy {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"move";
		Format_=L"move Entry1 Entry2";
		Returnval_=L"(NULL)";
		Information_=L"move words of Entry1 to Entry2";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args){
		_Function(args, true);
		return (L"");
	}
};
//---------------------------------------------------------------------------
class KIS_movetree : public KIS_copytree {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"movetree";
		Format_=L"movetree Entry1 Entry2";
		Returnval_=L"(NULL)";
		Information_=L"move Entry1 to Entry2 recursively";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args){
		_Function(args, true);
		return (L"");
	}
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
// 補助
// encode_entryname/eval/size
//
// 書き込み
// +--------------------------------------------+
// |              | 文字列       | KIS          |
// |--------------+--------------+--------------|
// | 任意位置     | setstr       | set          |
// | 末尾追加     | adddictstr   | adddict      |
// | 先頭追加     | unshiftstr   | unshift      |
// +--------------------------------------------+
//
// 読み出し
// +--------------------------------------------+
// |              | eval         | 非eval       |
// |--------------+--------------+--------------|
// | ランダム選択 | getrandom    |              |
// | 指定         | get          | getcode      |
// | 先頭         | shift        | shiftcode    |
// | 末尾         | pop          | popcode      |
// +--------------------------------------------+
//
// 消去
// clear
//---------------------------------------------------------------------------
