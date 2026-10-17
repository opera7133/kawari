//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// KawariInlineScript基本クラス
//
//      Programed by NAKAUE.T (Meister)
//
//  2001.05.14  Phase 0.50    インラインスクリプト整理
//  2001.05.24  Phase 5.1     インタープリタ・コンパイラ化
//  2002.03.17  Phase 7.9.0   TKisEngineからKIUに合わせてTKawariVMに名称変更
//  2002.03.19  Phase 8.0     今度はTKawariEngineに変更
//  2026.10.18  Phase 9.0.0   wchar_t化 (関数情報はconst wchar_t*、引数/戻り値はwstring)
//
//---------------------------------------------------------------------------
#ifndef KIS_BASE_H
#define KIS_BASE_H
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "libkawari/kawari_engine.h"
#include "libkawari/kawari_log.h"
//---------------------------------------------------------------------------
#include <cstdlib>
#include <string>
#include <vector>
using namespace std;
//---------------------------------------------------------------------------
// KIS関数の情報を表す構造体
struct TKisFunctionInfo {
	wstring name;			// 関数名
	wstring format;			// 文法
	wstring returnval;		// 戻り値
	wstring information;	// 解説
	TKisFunctionInfo (void) {}
	TKisFunctionInfo (const wchar_t *n, const wchar_t *f, const wchar_t *r, const wchar_t *i)
		: name(wstring(n)), format(wstring(f)),
		  returnval(wstring(r)), information(wstring(i)) {}
	TKisFunctionInfo &operator = (const TKisFunctionInfo &r){
		name=r.name;
		format=r.format;
		returnval=r.returnval;
		information=r.information;
		return (*this);
	}
};
//---------------------------------------------------------------------------
// インラインスクリプト関数の基本クラス
// (Th!rdのアイデアをリファイン)
class TKisFunction_base {
protected:

	// 関数情報
	const wchar_t *Name_;
	const wchar_t *Format_;
	const wchar_t *Returnval_;
	const wchar_t *Information_;

	// 華和梨エンジン
	class TKawariEngine *Engine;

	inline TKawariLogger &GetLogger(void) { return Engine->GetLogger(); }
public:

	TKisFunction_base(void)
	{
		Name_=L"none.";
		Format_=L"none.";
		Returnval_=L"none.";
		Information_=L"none.";
	}

	// ですとらくた
	virtual ~TKisFunction_base() { }

	// 関数情報取得
	const TKisFunctionInfo GetInformation (void) const {
		return TKisFunctionInfo(Name_, Format_, Returnval_, Information_);
	}

	const wchar_t* Name(void) const {return(Name_);}
	const wchar_t* Format(void) const {return(Format_);}
	const wchar_t* Returnval(void) const {return(Returnval_);}
	const wchar_t* Information(void) const {return(Information_);}

	// 華和梨エンジン設定
	void NotifyEngine(class TKawariEngine *engine)
	{
		Engine=engine;
	}

	// 初期化
	virtual bool Init(void)=0;

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args)=0;

	// 引数指定エラーのログ出力
	bool AssertArgument(const vector<wstring>& args, const unsigned int min){
		using namespace kawari_log;
		bool ret=true;
		if(args.size()<min){
			if(GetLogger().Check(LOG_WARNING))
				GetLogger().GetStream() << L"KIS[" << args[0] << L"] error : too few arguments." << endl;
			ret=false;
		}
		if ((!ret)&&(GetLogger().Check(LOG_INFO)))
			GetLogger().GetStream() << L"usage> " << Format() << endl;
		return ret;
	}

	// 引数指定エラーのログ出力
	bool AssertArgument(const vector<wstring>& args, const unsigned int min, const unsigned int max){
		using namespace kawari_log;
		bool ret=true;
		if(args.size()<min){
			if(GetLogger().Check(LOG_WARNING))
				GetLogger().GetStream() << L"KIS[" << args[0] << L"] error : too few arguments." << endl;
			ret=false;
		}else if(args.size()>max){
			if(GetLogger().Check(LOG_WARNING))
				GetLogger().GetStream() << L"KIS[" << args[0] << L"] error : too many arguments." << endl;
			ret=false;
		}
		if ((!ret)&&(GetLogger().Check(LOG_INFO)))
			GetLogger().GetStream() << L"usage> " << Format() << endl;
		return ret;
	}
};
//---------------------------------------------------------------------------
#if 0
class KIS_SampleFunction : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_=L"SampleFunction";
		Format_=L"SampleFunction [Arg1 ...]";
		Returnval_=L"string which join all argument";
		Information_=L"KawariInlineScript sample";

		return(true);
	}

	// インタープリタ
	virtual wstring Function(const vector<wstring>& args)
	{
		wstring ret;
		for(unsigned int i=0;i<args;i++) ret=ret+args[i]+L" ";
		return(ret);
	}
};
// これで登録
INLINE_SCRIPT_REGIST(KIS_SampleFunction);
#endif
//---------------------------------------------------------------------------
#endif

