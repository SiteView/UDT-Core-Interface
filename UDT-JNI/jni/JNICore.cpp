#include "JNICore.h"
#include "JNIMain.h"


JNICore::JNICore()
	: m_delegateObj(NULL)
{
	m_core = new CUdtCore(this);
}

JNICore::~JNICore()
{
	delete m_core;
	if (m_delegateObj)
	{
		VMGuard vmguard;
		if (JNIEnv *env = vmguard.env())
		{
			env->DeleteGlobalRef(m_delegateObj);
		}	
	}
}


CUdtCore *JNICore::core() const
{
	return m_core;
}

bool JNICore::init(JNIEnv *env, jobject delegateObj)
{
	m_delegateObj = env->NewGlobalRef(delegateObj);
	return true;
}

void JNICore::onAccept(const char* pstrIpAddr, const char* pstrFileName, int nFileCount, const char* pstrRecdevice, const char* pstrRectype, const char* pstrOwndevice, const char* pstrOwntype, const char* pstrSendType, int sock)
{
	VMGuard vmguard;
	if (JNIEnv *env = vmguard.env())
	{
		jstring jsAddr = env->NewStringUTF(pstrIpAddr);
		jstring jsFile = env->NewStringUTF(pstrFileName);
		jstring jsRecdevice = env->NewStringUTF(pstrRecdevice);
		jstring jsRectype = env->NewStringUTF(pstrRectype);
		jstring jsOwndevice = env->NewStringUTF(pstrOwndevice);
		jstring jsOwntype = env->NewStringUTF(pstrOwntype);
		jstring jsSndType = env->NewStringUTF(pstrSendType);
		env->CallVoidMethod(m_delegateObj, CG::m_OnAccept, jsAddr, jsFile, nFileCount, jsRecdevice, jsRectype, jsOwndevice, jsOwntype, jsSndType, sock);
	}
}

void JNICore::onAcceptonFinish(const char* pstrAddr, const char* pstrFileName, int Type, int sock)
{
	VMGuard vmguard;
	if (JNIEnv *env = vmguard.env())
	{
		jstring jsAddr = env->NewStringUTF(pstrAddr);
		jstring jsFile = env->NewStringUTF(pstrFileName);
		env->CallVoidMethod(m_delegateObj, CG::m_OnAcceptonFinish, jsAddr, jsFile, Type, sock);
	}
}

void JNICore::onFinished(const char * pstrMsg, int Type, int sock)
{
	VMGuard vmguard;
	if (JNIEnv *env = vmguard.env())
	{
		jstring jsText = env->NewStringUTF(pstrMsg);
		env->CallVoidMethod(m_delegateObj, CG::m_OnFinished, jsText, Type, sock);
	}
}


void JNICore::onTransfer(const int64_t nFileTotalSize, const int64_t nCurrent, const char* pstrFileName, int Type, int sock)
{
	VMGuard vmguard;
	if (JNIEnv *env = vmguard.env())
	{
		jstring jsText = env->NewStringUTF(pstrFileName);
		env->CallVoidMethod(m_delegateObj, CG::m_OnTransfer, nFileTotalSize, nCurrent, jsText, Type, sock);
	}
}

void JNICore::onRecvMessage(const char* pstrMsg, const char* pstrIpAddr, const char* pstrHostName)
{
	VMGuard vmguard;
	if (JNIEnv *env = vmguard.env())
	{
		jstring jsAddr = env->NewStringUTF(pstrIpAddr);
		jstring jsHost = env->NewStringUTF(pstrHostName);
		jstring jsText = env->NewStringUTF(pstrMsg);
		env->CallVoidMethod(m_delegateObj, CG::m_OnRecvMessage, jsText, jsAddr, jsHost);
	}
}



VMGuard::VMGuard()
: m_env(NULL), m_attached(true), m_valid(false)
{
	JNIEnv *env = NULL;
	jint r = CG::javaVM->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_2);
	if (r == JNI_OK) {
		m_attached = false;
	} else if (r == JNI_EDETACHED) {
		r = CG::javaVM->AttachCurrentThread(&env, NULL);
	}

	if (r == JNI_OK) {
		m_env = env;
		m_valid = true;
	}
}

VMGuard::~VMGuard()
{
	if (m_valid && m_attached) {
		CG::javaVM->DetachCurrentThread();
	}
}

bool VMGuard::isValid() const
{
	return m_valid;
}

JNIEnv *VMGuard::env() const
{
	return m_env;
}
