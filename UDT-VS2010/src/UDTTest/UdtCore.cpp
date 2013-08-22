#include "UdtCore.h"


using namespace std;

CUdtCore::CUdtCore(CUDTCallBack * pCallback)
	: m_pCallBack(pCallback)
	, m_bListenStatus(false)
	, m_bSendStatus(false)
	, m_bRecvStatus(false)
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
#else
	m_Lock				= CreateMutex(NULL, false, NULL);
	m_LockLis			= CreateMutex(NULL, false, NULL);
	m_LockSnd		= CreateMutex(NULL, false, NULL);
	m_LockRcv		= CreateMutex(NULL, false, NULL);
	m_CondLisCtrl	= CreateEvent(NULL, false, false, NULL);
	m_CondLisFile	= CreateEvent(NULL, false, false, NULL);
	m_CondSnd		= CreateEvent(NULL, false, false, NULL);
	m_CondRcv		= CreateEvent(NULL, false, false, NULL);
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
#else
	CloseHandle(m_Lock);
	CloseHandle(m_LockLis);
	CloseHandle(m_LockSnd);
	CloseHandle(m_LockRcv);
	CloseHandle(m_CondLisCtrl);
	CloseHandle(m_CondLisFile);
	CloseHandle(m_CondSnd);
	CloseHandle(m_CondRcv);
#endif
}

int CUdtCore::StartListen(const int nCtrlPort, const int nFilePort)
{
	UDT::startup();

	if (m_bListenStatus)
		return 0;

	m_nCtrlPort = nCtrlPort;
	m_nFilePort = nFilePort;
	// Init listen ContrlSocket/RecvFileSocket
	if (InitListenSocket(nCtrlPort, m_sockListenCtrlCmd) == 0 && InitListenSocket(nFilePort, m_sockListenRcvFile) == 0)
	{
		cout << "Local listen CtrlPort SUCCESS:" << nCtrlPort << endl;
		cout << "Local listen FilePort SUCCESS:" << nFilePort << endl;

		// create listen thread for accept client connect
#ifndef WIN32
		pthread_create(&m_hThrLisCtrl, NULL, _ListenRcvCtrlThread, this);
		pthread_create(&m_hThrLisFile, NULL, _ListenRcvFileThread, this);
#else
		DWORD dwThreadID;
		m_hThrLisCtrl = CreateThread(NULL, 0, _ListenRcvCtrlThread, this, NULL, &dwThreadID);
		m_hThrLisFile = CreateThread(NULL, 0, _ListenRcvFileThread, this, NULL, &dwThreadID);
#endif

		m_bListenStatus = true;
		return 0;
	}

	return -1;
}

int CUdtCore::SendMsg(const char* pstrAddr, const char* pstrMsg, const char* pstrHostName)
{
	int nLen = 0, nReturnCode = 108;
	char Head[8];

	// create concurrent UDT sockets
	UDTSOCKET client;
	char strPort[32];
	sprintf(strPort, "%d", m_nCtrlPort);
	if (CreateUDTSocket(client, strPort) < 0)
		goto Loop;
	if (UDT_Connect(client, pstrAddr, strPort) < 0)
		goto Loop;

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
	// create concurrent UDT sockets
	UDT::startup();
	UDTSOCKET client;
	int nReturnCode = 108;
	char strCtrlPort[32];
	char strFilePort[32];
	sprintf(strCtrlPort, "%d", 7777);
	sprintf(strFilePort, "%d", 7778);

	if (CreateUDTSocket(client, strCtrlPort) < 0)
	{
		m_pCallBack->onFinished("RETURN", nReturnCode, client);
		return 0;
	}
	if (UDT_Connect(client, pstrAddr, strCtrlPort) < 0)
	{
		m_pCallBack->onFinished("NETFAIL", nReturnCode, client);
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
	sxt->sockCtrl = client;
	sxt->vecFile = vecFiles;
	sxt->bSendFile = false;

	pthread_t hHandle;
#ifndef WIN32
	pthread_mutex_init(&sxt->lock, NULL);
	pthread_cond_init(&sxt->cond, NULL);
	VEC_SXT.push_back(sxt);
	pthread_create(&hHandle, NULL, _SendThread, this);
	pthread_detach(hHandle);
#else
	DWORD ID;
	sxt->lock = CreateMutex(NULL, false, NULL);
	sxt->cond = CreateEvent(NULL, false, false, NULL);
	VEC_SXT.push_back(sxt);
	hHandle = CreateThread(NULL, 0, _SendThread, this, 0, &ID);
#endif
	CGuard::leaveCS(m_Lock);

	return client;
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
//#ifndef WIN32
//			pthread_cond_signal(&tmp->cond);
//			pthread_join(tmp->hThread, NULL);
//			pthread_mutex_destroy(&tmp->lock);
//			pthread_cond_destroy(&tmp->cond);
//#else
//			SetEvent(tmp->cond);
//			WaitForSingleObject(tmp->hThread, INFINITE);
//#endif
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
//#ifndef WIN32
//			pthread_cond_signal(&sxt->cond);
//			pthread_join(sxt->hThread, NULL);
//			pthread_mutex_destroy(&sxt->lock);
//			pthread_cond_destroy(&sxt->cond);
//#else
//			SetEvent(sxt->cond);
//			WaitForSingleObject(sxt->hThread, INFINITE);
//#endif
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
#else
	SetEvent(m_CondLisCtrl);
	SetEvent(m_CondLisFile);
#endif
}

#ifndef WIN32
void * CUdtCore::_ListenRcvCtrlThread(void * pParam)
#else
DWORD WINAPI CUdtCore::_ListenRcvCtrlThread(LPVOID pParam)
#endif
{
	CUdtCore * pThis = (CUdtCore *)pParam;

	sockaddr_storage clientaddr;
	int addrlen = sizeof(clientaddr);
	UDTSOCKET client;
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

		if (UDT::INVALID_SOCK == (client = UDT::accept(pThis->m_sockListenCtrlCmd, (sockaddr*)&clientaddr, &addrlen)))
		{
			std::cout << "accept: " << UDT::getlasterror().getErrorMessage() << endl;
			return 0;
		}
		char clienthost[NI_MAXHOST];
		char clientservice[NI_MAXSERV];
		getnameinfo((sockaddr *)&clientaddr, sizeof(clientaddr), clienthost, sizeof(clienthost), clientservice, sizeof(clientservice), NI_NUMERICHOST|NI_NUMERICSERV);
		cout << "new connection: " << clienthost << ":" << clientservice << endl;
		memset(cxt, 0, sizeof(cxt));
		sprintf(cxt->strAddr, "%s", clienthost);
		sprintf(cxt->strCtrlPort, "%s", clientservice);
		cxt->sockCtrl = client;

		pthread_t hHandle;
#ifndef WIN32
		pthread_mutex_init(&cxt->lock, NULL);
		pthread_cond_init(&cxt->cond, NULL);
		pThis->VEC_CXT.push_back(cxt);
		pthread_create(&hHandle, NULL, _RecvThread, pThis);
		pthread_detach(hHandle);
#else
		DWORD ID;
		cxt->lock = CreateMutex(NULL, false, NULL);
		cxt->cond = CreateEvent(NULL, false, false, NULL);
		pThis->VEC_CXT.push_back(cxt);
		hHandle = CreateThread(NULL, 0, _RecvThread, pThis, 0, &ID);
#endif
	}

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

	sockaddr_storage clientaddr;
	int addrlen = sizeof(clientaddr);
	UDTSOCKET client;
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

		if (UDT::INVALID_SOCK == (client = UDT::accept(pThis->m_sockListenRcvFile, (sockaddr*)&clientaddr, &addrlen)))
		{
			std::cout << "accept: " << UDT::getlasterror().getErrorMessage() << endl;
			return 0;
		}
		char clienthost[NI_MAXHOST];
		char clientservice[NI_MAXSERV];
		getnameinfo((sockaddr *)&clientaddr, sizeof(clientaddr), clienthost, sizeof(clienthost), clientservice, sizeof(clientservice), NI_NUMERICHOST|NI_NUMERICSERV);
		std::cout << "new connection: " << clienthost << ":" << clientservice << endl;
		for (vector<LPCLIENTCONTEXT>::iterator it = pThis->VEC_CXT.begin(); it != pThis->VEC_CXT.end();)
		{
			if ((*it)->sockCtrl < 0)
			{
				it = pThis->VEC_CXT.erase(it);
			}
			else
			{
				if (strcmp((*it)->strAddr, clienthost) == 0)
				{
					(*it)->sockFile = client;
					sprintf((*it)->strFilePort, "%s", clientservice);
				}
				it++;
			}
		}

		pthread_t hHandle;
#ifndef WIN32
		pthread_create(&hHandle, NULL, _RecvFiles, pThis);
		pthread_detach(hHandle);
#else
		DWORD ID;
		hHandle = CreateThread(NULL, 0, _RecvFiles, pThis, 0, &ID);
#endif
	}

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

	char Head[8];
	int nReturnCode = 108, nLen = 0, nPos = 0, nFileCount = 0;
	int64_t nFileTotalSize = 0, nSendSize = 0, nLastSendSize = 0;
	string szAddr, szPort;
	string szTmp, szFolderName, szFileName, szFilePath, szFinish = "NETFAIL", szError = "";
	vector<string> vecFileName;
	vector<string> vecDirName;
	vector<string> vecTmp;

	LPSERVERCONTEXT sxt = new SERVERCONTEXT;
	sxt = pThis->VEC_SXT.back();
	UDTSOCKET client = sxt->sockCtrl;

	// send flags
	memset(Head, 0, 8);
	memcpy(Head, "FSR", 3);
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
	szTmp = szFileName + "\\" + sxt->ownDev + "\\" + sxt->ownType + "\\" + sxt->recvDev + "\\" + sxt->recvType + "\\" + sxt->sendType + "\\";
	nLen = szTmp.size();
	if (UDT::ERROR == UDT::send(client, (char*)&nLen, sizeof(int), 0))
		goto Loop;
	if (UDT::ERROR == UDT::send(client, szTmp.c_str(), nLen, 0))
		goto Loop;

	while (true)
	{
		/*
#ifndef WIN32
		pthread_mutex_lock(&sxt->lock);
		timeval now;
		timespec timeout;
		gettimeofday(&now, 0);
		timeout.tv_sec = now.tv_sec + 1;
		timeout.tv_nsec = now.tv_usec * 1000;
		int rc = pthread_cond_timedwait(&sxt->cond, &sxt->lock, &timeout);
		pthread_mutex_unlock(&sxt->lock);
		if (rc != ETIMEDOUT)
		{
			szFinish = "STOP";
			break;
		}
#else
		if (WAIT_TIMEOUT != WaitForSingleObject(sxt->cond, 1000))
		{
			std::cout << "_SendThread timeout" << endl;
			szFinish = "STOP";
			break;
		}
#endif	*/

		memset(Head, 0, 8);
		if (UDT::ERROR == UDT::recv(client, (char*)Head, 3, 0))
			goto Loop;
		if (memcmp(Head, "FRA", 3) == 0)
		{
			sxt->bSendFile = true;
			pthread_t hHandle;
#ifndef WIN32
			pthread_create(&hHandle, NULL, _SendFiles, pThis);
			pthread_detach(hHandle);
#else
			hHandle = CreateThread(NULL, 0, _SendFiles, pThis, 0, NULL);
#endif
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
				pThis->m_pCallBack->onTransfer(nFileTotalSize, nLastSendSize, szFileName.c_str(), 2, client);
				if (nSendSize == nFileTotalSize)
				{
					memset(Head, 0, 8);
					memcpy(Head, "FSF", 3);
					if (UDT::ERROR == UDT::send(client, (char*)Head, 3, 0))
						goto Loop;
					nReturnCode = 109;
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
	delete sxt;

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

	char Head[8];
	int nLen = 0, nPos = 0, nFileCount = 0;
	int64_t nFileTotalSize = 0, nSendSize = 0, iLastPercent = 0;
	string szTmp, szFolder, szAccept, szFileName, szFilePath, szFinish = "NETFAIL", szError = "";
	vector<string> vecFile;
	vector<string> vecFilePath;
	vector<string> vecDirName;

	LPSERVERCONTEXT sxt = new SERVERCONTEXT;
	sxt = pThis->VEC_SXT.back();
	vecFile = sxt->vecFile;
	vecFilePath = sxt->vecFiles;

	UDTSOCKET client = -1;
	addrinfo hints, *res;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	if (0 != getaddrinfo(NULL, sxt->strFilePort, &hints, &res))
	{
		cout << "illegal port number or port is busy.\n" << endl;
		goto Loop;
	}
	client = UDT::socket(res->ai_family, res->ai_socktype, res->ai_protocol);

	// UDT Options
	//UDT::setsockopt(client, 0, UDT_CC, new CCCFactory<CUDPBlast>, sizeof(CCCFactory<CUDPBlast>));
	//UDT::setsockopt(client, 0, UDT_MSS, new int(9000), sizeof(int));
	//UDT::setsockopt(client, 0, UDT_SNDBUF, new int(10000000), sizeof(int));
	//UDT::setsockopt(client, 0, UDP_SNDBUF, new int(10000000), sizeof(int));
	//UDT::setsockopt(client, 0, UDT_MAXBW, new int64_t(12500000), sizeof(int));

	// Windows UDP issue
	// For better performance, modify HKLM\System\CurrentControlSet\Services\Afd\Parameters\FastSendDatagramThreshold
#ifdef WIN32
	UDT::setsockopt(client, 0, UDT_MSS, new int(1052), sizeof(int));
#endif

	freeaddrinfo(res);
	if (0 != getaddrinfo(sxt->strAddr, sxt->strFilePort, &hints, &res))
	{
		cout << "incorrect server/peer address. " << sxt->strAddr << ":" << sxt->strFilePort << endl;
		goto Loop;
	}
	// connect to the server, implict bind
	if (UDT::ERROR == UDT::connect(client, res->ai_addr, res->ai_addrlen))
	{
		cout << "connect: " << UDT::getlasterror().getErrorMessage() << endl;
		goto Loop;
	}
	freeaddrinfo(res);

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
				memcpy(Head, "FCS", 3);
				if (UDT::ERROR == UDT::send(client, (char*)Head, 3, 0))
					goto Loop;
				nLen = szFileName.size();
				if (UDT::ERROR == UDT::send(client, (char*)&nLen, sizeof(int), 0))
					goto Loop;
				if (UDT::ERROR == UDT::send(client, szFileName.c_str(), nLen, 0))
					goto Loop;

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

					while (true)
					{
						int64_t send = 0;
						if (left > TO_SND)
							send = UDT::sendfile(client, ifs, nOffset, TO_SND);
						else
							send = UDT::sendfile(client, ifs, nOffset, left);

						if (UDT::ERROR == send)
							goto Loop;
						left -= send;
						nSendSize += send;
						if (left <= 0)
							break;
					}
					ifs.close();
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
							pThis->m_pCallBack->onAcceptonFinish((char *)sxt->strAddr, szAccept.c_str(), 2, client);
						}
					}
					else
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
	memcpy(Head, "FSF", 3);
	if (UDT::ERROR == UDT::send(client, (char*)Head, 3, 0))
		goto Loop;
	// after recv FRF, then to return
	memset(Head, 0, 8);
	if (UDT::ERROR == UDT::recv(client, (char *)Head, 3, 0))
		goto Loop;

	// goto loop for end
Loop:
	UDT::close(client);

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
	//log << "***_RecvThreadProc***" << endl;

	CUdtCore * pThis = (CUdtCore *)pParam;

	char Head[8];
	int nReturnCode = 108, nLen = 0, nPos = 0, nCount = 0;
	int64_t nFileTotalSize = 0, nRecvSize = 0, iLastPercent = 0; 
	vector<string> vecFileName;
	string szTmp, strReplyPath = "", szFinish = "NETFAIL", szError = "", szFilePath = "";
	string szHostName, szFileName, szRecdevice, szRectype, szOwndevice, szOwntype, szSendType;

	LPCLIENTCONTEXT cxt = new CLIENTCONTEXT;
	cxt = pThis->VEC_CXT.back();
	UDTSOCKET fhandle = cxt->sockCtrl;

	while (true)
	{
		/*
#ifndef WIN32
		pthread_mutex_lock(&cxt->lock);
		timeval now;
		timespec timeout;
		gettimeofday(&now, 0);
		timeout.tv_sec = now.tv_sec + 1;
		timeout.tv_nsec = now.tv_usec * 1000;
		int rc = pthread_cond_timedwait(&cxt->cond, &cxt->lock, &timeout);
		pthread_mutex_unlock(&cxt->lock);
		if (rc != ETIMEDOUT)
		{
			szFinish = "STOP";
			goto Loop;
		}
#else
		if (WAIT_TIMEOUT != WaitForSingleObject(cxt->cond, 1000))
		{
			std::cout << "_RecvThread timeout" << endl;
			szFinish = "STOP";
			goto Loop;
		}
#endif	*/
		
		memset(Head, 0, 8);
		if (UDT::ERROR == UDT::recv(fhandle, (char *)Head, 3, 0))
			goto Loop;
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
		else if (memcmp(Head,"FSR", 3) == 0)
		{
			// recv file total size, and file count
			if (UDT::ERROR == UDT::recv(fhandle, (char*)&nFileTotalSize, sizeof(nFileTotalSize), 0))
				goto Loop;
			if (UDT::ERROR == UDT::recv(fhandle, (char*)&nCount, sizeof(nCount), 0))
				goto Loop;
			cxt->nFileTotalSize = nFileTotalSize;
			// recv filename hostname sendtype
			if (UDT::ERROR == UDT::recv(fhandle, (char*)&nLen, sizeof(nLen), 0))
				goto Loop;
			char * pstrFileName = new char[nLen+2];
			if (UDT::ERROR == UDT::recv(fhandle, pstrFileName, nLen, 0))
				goto Loop;
			pstrFileName[nLen] = '\0';

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

					szTmp = szTmp.substr(nPos+1);
					nLen++;
				}
			} while (nPos >= 0);
			pThis->m_pCallBack->onAccept((char*)cxt->strAddr, szFileName.c_str(), nCount, szRecdevice.c_str(), szRectype.c_str(), szOwndevice.c_str(), szOwntype.c_str(), szSendType.c_str(), fhandle);
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
		else if (memcmp(Head,"FSE",3) == 0)
		{
			nReturnCode = 105;
			goto Loop;
		}
		else if (memcmp(Head,"FSV",3) == 0)
		{
			nReturnCode = 112;
			goto Loop;
		}
		else if (memcmp(Head,"FSF",3) == 0 || memcmp(Head,"DSF",3) == 0)
		{
			nReturnCode = 109;
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
	string szTmp, strReplyPath = "", szFinish = "NETFAIL", szError = "", szFilePath = "", szFolder = "";
	string szHostName, szFileName, szSendType;

	LPCLIENTCONTEXT cxt = new CLIENTCONTEXT;
	for (vector<LPCLIENTCONTEXT>::iterator it = pThis->VEC_CXT.begin(); it != pThis->VEC_CXT.end(); it++)
	{
		cxt = *it;
		if (cxt->sockFile > 0)
		{
			break;
		}
	}
	UDTSOCKET client = cxt->sockFile;
	nFileTotalSize = cxt->nFileTotalSize;

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
			szFileName = pstrFileName;
			nCount++;
			vecFileName.push_back(szFileName);
			// 是否被重命名
			nPos = pThis->m_szReplyfilepath.find_last_of(".");
			if (nPos >= 0 && nCount <= 1)
			{
				szFilePath = pThis->m_szReplyfilepath;
			}
			else
			{
				szFilePath = pThis->m_szReplyfilepath + szFileName;
				// 对文件进行重命名
				do 
				{
#ifdef WIN32
					nRet = _access(szFilePath.c_str(), 0);
#else
					nRet = access(szFilePath.c_str(), F_OK);
#endif
					if (nRet == 0)
					{
						nPos = szFilePath.find_last_of(".");
						if (nPos >= 0)
						{
							szFileName = szFilePath.substr(0, nPos);
							szTmp = szFilePath.substr(nPos);
							nLen = szFileName.size();
							if (szFileName[nLen-1] == ')')
							{
								if (szFileName[nLen-3] == '(' && szFileName[nLen-2] > '0')
								{
									szFileName[nLen-2] = szFileName[nLen-2] + 1;
									szFilePath = szFileName + szTmp;
								}
								else if (szFileName[nLen-4] == '(' && szFileName[nLen-2] > '0')
								{
									szFileName[nLen-2] = szFileName[nLen-2] + 1;
									szFilePath = szFileName + szTmp;
								}
							}
							else
								szFilePath = szFileName + "(1)" + szTmp;
						}
						else
							break;
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
				pThis->m_pCallBack->onTransfer((long)nFileTotalSize, nRecvSize, szFileName.c_str(), 1, client);
			}
			ofs.close();
			// 文件夹，只回调一次
			nPos = szFileName.find_first_of('/');
			if (nPos < 0)
			{
				nPos = szFileName.find_first_of("\\");
			}
			if (nPos >= 0)
			{
				szTmp = szFileName.substr(0, nPos);
				if (szTmp != szFolder)
				{
					szFolder = szTmp;
					pThis->m_pCallBack->onAcceptonFinish((char *)cxt->strAddr, szFolder.c_str(), 1, client);
				}
			}
			else
				pThis->m_pCallBack->onAcceptonFinish((char *)cxt->strAddr, szFileName.c_str(), 1, client);
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
		else if (memcmp(Head,"FSF",3) == 0)
		{
			memset(Head, 0, 8);
			memcpy(Head, "FRF", 3);
			if (UDT::ERROR == UDT::send(client, (char*)Head, 3, 0))
				goto Loop;
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


//////////////////////////////////////////////////////////////////////////
// private method
int CUdtCore::CreateUDTSocket(UDTSOCKET & usock, const char* pstrPort, bool rendezvous)
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

	// since we will start a lot of connections, we set the buffer size to smaller value.
	//int snd_buf = 16000;
	//int rcv_buf = 16000;
	//UDT::setsockopt(usock, 0, UDT_SNDBUF, &snd_buf, sizeof(int));
	//UDT::setsockopt(usock, 0, UDT_RCVBUF, &rcv_buf, sizeof(int));
	//snd_buf = 8192;
	//rcv_buf = 8192;
	//UDT::setsockopt(usock, 0, UDP_SNDBUF, &snd_buf, sizeof(int));
	//UDT::setsockopt(usock, 0, UDP_RCVBUF, &rcv_buf, sizeof(int));
	//int fc = 16;
	//UDT::setsockopt(usock, 0, UDT_FC, &fc, sizeof(int));
	//bool reuse = true;
	//UDT::setsockopt(usock, 0, UDT_REUSEADDR, &reuse, sizeof(bool));
	//UDT::setsockopt(usock, 0, UDT_RENDEZVOUS, &rendezvous, sizeof(bool));

	if (rendezvous)
	{
		if (UDT::ERROR == UDT::bind(usock, res->ai_addr, res->ai_addrlen))
		{
			cout << "bind: " << UDT::getlasterror().getErrorMessage() << endl;
			return -1;
		}
	}
	freeaddrinfo(res);
	return 0;
}

int CUdtCore::CreateTCPSocket(SYSSOCKET & ssock, const char* pstrPort, bool rendezvous)
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

int CUdtCore::InitListenSocket(const int nPort, UDTSOCKET & sockListen)
{
	addrinfo hints, *res;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	char service[16];
	sprintf(service, "%d", nPort);
	if (0 != getaddrinfo(NULL, service, &hints, &res))
	{
		cout << "illegal port number or port is busy.\n" << endl;
		return -1;
	}

	// create socket
	sockListen = UDT::socket(res->ai_family, res->ai_socktype, res->ai_protocol);

	// UDT Options
	//UDT::setsockopt(sockListen, 0, UDT_CC, new CCCFactory<CUDPBlast>, sizeof(CCCFactory<CUDPBlast>));
	//UDT::setsockopt(sockListen, 0, UDT_MSS, new int(9000), sizeof(int));
	//UDT::setsockopt(sockListen, 0, UDT_RCVBUF, new int(10000000), sizeof(int));
	//UDT::setsockopt(sockListen, 0, UDP_RCVBUF, new int(10000000), sizeof(int));

	// bind socket
	if (UDT::ERROR == UDT::bind(sockListen, res->ai_addr, res->ai_addrlen))
	{
		cout << "bind: " << UDT::getlasterror().getErrorMessage() << endl;
		return -1;
	}
	freeaddrinfo(res);
	// listen socket
	if (UDT::ERROR == UDT::listen(sockListen, 10))
	{
		cout << "listen fail: " << UDT::getlasterror().getErrorMessage() << endl;
		return -1;
	}

	return 0;
}

void CUdtCore::SearchFileInDirectroy(const std::string & szPath, int64_t & nTotalSize, std::vector<std::string> & vecDirName, std::vector<std::string> & vecFileName)
{
	int nPos = 0;
	string szTmp = "";
	char strPath[256];
	memset(strPath, 0, 256);
	memcpy(strPath, szPath.c_str(), 256);

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