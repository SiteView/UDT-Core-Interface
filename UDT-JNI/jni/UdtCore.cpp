#include "UdtCore.h"


using namespace std;

CUdtCore::CUdtCore(CUDTCallBack * pCallback)
	: m_pCallBack(pCallback)
	, m_bListenStatus(false)
	, m_bSendStatus(false)
	, m_bRecvStatus(false)
	, m_bTTSPing(false)
	, m_nCtrlPort(7777)
	, m_nFilePort(7778)
{
#ifndef WIN32
	pthread_mutex_init(&m_Lock, NULL);
	pthread_mutex_init(&m_LockLis, NULL);
	pthread_mutex_init(&m_LockSnd, NULL);
	pthread_mutex_init(&m_LockRcv, NULL);
	pthread_cond_init(&m_CondLisCtrl, NULL);
	pthread_cond_init(&m_CondLisFile, NULL);
	pthread_cond_init(&m_CondSnd, NULL);
	pthread_cond_init(&m_CondRcv, NULL);
	pthread_mutex_init(&m_LockTTS, NULL);
	pthread_cond_init(&m_CondTTS, NULL);
#else
	m_Lock				= CreateMutex(NULL, false, NULL);
	m_LockLis			= CreateMutex(NULL, false, NULL);
	m_LockSnd		= CreateMutex(NULL, false, NULL);
	m_LockRcv		= CreateMutex(NULL, false, NULL);
	m_CondLisCtrl	= CreateEvent(NULL, false, false, NULL);
	m_CondLisFile	= CreateEvent(NULL, false, false, NULL);
	m_CondSnd		= CreateEvent(NULL, false, false, NULL);
	m_CondRcv		= CreateEvent(NULL, false, false, NULL);
	m_LockTTS		= CreateMutex(NULL, false, NULL);
	m_CondTTS	= CreateEvent(NULL, false, false, NULL);
#endif
}


CUdtCore::~CUdtCore()
{
#ifndef WIN32
	pthread_mutex_destroy(&m_Lock);
	pthread_mutex_destroy(&m_LockLis);
	pthread_mutex_destroy(&m_LockSnd);
	pthread_mutex_destroy(&m_LockRcv);
	pthread_cond_destroy(&m_CondLisCtrl);
	pthread_cond_destroy(&m_CondLisFile);
	pthread_cond_destroy(&m_CondSnd);
	pthread_cond_destroy(&m_CondRcv);
	pthread_mutex_destroy(&m_LockTTS);
	pthread_cond_destroy(&m_CondTTS);
#else
	CloseHandle(m_Lock);
	CloseHandle(m_LockLis);
	CloseHandle(m_LockSnd);
	CloseHandle(m_LockRcv);
	CloseHandle(m_CondLisCtrl);
	CloseHandle(m_CondLisFile);
	CloseHandle(m_CondSnd);
	CloseHandle(m_CondRcv);
	CloseHandle(m_LockTTS);
	CloseHandle(m_CondTTS);
#endif
}

int CUdtCore::StartListen(const int nCtrlPort, const int nFilePort)
{
	UDT::startup();

	if (m_bListenStatus)
		return 0;

	m_nCtrlPort = nCtrlPort;
	m_nFilePort = nFilePort;
	/*char strCtrlPort[32];
	char strFilePort[32];
	sprintf(strCtrlPort, "%d", nCtrlPort);
	sprintf(strFilePort, "%d", nFilePort);

	// Init listen ContrlSocket
	if (CreateUDTSocket(m_sockListenCtrlCmd, strCtrlPort, true) < 0)
	{
		m_pCallBack->onFinished("Create fail!", 108, 0);
		return -1;
	}
	// listen socket
	if (UDT::ERROR == UDT::listen(m_sockListenCtrlCmd, 10))
	{
		m_pCallBack->onFinished("Create fail!", 108, 0);
		return -1;
	}

	// Init listen RecvFileSocket
	if (CreateUDTSocket(m_sockListenRcvFile, strFilePort, true) < 0)
	{
		m_pCallBack->onFinished("Create fail!", 108, 0);
		return -1;
	}
	// listen socket
	if (UDT::ERROR == UDT::listen(m_sockListenRcvFile, 10))
	{
		m_pCallBack->onFinished("Create fail!", 108, 0);
		return -1;
	}*/

	// create listen thread for accept client connect
#ifndef WIN32
	pthread_create(&m_hThrLisCtrl, NULL, _ListenRcvCtrlThread, this);
	pthread_detach(m_hThrLisCtrl);
	sleep(1);
	pthread_create(&m_hThrLisFile, NULL, _ListenRcvFileThread, this);
	pthread_detach(m_hThrLisFile);
#else
	m_hThrLisCtrl = CreateThread(NULL, 0, _ListenRcvCtrlThread, this, NULL, NULL);
	Sleep(1000);
	m_hThrLisFile = CreateThread(NULL, 0, _ListenRcvFileThread, this, NULL, NULL);
#endif

	m_bListenStatus = true;

	return 0;
}

int CUdtCore::SendMsg(const char* pstrAddr, const char* pstrMsg, const char* pstrHostName)
{
	UDT::startup();
	int nLen = 0, nReturnCode = 108;
	char Head[8];

	// create concurrent UDT sockets
	UDTSOCKET client;
	char strPort[32];
	sprintf(strPort, "%d", 7777);
	if (CreateUDTSocket(client, strPort) < 0)
	{
		m_pCallBack->onFinished("Create fail!", nReturnCode, client);
		return 0;
	}
	if (UDT_Connect(client, pstrAddr, strPort) < 0)
	{
		m_pCallBack->onFinished("Connect fail!", nReturnCode, client);
		return 0;
	}

	// send flags
	memset(Head, 0, 8);
	memcpy(Head, "TSR", 3);
	if (UDT::ERROR == UDT::send(client, (char*)Head, 3, 0))
		goto Loop;

	// send text message
	nLen = strlen(pstrHostName);
	if (UDT::ERROR == UDT::send(client, (char*)&nLen, sizeof(int), 0))
		goto Loop;
	if (UDT::ERROR == UDT::send(client, pstrHostName, nLen, 0))
		goto Loop;
	// send message size and information
	nLen = strlen(pstrMsg);
	if (UDT::ERROR == UDT::send(client, (char*)&nLen, sizeof(int), 0))
		goto Loop;
	if (UDT::ERROR == UDT::send(client, pstrMsg, nLen, 0))
		goto Loop;

	nReturnCode = 0;
	// goto loop for end
Loop:
	if (nReturnCode > 0)
	{
		m_pCallBack->onFinished("RETURN", nReturnCode, client);
	}
	UDT::close(client);
	return 0;
}

int CUdtCore::SendFiles(const char* pstrAddr, const std::vector<std::string> vecFiles, const char* owndevice, const char* owntype, const char* recdevice, const char* rectype, const char* pstrSendtype)
{
	if (vecFiles.empty())
		return 0;

	// create concurrent UDT sockets
	UDT::startup();
	UDTSOCKET sockCtrl;
	int nReturnCode = 108;
	char strCtrlPort[32];
	char strFilePort[32];
	sprintf(strCtrlPort, "%d", 7777);
	sprintf(strFilePort, "%d", 7778);

	// connect to CtrlPort
	if (CreateUDTSocket(sockCtrl, strCtrlPort) < 0)
	{
		m_pCallBack->onFinished("Create fail!", 108, sockCtrl);
		return 0;
	}
	if (UDT_Connect(sockCtrl, pstrAddr, strCtrlPort) < 0)
	{
		m_pCallBack->onFinished("Connect fail!", 108, sockCtrl);
		return 0;
	}

	// Remove ServerContext
	CGuard::enterCS(m_Lock);
	for (vector<LPSERVERCONTEXT>::iterator it = VEC_SXT.begin(); it != VEC_SXT.end();)
	{
		if ((*it)->sockCtrl < 0)
		{
			it = VEC_SXT.erase(it);
		}
		else
			it++;
	}

	LPSERVERCONTEXT sxt = new SERVERCONTEXT;
	memset(sxt, 0, sizeof(SERVERCONTEXT));
	memcpy(sxt->strAddr, pstrAddr, 32);
	memcpy(sxt->strCtrlPort, strCtrlPort, 32);
	memcpy(sxt->strFilePort, strFilePort, 32);
	memcpy(sxt->ownDev, owndevice, 128);
	memcpy(sxt->ownType, owntype, 128);
	memcpy(sxt->recvDev, recdevice, 128);
	memcpy(sxt->recvType, rectype, 128);
	memcpy(sxt->sendType, pstrSendtype, 128);
	sxt->sockCtrl = sockCtrl;
	sxt->vecFile = vecFiles;
	sxt->bSendFile = false;
	VEC_SXT.push_back(sxt);
	CGuard::leaveCS(m_Lock);

	pthread_t hHandle;
#ifndef WIN32
	//pthread_mutex_init(&sxt->lock, NULL);
	//pthread_cond_init(&sxt->cond, NULL);
	pthread_create(&hHandle, NULL, _SendThread, this);
	pthread_detach(hHandle);
#else
	//sxt->lock = CreateMutex(NULL, false, NULL);
	//sxt->cond = CreateEvent(NULL, false, false, NULL);	
	hHandle = CreateThread(NULL, 0, _SendThread, this, 0, NULL);
#endif

	return sockCtrl;
}

void CUdtCore::ReplyAccept(const UDTSOCKET sock, const char* pstrReply)
{
	m_szReplyfilepath = pstrReply;
	// file transfer response(FSP0/MSP0/DSP0)
	char Head[8];
	memset(Head, 0, 8);
	if (m_szReplyfilepath.compare("REJECT")==0)
		memcpy(Head, "FRR", 3);
	else if (m_szReplyfilepath.compare("REJECTBUSY") == 0 || m_szReplyfilepath.empty())
		memcpy(Head, "FRB", 3);
	else
		memcpy(Head, "FRA", 3);

	if (UDT::ERROR == UDT::send(sock, (char*)Head, 3, 0))
		return ;
}

void CUdtCore::StopTransfer(const UDTSOCKET sock, const int nType)
{
	char Head[8];

	CGuard::enterCS(m_Lock);
	for (vector<LPCLIENTCONTEXT>::iterator it = VEC_CXT.begin(); it != VEC_CXT.end(); it++)
	{
		if ((*it)->sockCtrl == sock)
		{
			memset(Head, 0, 8);
			memcpy(Head, "FRS", 3);
			if (UDT::ERROR == UDT::send(sock, (char*)Head, 3, 0))
				return ;
			break;
		}
	}

	for (vector<LPSERVERCONTEXT>::iterator it = VEC_SXT.begin(); it != VEC_SXT.end(); it++)
	{
		if ((*it)->sockCtrl == sock)
		{
			memset(Head, 0, 8);
			if ((*it)->bSendFile)
				memcpy(Head, "FSS", 3);
			else
				memcpy(Head, "FSC", 3);
			if (UDT::ERROR == UDT::send(sock, (char*)Head, 3, 0))
				return ;
			break;
		}
	}
	CGuard::leaveCS(m_Lock);
}

void CUdtCore::StopListen()
{
#ifndef WIN32
	pthread_mutex_lock(&m_LockLis);
	pthread_cond_signal(&m_CondLisCtrl);
	pthread_mutex_unlock(&m_LockLis);
	pthread_mutex_lock(&m_LockLis);
	pthread_cond_signal(&m_CondLisFile);
	pthread_mutex_unlock(&m_LockLis);
	//pthread_mutex_lock(&m_LockTTS);
	//pthread_cond_signal(&m_CondTTS);
	//pthread_mutex_unlock(&m_LockTTS);
#else
	SetEvent(m_CondLisCtrl);
	SetEvent(m_CondLisFile);
	SetEvent(m_CondTTS);
#endif

	UDT::close(m_sockListenCtrlCmd);
	UDT::close(m_sockListenRcvFile);
	m_bListenStatus = false;
}

#ifndef WIN32
void * CUdtCore::_ListenRcvCtrlThread(void * pParam)
#else
DWORD WINAPI CUdtCore::_ListenRcvCtrlThread(LPVOID pParam)
#endif
{
	CUdtCore * pThis = (CUdtCore *)pParam;
	//fstream log("/mnt/sdcard/UdtLog1.txt", ios::out | ios::binary | ios::trunc);

	char strCtrlPort[32];
	sprintf(strCtrlPort, "%d", 7777);

	sockaddr_storage clientaddr;
	int addrlen = sizeof(clientaddr);
	UDTSOCKET client, sockListen;
	bool bListen = false;

	while (true)
	{
#ifndef WIN32
		pthread_mutex_lock(&pThis->m_LockLis);
		timeval now;
		timespec timeout;
		gettimeofday(&now, 0);
		timeout.tv_sec = now.tv_sec + 1;
		timeout.tv_nsec = now.tv_usec * 1000;

		int rc = pthread_cond_timedwait(&pThis->m_CondLisCtrl, &pThis->m_LockLis, &timeout);
		pthread_mutex_unlock(&pThis->m_LockLis);
		if (rc != ETIMEDOUT)
		{
			cout << "_ListenThreadProc timeout" << endl;
			break;
		}

#else
		if (WAIT_TIMEOUT != WaitForSingleObject(pThis->m_CondLisCtrl, 1000))
		{
			std::cout << "_ListenThreadProc timeout" << endl;
			break;
		}
#endif

		if (!bListen)
		{
			if (pThis->CreateUDTSocket(sockListen, strCtrlPort, true) < 0)
			{
				pThis->m_pCallBack->onFinished("CreateUDTSocket 7777 fail!", 108, 0);
				continue;
			}
			// listen socket
			UDT::listen(sockListen, 10);
			//if (UDT::ERROR == UDT::listen(sockListen, 10))
			//{
			//	pThis->m_pCallBack->onFinished("listen 7777 fail!", 108, 0);
			//	UDT::close(sockListen);
			//	continue;
			//}

			pThis->m_sockListenCtrlCmd = sockListen;
			bListen = true;
		}

		//log << "Accept port: 7777" << endl;
		if (UDT::INVALID_SOCK == (client = UDT::accept(sockListen, (sockaddr*)&clientaddr, &addrlen)))
		{
			//log << "accept port 7777: " << UDT::getlasterror().getErrorMessage() << endl;
			bListen = false;
			UDT::close(sockListen);
			sleep(1);
			continue;
		}
		char clienthost[NI_MAXHOST];
		char clientservice[NI_MAXSERV];
		getnameinfo((sockaddr *)&clientaddr, sizeof(clientaddr), clienthost, sizeof(clienthost), clientservice, sizeof(clientservice), NI_NUMERICHOST|NI_NUMERICSERV);
		//log << "Recv Ctrl connection: " << clienthost << ":" << clientservice << endl;

		CLIENTCONTEXT * cxt = new CLIENTCONTEXT;
		memset(cxt, 0, sizeof(cxt));
		sprintf(cxt->strAddr, "%s", clienthost);
		sprintf(cxt->strCtrlPort, "%s", clientservice);
		cxt->sockCtrl = client;

		CGuard::enterCS(pThis->m_Lock);
		for (vector<LPCLIENTCONTEXT>::iterator it = pThis->VEC_CXT.begin(); it != pThis->VEC_CXT.end();)
		{
			if ((*it)->sockCtrl < 0)
			{
				it = pThis->VEC_CXT.erase(it);
			}
			else
			{
				it++;
			}
		}
		pThis->VEC_CXT.push_back(cxt);
		CGuard::leaveCS(pThis->m_Lock);

		pthread_t hHandle;
#ifndef WIN32
		pthread_create(&hHandle, NULL, _RecvThread, pThis);
		pthread_detach(hHandle);
#else
		hHandle = CreateThread(NULL, 0, _RecvThread, pThis, 0, NULL);
#endif
	}

	//log << "_RecvThread log close" << endl;
	//log.close();

#ifndef WIN32
	return NULL;
#else
	return 0;
#endif
}

#ifndef WIN32
void * CUdtCore::_ListenRcvFileThread(void * pParam)
#else
DWORD WINAPI CUdtCore::_ListenRcvFileThread(LPVOID pParam)
#endif
{
	CUdtCore * pThis = (CUdtCore *)pParam;
	//fstream log("/mnt/sdcard/UdtLog2.txt", ios::out | ios::binary | ios::trunc);

	char strCtrlPort[32];
	sprintf(strCtrlPort, "%d", 7778);
	sockaddr_storage clientaddr;
	int addrlen = sizeof(clientaddr);
	UDTSOCKET client, sockListen;
	bool bListen = false;
	LPCLIENTCONTEXT cxt = new CLIENTCONTEXT;

	while (true)
	{
#ifndef WIN32
		pthread_mutex_lock(&pThis->m_LockLis);
		timeval now;
		timespec timeout;
		gettimeofday(&now, 0);
		timeout.tv_sec = now.tv_sec + 1;
		timeout.tv_nsec = now.tv_usec * 1000;

		int rc = pthread_cond_timedwait(&pThis->m_CondLisFile, &pThis->m_LockLis, &timeout);
		pthread_mutex_unlock(&pThis->m_LockLis);
		if (rc != ETIMEDOUT)
		{
			std::cout << "_ListenThreadProc timeout" << endl;
			break;
		}

#else
		if (WAIT_TIMEOUT != WaitForSingleObject(pThis->m_CondLisFile, 1000))
		{
			std::cout << "_ListenThreadProc timeout" << endl;
			break;
		}
#endif


		if (!bListen)
		{
			if (pThis->CreateUDTSocket(sockListen, strCtrlPort, true) < 0)
			{
				pThis->m_pCallBack->onFinished("CreateUDTSocket 7778 fail!", 108, 0);
				continue;
			}
			// listen socket
			UDT::listen(sockListen, 10);
			//if (UDT::ERROR == UDT::listen(sockListen, 10))
			//{
			//	pThis->m_pCallBack->onFinished("listen 7777 fail!", 108, 0);
			//	UDT::close(sockListen);
			//	continue;
			//}

			pThis->m_sockListenRcvFile = sockListen;
			bListen = true;
		}
		//log << "Accept port: 7778" << endl;
		if (UDT::INVALID_SOCK == (client = UDT::accept(sockListen, (sockaddr*)&clientaddr, &addrlen)))
		{
			//log << "accept port 7778: " << UDT::getlasterror().getErrorMessage() << endl;
			bListen = false;
			UDT::close(sockListen);
			sleep(1);
			continue;
		}
		char clienthost[NI_MAXHOST];
		char clientservice[NI_MAXSERV];
		getnameinfo((sockaddr *)&clientaddr, sizeof(clientaddr), clienthost, sizeof(clienthost), clientservice, sizeof(clientservice), NI_NUMERICHOST|NI_NUMERICSERV);
		//log << "Recv File connection: " << clienthost << ":" << clientservice << endl;

		//CGuard::enterCS(pThis->m_Lock);
		CLIENTCONTEXT * cxt = new CLIENTCONTEXT;
		cxt = pThis->VEC_CXT.back();
		if (strcmp(cxt->strAddr, clienthost) == 0)
		{
			cxt->sockFile = client;
			sprintf(cxt->strFilePort, "%s", clientservice);
		}
		//CGuard::leaveCS(pThis->m_Lock);

		//log << "CreateThread _RecvFiles" << endl;

		pthread_t hHandle;
#ifndef WIN32
		pthread_create(&hHandle, NULL, _RecvFiles, pThis);
		pthread_detach(hHandle);
#else
		hHandle = CreateThread(NULL, 0, _RecvFiles, pThis, 0, NULL);
#endif
	}

	//log << "_RecvFiles log close" << endl;
	//log.close();

#ifndef WIN32
	return NULL;
#else
	return 0;
#endif
}

#ifndef WIN32
void * CUdtCore::_SendThread(void * pParam)
#else
DWORD WINAPI CUdtCore::_SendThread(LPVOID pParam)
#endif
{
	CUdtCore * pThis = (CUdtCore *)pParam;

	//fstream log("/mnt/sdcard/UdtLog1.txt", ios::out | ios::binary | ios::trunc);
	//log << "***_SendThread***" << endl;

	char Head[8];
	int nReturnCode = 108, nLen = 0, nPos = 0, nFileCount = 0;
	int64_t nFileTotalSize = 0, nSendSize = 0, nLastSendSize = 0;
	double iProgress;
	string szAddr, szPort;
	string szTmp, szFolderName, szFileName, szOldFileName = "", szFilePath, szFinish = "NETFAIL", szError = "";
	vector<string> vecFileName;
	vector<string> vecDirName;
	vector<string> vecTmp;

	SERVERCONTEXT * sxt = new SERVERCONTEXT;
	sxt = pThis->VEC_SXT.back();
	UDTSOCKET client = sxt->sockCtrl;

	// send flags
	memset(Head, 0, 8);
	memcpy(Head, "MSR", 3);
	if (UDT::ERROR == UDT::send(client, (char*)Head, 3, 0))
		goto Loop;

	vecTmp = sxt->vecFile;
	for (vector<string>::iterator it = vecTmp.begin(); it != vecTmp.end(); it++)
	{
		szTmp = *it;
		if ('\\' == szTmp[szTmp.size()-1] || '/' == szTmp[szTmp.size()-1])
			szTmp = szTmp.substr(0, szTmp.size()-1);

		pThis->SearchFileInDirectroy(szTmp, nFileTotalSize, vecDirName, vecFileName);
	}
	nFileCount = vecTmp.size();
	sxt->vecFiles = vecFileName;

	// send file total size
	if (UDT::ERROR == UDT::send(client, (char*)&nFileTotalSize, sizeof(nFileTotalSize), 0))
		goto Loop;
	// send file count
	if (UDT::ERROR == UDT::send(client, (char*)&nFileCount, sizeof(nFileCount), 0))
		goto Loop;

	// send file name,(filename\hostname\sendtype)
	szFilePath = sxt->vecFile[0];
	nPos = szFilePath.find_last_of('/');
	if (nPos < 0)
	{
		nPos = szFilePath.find_last_of("\\");
	}
	szFileName = szFilePath.substr(nPos+1);
	szTmp = szFileName + "\\" + sxt->ownDev + "\\" + sxt->ownType + "\\" + sxt->recvDev + "\\" + sxt->recvType + "\\" + sxt->sendType;
	if (vecDirName.empty())
		szTmp = szTmp + "\\F\\";
	else
		szTmp = szTmp + "\\D\\";
	nLen = szTmp.size();
	if (UDT::ERROR == UDT::send(client, (char*)&nLen, sizeof(int), 0))
		goto Loop;
	if (UDT::ERROR == UDT::send(client, szTmp.c_str(), nLen, 0))
		goto Loop;

	while (true)
	{
		memset(Head, 0, 8);
		if (UDT::ERROR == UDT::recv(client, (char*)Head, 3, 0))
			goto Loop;
		//log << "Head" << Head << endl;
		if (memcmp(Head, "FRA", 3) == 0)
		{
			if (pThis->CreateUDTSocket(sxt->sockFile, sxt->strFilePort) < 0)
			{
				goto Loop;
			}
			int i = 0;
			while (true)
			{
				if (pThis->UDT_Connect(sxt->sockFile, sxt->strAddr, sxt->strFilePort) < 0)
				{
					i++;
					if (i >= 2)
						goto Loop;
#ifndef WIN32
					sleep(1);
#else
					Sleep(1000);
#endif
				}
				else
				{
					sxt->bSendFile = true;
					pthread_t hHandle;
#ifndef WIN32
					pthread_create(&hHandle, NULL, _SendFiles, pThis);
					pthread_detach(hHandle);
#else
					hHandle = CreateThread(NULL, 0, _SendFiles, pThis, 0, NULL);
#endif
					break;
				}
			}
		}
		else if (memcmp(Head, "FRR", 3) == 0)
		{
			nReturnCode = 103;
			memset(Head, 0, 8);
			memcpy(Head, "FRR", 3);
			if (UDT::ERROR == UDT::send(client, (char*)Head, 3, 0))
				goto Loop;
			goto Loop;
		}
		else if (memcmp(Head, "FRC", 3) == 0)
		{
			nReturnCode = 103;
			memset(Head, 0, 8);
			memcpy(Head, "FRC", 3);
			if (UDT::ERROR == UDT::send(client, (char*)Head, 3, 0))
				goto Loop;
			goto Loop;
		}
		else if (memcmp(Head, "FRB", 3) == 0)
		{
			nReturnCode = 114;
			memset(Head, 0, 8);
			memcpy(Head, "FRB", 3);
			if (UDT::ERROR == UDT::send(client, (char*)Head, 3, 0))
				goto Loop;
			goto Loop;
		}
		else if (memcmp(Head, "FSC", 3) == 0)
		{
			nReturnCode = 100;
			goto Loop;
		}
		else if (memcmp(Head, "FRF", 3) == 0)
		{
			nReturnCode = 109;
			memset(Head, 0, 8);
			memcpy(Head, "FSF", 3);
			if (UDT::ERROR == UDT::send(client, (char*)Head, 3, 0))
				goto Loop;
			goto Loop;
		}
		else if (memcmp(Head, "FRS", 3) == 0)
		{
			nReturnCode = 107;
			memset(Head, 0, 8);
			memcpy(Head, "FRE", 3);
			if (UDT::ERROR == UDT::send(client, (char*)Head, 3, 0))
				goto Loop;
			goto Loop;
		}
		else if (memcmp(Head, "FSS", 3) == 0)
		{
			nReturnCode = 104;
			memset(Head, 0, 8);
			memcpy(Head, "FSE", 3);
			if (UDT::ERROR == UDT::send(client, (char*)Head, 3, 0))
				goto Loop;
			goto Loop;
		}
		else if (memcmp(Head, "FPR", 3) == 0)
		{
			if (UDT::ERROR == UDT::recv(client, (char*)&nSendSize, sizeof(nSendSize), 0))
				goto Loop;
			if (nSendSize <= nFileTotalSize && nSendSize > nLastSendSize)
			{
				nLastSendSize = nSendSize;
				//CGuard::enterCS(pThis->m_Lock);
				//szTmp = pThis->m_szFileName;
				//szTmp = sxt->fileName;
				//iProgress = sxt->iProgress;
				//CGuard::leaveCS(pThis->m_Lock);

				pThis->m_pCallBack->onTransfer(nFileTotalSize, nLastSendSize, iProgress, szFileName.c_str(), 2, client);
				if (nSendSize == nFileTotalSize)
				{
					nReturnCode = 109;
					memset(Head, 0, 8);
					memcpy(Head, "MSF", 3);
					if (UDT::ERROR == UDT::send(client, (char*)Head, 3, 0))
						goto Loop;
					goto Loop;
				}
			}
		}
		else if (strlen(Head) >= 3)
		{
			nReturnCode = 111;
			memset(Head, 0, 8);
			memcpy(Head, "FSV", 3);
			if (UDT::ERROR == UDT::send(client, (char*)Head, 3, 0))
				goto Loop;
			goto Loop;
		}
	}

	// goto loop for end
Loop:
	if (nReturnCode > 0)
	{
		pThis->m_pCallBack->onFinished("RETURN", nReturnCode, client);
	}

	UDT::close(client);
	//log << "log .close" << endl;
	//log.close();

#ifndef WIN32
	return NULL;
#else
	return 0;
#endif
}

#ifndef WIN32
void * CUdtCore::_SendFiles(void * pParam)
#else
DWORD WINAPI CUdtCore::_SendFiles(LPVOID pParam)
#endif
{
	CUdtCore * pThis = (CUdtCore *)pParam;
	//delete (SERVERCONTEXT *)pParam;

	//fstream log("/mnt/sdcard/UdtLog.txt", ios::out | ios::binary | ios::trunc);
	//log << "***_SendFiles***" << endl;

	char Head[8];
	int nLen = 0, nPos = 0, nFileCount = 0;
	int64_t nFileTotalSize = 0, nSendSize = 0, iLastPercent = 0;
	string szTmp, szFolder, szAccept, szFileName, szFilePath, szFinish = "NETFAIL", szError = "";
	vector<string> vecFile;
	vector<string> vecFilePath;
	vector<string> vecDirName;

	SERVERCONTEXT * sxt = new SERVERCONTEXT;
	CGuard::enterCS(pThis->m_Lock);
	sxt = pThis->VEC_SXT.back();
	CGuard::leaveCS(pThis->m_Lock);
	UDTSOCKET client = sxt->sockFile;
	vecFile = sxt->vecFile;
	vecFilePath = sxt->vecFiles;

	// send file
	for (vector<string>::iterator it2 = vecFile.begin(); it2 != vecFile.end(); it2++)
	{
		for (vector<string>::iterator it3 = vecFilePath.begin(); it3 != vecFilePath.end(); it3++)
		{
			szFilePath = *it3;
			if (szFilePath.compare(0, (*it2).size(), (*it2)) == 0)
			{
				nPos = (*it2).find_last_of('/');
				if (nPos < 0)
				{
					nPos = (*it2).find_last_of("\\");
				}
				szFileName = szFilePath.substr(nPos);
				nPos = szFileName.find_last_of('/');
				if (nPos < 0)
				{
					nPos = szFileName.find_last_of("\\");
				}
				szFolder = szFileName.substr(1, nPos);
				szFileName = szFileName.substr(1);

				// send folder name
				if (szFolder.size() > 0)
				{
					if ('\\' == szFolder[szFolder.size()-1] || '/' == szFolder[szFolder.size()-1])
						szFolder = szFolder.substr(0, szFolder.size()-1);

					nLen = szFolder.size();
					memset(Head, 0, 8);
					memcpy(Head, "DCR", 3);
					if (UDT::ERROR == UDT::send(client, (char*)Head, 3, 0))
						goto Loop;
					if (UDT::ERROR == UDT::send(client, (char*)&nLen, sizeof(int), 0))
						goto Loop;
					if (UDT::ERROR == UDT::send(client, szFolder.c_str(), nLen, 0))
						goto Loop;
				}

				// send filename size and filename
				memset(Head, 0, 8);
				memcpy(Head, "MCS", 3);
				if (UDT::ERROR == UDT::send(client, (char*)Head, 3, 0))
					goto Loop;
				nLen = szFileName.size();
				if (UDT::ERROR == UDT::send(client, (char*)&nLen, sizeof(int), 0))
					goto Loop;
				if (UDT::ERROR == UDT::send(client, szFileName.c_str(), nLen, 0))
					goto Loop;

				// 文件夹，只回调一次
				nPos = szFileName.find_first_of('/');
				if (nPos < 0)
				{
					nPos = szFileName.find_first_of("\\");
				}
				if (nPos >= 0)
				{
					szTmp = szFileName.substr(0, nPos);
					if (szTmp != szAccept)
					{
						szAccept = szTmp;
						szFileName = (*it2).c_str();
					}
				}
				else
					szFileName = szFilePath;

				//log << "fileName:" << szFileName << endl;
				//CGuard::enterCS(pThis->m_Lock);
				//sxt->fileName = szFileName;
				//pThis->m_szFileName = szFileName;
				//CGuard::leaveCS(pThis->m_Lock);
				//log << "fileName:" << szFileName << endl;

				// open the file
				int64_t nFileSize = 0, nOffset = 0, left = 0;
				fstream ifs(szFilePath.c_str(), ios::binary | ios::in);
				try
				{
					ifs.seekg(0, ios::end);
					nFileSize = ifs.tellg();
					ifs.seekg(0, ios::beg);

					left = nFileSize;
					if (UDT::ERROR == UDT::send(client, (char*)&nFileSize, sizeof(nFileSize), 0))
						goto Loop;

					while (left > 0)
					{
						UDT::TRACEINFO trace;
						UDT::perfmon(client, &trace);

						int64_t send = 0;
						if (left > TO_SND)
							send = UDT::sendfile(client, ifs, nOffset, TO_SND);
						else
							send = UDT::sendfile(client, ifs, nOffset, left);

						if (UDT::ERROR == send)
							goto Loop;
						left -= send;
						nSendSize += send;

						//CGuard::enterCS(pThis->m_Lock);
						//sxt->iProgress = trace.mbpsSendRate / 8;
						//CGuard::leaveCS(pThis->m_Lock);
					}
					ifs.close();
					pThis->m_pCallBack->onAcceptonFinish((char *)sxt->strAddr, szFileName.c_str(), 2, client);
				}
				catch (...)
				{
					goto Loop;
				}
			}
		}
	}

	memset(Head, 0, 8);
	memcpy(Head, "MSF", 3);
	if (UDT::ERROR == UDT::send(client, (char*)Head, 3, 0))
		goto Loop;

	// goto loop for end
Loop:
	UDT::close(client);
	//log.close();

#ifndef WIN32
	return NULL;
#else
	return 0;
#endif
}


#ifndef WIN32
void * CUdtCore::_RecvThread(void * pParam)
#else
DWORD WINAPI CUdtCore::_RecvThread(LPVOID pParam)
#endif
{
	//fstream log("/mnt/sdcard/UdtLog.txt", ios::out | ios::binary | ios::trunc);
	//log << "***_RecvThread***" << endl;

	CUdtCore * pThis = (CUdtCore *)pParam;

	char Head[8];
	int nReturnCode = 108, nLen = 0, nPos = 0, nCount = 0;
	int64_t nFileTotalSize = 0, nRecvSize = 0, iLastPercent = 0; 
	vector<string> vecFileName;
	string szTmp, strReplyPath = "", szFinish = "NETFAIL", szError = "", szFilePath = "";
	string szHostName, szFileName, szRecdevice, szRectype, szOwndevice, szOwntype, szSendType, szFileType = "";

	LPCLIENTCONTEXT cxt = new CLIENTCONTEXT;
	cxt = pThis->VEC_CXT.back();
	UDTSOCKET fhandle = cxt->sockCtrl;

	while (true)
	{
		memset(Head, 0, 8);
		if (UDT::ERROR == UDT::recv(fhandle, (char *)Head, 3, 0))
			goto Loop;
		//log << "Head:" << Head << endl;
		if (memcmp(Head,"TSR", 3) == 0)	// 1.	recv message response（TSR）
		{
			// recv hostname
			if (UDT::ERROR == UDT::recv(fhandle, (char*)&nLen, sizeof(nLen), 0))
				goto Loop;
			char * pstrHostName = new char[nLen+2];
			if (UDT::ERROR == UDT::recv(fhandle, pstrHostName, nLen, 0))
				goto Loop;
			pstrHostName[nLen] = '\0';

			// recv message
			if (UDT::ERROR == UDT::recv(fhandle, (char*)&nLen, sizeof(nLen), 0))
				goto Loop;
			char * pstrMsg = new char[nLen+2];
			if (UDT::ERROR == UDT::recv(fhandle, pstrMsg, nLen, 0))
				goto Loop;
			pstrMsg[nLen] = '\0';

			// notify to up
			pThis->m_pCallBack->onRecvMessage(pstrMsg, (char*)cxt->strAddr, pstrHostName);
			nReturnCode = 0;
			goto Loop;
		}
		else if (memcmp(Head,"MSR", 3) == 0 || memcmp(Head,"FSR", 3) == 0 || memcmp(Head,"DSR", 3) == 0)
		{
			// recv file total size, and file count
			if (memcmp(Head,"MSR", 3) == 0 || memcmp(Head,"DSR", 3) == 0)
			{
				if (UDT::ERROR == UDT::recv(fhandle, (char*)&nFileTotalSize, sizeof(nFileTotalSize), 0))
					goto Loop;
				if (memcmp(Head,"MSR", 3) == 0)
				{
					if (UDT::ERROR == UDT::recv(fhandle, (char*)&nCount, sizeof(nCount), 0))
						goto Loop;
				}
			}

			cxt->nFileTotalSize = nFileTotalSize;
			cxt->nFileCount = nCount;
			// recv filename hostname sendtype
			if (UDT::ERROR == UDT::recv(fhandle, (char*)&nLen, sizeof(nLen), 0))
				goto Loop;
			char * pstrFileName = new char[nLen+2];
			if (UDT::ERROR == UDT::recv(fhandle, pstrFileName, nLen, 0))
				goto Loop;
			pstrFileName[nLen] = '\0';

			//log << "fileName:" << pstrFileName << endl;
			szTmp = pstrFileName;
			nPos = 0;
			nLen = 0;
			do 
			{
				nPos = szTmp.find_first_of('\\');
				if (nPos >= 0)
				{
					if (nLen == 0)
						szFileName = szTmp.substr(0, nPos);
					else if (nLen == 1)
						szRecdevice = szTmp.substr(0, nPos);
					else if (nLen == 2)
						szRectype = szTmp.substr(0, nPos);
					else if (nLen == 3)
						szOwndevice = szTmp.substr(0, nPos);
					else if (nLen == 4)
						szOwntype = szTmp.substr(0, nPos);
					else if (nLen == 5)
						szSendType = szTmp.substr(0, nPos);
					else if (nLen == 6)
						szFileType = szTmp.substr(0, nPos);

					szTmp = szTmp.substr(nPos+1);
					nLen++;
				}
			} while (nPos >= 0);
			if (szFileType == "")
			{
				char Head[8];
				memset(Head, 0, 8);
				memcpy(Head, "FRR", 3);
				if (UDT::ERROR == UDT::send(fhandle, (char*)Head, 3, 0))
					goto Loop;
				nReturnCode = 112;
				goto Loop;
			}
			else
				pThis->m_pCallBack->onAccept((char*)cxt->strAddr, szFileName.c_str(), nCount, nFileTotalSize,
						szRecdevice.c_str(), szRectype.c_str(), szOwndevice.c_str(), szOwntype.c_str(), szSendType.c_str(), szFileType.c_str(), fhandle);
		}
		else if (memcmp(Head,"DCR",3) == 0)
		{
			if (UDT::ERROR == UDT::recv(fhandle, (char*)&nLen, sizeof(nLen), 0))
				goto Loop;
			char * pstrFileName = new char[nLen+2];
			if (UDT::ERROR == UDT::recv(fhandle, pstrFileName, nLen, 0))
				goto Loop;
			pstrFileName[nLen] = '\0';

			string szFolder = strReplyPath + pstrFileName;
			pThis->CreateDirectroy(szFolder);
		}
		else if (memcmp(Head,"FSC",3) == 0)
		{
			nReturnCode = 101;
			memset(Head, 0, 8);
			memcpy(Head, "FSC", 3);
			if (UDT::ERROR == UDT::send(fhandle, (char*)Head, 3, 0))
				goto Loop;
			goto Loop;
		}
		else if (memcmp(Head,"FSS",3) == 0)
		{
			memset(Head, 0, 8);
			memcpy(Head, "FSS", 3);
			if (UDT::ERROR == UDT::send(fhandle, (char*)Head, 3, 0))
				goto Loop;
			nReturnCode = 105;
		}
		else if (memcmp(Head,"FRR",3) == 0)
		{
			nReturnCode = 102;
			goto Loop;
		}
		else if (memcmp(Head,"FRB",3) == 0)
		{
			nReturnCode = 113;
			goto Loop;
		}
		else if (memcmp(Head,"FRC",3) == 0)
		{
			nReturnCode = 102;
			goto Loop;
		}
		else if (memcmp(Head,"FRE",3) == 0)
		{
			nReturnCode = 106;
			goto Loop;
		}
		else if (memcmp(Head,"FSV",3) == 0)
		{
			nReturnCode = 112;
			goto Loop;
		}
		else if (memcmp(Head,"MSF",3) == 0)
		{
			nReturnCode = 110;
			goto Loop;
		}
		else if (memcmp(Head,"FSE",3) == 0)
		{
			if (nReturnCode != 105)
				nReturnCode = 0;
			goto Loop;
		}
	}

	// goto loop for end
Loop:
	// SUCCESS, FAIL
	if (nReturnCode > 0)
	{
		pThis->m_pCallBack->onFinished("RETURN", nReturnCode, fhandle);
	}

	UDT::close(fhandle);
	//log << "log close" << endl;
	//log.close();

	#ifndef WIN32
		return NULL;
	#else
		return 0;
	#endif
}

#ifndef WIN32
void * CUdtCore::_RecvFiles(void * pParam)
#else
DWORD WINAPI CUdtCore::_RecvFiles(LPVOID pParam)
#endif
{
	CUdtCore * pThis = (CUdtCore *)pParam;

	char Head[8];
	int64_t size;
	int nLen = 0, nPos = 0, nRet = 0, nCount = 0;
	int64_t nFileTotalSize = 0, nRecvSize = 0, iLastPercent = 0;
	vector<string> vecFileName;
	string szSlash = "";
	string szTmp, strReplyPath = "", szFinish = "NETFAIL", szError = "", szFilePath = "", szFolder = "", szRcvFileName = "";
	string szHostName, szFileName, szSendType;

	CLIENTCONTEXT * cxt = new CLIENTCONTEXT;
	cxt = pThis->VEC_CXT.back();

	UDTSOCKET client = cxt->sockFile;
	nFileTotalSize = cxt->nFileTotalSize;
	nCount = cxt->nFileCount;

	while (true)
	{
		memset(Head, 0, 8);
		if (UDT::ERROR == UDT::recv(client, (char *)Head, 3, 0))
			goto Loop;
		if (memcmp(Head, "FCS", 3) == 0 || memcmp(Head, "MCS", 3) == 0 || memcmp(Head, "DFS", 3) == 0)
		{
			if (UDT::ERROR == UDT::recv(client, (char*)&nLen, sizeof(nLen), 0))
				goto Loop;
			char * pstrFileName = new char[nLen+2];
			if (UDT::ERROR == UDT::recv(client, pstrFileName, nLen, 0))
				goto Loop;
			pstrFileName[nLen] = '\0';
			szRcvFileName = pstrFileName;

			// 单个文件上层已经合并路径跟名称
			if (nCount <= 1)
			{
				if (szRcvFileName.find("/") != string::npos ||
					szRcvFileName.find("\\") != string::npos)
				{
					szFilePath = pThis->m_szReplyfilepath + szRcvFileName;
				}
				else
				{
					szFilePath = pThis->m_szReplyfilepath;
				}
			}
			else
			{
				szFilePath = pThis->m_szReplyfilepath + szRcvFileName;
				do 
				{
#ifdef WIN32
					nRet = _access(szFilePath.c_str(), 0);
#else
					nRet = access(szFilePath.c_str(), F_OK);
#endif
					// 文件已经存在、进行重命名
					if (nRet == 0)
					{
						nPos = szFilePath.find_last_of(".");
						if (nPos > 0)
						{
							szFileName = szFilePath.substr(0, nPos);
							szTmp = szFilePath.substr(nPos);
						}
						else
						{
							szFileName = szFilePath;
							szTmp = "";
						}
						nLen = szFileName.size();
						if (szFileName[nLen-1] == ')')
						{
							if (szFileName[nLen-3] == '(' && szFileName[nLen-2] > '0' && szFileName[nLen-2] <= '9')
							{
								if (szFileName[nLen-2] < '9')
								{
									szFileName[nLen-2] = szFileName[nLen-2] + 1;
									szFilePath = szFileName + szTmp;
								}
								else 
								{
									szFilePath = szFileName.substr(0, nPos-2) + "10)" + szTmp;
								}
							}
							else if (szFileName[nLen-4] == '(' && szFileName[nLen-2] >= '0')
							{
								if (szFileName[nLen-2] < '9')
								{
									szFileName[nLen-2] = szFileName[nLen-2] + 1;
									szFilePath = szFileName + szTmp;
								}
								else
								{
									szFileName[nLen-3] = szFileName[nLen-3] + 1;
									szFileName[nLen-2] = '0';
									szFilePath = szFileName + szTmp;
								}
							}
						}
						else
							szFilePath = szFileName + "(1)" + szTmp;
					}
				}while (nRet == 0);
			}

			// 替换正反斜杠
			do 
			{
#ifdef WIN32
				szSlash = '\\';
				szTmp = '/';
#else
				szSlash = '/';
				szTmp = '\\';
#endif
				nPos = szFilePath.find(szTmp, 0);
				if (nPos >= 0)
				{
					szFilePath.replace(nPos, 1, szSlash);
				}
			}while (nPos >= 0);

			if (UDT::ERROR == UDT::recv(client, (char*)&size, sizeof(int64_t), 0))
				goto Loop;

			// receive the file
			fstream ofs(szFilePath.c_str(), ios::out | ios::binary | ios::trunc);
			int64_t offset = 0;
			int64_t left = size;
			while(left > 0)
			{
				UDT::TRACEINFO trace;
				UDT::perfmon(client, &trace);

				int64_t recv = 0;
				if (left > TO_SND)
					recv = UDT::recvfile(client, ofs, offset, TO_SND);
				else
					recv = UDT::recvfile(client, ofs, offset, left);

				if (UDT::ERROR == recv)
					goto Loop;

				left -= recv;
				nRecvSize += recv;

				memset(Head, 0, 8);
				memcpy(Head, "FPR", 3);
				if (UDT::ERROR == UDT::send(cxt->sockCtrl, (char*)Head, 3, 0))
					goto Loop;
				if (UDT::ERROR == UDT::send(cxt->sockCtrl, (char*)&nRecvSize, sizeof(nRecvSize), 0))
					goto Loop;
				pThis->m_pCallBack->onTransfer((long)nFileTotalSize, nRecvSize, trace.mbpsRecvRate / 8, pstrFileName, 1, cxt->sockCtrl);
			}
			ofs.close();
			// 文件夹，只回调一次
			nPos = szRcvFileName.find_first_of('/');
			if (nPos < 0)
			{
				nPos = szRcvFileName.find_first_of("\\");
			}
			if (nPos >= 0)
			{
				szTmp = szRcvFileName.substr(0, nPos);
				if (szTmp != szFolder)
				{
					szFolder = szTmp;
					szTmp = pThis->m_szReplyfilepath + szFolder;
					pThis->m_pCallBack->onAcceptonFinish((char *)cxt->strAddr, szTmp.c_str(), 1, cxt->sockCtrl);
				}
			}
			else
				pThis->m_pCallBack->onAcceptonFinish((char *)cxt->strAddr, szFilePath.c_str(), 1, cxt->sockCtrl);
		}
		else if (memcmp(Head,"DCR",3) == 0)
		{
			if (UDT::ERROR == UDT::recv(client, (char*)&nLen, sizeof(nLen), 0))
				goto Loop;
			char * pstrFileName = new char[nLen+2];
			if (UDT::ERROR == UDT::recv(client, pstrFileName, nLen, 0))
				goto Loop;
			pstrFileName[nLen] = '\0';
			szTmp = pThis->m_szReplyfilepath + pstrFileName;
			pThis->CreateDirectroy(szTmp);
		}
		else if (memcmp(Head,"MSF",3) == 0)
		{
			goto Loop;
		}
	}

	// goto loop for end
Loop:
	UDT::close(client);

#ifndef WIN32
	return NULL;
#else
	return 0;
#endif
}

void CUdtCore::TTSPing(const std::vector<std::string> vecIpAddress)
{
	CGuard::enterCS(m_LockTTS);
	VEC_IP = vecIpAddress;
	CGuard::leaveCS(m_LockTTS);
}

#ifndef WIN32
void * CUdtCore::_WorkThreadProc(void * pParam)
#else
DWORD WINAPI CUdtCore::_WorkThreadProc(LPVOID pParam)
#endif
{
	CUdtCore * pThis = (CUdtCore *)pParam;

	UDTSOCKET client;
	vector<string> vecIP;
	int nLen = 0;
	char Head[8];
	char strPort[32] = "7777";

	while (true)
	{
#ifndef WIN32
		pthread_mutex_lock(&pThis->m_LockTTS);
		timeval now;
		timespec timeout;
		gettimeofday(&now, 0);
		timeout.tv_sec = now.tv_sec + 5;
		timeout.tv_nsec = now.tv_usec * 1000;

		int rc = pthread_cond_timedwait(&pThis->m_CondTTS, &pThis->m_LockTTS, &timeout);
		pthread_mutex_unlock(&pThis->m_LockTTS);
		if (rc != ETIMEDOUT)
		{
			cout << "_ListenThreadProc timeout" << endl;
			break;
		}

#else
		if (WAIT_TIMEOUT != WaitForSingleObject(pThis->m_CondTTS, 5000))
		{
			std::cout << "_ListenThreadProc timeout" << endl;
			break;
		}
#endif

		CGuard::enterCS(pThis->m_LockTTS);
		vecIP = pThis->VEC_IP;
		CGuard::leaveCS(pThis->m_LockTTS);

		for (vector<string>::iterator it = vecIP.begin(); it != vecIP.end(); it++)
		{
			for (int i = 0; i < 3; i++)
			{
				if (pThis->CreateUDTSocket(client, strPort) < 0)
				{
					UDT::close(client);
					continue;
				}

				if (pThis->UDT_Connect(client, it->c_str(), strPort) < 0)
				{
					// wait 1s
#ifndef WIN32
					sleep(1);
#else
					Sleep(1000);
#endif
					if (i == 2)
					{
						pThis->m_pCallBack->onTTSPing(it->c_str(), 1);
						CGuard::enterCS(pThis->m_LockTTS);
						for (vector<string>::iterator it1 = pThis->VEC_IP.begin(); it1 != pThis->VEC_IP.end();)
						{
							if (*it1 == *it)
							{
								it1 = pThis->VEC_IP.erase(it1);
							}
							else
								it1++;
						}
						CGuard::leaveCS(pThis->m_LockTTS);
						break;
					}
				}
				else
				{
					// send flags
					memset(Head, 0, 8);
					memcpy(Head, "HRT", 3);
					if (UDT::ERROR == UDT::send(client, (char*)Head, 3, 0))
					{
						UDT::close(client);
						break;
					}
					// send exit
					memset(Head, 0, 8);
					memcpy(Head, "MSE", 3);
					if (UDT::ERROR == UDT::send(client, (char*)Head, 3, 0))
					{
						UDT::close(client);
						break;
					}
					UDT::close(client);
					break;
				}
			}
		}
	}

#ifndef WIN32
	return NULL;
#else
	return 0;
#endif
}

//////////////////////////////////////////////////////////////////////////
// private method
int CUdtCore::CreateUDTSocket(UDTSOCKET & usock, const char* pstrPort, bool bBind, bool rendezvous)
{
	addrinfo hints, *res;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if (0 != getaddrinfo(NULL, pstrPort, &hints, &res))
	{
		cout << "illegal port number or port is busy.\n" << endl;
		return -1;
	}

	usock = UDT::socket(res->ai_family, res->ai_socktype, res->ai_protocol);

	// server port need to bind and setsockopt
	if (bBind)
	{
		/* Windows UDP issue
		// For better performance, modify HKLM\System\CurrentControlSet\Services\Afd\Parameters\FastSendDatagramThreshold
#ifdef WIN32
		int mss = 1052;
		UDT::setsockopt(usock, 0, UDT_MSS, &mss, sizeof(int));
#endif

#if defined(BSD) || defined(OSX)
		UDT::setsockopt(usock, 0, UDT_SNDBUF, new int(64000), sizeof(int));
		UDT::setsockopt(usock, 0, UDT_RCVBUF, new int(64000), sizeof(int));
		UDT::setsockopt(usock, 0, UDP_SNDBUF, new int(64000), sizeof(int));
		UDT::setsockopt(usock, 0, UDP_RCVBUF, new int(64000), sizeof(int));
#endif*/

		if (UDT::ERROR == UDT::bind(usock, res->ai_addr, res->ai_addrlen))
		{
			cout << "bind: " << UDT::getlasterror().getErrorMessage() << endl;
			return -1;
		}
	}
	freeaddrinfo(res);

	return 0;
}

int CUdtCore::CreateTCPSocket(SYSSOCKET & ssock, const char* pstrPort, bool bBind, bool rendezvous)
{
	addrinfo hints;
	addrinfo* res;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if (0 != getaddrinfo(NULL, pstrPort, &hints, &res))
	{
		cout << "illegal port number or port is busy.\n" << endl;
		return -1;
	}

	ssock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (bind(ssock, res->ai_addr, res->ai_addrlen) != 0)
	{
		return -1;
	}
	freeaddrinfo(res);
	return 0;
}

int CUdtCore::TCP_Connect(SYSSOCKET& ssock, const char* pstrAddr, const char* pstrPort)
{
	addrinfo hints, *peer;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if (0 != getaddrinfo(pstrAddr, pstrPort, &hints, &peer))
	{
		return -1;
	}

	connect(ssock, peer->ai_addr, peer->ai_addrlen);

	freeaddrinfo(peer);
	return 0;
}

int CUdtCore::UDT_Connect(UDTSOCKET & usock, const char* pstrAddr, const char* pstrPort)
{
	addrinfo hints, *peer;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family =  AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if (0 != getaddrinfo(pstrAddr, pstrPort, &hints, &peer))
	{
		return -1;
	}

	// connect to the server, implict bind
	if (UDT::ERROR == UDT::connect(usock, peer->ai_addr, peer->ai_addrlen))
	{
		cout << "connect: " << UDT::getlasterror().getErrorMessage() << endl;
		return -1;
	}
	freeaddrinfo(peer);

	return 0;
}

void CUdtCore::SearchFileInDirectroy(const std::string & szPath, int64_t & nTotalSize, std::vector<std::string> & vecDirName, std::vector<std::string> & vecFileName)
{
	int nPos = 0;
	string szTmp = "";
	char strPath[256];
	memset(strPath, 0, 256);
	sprintf(strPath, "%s", szPath.c_str());

#ifdef WIN32
	WIN32_FIND_DATA sFindFileData = {0};
	HANDLE hFind = INVALID_HANDLE_VALUE;
	hFind = FindFirstFile(strPath, &sFindFileData);
	if (sFindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		vecDirName.push_back(strPath);
		if ('/' == strPath[strlen(strPath)-1])
		{
			strcat_s(strPath, ("/*.*"));
		}
		else
			strcat_s(strPath, ("/*.*"));

		hFind = INVALID_HANDLE_VALUE;
		if (INVALID_HANDLE_VALUE != (hFind = FindFirstFile(strPath, &sFindFileData)))
		{
			do 
			{
				if (sFindFileData.cFileName[0] == '.')
					continue;
				sprintf_s(strPath, "%s/%s", szPath.c_str(), sFindFileData.cFileName);
				if (sFindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					SearchFileInDirectroy(strPath, nTotalSize, vecDirName, vecFileName);
				}
				else
				{
					fstream ifs(strPath, ios::in | ios::binary);
					ifs.seekg(0, ios::end);
					int64_t size = ifs.tellg();
					nTotalSize += size;
					vecFileName.push_back(strPath);
				}
			} while (FindNextFile(hFind, &sFindFileData));

			FindClose(hFind);
			hFind = INVALID_HANDLE_VALUE;
		}
	}
	else
	{
		fstream ifs(strPath, ios::in | ios::binary);
		ifs.seekg(0, ios::end);
		int64_t size = ifs.tellg();
		nTotalSize += size;
		vecFileName.push_back(strPath);

		FindClose(hFind);
		hFind = INVALID_HANDLE_VALUE;
	}
#else
	DIR * pDir;
	if ((pDir = opendir(strPath)) == NULL)
	{
		fstream ifs(strPath, ios::in | ios::binary);
		ifs.seekg(0, ios::end);
		int64_t size = ifs.tellg();
		nTotalSize += size;
		vecFileName.push_back(strPath);
		return ;
	}

	struct stat dirInfo;
	struct dirent * pDT;
	char strFileName[256];
	memset(strFileName, 0, sizeof(strFileName));
	vecDirName.push_back(strPath);
	while ((pDT = readdir(pDir)))
	{
		if (strcmp(pDT->d_name, ".") == 0 || strcmp(pDT->d_name, "..") == 0)
			continue;
		else
		{
			sprintf(strFileName, "%s/%s", strPath, pDT->d_name);
			struct stat buf;
			if (lstat(strFileName, &buf) >= 0 && S_ISDIR(buf.st_mode))
			{
				SearchFileInDirectroy(strFileName, nTotalSize, vecDirName, vecFileName);
			}
			else
			{
				fstream ifs(strFileName, ios::in | ios::binary);
				ifs.seekg(0, ios::end);
				int64_t size = ifs.tellg();
				nTotalSize += size;
				vecFileName.push_back(strFileName);
			}
		}
		memset(strFileName, 0, 256);
	}
	closedir(pDir);
#endif
}

void CUdtCore::CreateDirectroy(const std::string & szPath)
{
	int nLen = 0, nPos = 0;
	string szFolder = "", szSlash = "";
	string szTmpPath = szPath;
	// WIN32路径分隔符换成："\", LINUX：'/'
	do 
	{
#ifdef WIN32
		szSlash = '\\';
		nPos = szTmpPath.find('/', 0);
		if (nPos >= 0)
		{
			szTmpPath.replace(nPos, 1, szSlash);
		}
#else
		szSlash = '/';
		nPos = szTmpPath.find('\\', 0);
		if (nPos >= 0)
		{
			szTmpPath.replace(nPos, 1, szSlash);
		}
#endif
	} while (nPos >= 0);

	if ('\\' != szTmpPath[szTmpPath.size()-1] || '/' != szTmpPath[szTmpPath.size()-1])
	{
		szTmpPath += szSlash;
	}
	string szTmp = szTmpPath;
	while (true)
	{
		int nPos = szTmp.find_first_of(szSlash);
		if (nPos >= 0)
		{
			nLen += nPos;
			szTmp = szTmp.substr(nPos+1);
			szFolder = szTmpPath.substr(0, nLen);
			nPos = szTmp.find_first_of(szSlash);
			if (nPos >= 0)
			{
				nLen = nLen + 1 + nPos;
				szTmp = szTmp.substr(nPos);
				szFolder = szTmpPath.substr(0, nLen);
#ifdef WIN32
				if (_access(szFolder.c_str(), 0))
					_mkdir(szFolder.c_str());
#else
				if (access(szFolder.c_str(), F_OK) != 0)
					mkdir(szFolder.c_str(),770);
#endif
			}
			else
				break;
		}
		else
			break;
	}
}
