#ifndef __JNIMain_h__
#define __JNIMain_h__

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

JNIEXPORT jint  JNICALL JNI_OnLoad(JavaVM* jvm, void* reserved);
JNIEXPORT void  JNICALL JNI_OnUnload(JavaVM *jvm, void *reserved);

JNIEXPORT jlong JNICALL Java_com_dragonflow_FileTransfer_init(JNIEnv *env, jclass clazz, jobject delegateObj);
JNIEXPORT void  JNICALL Java_com_dragonflow_FileTransfer_destroy(JNIEnv *env, jclass clazz, jint ptr);
JNIEXPORT jint  JNICALL Java_com_dragonflow_FileTransfer_startListen(JNIEnv* env,  jclass clazz, jlong ptr, jint portCtrl, jint portFile);
JNIEXPORT jint  JNICALL Java_com_dragonflow_FileTransfer_sendMessage(JNIEnv* env, jclass clazz, jlong ptr, jstring host,jstring message, jstring hostname);
JNIEXPORT jint  JNICALL Java_com_dragonflow_FileTransfer_sendFiles(JNIEnv* env, jclass clazz, jlong ptr, jstring host, jobjectArray filelist, jstring owndevice, jstring owntype, jstring recdevice, jstring rectype, jstring sendtype);
JNIEXPORT jint  JNICALL Java_com_dragonflow_FileTransfer_replyAccept(JNIEnv *env, jclass clazz, jlong ptr, jstring szReply, jint sock);
JNIEXPORT void  JNICALL Java_com_dragonflow_FileTransfer_heart(JNIEnv *env, jclass clazz, jlong ptr, jobjectArray iplist);
JNIEXPORT jint  JNICALL Java_com_dragonflow_FileTransfer_stopTransfer(JNIEnv *env, jclass clazz, jlong ptr, jint nType, jint sock);
JNIEXPORT jint  JNICALL Java_com_dragonflow_FileTransfer_stopListen(JNIEnv *env, jclass clazz, jlong ptr);
JNIEXPORT jstring  JNICALL Java_com_dragonflow_FileTransfer_Test(JNIEnv *env, jclass clazz, jlong ptr);

#ifdef __cplusplus
}
#endif // __cplusplus


class CG
{
public:
	static bool init(JavaVM *jvm);
	static void term();

	static JavaVM *javaVM;
	static jclass c_String;
	static jclass c_FileTransfer;
	
	static jmethodID m_CtorID;
	static jmethodID m_OnAccept;
	static jmethodID m_OnAcceptonFinish;
	static jmethodID m_OnFinished;
	static jmethodID m_OnTransfer;
	static jmethodID m_OnRecvMessage;
	static jmethodID m_OnHeartAccept;
};

#endif // __JNIMain_h__
