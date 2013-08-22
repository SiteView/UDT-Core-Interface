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

#define TO_SND 1024*500		// 文件发送数据块大小：((1(byte) * 1024)(kb) * 1024)(mb)

class CUDTCallBack
{
public:
	virtual void onAccept(const char* pstrAddr, const char* pstrFileName, int nFileCount, const char* recdevice, const char* rectype, const char* owndevice, const char* owntype, const char* SendType, int sock) = 0;
	virtual void onAcceptonFinish(const char* pstrAddr, const char* pFileName, int Type, int sock) = 0;
	virtual void onFinished(const char * pstrMsg, int Type, int sock) = 0;
	virtual void onTransfer(const int64_t nFileTotalSize, const int64_t nCurrent, const char* pstrFileName, int Type, int sock) = 0;
	virtual void onRecvMessage(const char* pstrMsg, const char* pIpAddr, const char* pHostName) = 0;
};

class CUdtCore
{
public:
	CUdtCore(CUDTCallBack * pCallback);
	~CUdtCore();

	int StartListen(const int nCtrlPort, const int nFilePort);
	int SendMsg(const char* pstrAddr, const char* pstrMsg, const char* pstrHostName);
	int SendFiles(const char* pstrAddr, const std::vector<std::string> vecFiles, const char* owndevice, const char* owntype, const char* recdevice, const char* rectype, const char* pstrSendtype);
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
		int64_t nFileTotalSize;
		int64_t nRecvSize;
		sockaddr_storage clientaddr;
		pthread_t hThread;
		pthread_cond_t cond;
		pthread_mutex_t lock;
	}CLIENTCONTEXT, *LPCLIENTCONTEXT;

	typedef struct _ServerContext
	{
		UDTSOCKET sockCtrl;
		UDTSOCKET sockFile;
		char strAddr[32];
		char strCtrlPort[32];
		char strFilePort[32];
		char ownDev[128];
		char ownType[128];
		char recvDev[128];
		char recvType[128];
		char sendType[128];
		std::vector<std::string> vecFile;
		std::vector<std::string> vecDirs;
		std::vector<std::string> vecFiles;
		bool bSendFile;
		pthread_t hThread;
		pthread_cond_t cond;
		pthread_mutex_t lock;
	}SERVERCONTEXT, *LPSERVERCONTEXT;

	void SearchFileInDirectroy(const std::string & szPath, int64_t & nTotalSize, std::vector<std::string> & vecDirName, std::vector<std::string> & vecFileName);
	void CreateDirectroy(const std::string & szPath);
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
	int m_nCtrlPort;
	int m_nFilePort;
	std::string m_szReplyfilepath;
	bool m_bSendStatus;
	bool m_bRecvStatus;
	bool m_bListenStatus;

	pthread_mutex_t m_LockLis;
	pthread_mutex_t m_LockSnd;
	pthread_mutex_t m_LockRcv;
	pthread_mutex_t m_Lock;

	pthread_cond_t m_CondLisCtrl;
	pthread_cond_t m_CondLisFile;
	pthread_cond_t m_CondSnd;
	pthread_cond_t m_CondRcv;

	pthread_t m_hThrLisCtrl;
	pthread_t m_hThrLisFile;
	pthread_t m_hThrSnd;
	pthread_t m_hThrRcv;
#ifndef WIN32
	static void * _ListenRcvCtrlThread(void * pParam);
	static void * _ListenRcvFileThread(void * pParam);
	static void * _SendThread(void * pParam);
	static void * _RecvThread(void * pParam);
	static void * _SendFiles(void * pParam);
	static void * _RecvFiles(void * pParam);
#else
	static DWORD WINAPI _ListenRcvCtrlThread(LPVOID pParam);
	static DWORD WINAPI _ListenRcvFileThread(LPVOID pParam);
	static DWORD WINAPI _SendThread(LPVOID pParam);
	static DWORD WINAPI _RecvThread(LPVOID pParam);
	static DWORD WINAPI _SendFiles(LPVOID pParam);
	static DWORD WINAPI _RecvFiles(LPVOID pParam);
#endif
};

#endif	// __UDTCORE_H__
