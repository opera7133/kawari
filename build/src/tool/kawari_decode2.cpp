//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// 復号ツール
//
//      Programed by NAKAUE.T (Meister)
//
//  2001.03.19  Phase 7.9.0   新暗号!KAWA0001形式
//  2026.10.18  Phase 9.0.0   wchar_t化 (ファイルはバイト列のまま処理。UTF-8はBOMで区別)
//
//---------------------------------------------------------------------------
#include "libkawari/kawari_crypt.h"
#include "misc/charset.h"
#include "misc/wfile.h"
#include "tool/wargs.h"
#include <iostream>
#include <string>
#include <vector>
using namespace std;
//---------------------------------------------------------------------------
#ifdef KAWARI_MS
#	define OUTPUT_EOL	"\r\n"
#else
#	define OUTPUT_EOL	"\n"
#endif
//---------------------------------------------------------------------------

// 文字列の後ろの改行を取り除く
string StringTrim2(string str)
{
	string::size_type lineend=str.find_last_not_of("\r\n");
	return(str.substr(0,lineend+1));
}
//---------------------------------------------------------------------------
// バイト列を行に分割する (改行コードは含まない)
static void SplitLines(const string &data, vector<string> &lines)
{
	string::size_type pos=0;
	while(pos<data.size()){
		string::size_type eol=data.find('\n', pos);
		if(eol==string::npos){
			lines.push_back(data.substr(pos));
			break;
		}
		lines.push_back(data.substr(pos, eol-pos));
		pos=eol+1;
	}
}
//---------------------------------------------------------------------------
// 入力がUTF-8かどうか (BOMあり、またはUTF-8として正当で非ASCIIを含む)
static bool IsUTF8Text(const string &data, bool hasbom)
{
	if(hasbom) return true;
	if(DetectCharset(data)!=KC_UTF8) return false;
	for(string::size_type i=0; i<data.size(); i++)
		if(((unsigned char)data[i])>=0x80) return true;
	return false;
}
//---------------------------------------------------------------------------
int main(int argc,char *argv[])
{
	vector<wstring> args=GetWideArgs(argc, argv);
	wstring key;
	{
		string keybytes;
		cout << "Input Keyword : ";
		cin >> keybytes;
		key=ACPtoW(keybytes);
	}

	for(int i=1;i<(int)args.size();i++) {
		wstring infile=args[i];
		wstring::size_type pos=infile.find_last_of(L".");
		wstring outfile=infile.substr(0,pos)+L".txt";

		string data;
		if(!ReadWholeFile(infile, data)) continue;

		bool hasbom=HasUTF8BOM(data);
		if(hasbom) data.erase(0,3);
		vector<string> lines;
		SplitLines(data, lines);

		string out;
		for(vector<string>::size_type n=0;n<lines.size();n++) {
			string buff=StringTrim2(lines[n]);
			if((buff.size()>0)&&(buff[0]=='!')) {
				if(CheckCrypt(buff)) {
					buff=DecryptString2(buff,key);
				}
			}
			out+=buff+OUTPUT_EOL;
		}
		// 入力にBOMがあれば出力にも付ける
		if(hasbom) out="\xEF\xBB\xBF"+out;
		WriteWholeFile(outfile, out);
	}

	return(0);
}
