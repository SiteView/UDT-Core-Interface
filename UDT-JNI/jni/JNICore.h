#ifndef __JNICore_h__
#define __JNICore_h__

#include <jni.h>
#include <stdlib.h>
#include <vector>

#include "UdtCore.h"

class JNICore : public CUDTCallBack
{
public:
	JNICore();
	~JNICore();

	bool init(JNIEnv *env, jobject delegateObj);
	
	CUdtCore *core() const;

protected:
	// call back
	virtual void onAccept(const char* pstrIpAddr, const char* pstrFileName, int nFileCount, const char* pstrRecdevice, const char* pstrRectype, const char* pstrOwndevice, const char* pstrOwntype, const char* pstrSendType, int sock);
	virtual void onAcceptonFinish(const char* pstrAddr, const char* pstrFileName, int Type, int sock);
	virtual void onFinished(const char * pstrMsg, int Type, int sock);
	virtual void onTransfer(const int64_t nFileTotalSize, const int64_t nCurrent, const char* pstrFileName, int Type, int sock);
	virtual void onRecvMessage(const char* pstrMsg, const char* pstrIpAddr, const char* pstrHostName);

private:
	CUdtCore *m_core;
	jobject m_delegateObj;
};

class VMGuard
{
public:
	VMGuard();
	~VMGuard();
	
	bool isValid() const;
	JNIEnv *env() const;
	
private:
	JNIEnv *m_env;
	bool m_attached;
	bool m_valid;
};

#endif // __JNICore_h__
