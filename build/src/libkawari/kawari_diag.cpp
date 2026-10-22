//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// 診断 (定義位置の記録と診断コード)
//
//  2026.10.22  Phase 9.2.0   作成
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "libkawari/kawari_diag.h"
#include "libkawari/kawari_log.h"
//---------------------------------------------------------------------------
using namespace std;
//---------------------------------------------------------------------------
TSourceMap::TSourceMap(void)
{
	// 添字 0 は「不明」
	files.push_back(L"");
}
//---------------------------------------------------------------------------
unsigned int TSourceMap::FileIndex(const wstring &name)
{
	map<wstring, unsigned int>::const_iterator it=fileindex.find(name);
	if (it!=fileindex.end()) return it->second;
	unsigned int idx=files.size();
	files.push_back(name);
	fileindex[name]=idx;
	return idx;
}
//---------------------------------------------------------------------------
const wstring &TSourceMap::FileName(unsigned int index) const
{
	return (index<files.size()) ? files[index] : files[0];
}
//---------------------------------------------------------------------------
void TSourceMap::RecordWord(unsigned int entry, unsigned int word, unsigned int file, unsigned int line)
{
	if ((!word)||(!file)) return;

	Rec r;
	r.entry=entry;
	r.file=file;
	r.line=line;

	if (word>=primary.size()) primary.resize(word+1);
	if (primary[word].file==0) {
		primary[word]=r;
	} else if (primary[word].entry!=entry) {
		pair<multimap<unsigned int, Rec>::const_iterator, multimap<unsigned int, Rec>::const_iterator> range=extra.equal_range(word);
		bool found=false;
		for (multimap<unsigned int, Rec>::const_iterator it=range.first; it!=range.second; ++it)
			if (it->second.entry==entry) { found=true; break; }
		if (!found) extra.insert(make_pair(word, r));
	}

	if (entry) {
		if (entry>=entryfirst.size()) entryfirst.resize(entry+1);
		if (entryfirst[entry].file==0) entryfirst[entry]=r;
	}
}
//---------------------------------------------------------------------------
void TSourceMap::EraseWord(unsigned int word)
{
	if (word<primary.size()) primary[word]=Rec();
	if (!extra.empty()) extra.erase(word);
}
//---------------------------------------------------------------------------
bool TSourceMap::FindWord(unsigned int entry, unsigned int word, Loc &loc) const
{
	if ((!word)||(word>=primary.size())||(primary[word].file==0)) return false;

	const Rec *p=NULL;
	if ((!entry)||(primary[word].entry==entry)) {
		p=&primary[word];
	} else {
		pair<multimap<unsigned int, Rec>::const_iterator, multimap<unsigned int, Rec>::const_iterator> range=extra.equal_range(word);
		for (multimap<unsigned int, Rec>::const_iterator it=range.first; it!=range.second; ++it)
			if (it->second.entry==entry) { p=&it->second; break; }
	}
	if (!p) return false;

	loc.file=p->file;
	loc.line=p->line;
	return true;
}
//---------------------------------------------------------------------------
bool TSourceMap::FindEntry(unsigned int entry, Loc &loc) const
{
	if ((!entry)||(entry>=entryfirst.size())||(entryfirst[entry].file==0)) return false;
	loc.file=entryfirst[entry].file;
	loc.line=entryfirst[entry].line;
	return true;
}
//---------------------------------------------------------------------------
namespace kawari {
namespace diag {
//---------------------------------------------------------------------------
unsigned int NumberOf(kawari::resource::ID id)
{
	using namespace kawari::resource;
	using namespace kawari_log;
	switch (id) {
	case ERR_LEXER_EOL_IN_QUOTED_STRING:			return E_QUOTE_EOL;
	case ERR_LEXER_EOF_IN_QUOTED_STRING:			return E_QUOTE_EOF;
	case ERR_COMPILER_UNKNOWN_MODE:					return E_UNKNOWN_MODE;
	case ERR_COMPILER_NO_ENTRYNAMES:				return E_NO_ENTRYNAMES;
	case ERR_COMPILER_MLENTRYDEF_NOT_CLOSED:		return E_MLENTRYDEF_NOT_CLOSED;
	case ERR_COMPILER_INVALID_ENTRYDEF:				return E_INVALID_ENTRYDEF;
	case ERR_COMPILER_ILLCHAR_IN_SCRIPT:			return E_ILLCHAR_IN_SCRIPT;
	case ERR_COMPILER_ENTRYID_NOT_FOUND:			return E_ENTRYID_NOT_FOUND;
	case ERR_COMPILER_FIRST_STATEMENT_NOT_FOUND:	return E_FIRST_STATEMENT_NOT_FOUND;
	case ERR_COMPILER_STATEMENTLIST_SEPARATOR_NOT_FOUND:	return E_STATEMENTLIST_SEPARATOR_NOT_FOUND;
	case ERR_COMPILER_INTERNAL_SUBST:				return E_INTERNAL_SUBST;
	case ERR_COMPILER_INTERNAL_INLINE_SCRIPT:		return E_INTERNAL_INLINE_SCRIPT;
	case ERR_COMPILER_SCRIPT_SUBST_NOT_CLOSED:		return E_SCRIPT_SUBST_NOT_CLOSED;
	case ERR_COMPILER_INTERNAL_BLOCK:				return E_INTERNAL_BLOCK;
	case ERR_COMPILER_BLOCK_NOT_CLOSED:				return E_BLOCK_NOT_CLOSED;
	case ERR_COMPILER_INTERNAL_INDEX:				return E_INTERNAL_INDEX;
	case ERR_COMPILER_INDEX_BRACKET_NOT_FOUND:		return E_INDEX_BRACKET_NOT_FOUND;
	case ERR_COMPILER_INDEX_EXPR_NOT_FOUND:			return E_INDEX_EXPR_NOT_FOUND;
	case ERR_COMPILER_INTERNAL_EXPR:				return E_INTERNAL_EXPR;
	case ERR_COMPILER_EXPR_NOT_CLOSED:				return E_EXPR_NOT_CLOSED;
	case ERR_COMPILER_PARSE_ERROR_AFTER:			return E_PARSE_ERROR_AFTER;
	case ERR_COMPILER_EXPR_BLOCK_NOT_CLOSED:		return E_EXPR_BLOCK_NOT_CLOSED;
	case ERR_COMPILER_ILLCHAR_IN_EXPRESSION:		return E_ILLCHAR_IN_EXPRESSION;
	case ERR_COMPILER_INTERNAL_ENTRYCALL:			return E_INTERNAL_ENTRYCALL;
	case ERR_COMPILER_ENTRYCALL_NOT_CLOSED:			return E_ENTRYCALL_NOT_CLOSED;
	case ERR_COMPILER_SETEXPR_BLOCK_NOT_CLOSED:		return E_SETEXPR_BLOCK_NOT_CLOSED;
	case ERR_CODEEXPR_DIVIDED_BY_ZERO:				return E_DIVIDED_BY_ZERO;
	case ERR_VM_UNDEFINED_FUNCTION1:
	case ERR_VM_UNDEFINED_FUNCTION2:				return E_UNDEFINED_FUNCTION;
	case ERR_ENGINE_UNKNOWN_MODE:					return E_ENGINE_UNKNOWN_MODE;
	case ERR_NS_ASSERT_PROTECTED_ENTRY1:
	case ERR_NS_ASSERT_PROTECTED_ENTRY2:			return E_PROTECTED_ENTRY;
	case ERR_DICT_CAN_NOT_DELETE_WORD1:
	case ERR_DICT_CAN_NOT_DELETE_WORD2:				return E_CAN_NOT_DELETE_WORD;
	case ERR_KIS_DICT_INVALID_INDEX:				return E_KIS_INVALID_INDEX;
	case ERR_KIS_DICT_ILLEGAL_COPY_OPERATION:		return E_KIS_ILLEGAL_COPY_OPERATION;
	case ERR_KIS_FILE_SAVE_FAILED:					return E_KIS_FILE_SAVE_FAILED;
	case ERR_KIS_FILE_LOAD_FAILED:					return E_KIS_FILE_LOAD_FAILED;
	case ERR_KIS_SAORI_CALL_FAILED1:
	case ERR_KIS_SAORI_CALL_FAILED2:				return E_KIS_SAORI_CALL_FAILED;
	case WARN_COMPILER_BLANK_DEFINITION:			return W_BLANK_DEFINITION;
	case WARN_COMPILER_ENTRYIDLIST_ENDS_WITH_COMMA:	return W_ENTRYIDLIST_ENDS_WITH_COMMA;
	case WARN_NS_READ_EMPTY_ENTRY1:
	case WARN_NS_READ_EMPTY_ENTRY2:					return W_READ_EMPTY_ENTRY;
	case ERR_CODE_BREAK_OUTOF_LOOP:					return E_BREAK_OUTOF_LOOP;
	case ERR_CODE_CONTINUE_OUTOF_LOOP:				return E_CONTINUE_OUTOF_LOOP;
	default:										return 0;
	}
}
//---------------------------------------------------------------------------
}
}
//---------------------------------------------------------------------------
