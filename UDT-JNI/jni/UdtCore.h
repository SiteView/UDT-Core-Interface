#ifndef __UDTCORE_H__
#define __UDTCORE_H__

#ifndef WIN32
	#include <sys/time.h>
	#include <sys/uio.h>
	#include <arpa/inet.h>
	#include <unistd.h>
	#include <cstdlib>
	#include <cstring>
	#include <netdb.h>
	#include <pthread.h>
	#include <errno.h>
	#include <dirent.h>
	#include <time.h>
#else
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <wspiapi.h>
	#include <direct.h>
	#include <io.h>
#endif

#include <iostream>
#include <sys/stat.h>
#include <stdio.h>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <string.h>
#include <fstream>
#include <iostream>
#include <stdio.h>

#include "udt.h"
#include "cc.h"
#include "common.h"

class CUDTCallBack
{
public:
	virtual void onAccept(const char* pstrIpAddr, const char* pstrHostName, const char* pstrSendType, const char* pstrFileName, int nFileCount) = 0;
	virtual void onAcceptFolder(const char* pstrAddr, const char* pstrHostName, const char* pstrSendType, const char* pstrFileName, int nFileCount) = 0;
	virtual void onAcceptonFinish(const char* pstrAddr, std::vector<std::string> vecFileName) = 0;
	virtual void onSendFinished(const char * pstrMsg) = 0;
	virtual void onRecvFinished(const char * pstrMsg) = 0;
	virtual void onSendTransfer(const int64_t nFileTotalSize, const int64_t nCurrent, const char* pstrFileName) = 0;
	virtual void onRecvTransfer(const int64_t nFileTotalSize, const int64_t nCurrent, const char* pstrFileName) = 0;
	virtual void onRecvMessage(const char* pstrIpAddr, const char* pstrHostName, const char* pstrMsg) = 0;
};


class CUdtCore
{
public:
	CUdtCore(CUDTCallBack * pCallback);
	~CUdtCore();

	int StartListen(const int nCtrlPort, const int nRcvPort);
	void SendMessage(const char* pstrAddr, const char* pstrMsg, const char* pstrHostName);
	void SendFile(const char* pstrAddr, const char* pstrOwnDev, const char* pstrOwnType, const char* pstrRcvDev, const char* pstrRcvType, const char* pstrSndType, const std::vector<std::string> vecArray, int nType);
	void ReplyAccept(const UDTSOCKET sock, const char* pstrReply);
	void StopTransfer(const UDTSOCKET sock, const int nType);
	void StopListen();

private:
	typedef struct _ClientContext
	{
		UDTSOCKET sockCtrl;
		UDTSOCKET sockFile;
		char strAddr[32];
		char strCtrlPort[32];
		char strFilePort[32];
		sockaddr_storage clientaddr;
	}CLIENTCONTEXT, *LPCLIENTCONTEXT;

	typedef struct _ServerContext
	{
		UDTSOCKET sock;
		char strAddr[32];
		char strPort[32];
		char strHostName[128];
		char strSendType[128];
		char strXSR[8];
		char strXSP[8];
		char strXCS[8];
		char strXSF[8];
		std::vector<std::string> vecArray;
	}SERVERCONTEXT, *LPSERVERCONTEXT;

	void SearchFileInDirectroy(const std::string & szPath, int64_t & nTotalSize, std::vector<std::string> & vecDirName, std::vector<std::string> & vecFileName);
	void CreateDirectroy(const std::string & szPath);
	void ProcessCMD(const UDTSOCKET & sock, const char* pstrCMD);
	void ProcessAccept(CLIENTCONTEXT & cxt);
	void ProcessSendFile(SERVERCONTEXT & sxt);
	void ProcessRecvFile(CLIENTCONTEXT & cxt);
	int InitListenSocket(const int nPort, UDTSOCKET & sockListen);
	int CreateTCPSocket(SYSSOCKET & ssock, const char* pstrPort, bool rendezvous = false);
	int CreateUDTSocket(UDTSOCKET & usock, const char* pstrPort, bool rendezvous = false);
	int TCP_Connect(SYSSOCKET& ssock, const char* pstrAddr, const char* pstrPort);
	int UDT_Connect(UDTSOCKET & usock, const char* pstrAddr, const char* pstrPort);

private:
	CUDTCallBack * m_pCallBack;

	std::vector<LPCLIENTCONTEXT> VEC_CXT;
	std::vector<LPSERVERCONTEXT> VEC_SXT;

	int m_eid;
	UDTSOCKET m_sockListenCtrlCmd;
	UDTSOCKET m_sockListenRcvFile;
	std::string m_szReplyfilepath;
	bool m_bSendStatus;
	bool m_bRecvStatus;
	bool m_bListenStatus;

	pthread_mutex_t m_AcceptLock;
	pthread_mutex_t m_SendLock;
	pthread_mutex_t m_RecvLock;

	pthread_cond_t m_AcceptCond;
	pthread_cond_t m_SendCond;
	pthread_cond_t m_RecvCond;

	pthread_t m_hListenThread;
	pthread_t m_hSendThread;
	pthread_t m_hRecvThread;
#ifndef WIN32
	static void * _ListenCtrlCmdThread(void * pParam);
	static void * _ListenRcvFileThread(void * pParam);
	static void * _SendThread(void * pParam);
	static void * _RecvThread(void * pParam);
	static void * _SendFile(void * pParam);
	static void * _RecvFile(void * pParam);
#else
	static DWORD WINAPI _ListenCtrlCmdThread(LPVOID pParam);
	static DWORD WINAPI _ListenRcvFileThread(LPVOID pParam);
	static DWORD WINAPI _SendThread(LPVOID pParam);
	static DWORD WINAPI _RecvThread(LPVOID pParam);
	static DWORD WINAPI _SendFile(LPVOID pParam);
	static DWORD WINAPI _RecvFile(LPVOID pParam);
#endif
};

#endif	// __UDTCORE_H__
