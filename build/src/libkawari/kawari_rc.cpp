//--------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// リソース
//
//      Programed by Suikyo.
//
//  2002.06.16  Phase 8.0.0   i10nバージョン作成
//  2026.10.18  Phase 9.0.0   wchar_t化 (SJIS表は実行時にwide化)
//
//--------------------------------------------------------------------------
#include "config.h"
//--------------------------------------------------------------------------
#include "libkawari/kawari_rc.h"
#include "libkawari/kawari_rc_sjis_encoded.h"
#include "misc/charset.h"
using namespace kawari::resource;
//--------------------------------------------------------------------------
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <cctype>
using namespace std;
//--------------------------------------------------------------------------
namespace {
	string TResourceASCII[ID__END] = {
		"",
		"unexpected EOL in quoted string.",
		"unexpected EOF in quoted string.",

		"unknown mode found :",
		"no entry names.",
		"')' not found. runaway entry definition?",
		"':' not found.",
		"illegal character found in inline script.",
		"could not find EntryId at start of entry definition.",
		"could not find first statement.",
		"could not find statement list separator ','.",
		"INTERNAL ERROR : '$' not found, when parsing subst.",
		"INTERNAL ERROR : '(' not found, when parsing inline script subst.",
		"')' not found. runaway script?",
		"INTERNAL ERROR : '(' not found, when parsing block.",
		"')' not found. runaway block?",
		"illegal EntryId found at entry index subst.",
		"'[' expected.",
		"an expression exptected in index of entry index subst.",
		"INTERNAL ERROR : '[' not found, when parsing expression subst.",
		"']' not found. runaway entry index?",
		"parse error after ",
		"')' not found. runaway block?",
		"illegal character found in expression.",
		"INTERNAL ERROR : '{' not found, when parsing entry call subst.",
		"'}' not found. runaway entry call?",
		"')' not found. runaway block?",

		"divide by zero.",
		"undefined function \"",
		"\" called.",
		"can not change to unknown mode.",
		"access violation. \"",
		"\" is write protected.",
		"INTERNAL ERROR : can not delete word. WORDID(",
		") not found.",
		" : invalid index",
		" : can not copy parent to child",
		" : save failed",
		" : load failed",
		"calling SAORI (",
		") failed.",

		"blank definition.",
		"EntryIdList can not end with ','",
		"read access to empty entry '",
		"'",

		"delete word(",
		") : ",

		"'break' is used out of loop",
		"'continue' is used out of loop",

//		""
	};

}
//--------------------------------------------------------------------------
namespace kawari{
namespace resource{
//--------------------------------------------------------------------------
TResourceManager::TResourceManager(void){
	initialized = false;
	use_japanese = false;
}
//--------------------------------------------------------------------------
// 静的初期化順序の問題を避けるため、初回利用時にwstring化する
void TResourceManager::Initialize(void)
{
	ascii.resize(ID__END);
	japanese.resize(ID__END);
	for (int i=0; i<ID__END; i++){
		ascii[i] = AtoW(TResourceASCII[i]);
		japanese[i] = SJIStoW(TResourceSJIS[i]);
	}
	initialized = true;
}
//--------------------------------------------------------------------------
void TResourceManager::SwitchTo(const wstring &charset)
{
	TKawariCharset cs = CharsetFromName(charset);
	use_japanese = ((cs == KC_SJIS) || (cs == KC_UTF8));
}
//--------------------------------------------------------------------------
TResourceManager ResourceManager;
//--------------------------------------------------------------------------
}
}
//--------------------------------------------------------------------------
