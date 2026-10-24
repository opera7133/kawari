//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// SAORI インターフェース(Java)
//
//      Programed by Suikyo.
//
//  2003.02.25  Phase 8.1.0   作成
//  2026.10.18  Phase 9.0.0   wchar_t化 (Javaとはパスを従来どおりSJISで受け渡す)
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "saori/saori_java.h"
#include "libkawari/kawari_log.h"
#include "misc/misc.h"
//---------------------------------------------------------------------------
#include <string>
#include <map>
// DEBUG
#include <iostream>
//---------------------------------------------------------------------------
using namespace std;
using namespace saori;
using namespace kawari_log;
//---------------------------------------------------------------------------
namespace{
	inline jbyteArray string2jba(JNIEnv *env, const string &s){
		int len=s.length();
		jbyteArray ret=env->NewByteArray(len);
		char *str=(char *)env->GetByteArrayElements(ret, NULL);
		s.copy(str, len);
		env->ReleaseByteArrayElements(ret, (jbyte *)str, 0);
		return ret;
	}
	inline string jba2string(JNIEnv *env, jbyteArray ba){
		int len=(int)env->GetArrayLength(ba);
		char *str=(char *)env->GetByteArrayElements(ba, NULL);
		string ret(str, len);
		env->ReleaseByteArrayElements(ba, (jbyte *)str, JNI_ABORT);
		return ret;
	}
	jint setup(TKawariLogger &lgr, JavaVM **vm, JNIEnv **env){
	    cout << "[SAORI Java] setup" << endl;
		jsize nVMs;
		if (JNI_OK!=JNI_GetCreatedJavaVMs(vm, (jsize)1, &nVMs)){
		    cout << "[SAORI Java] can not find Java VM" << endl;
			lgr.Diag(LOG_ERROR, kawari_log::E_SAORI) << L"[SAORI Java] JNI_GetCreatedJavaVMs failed.";
			return JNI_ERR;
		}
	    cout << "[SAORI Java] " << (int)nVMs << "Java VM(s) found." << endl;
		if (JNI_OK!=(*vm)->GetEnv((void **)env, JNI_VERSION_1_2)){
		    cout << "[SAORI Java] current thread is not attached to VM." << endl;
			if (JNI_OK!=(*vm)->AttachCurrentThread((void **)env, NULL)){
				lgr.Diag(LOG_ERROR, kawari_log::E_SAORI) << L"[SAORI Java] AttachCurrentThread failed";
				return JNI_ERR;
			}
		}
	    cout << "[SAORI Java] thread attach success." << endl;
		return JNI_OK;
	}
}
//---------------------------------------------------------------------------
namespace saori{
//---------------------------------------------------------------------------
// コンストラクタ
TModuleFactoryJava::TModuleFactoryJava(TKawariLogger &lgr) : IModuleFactory(lgr) {
	JavaVM *vm; JNIEnv *env;
	if (JNI_OK!=setup(GetLogger(), &vm, &env)) return;

	// java.lang.System.getProperty("jp.nanika.saoriinterface");
	jclass cls_System=env->FindClass("java/lang/System");
	jmethodID mid_getProperty=env->GetStaticMethodID(
		cls_System, "getProperty", "(Ljava/lang/String;)Ljava/lang/String;");

	jstring jpropKey=env->NewStringUTF("jp.nanika.saoriinterface");
	jstring val=(jstring)env->CallStaticObjectMethod(
		cls_System, mid_getProperty, jpropKey);
	if ((val==NULL)||(0==env->GetStringUTFLength(val))){
		GetLogger().Diag(LOG_ERROR, kawari_log::E_SAORI) << L"[SAORI Java] No factory class is specified.";
		return;
	}
	const char *clazznamestr=env->GetStringUTFChars(val, NULL);

	// FindClass(clazzname);
	jclass lref_cls=env->FindClass(clazznamestr);
	if (lref_cls==NULL){
		GetLogger().Diag(LOG_ERROR, kawari_log::E_SAORI) << L"[SAORI Java] Can not find factory class \"" << UTF8toW(clazznamestr) << L"\"";
		env->ReleaseStringUTFChars(val, clazznamestr);
		return;
	}
	env->ReleaseStringUTFChars(val, clazznamestr);
	cls_SaoriFactory=(jclass)env->NewGlobalRef(lref_cls);

	jmethodID mid_init=env->GetMethodID(
		cls_SaoriFactory, "<init>", "()V");
	mid_createModule=env->GetMethodID(
		cls_SaoriFactory, "createModule", "(Ljava/lang/String;)Ljava/lang/Object;");

	if ((mid_init==NULL)||(mid_createModule==NULL)){
		GetLogger().Diag(LOG_ERROR, kawari_log::E_SAORI) << L"[SAORI Java] factory class has illegal structure.";
		env->DeleteGlobalRef(cls_SaoriFactory);
		cls_SaoriFactory=NULL;
		return;
	}

	// clazz.newInstance();
	jobject lref_obj=env->NewObject(cls_SaoriFactory, mid_init);
	if (lref_obj==NULL){
		GetLogger().Diag(LOG_ERROR, kawari_log::E_SAORI) << L"[SAORI Java] Can not create factory object.";
		env->DeleteGlobalRef(cls_SaoriFactory);
		cls_SaoriFactory=NULL;
		return;
	}
	obj_SaoriFactory=env->NewGlobalRef(lref_obj);
}
//---------------------------------------------------------------------------
// デストラクタ
TModuleFactoryJava::~TModuleFactoryJava(){
	JavaVM *vm; JNIEnv *env;
	if (JNI_OK!=setup(GetLogger(), &vm, &env)) return;

	if (obj_SaoriFactory) env->DeleteGlobalRef(obj_SaoriFactory);
	if (cls_SaoriFactory) env->DeleteGlobalRef(cls_SaoriFactory);
}
//---------------------------------------------------------------------------
TModule *TModuleFactoryJava::CreateModule(const wstring &path){
	if (obj_SaoriFactory==NULL){
		GetLogger().GetStream(LOG_INFO) << L"[SAORI Java] Factory not found." << endl;
		return NULL;
	}
	GetLogger().GetStream(LOG_INFO) << L"[SAORI Java] CreateModule(" << path << L")" << endl;

	JavaVM *vm; JNIEnv *env;
	if (JNI_OK!=setup(GetLogger(), &vm, &env)) return NULL;

	wstring fn=CanonicalPath(path);
	jstring jfn=env->NewStringUTF(WtoUTF8(fn).c_str());
	jobject jmodule=env->CallObjectMethod(
		obj_SaoriFactory, mid_createModule, jfn);
	if (jmodule==NULL){
		GetLogger().Diag(LOG_ERROR, kawari_log::E_SAORI) << L"[SAORI Java] Create module failed.";
		return NULL;
	}
	jobject gref_jmodule=env->NewGlobalRef(jmodule);
	TModule *ret=new TModuleJava(*this, fn, (SAORI_HANDLE)gref_jmodule);
	if (ret->Initialize()){
		return ret;
	}else{
		GetLogger().Diag(LOG_ERROR, kawari_log::E_SAORI) << L"[SAORI Java] Initialize module failed.";
		ret->Unload();
		DeleteModule(ret);
		return NULL;
	}
}
//---------------------------------------------------------------------------
// モジュールの完全破棄
void TModuleFactoryJava::DeleteModule(TModule *module){
	// finalizeはしない
	if (module){
		GetLogger().GetStream(LOG_INFO) << L"[SAORI Java] Dispose module." << endl;
		delete module;
	}
}
//---------------------------------------------------------------------------
// デストラクタ
TModuleJava::~TModuleJava(){
	JavaVM *vm; JNIEnv *env;
	if (JNI_OK!=setup(GetFactory().GetLogger(), &vm, &env)) return;

	if (obj_saori) env->DeleteGlobalRef(obj_saori);
	if (cls_saori) env->DeleteGlobalRef(cls_saori);
}
//---------------------------------------------------------------------------
// 初期化
bool TModuleJava::Initialize(void){
	JavaVM *vm; JNIEnv *env;
	if (JNI_OK!=setup(GetFactory().GetLogger(), &vm, &env)) return false;

	obj_saori=(jobject)handle;
	jclass lref_cls=env->GetObjectClass(obj_saori);
	cls_saori=(jclass)env->NewGlobalRef(lref_cls);

	mid_load=env->GetMethodID(cls_saori, "load", "([B)Z");
	mid_unload=env->GetMethodID(cls_saori, "unload", "()Z");
	mid_request=env->GetMethodID(cls_saori, "request", "([B)[B");

	if (mid_request==NULL){
		GetFactory().GetLogger().Diag(LOG_ERROR, kawari_log::E_SAORI) << L"[SAORI Java] importing 'request' from ("+path+L") failed.";
		return false;
	}
	return true;
}
//---------------------------------------------------------------------------
// SAORI/1.0 Load
bool TModuleJava::Load(void){
	if (!mid_load) return TRUE;

	JavaVM *vm; JNIEnv *env;
	if (JNI_OK!=setup(GetFactory().GetLogger(), &vm, &env)) return false;

	jobject obj_saori=(jobject)handle;

	wstring basepath;
	wstring::size_type pos=path.find_last_of(FILE_SEPARATOR);
	if (pos==wstring::npos){
		basepath=path+FILE_SEPARATOR;
	}else{
		basepath=path.substr(0, pos+1);
	}

	GetFactory().GetLogger().GetStream(LOG_INFO) << L"[SAORI Java] load(" << basepath << L")." << endl;
	return (env->CallBooleanMethod(obj_saori, mid_load, string2jba(env, WtoSJIS(basepath)))==JNI_TRUE);
}
//---------------------------------------------------------------------------
// SAORI/1.0 Unload
bool TModuleJava::Unload(void){
	if (!mid_unload) return true;

	JavaVM *vm; JNIEnv *env;
	if (JNI_OK!=setup(GetFactory().GetLogger(), &vm, &env)) return false;

	jobject obj_saori=(jobject)handle;

	GetFactory().GetLogger().GetStream(LOG_INFO) << L"[SAORI Java] unload()" << endl;
	return (env->CallBooleanMethod(obj_saori, mid_unload)==JNI_TRUE);
}
//---------------------------------------------------------------------------
// SAORI/1.0 Request
string TModuleJava::Request(const string &req){
	if (!mid_request) return ("");

	JavaVM *vm; JNIEnv *env;
	if (JNI_OK!=setup(GetFactory().GetLogger(), &vm, &env)) return ("");

	jobject obj_saori=(jobject)handle;

	return jba2string(env, (jbyteArray)env->CallObjectMethod(
		obj_saori, mid_request, string2jba(env, req)));
}
//---------------------------------------------------------------------------
}// namespace saori
//---------------------------------------------------------------------------
