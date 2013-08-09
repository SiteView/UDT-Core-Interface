#include "UdtCore.h"


using namespace std;

CUdtCore::CUdtCore(CUDTCallBack * pCallback)
	: m_pCallBack(pCallback)
	, m_bListenStatus(false)
	, m_bSendStatus(false)
	, m_bRecvStatus(false)
{
}


CUdtCore::~CUdtCore()
{
}

int CUdtCore::StartListen(const int nCtrlPort, const int nRcvPort)
{
	UDT::startup();

	if (m_bListenStatus)
		return 0;

	// Init listen ContrlSocket/RecvFileSocket
	if (InitListenSocket(nCtrlPort, m_sockListenCtrlCmd) == 0 && InitListenSocket(nRcvPort, m_sockListenRcvFile) == 0)
	{
		cout << "Local listen ctrl port SUCCESS:" << nCtrlPort << endl;
		cout << "Local listen rcv port SUCCESS:" << nRcvPort << endl;

		// create listen thread for accept client connect
#ifndef WIN32
		pthread_mutex_init(&m_AcceptLock, NULL);
		pthread_cond_init(&m_AcceptCond, NULL);
		pthread_create(&m_hListenThread, NULL, _ListenCtrlCmdThread, this);
		pthread_create(&m_hListenThread, NULL, _ListenRcvFileThread, this);
#else
		DWORD dwThreadID;
		m_AcceptLock = CreateMutex(NULL, false, NULL);
		m_AcceptCond = CreateEvent(NULL, false, false, NULL);
		m_hListenThread = CreateThread(NULL, 0, _ListenCtrlCmdThread, this, NULL, &dwThreadID);
		m_hListenThread = CreateThread(NULL, 0, _ListenRcvFileThread, this, NULL, &dwThreadID);
#endif

		m_bListenStatus = true;
		return 0;
	}

	return -1;
}

void CUdtCore::SendMessage(const char* pstrAddr, const char* pstrMsg, const char* pstrHostName)
{
}

void CUdtCore::SendFile(const char* pstrAddr, const char* pstrOwnDev, const char* pstrOwnType, const char* pstrRcvDev, const char* pstrRcvType, const char* pstrSndType, const std::vector<std::string> vecArray, int nType)
{
	LPSERVERCONTEXT sxt = new SERVERCONTEXT;
	memset(sxt, 0, sizeof(SERVERCONTEXT));
	memcpy(sxt->strAddr, pstrAddr, 32);
	memcpy(sxt->strHostName, pstrOwnDev, 128);
	memcpy(sxt->strSendType, pstrSndType, 128);
	sprintf(sxt->strPort, "%d", 7777);
	sxt->vecArray = vecArray;
	if (nType == 1)
	{
		memcpy(sxt->strXSR, "TSR", 3);
		memcpy(sxt->strXSP, "TSP1", 4);
		memcpy(sxt->strXCS, "TCS", 3);
		memcpy(sxt->strXSF, "TSF", 3);
	}
	else if (nType == 2)
	{
		memcpy(sxt->strXSR, "FSR", 3);
		memcpy(sxt->strXSP, "FSP1", 4);
		memcpy(sxt->strXCS, "FCS", 3);
		memcpy(sxt->strXSF, "FSF", 3);
	}
	else if (nType == 3)
	{
		memcpy(sxt->strXSR, "MSR", 3);
		memcpy(sxt->strXSP, "MSP1", 4);
		memcpy(sxt->strXCS, "MCS", 3);
		memcpy(sxt->strXSF, "MSF", 3);
	}
	else if (nType == 4)
	{
		memcpy(sxt->strXSR, "DSR", 3);
		memcpy(sxt->strXSP, "DSP1", 4);
		memcpy(sxt->strXCS, "DFS", 3);
		memcpy(sxt->strXSF, "DSF", 3);
	}

	// save server context
	VEC_SXT.push_back(sxt);

#ifndef WIN32
	pthread_mutex_init(&m_SendLock, NULL);
	pthread_cond_init(&m_SendCond, NULL);
	pthread_create(&m_hSendThread, NULL, _SendThread, this);
	pthread_detach(m_hSendThread);
#else
	DWORD ThreadID;
	m_SendLock = CreateMutex(NULL, false, NULL);
	m_SendCond = CreateEvent(NULL, false, false, NULL);
	m_hSendThread = CreateThread(NULL, 0, _SendThread, this, NULL, &ThreadID);
#endif
}

void CUdtCore::ReplyAccept(const UDTSOCKET sock, const char* pstrReply)
{
	m_szReplyfilepath = pstrReply;
}

void CUdtCore::StopTransfer(const UDTSOCKET sock, const int nType)
{
	// 1:stop recv; 2:stop send
	if (nType == 1)
	{
		m_bSendStatus = false;
		//UDT::close(m_pClientSocket->sock);
		#ifndef WIN32
			pthread_cond_signal(&m_SendCond);
			pthread_join(m_hSendThread, NULL);
			pthread_mutex_destroy(&m_SendLock);
			pthread_cond_destroy(&m_SendCond);
		#else
			SetEvent(m_SendCond);
			WaitForSingleObject(m_hSendThread, INFINITE);
			CloseHandle(m_hSendThread);
			CloseHandle(m_SendLock);
			CloseHandle(m_SendCond);
		#endif
	}
	else if (nType == 2)
	{
		m_bRecvStatus = false;
		//UDT::close(m_pSrvCxt->sock);
		#ifndef WIN32
			pthread_cond_signal(&m_RecvCond);
			pthread_join(m_hRecvThread, NULL);
			pthread_mutex_destroy(&m_RecvLock);
			pthread_cond_destroy(&m_RecvCond);
		#else
			SetEvent(m_RecvCond);
			WaitForSingleObject(m_hRecvThread, INFINITE);
			CloseHandle(m_hRecvThread);
			CloseHandle(m_RecvLock);
			CloseHandle(m_RecvCond);
		#endif
	}
}

void CUdtCore::StopListen()
{
#ifndef WIN32
	pthread_mutex_lock(&m_AcceptLock);
	pthread_cond_signal(&m_AcceptCond);
	pthread_mutex_unlock(&m_AcceptLock);
#else
	SetEvent(m_AcceptCond);
#endif
}

#ifndef WIN32
void * CUdtCore::_ListenCtrlCmdThread(void * pParam)
#else
DWORD WINAPI CUdtCore::_ListenCtrlCmdThread(LPVOID pParam)
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
		pthread_mutex_lock(&pThis->m_AcceptLock);
		timeval now;
		timespec timeout;
		gettimeofday(&now, 0);
		timeout.tv_sec = now.tv_sec + 1;
		timeout.tv_nsec = now.tv_usec * 1000;

		int rc = pthread_cond_timedwait(&pThis->m_AcceptCond, &pThis->m_AcceptLock, &timeout);
		pthread_mutex_unlock(&pThis->m_AcceptLock);
		if (rc != ETIMEDOUT)
		{
			cout << "_ListenThreadProc timeout" << endl;
			break;
		}

#else
		if (WAIT_TIMEOUT != WaitForSingleObject(pThis->m_AcceptCond, 1000))
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

		memset(cxt, 0, sizeof(cxt));
		sprintf(cxt->strAddr, "%s", clienthost);
		sprintf(cxt->strCtrlPort, "%s", clientservice);
		cxt->sockCtrl = client;
		pThis->VEC_CXT.push_back(cxt);
		cout << "new connection: " << clienthost << ":" << clientservice << endl;

#ifndef WIN32
		pthread_mutex_init(&pThis->m_RecvLock, NULL);
		pthread_cond_init(&pThis->m_RecvCond, NULL);
		pthread_create(&pThis->m_hRecvThread, NULL, _RecvThread, pThis);
		pthread_detach(pThis->m_hRecvThread);
#else
		DWORD ThreadID;
		pThis->m_RecvLock = CreateMutex(NULL, false, NULL);
		pThis->m_RecvCond = CreateEvent(NULL, false, false, NULL);
		pThis->m_hRecvThread = CreateThread(NULL, 0, _RecvThread, pThis, 0, &ThreadID);
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
		pthread_mutex_lock(&pThis->m_AcceptLock);
		timeval now;
		timespec timeout;
		gettimeofday(&now, 0);
		timeout.tv_sec = now.tv_sec + 1;
		timeout.tv_nsec = now.tv_usec * 1000;

		int rc = pthread_cond_timedwait(&pThis->m_AcceptCond, &pThis->m_AcceptLock, &timeout);
		pthread_mutex_unlock(&pThis->m_AcceptLock);
		if (rc != ETIMEDOUT)
		{
			std::cout << "_ListenThreadProc timeout" << endl;
			break;
		}

#else
		if (WAIT_TIMEOUT != WaitForSingleObject(pThis->m_AcceptCond, 1000))
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

		LPCLIENTCONTEXT tmp = new CLIENTCONTEXT;
		for (vector<LPCLIENTCONTEXT>::iterator it = pThis->VEC_CXT.begin(); it != pThis->VEC_CXT.end(); it++)
		{
			tmp = *it;
			if (strcmp(tmp->strAddr, clienthost) == 0)
			{
				//sprintf(cxt->strAddr, "%s", clienthost);
				tmp->sockFile = client;
				sprintf(tmp->strFilePort, "%s", clientservice);
				break;
			}
		}
		std::cout << "new connection: " << clienthost << ":" << clientservice << endl;

#ifndef WIN32
		pthread_mutex_init(&pThis->m_RecvLock, NULL);
		pthread_cond_init(&pThis->m_RecvCond, NULL);
		pthread_create(&pThis->m_hRecvThread, NULL, _RecvFile, pThis);
		pthread_detach(pThis->m_hRecvThread);
#else
		DWORD ThreadID;
		pThis->m_RecvLock = CreateMutex(NULL, false, NULL);
		pThis->m_RecvCond = CreateEvent(NULL, false, false, NULL);
		pThis->m_hRecvThread = CreateThread(NULL, 0, _RecvFile, pThis, 0, &ThreadID);
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
	int nLen = 0, nPos = 0, nFileCount = 0;
	int64_t nFileTotalSize = 0, nSendSize = 0, iLastPercent = 0;
	string szAddr, szPort;
	string szTmp, szFolderName, szFileName, szFilePath, szFinish = "FAIL", szError = "";
	vector<string> vecFileName;
	vector<string> vecDirName;

	LPSERVERCONTEXT sxt = new SERVERCONTEXT;
	sxt = pThis->VEC_SXT.back();
	pThis->VEC_SXT.pop_back();

	// create concurrent UDT sockets
	UDTSOCKET client;
	if (pThis->CreateUDTSocket(client, sxt->strPort) < 0)
		return 0;
	if (pThis->UDT_Connect(client, sxt->strAddr, sxt->strPort) < 0)
		return 0;

	memset(Head, 0, 8);
	memcpy(Head, sxt->strXSR, 3);
	if (UDT::ERROR == UDT::send(client, (char*)Head, 3, 0))
		goto Loop;

	if (memcmp(Head, "TSR", 3) == 0)
	{
		// send text message
		szFileName = sxt->strHostName;
		nLen = szFileName.size();
		if (UDT::ERROR == UDT::send(client, (char*)&nLen, sizeof(int), 0))
			goto Loop;
		if (UDT::ERROR == UDT::send(client, szFileName.c_str(), nLen, 0))
			goto Loop;
		// send message size and information
		int nLen = sxt->vecArray[0].size();
		if (UDT::ERROR == UDT::send(client, (char*)&nLen, sizeof(int), 0))
			goto Loop;
		if (UDT::ERROR == UDT::send(client, sxt->vecArray[0].c_str(), nLen, 0))
			goto Loop;

		szFinish = "";
		goto Loop;
	}
	else if (memcmp(Head, "FSR", 3) == 0 || memcmp(Head, "MSR", 3) == 0)
	{
		vecFileName = sxt->vecArray;
		//for (int i = 0; i < vecFileName.size(); i++)
		for (vector<string>::iterator it = vecFileName.begin(); it != vecFileName.end(); it++)
		{
			fstream ifs(it->c_str(), ios::in | ios::binary);
			ifs.seekg(0, ios::end);
			int64_t size = ifs.tellg();
			nFileTotalSize += size;
			nFileCount++;
			ifs.close();
		}
		if (memcmp(Head, "MSR", 3) == 0)
		{
			// send file total size
			if (UDT::ERROR == UDT::send(client, (char*)&nFileTotalSize, sizeof(nFileTotalSize), 0))
				goto Loop;
			// send file count
			if (UDT::ERROR == UDT::send(client, (char*)&nFileCount, sizeof(nFileCount), 0))
				goto Loop;
		}
	}
	else if (memcmp(Head, "DSR", 3) == 0)
	{
		szTmp = sxt->vecArray[0];
		if ('\\' == szTmp[szTmp.size()-1] || '/' == szTmp[szTmp.size()-1])
			sxt->vecArray[0] = szTmp.substr(0, szTmp.size()-1);
		else
			sxt->vecArray[0] = szTmp;

		szFolderName = sxt->vecArray[0];
		pThis->SearchFileInDirectroy(szFolderName, nFileTotalSize, vecDirName, vecFileName);
		// send file total size
		if (UDT::ERROR == UDT::send(client, (char*)&nFileTotalSize, sizeof(nFileTotalSize), 0))
			goto Loop;
	}

	// send file name,(filename\hostname\sendtype)
	szFilePath = sxt->vecArray[0];
	nPos = szFilePath.find_last_of('/');
	if (nPos < 0)
	{
		nPos = szFilePath.find_last_of("\\");
	}
	szFileName = szFilePath.substr(nPos+1);
	// add hostname and sendtype
	szTmp = szFileName + "\\" + sxt->strHostName;
	if (0 == strcmp("GENIETURBO", sxt->strSendType))
	{
		szTmp += "\\";
		szTmp += sxt->strSendType;
	}
	// send name information of the requested file
	nLen = szTmp.size();
	if (UDT::ERROR == UDT::send(client, (char*)&nLen, sizeof(int), 0))
		goto Loop;
	if (UDT::ERROR == UDT::send(client, szTmp.c_str(), nLen, 0))
		goto Loop;

	// recv accept response
	memset(Head, 0, 8);
	if (UDT::ERROR == UDT::recv(client, (char*)Head, 4, 0))
		goto Loop;
	if (memcmp(Head, sxt->strXSP, 4) != 0)
	{
		szFinish = "REJECT";
		goto Loop;
	}

	memset(sxt->strPort, 0, sizeof(sxt->strPort));
	sprintf(sxt->strPort, "%s", "7778");

#ifndef WIN32
	pthread_mutex_init(&pThis->m_RecvLock, NULL);
	pthread_cond_init(&pThis->m_RecvCond, NULL);
	pthread_create(&pThis->m_hRecvThread, NULL, _SendFile, sxt);
	pthread_detach(pThis->m_hRecvThread);
#else
	DWORD ThreadID;
	pThis->m_RecvLock = CreateMutex(NULL, false, NULL);
	pThis->m_RecvCond = CreateEvent(NULL, false, false, NULL);
	pThis->m_hRecvThread = CreateThread(NULL, 0, _SendFile, sxt, 0, &ThreadID);
#endif

	while (true)
	{
		memset(Head, 0, 8);
		if (UDT::ERROR == UDT::recv(client, (char*)Head, 4, 0))
			goto Loop;
		if (memcmp(Head, "FSF", 3) == 0)
		{
			memset(Head, 0, 8);
			memcpy(Head, "FSF", 3);
			if (UDT::ERROR == UDT::send(client, (char*)Head, 3, 0))
				goto Loop;
			szFinish = "SUCCESS";
			goto Loop;
		}
		else if (memcmp(Head, "FRP", 3) == 0)
		{
			if (UDT::ERROR == UDT::recv(client, (char*)&nSendSize, sizeof(nSendSize), 0))
				goto Loop;
			pThis->m_pCallBack->onSendTransfer(nFileTotalSize, nSendSize, szFileName.c_str());
		}
	}

	// goto loop for end
Loop:
	if (!szFinish.empty())
	{
		pThis->m_pCallBack->onSendFinished(szFinish.c_str());
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
void * CUdtCore::_SendFile(void * pParam)
#else
DWORD WINAPI CUdtCore::_SendFile(LPVOID pParam)
#endif
{
	LPSERVERCONTEXT cxt = (LPSERVERCONTEXT)pParam;

	char Head[8];
	int nLen = 0, nPos = 0, nFileCount = 0;
	int64_t nFileTotalSize = 0, nSendSize = 0, iLastPercent = 0;
	string szTmp, szFolderName, szFileName, szFilePath, szFinish = "FAIL", szError = "";
	vector<string> vecFileName;
	vector<string> vecDirName;

	addrinfo hints, *res;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	if (0 != getaddrinfo(NULL, cxt->strPort, &hints, &res))
	{
		cout << "illegal port number or port is busy.\n" << endl;
		return NULL;
	}
	UDTSOCKET client = UDT::socket(res->ai_family, res->ai_socktype, res->ai_protocol);

	// UDT Options
	//UDT::setsockopt(client, 0, UDT_CC, new CCCFactory<CUDPBlast>, sizeof(CCCFactory<CUDPBlast>));
	//UDT::setsockopt(client, 0, UDT_MSS, new int(9000), sizeof(int));
	//UDT::setsockopt(client, 0, UDT_SNDBUF, new int(10000000), sizeof(int));
	//UDT::setsockopt(client, 0, UDP_SNDBUF, new int(10000000), sizeof(int));
	//UDT::setsockopt(client, 0, UDT_MAXBW, new int64_t(12500000), sizeof(int));

	// Windows UDP issue
	// For better performance, modify HKLM\System\CurrentControlSet\Services\Afd\Parameters\FastSendDatagramThreshold
//#ifdef WIN32
	UDT::setsockopt(client, 0, UDT_MSS, new int(1052), sizeof(int));
//#endif

	freeaddrinfo(res);
	if (0 != getaddrinfo(cxt->strAddr, cxt->strPort, &hints, &res))
	{
		cout << "incorrect server/peer address. " << cxt->strAddr << ":" << cxt->strPort << endl;
		return NULL;
	}
	// connect to the server, implict bind
	if (UDT::ERROR == UDT::connect(client, res->ai_addr, res->ai_addrlen))
	{
		cout << "connect: " << UDT::getlasterror().getErrorMessage() << endl;
		return NULL;
	}
	freeaddrinfo(res);

	vecFileName = cxt->vecArray;
	for (vector<string>::iterator it = vecFileName.begin(); it != vecFileName.end(); it++)	
	{
		// send file tage
		memset(Head, 0, 8);
		memcpy(Head, "FCS", 3);
		if (UDT::ERROR == UDT::send(client, (char*)Head, 3, 0))
			goto Loop;

		szFilePath = *it;
		if (memcmp(cxt->strXCS, "DFS", 3) == 0)
		{
			nLen = szFolderName.size() - szFileName.size();
			szTmp = szFilePath.substr(nLen);
		}
		else
		{
			nPos = szFilePath.find_last_of('/');
			if (nPos < 0)
			{
				nPos = szFilePath.find_last_of("\\");
			}
			szTmp = szFilePath.substr(nPos+1);
		}

		// send filename size and filename
		int nLen = szTmp.size();
		if (UDT::ERROR == UDT::send(client, (char*)&nLen, sizeof(int), 0))
			goto Loop;
		if (UDT::ERROR == UDT::send(client, szTmp.c_str(), nLen, 0))
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
				if (left > 204800)
					send = UDT::sendfile(client, ifs, nOffset, 204800);
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
		}
		catch (...)
		{
			goto Loop;
		}
	}

	memset(Head, 0, 8);
	memcpy(Head, "FSF", 3);
	if (UDT::ERROR == UDT::send(client, (char*)Head, 3, 0))
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
	fstream log("/mnt/sdcard/UdtLog.txt", ios::out | ios::binary | ios::trunc);
	log << "***_RecvThread***" << endl;

	CUdtCore * pThis = (CUdtCore *)pParam;

	char Head[8];
	int nLen = 0, nPos = 0, nCount = 0;
	int64_t nFileTotalSize = 0, nRecvSize = 0, iLastPercent = 0; 
	vector<string> vecFileName;
	string szTmp, strReplyPath = "", szFinish = "FAIL", szError = "", szFilePath = "";
	string szHostName, szFileName, szSendType;

	log << "1" << endl;
	LPCLIENTCONTEXT cxt = new CLIENTCONTEXT;
	cxt = pThis->VEC_CXT.back();
	log << "2" << endl;
	UDTSOCKET fhandle = cxt->sockCtrl;
	log << "fhandle" << fhandle << endl;

	while (true)
	{
		log << "recv head!" << endl;
		memset(Head, 0, 8);
		if (UDT::ERROR == UDT::recv(fhandle, (char *)Head, 3, 0))
			goto Loop;
		log << "recv head:" << Head << endl;
		if (memcmp(Head,"TSR",3) == 0)	// 1.	recv message response£¨TSR£©
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
			pThis->m_pCallBack->onRecvMessage((char*)cxt->strAddr, pstrHostName, pstrMsg);

			szFinish = "";
			goto Loop;
		}
		else if (memcmp(Head,"FSR",3) == 0 || memcmp(Head,"MSR",3) == 0 || memcmp(Head,"DSR",3) == 0)
		{
			if (memcmp(Head, "MSR", 3) == 0 || memcmp(Head, "DSR", 3) == 0)
			{
				// recv file total size, and file count
				if (UDT::ERROR == UDT::recv(fhandle, (char*)&nFileTotalSize, sizeof(nFileTotalSize), 0))
					goto Loop;
				if (memcmp(Head, "MSR", 3) == 0)
				{
					if (UDT::ERROR == UDT::recv(fhandle, (char*)&nCount, sizeof(nCount), 0))
						goto Loop;
				}
			}
			// recv filename hostname sendtype
			if (UDT::ERROR == UDT::recv(fhandle, (char*)&nLen, sizeof(nLen), 0))
				goto Loop;
			char * pstrFileName = new char[nLen+2];
			if (UDT::ERROR == UDT::recv(fhandle, pstrFileName, nLen, 0))
				goto Loop;
			pstrFileName[nLen] = '\0';

			szTmp = pstrFileName;
			int nPos = szTmp.find_last_of('\\');
			if (nPos > 0)
			{
				int iFirstPos = szTmp.find_first_of('\\');
				if (nPos == iFirstPos)
				{
					szHostName = szTmp.substr(nPos+1);
					szFileName = szTmp.substr(0,nPos);
				}
				else
				{
					szHostName = szTmp.substr(iFirstPos+1,nPos-iFirstPos-1);
					szFileName = szTmp.substr(0,iFirstPos);
					szSendType = szTmp.substr(nPos+1);
				}
			}
			else
				goto Loop;

			log << "onAccept file name:" << szFileName << endl;
			pThis->m_szReplyfilepath = "";
			if (memcmp(Head, "FSR", 3) == 0)
			{
				pThis->m_pCallBack->onAccept((char*)cxt->strAddr, szHostName.c_str(), szSendType.c_str(), szFileName.c_str(), 1);
			}
			else
			{
				pThis->m_pCallBack->onAcceptFolder((char*)cxt->strAddr, szHostName.c_str(), szSendType.c_str(), szFileName.c_str(), nCount);
			}

			int nWaitTime = 0;
			while(nWaitTime < 31)
			{
				CGuard::enterCS(pThis->m_RecvLock);
				strReplyPath = pThis->m_szReplyfilepath;
				CGuard::leaveCS(pThis->m_RecvLock);
				if (!strReplyPath.empty())
				{
					cout << "getReply:" << strReplyPath << endl;
					break;
				}

				nWaitTime += 1;
#ifndef WIN32
				sleep(1);
#else
				Sleep(1000);
#endif
			}

			// file transfer response(FSP0/MSP0/DSP0)
			Head[2] = 'P';
			if (strReplyPath.compare("REJECT")==0)
				Head[3] = '0';
			else if (strReplyPath.compare("REJECTBUSY") == 0 || strReplyPath.empty())
				Head[3] = '2';
			else
				Head[3] = '1';

			log << "response:" << Head << endl;
			if (UDT::ERROR == UDT::send(fhandle, (char*)Head, 4, 0))
				goto Loop;

			if (Head[3] != '1')
			{
				szFinish = "";
				goto Loop;
			}
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
		else if (memcmp(Head,"FSF",3) == 0 || memcmp(Head,"DSF",3) == 0)
		{
			pThis->VEC_CXT.clear();
			//pThis->m_pCallBack->onAcceptonFinish((char*)pThis->m_pClientSocket->strAddr, vecFileName);
			szFinish = "SUCCESS";
			goto Loop;
		}
	}

	// goto loop for end
Loop:
	// SUCCESS, FAIL
	if (!szFinish.empty())
	{
		pThis->m_pCallBack->onRecvFinished(szFinish.c_str());
	}

	UDT::close(fhandle);
	log.close();

	#ifndef WIN32
		return NULL;
	#else
		return 0;
	#endif
}

#ifndef WIN32
void * CUdtCore::_RecvFile(void * pParam)
#else
DWORD WINAPI CUdtCore::_RecvFile(LPVOID pParam)
#endif
{
	fstream log("/mnt/sdcard/UdtLog.txt", ios::out | ios::binary | ios::trunc);
	log << "***_RecvFile***" << endl;

	CUdtCore * pThis = (CUdtCore *)pParam;

	char Head[8];
	int64_t size;
	int nLen = 0, nPos = 0, nCount = 0;
	int64_t nFileTotalSize = 0, nRecvSize = 0, iLastPercent = 0; 
	vector<string> vecFileName;
	string szSlash = "";
	string szTmp, strReplyPath = "", szFinish = "FAIL", szError = "", szFilePath = "";
	string szHostName, szFileName, szSendType;

	LPCLIENTCONTEXT cxt = new CLIENTCONTEXT;
	for (vector<LPCLIENTCONTEXT>::iterator it = pThis->VEC_CXT.begin(); it != pThis->VEC_CXT.end(); it++)
	{
		cxt = *it;
		if (cxt->sockFile > 0)
		{
			pThis->VEC_CXT.pop_back();
			break;
		}
	}
	UDTSOCKET & client = cxt->sockFile;

	while (true)
	{
		log << "recv head" << endl;
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
			vecFileName.push_back(szFileName);
		}
		else if (memcmp(Head,"DCR",3) == 0)
		{
			if (UDT::ERROR == UDT::recv(client, (char*)&nLen, sizeof(nLen), 0))
				goto Loop;
			char * pstrFileName = new char[nLen+2];
			if (UDT::ERROR == UDT::recv(client, pstrFileName, nLen, 0))
				goto Loop;
			pstrFileName[nLen] = '\0';

			string szFolder = strReplyPath + pstrFileName;
			pThis->CreateDirectroy(szFolder);
		}
		else if (memcmp(Head,"FSF",3) == 0)
		{
			memset(Head, 0, 8);
			memcpy(Head, "FSF", 3);
			if (UDT::ERROR == UDT::send(cxt->sockCtrl, (char*)Head, 3, 0))
				goto Loop;
			pThis->m_pCallBack->onAcceptonFinish((char*)cxt->strAddr, vecFileName);
			//szFinish = "SUCCESS";
			goto Loop;
		}

		szFilePath = pThis->m_szReplyfilepath + szFileName;

		do 
		{
#ifdef WIN32
			szSlash = '\\';
			nPos = szFilePath.find('/', 0);
			if (nPos >= 0)
			{
				szFilePath.replace(nPos, 1, szSlash);
			}
#else
			szSlash = '/';
			nPos = szFilePath.find('\\', 0);
			if (nPos >= 0)
			{
				szFilePath.replace(nPos, 1, szSlash);
			}
#endif
		}while (nPos >= 0);

		if (UDT::ERROR == UDT::recv(client, (char*)&size, sizeof(int64_t), 0))
			goto Loop;
		if (memcmp(Head, "FCS", 3) == 0)
			nFileTotalSize = size;

		log << "FilePath:" << szFilePath << endl;

		// receive the file
		fstream ofs(szFilePath.c_str(), ios::out | ios::binary | ios::trunc);
		int64_t offset = 0;
		int64_t left = size;
		while(left > 0)
		{
			int64_t recv = 0;
			if (left > 204800)
				recv = UDT::recvfile(client, ofs, offset, 204800);
			else
				recv = UDT::recvfile(client, ofs, offset, left);

			if (UDT::ERROR == recv)
				goto Loop;

			left -= recv;
			nRecvSize += recv;
			memset(Head, 0, 8);
			memcpy(Head, "FRP", 3);
			if (UDT::ERROR == UDT::send(cxt->sockCtrl, (char*)Head, 3, 0))
				goto Loop;
			if (UDT::ERROR == UDT::send(cxt->sockCtrl, (char*)&nRecvSize, sizeof(nRecvSize), 0))
				goto Loop;

			pThis->m_pCallBack->onRecvTransfer((long)nFileTotalSize, nRecvSize, szFileName.c_str());
		}
		ofs.close();
	}

	// goto loop for end
Loop:
	UDT::close(client);
	log.close();

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

	UDT::connect(usock, peer->ai_addr, peer->ai_addrlen);

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

void CUdtCore::ProcessCMD(const UDTSOCKET & sock, const char* pstrCMD)
{

}

void CUdtCore::ProcessAccept(CLIENTCONTEXT & cxt)
{
	char clienthost[NI_MAXHOST];
	char clientservice[NI_MAXSERV];
	getnameinfo((sockaddr *)&cxt.clientaddr, sizeof(cxt.clientaddr), clienthost, sizeof(clienthost), clientservice, sizeof(clientservice), NI_NUMERICHOST|NI_NUMERICSERV);

//	sprintf(cxt.strAddr, "%s", clienthost);
//	sprintf(cxt.strPort, "%s", clientservice);
//	VEC_CXT.push_back(cxt);
//	UDT::epoll_add_usock(m_eid, cxt.sock);

	cout << "new connection: " << clienthost << ":" << clientservice << endl;
}

void CUdtCore::ProcessSendFile(SERVERCONTEXT & sxt)
{

}

void CUdtCore::ProcessRecvFile(CLIENTCONTEXT & cxt)
{

}

void CUdtCore::SearchFileInDirectroy(const std::string & szPath, int64_t & nTotalSize, std::vector<std::string> & vecDirName, std::vector<std::string> & vecFileName)
{
	char strPath[256] = {0};
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
	// WIN32Â·¾¶·Ö¸ô·û»»³É£º"\", LINUX£º'/'
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
