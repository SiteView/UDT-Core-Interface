#include "UdtCore.h"

using namespace std;

CUdtCore::CUdtCore(CUDTCallBack * pCallback)
	: m_pCallBack(pCallback)
	, m_bListenStatus(false)
	, m_nCtrlPort(7777)
	, m_nFilePort(7778)
{
#ifndef WIN32
	pthread_mutex_init(&m_Lock, NULL);
#else
	m_Lock				= CreateMutex(NULL, false, NULL);
#endif
}


CUdtCore::~CUdtCore()
{
#ifndef WIN32
	pthread_mutex_destroy(&m_Lock);
#else
	CloseHandle(m_Lock);
#endif
}


int CUdtCore::StartListen(const int nCtrlPort, const int nFilePort)
{
	UDT::startup();

	if (m_bListenStatus)
		return 0;

	if (!VEC_LISTEN.empty())
		VEC_LISTEN.clear();

	m_bListenStatus = true;
	m_nCtrlPort = nCtrlPort;
	m_nFilePort = nFilePort;

	// Create listen Control thread function
	LISTENSOCKET * pCtrl = new LISTENSOCKET();
	char strPort[32];
	memset(strPort, 0, 32);
	sprintf(strPort, "%d", m_nCtrlPort);
	pCtrl->strServerPort = strPort;
	pCtrl->bListen = false;
	pCtrl->Type = OP_RCV_CTRL;
	pCtrl->pThis = this;
	VEC_LISTEN.push_back(pCtrl);
#ifndef WIN32
	pthread_t hCHandle;
	pthread_create(&hCHandle, NULL, _ListenThreadProc, pCtrl);
	pthread_detach(hCHand);
#else
	HANDLE hCHand = CreateThread(NULL, 0, _ListenThreadProc, pCtrl, 0, NULL);
#endif

	// Create listen file thread function
	LISTENSOCKET * pFile = new LISTENSOCKET();
	memset(strPort, 0, 32);
	sprintf(strPort, "%d", nFilePort);
	pFile->strServerPort = strPort;
	pFile->bListen = false;
	pFile->Type = OP_RCV_FILE;
	pFile->pThis = this;
	VEC_LISTEN.push_back(pFile);
#ifndef WIN32
	pthread_t hFHandle;
	pthread_create(&hFHand, NULL, _ListenThreadProc, pFile);
	pthread_detach(hFHand);
#else
	HANDLE hFHand = CreateThread(NULL, 0, _ListenThreadProc, pFile, 0, NULL);
#endif

	return 0;
}

int CUdtCore::SendMsg(const char* pstrAddr, const char* pstrMsg, const char* pstrHostName)
{
	UDT::startup();
	int nLen = 0, nReturnCode = 108;
	char Head[8];
	string szMsg;

#ifdef WIN32
	char* pstr = WIN32_WC2CU(WIN32_C2WC(pstrMsg));
	szMsg = pstr;
#else
	szMsg = pstrMsg;
#endif

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
	nLen = szMsg.size();
	if (UDT::ERROR == UDT::send(client, (char*)&nLen, sizeof(int), 0))
		goto Loop;
	if (UDT::ERROR == UDT::send(client, szMsg.c_str(), nLen, 0))
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

	int64_t nFileTotalSize = 0;
	string szTmp;
	vector<_FileInfo> info;
	vector<string> vecDirs;
	for (std::vector<std::string>::const_iterator it = vecFiles.begin(); it != vecFiles.end(); it++)
	{
		szTmp = *it;
		if ('\\' == szTmp[szTmp.size()-1] || '/' == szTmp[szTmp.size()-1])
			szTmp = szTmp.substr(0, szTmp.size()-1);
		UdtFile::ListDir(szTmp.c_str(), nFileTotalSize, info, vecDirs);
	}
	if (nFileTotalSize <= 0)
	{
		m_pCallBack->onFinished("File size error!", 115, 0);
		return 0;
	}

	UDT::startup();
	UDTSOCKET client;
	char strCtrlPort[32];
	char strFilePort[32];
	//sprintf(strCtrlPort, "%d", m_nCtrlPort);
	//sprintf(strFilePort, "%d", m_nFilePort);

	sprintf(strCtrlPort, "%d", 7777);
	sprintf(strFilePort, "%d", 7778);

	// connect to CtrlPort
	if (CreateUDTSocket(client, strCtrlPort) < 0)
	{
		m_pCallBack->onFinished("Create fail!", 108, client);
		return 0;
	}
	if (UDT_Connect(client, pstrAddr, strCtrlPort) < 0)
	{
		m_pCallBack->onFinished("Connect fail!", 108, client);
		return 0;
	}
	CLIENTCONEXT * pCtrl = new CLIENTCONEXT;
	pCtrl->strServerAddr = pstrAddr;
	pCtrl->strServerPort = strCtrlPort;
	pCtrl->ownDev = owndevice;
	pCtrl->ownType = owntype;
	pCtrl->recvDev = recdevice;
	pCtrl->recvType = rectype;
	pCtrl->sendType = pstrSendtype;
	pCtrl->vecFiles = vecFiles;
	pCtrl->fileInfo = info;
	pCtrl->nFileTotalSize = nFileTotalSize;
	pCtrl->nFileCount = vecFiles.size();
	pCtrl->sockAccept = client;
	pCtrl->nCtrlFileGroup = client;
	pCtrl->bTransfer = false;
	pCtrl->pThis = this;
	pCtrl->Type = OP_SND_CTRL;
	VEC_CLIENT.push_back(pCtrl);
#ifndef WIN32
	pthread_t hHandle;
	pthread_create(&hHandle, NULL,  _WorkThreadProc, pCtrl);
	pthread_detach(hHandle);
#else
	HANDLE hHandle = CreateThread(NULL, 0, _WorkThreadProc, pCtrl, 0, NULL);
#endif

	return client;
}


void CUdtCore::ReplyAccept(const UDTSOCKET sock, const char* pstrReply)
{
	CGuard::enterCS(m_Lock);
	for (vector<PCLIENTCONEXT>::iterator it = VEC_CLIENT.begin(); it != VEC_CLIENT.end(); it++)
	{
		if ((*it)->sockAccept == sock)
		{
			char Head[8];
			memset(Head, 0, 8);
			if (memcmp("REJECTBUSY", pstrReply, 10) == 0 || strlen(pstrReply) <= 0)
			{
				it = VEC_CLIENT.erase(it);
				memcpy(Head, "FRB", 3);
			}
			else if (memcmp("REJECT", pstrReply, 6) == 0)
			{
				it = VEC_CLIENT.erase(it);
				memcpy(Head, "FRR", 3);
			}
			else
			{
				(*it)->fileSavePath = pstrReply;
				memcpy(Head, "FRA", 3);
			}

			if (UDT::ERROR == UDT::send(sock, (char*)Head, 3, 0))
				return ;
			break;
		}
	}
	CGuard::leaveCS(m_Lock);
}

void CUdtCore::StopTransfer(const UDTSOCKET sock, const int nType)
{
	CGuard::enterCS(m_Lock);
	for (vector<PCLIENTCONEXT>::iterator it = VEC_CLIENT.begin(); it != VEC_CLIENT.end(); it++)
	{
		if ((*it)->sockAccept == sock)
		{
			char Head[8];
			if ((*it)->Type == OP_RCV_CTRL)
			{
				memset(Head, 0, 8);
				memcpy(Head, "FRS", 3);
				if (UDT::ERROR == UDT::send(sock, (char*)Head, 3, 0))
					return ;
			}
			else if ((*it)->Type == OP_SND_CTRL)
			{
				memset(Head, 0, 8);
				if ((*it)->fileName.empty())
					memcpy(Head, "FSC", 3);
				else
					memcpy(Head, "FSS", 3);
				if (UDT::ERROR == UDT::send(sock, (char*)Head, 3, 0))
					return ;
			}
			(*it)->bTransfer = false;
			break;
		}
	}
	CGuard::leaveCS(m_Lock);
}

void CUdtCore::StopListen()
{
	for (vector<PLISTENSOCKET>::iterator iter = VEC_LISTEN.begin(); iter != VEC_LISTEN.end(); iter++)
	{
		UDT::close((*iter)->sockListen);
	}
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
		// Windows UDP issue
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
#endif

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


void CUdtCore::ProcessAccept(LISTENSOCKET * cxt)
{
	// create USTSOCKET
	if (!cxt->bListen)
	{
		if (CreateUDTSocket(cxt->sockListen, cxt->strServerPort.c_str(), true) < 0)
		{
			m_pCallBack->onFinished("Fail create socket!", 108, 0);
			return ;
		}
		// listen socket
		if (UDT::ERROR == UDT::listen(cxt->sockListen, 10))
		{
			m_pCallBack->onFinished("Fail listen socket!", 108, 0);
			UDT::close(cxt->sockListen);
			return ;
		}
		cout << "Successful listening port: " << cxt->strServerPort << endl;
		cxt->bListen = true;
	}

	// accept port
	UDTSOCKET sockAccept = -1;
	//struct sockaddr_storage clientaddr;
	struct sockaddr_in clientaddr;
	int addrlen = sizeof(clientaddr);
	if (UDT::INVALID_SOCK == (sockAccept = UDT::accept(cxt->sockListen, (sockaddr*)&clientaddr, &addrlen)))
	{
		cout << "Fail accept port : " << cxt->strServerPort << ", ErrorMsg:" <<UDT::getlasterror().getErrorMessage() << endl;
		cxt->bListen = false;
		return ;
	}
	// get client sockaddr info
	char clienthost[NI_MAXHOST];
	char clientservice[NI_MAXSERV];
	//getnameinfo((sockaddr *)&clientaddr, sizeof(clientaddr), clienthost, sizeof(clienthost), clientservice, sizeof(clientservice), NI_NUMERICHOST|NI_NUMERICSERV);
	sprintf(clienthost, "%s", inet_ntoa(clientaddr.sin_addr));
	sprintf(clientservice, "%d", clientaddr.sin_port);
	cout << "Accept port:" << cxt->strServerPort << ", New client connect:" << clienthost << ", Port:" << clientservice << endl;

	CLIENTCONEXT * pData = new CLIENTCONEXT;
	pData->strClientAddr = clienthost;
	pData->strClientPort = clientservice;
	pData->sockAccept = sockAccept;
	pData->pThis = cxt->pThis;
	pData->Type = cxt->Type;
	// control port push to VEC_CLIENT
	if (cxt->Type == OP_RCV_CTRL)
	{
		CGuard::enterCS(m_Lock);
		VEC_CLIENT.push_back(pData);
		CGuard::leaveCS(m_Lock);
	}

#ifndef WIN32
	pthread_t hHandle;
	pthread_create(&hHandle, NULL, _WorkThreadProc, pData);
	pthread_detach(hHandle);
#else
	HANDLE hHandle = CreateThread(NULL, 0, _WorkThreadProc, pData, 0, NULL);
#endif
}


int CUdtCore::ProcessSendCtrl(CLIENTCONEXT * cxt)
{
	char Head[8];
	int nReturnCode = 108, nLen = 0, nPos = 0, nFileCount = 0;
	int64_t nFileTotalSize = 0, nSendSize = 0, nLastSendSize = 0;
	string szTmp, szFolderName, szFileName, szFilePath, szFinish = "NETFAIL", szError = "";
	UDTSOCKET client = cxt->sockAccept;
	nFileCount = cxt->nFileCount;
	nFileTotalSize = cxt->nFileTotalSize;
	szFilePath = cxt->vecFiles[0];

	// send flags
	memset(Head, 0, 8);
	memcpy(Head, "MSR", 3);
	if (UDT::ERROR == UDT::send(client, (char*)Head, 3, 0))
		return 108;
	// send file total size
	if (UDT::ERROR == UDT::send(client, (char*)&cxt->nFileTotalSize, sizeof(nFileTotalSize), 0))
		return 108;
	// send file count
	if (UDT::ERROR == UDT::send(client, (char*)&cxt->nFileCount, sizeof(nFileCount), 0))
		return 108;

	// send file name,(filename\hostname\sendtype)
	nPos = szFilePath.find_last_of('/');
	if (nPos < 0)
	{
		nPos = szFilePath.find_last_of("\\");
	}
	szFileName = szFilePath.substr(nPos+1);
	szTmp = szFileName + "\\" + cxt->ownDev + "\\" + cxt->ownType + "\\" + cxt->recvDev + "\\" + cxt->recvType + "\\" + cxt->sendType;
	if (cxt->vecFiles.size() < cxt->fileInfo.size())
		szTmp = szTmp + "\\D\\";
	else
		szTmp = szTmp + "\\F\\";
#ifdef WIN32
	szTmp = WIN32_WC2CU(WIN32_C2WC(szTmp.c_str()));
#endif
	nLen = szTmp.size();
	if (UDT::ERROR == UDT::send(client, (char*)&nLen, sizeof(int), 0))
		return 108;
	if (UDT::ERROR == UDT::send(client, szTmp.c_str(), nLen, 0))
		return 108;

	while (true)
	{
		memset(Head, 0, 8);
		if (UDT::ERROR == UDT::recv(client, (char*)Head, 3, 0))
			return 108;
		if (memcmp(Head, "FRA", 3) == 0)
		{
			cxt->bTransfer = true;
			UDTSOCKET sockFile;
			if (CreateUDTSocket(sockFile, "7778") < 0)
			{
				return 108;
			}
			if (UDT_Connect(sockFile, cxt->strServerAddr.c_str(), "7778") < 0)
			{
				UDT::close(sockFile);
				return 108;
			}
			CLIENTCONEXT * pFile = new CLIENTCONEXT();
			pFile->strServerAddr = cxt->strServerAddr;
			pFile->strServerPort = "7778";
			pFile->ownDev = cxt->ownDev;
			pFile->ownType = cxt->ownType;
			pFile->recvDev = cxt->recvDev;
			pFile->recvType = cxt->recvType;
			pFile->sendType = cxt->sendType;
			pFile->vecFiles = cxt->vecFiles;
			pFile->fileInfo = cxt->fileInfo;
			pFile->sockListen = cxt->sockAccept;
			pFile->sockAccept = sockFile;
			pFile->nCtrlFileGroup = cxt->sockAccept;
			pFile->bTransfer = false;
			pFile->pThis = this;
			pFile->Type = OP_SND_FILE;
#ifndef WIN32
			pthread_t hHandle;
			pthread_create(&hHandle, NULL, _WorkThreadProc, pFile);
			pthread_detach(hHandle);
#else
			HANDLE hHandle = CreateThread(NULL, 0, _WorkThreadProc, pFile, 0, NULL);
#endif
		}
		else if (memcmp(Head, "FRR", 3) == 0)
		{
			memset(Head, 0, 8);
			memcpy(Head, "FRR", 3);
			if (UDT::ERROR == UDT::send(client, (char*)Head, 3, 0))
				return 108;
			return 103;
		}
		else if (memcmp(Head, "FRC", 3) == 0)
		{
			memset(Head, 0, 8);
			memcpy(Head, "FRC", 3);
			if (UDT::ERROR == UDT::send(client, (char*)Head, 3, 0))
				return 108;
			return 103;
		}
		else if (memcmp(Head, "FRB", 3) == 0)
		{
			memset(Head, 0, 8);
			memcpy(Head, "FRB", 3);
			if (UDT::ERROR == UDT::send(client, (char*)Head, 3, 0))
				return 108;
			return 114;
		}
		else if (memcmp(Head, "FSC", 3) == 0)
		{
			return 100;
		}
		else if (memcmp(Head, "FRF", 3) == 0)
		{
			memset(Head, 0, 8);
			memcpy(Head, "FSF", 3);
			if (UDT::ERROR == UDT::send(client, (char*)Head, 3, 0))
				return 108;
			return 109;
		}
		else if (memcmp(Head, "FRS", 3) == 0)
		{
			memset(Head, 0, 8);
			memcpy(Head, "FRE", 3);
			if (UDT::ERROR == UDT::send(client, (char*)Head, 3, 0))
				return 108;
			return 107;
		}
		else if (memcmp(Head, "FSS", 3) == 0)
		{
			memset(Head, 0, 8);
			memcpy(Head, "FSE", 3);
			if (UDT::ERROR == UDT::send(client, (char*)Head, 3, 0))
				return 108;
			return 104;
		}
		else if (memcmp(Head, "FPR", 3) == 0)
		{
			if (UDT::ERROR == UDT::recv(client, (char*)&nSendSize, sizeof(nSendSize), 0))
				return 108;
			if (nSendSize <= nFileTotalSize && nSendSize > nLastSendSize)
			{
				nLastSendSize = nSendSize;
				CGuard::enterCS(m_Lock);
				// with recv file size from send size
				int64_t nTmp = 0;
				for (vector<_FileInfo>::iterator it = cxt->fileInfo.begin(); it != cxt->fileInfo.end(); it++)
				{
					nTmp += (*it).nFileSize;
					if (nSendSize <= nTmp)
					{
						szTmp = (*it).asciPath;
						break;
					}
				}
				if (cxt->bTransfer)
				{
					m_pCallBack->onTransfer(nFileTotalSize, nLastSendSize, 0.0, szTmp.c_str(), 2, client);
				}
				CGuard::leaveCS(m_Lock);
				if (nSendSize == nFileTotalSize)
				{
					memset(Head, 0, 8);
					memcpy(Head, "MSF", 3);
					if (UDT::ERROR == UDT::send(client, (char*)Head, 3, 0))
						return 108;
					return 109;
				}
			}
		}
		else if (memcmp(Head, "MSP", 3) == 0)
		{
			memset(Head, 0, 8);
			memcpy(Head, "FSV", 3);
			if (UDT::ERROR == UDT::send(client, (char*)Head, 3, 0))
				return 108;
			return 111;
		}
	}

	return 0;
}

int CUdtCore::ProcessSendFile(CLIENTCONEXT * cxt)
{
	char Head[8];
	int nLen = 0, nPos = 0, nFileCount = 0;
	int64_t nFileTotalSize = 0, nFileSize = 0, nSendSize = 0;
	string szTmp, szFolder, szAccept, szFileName, szFilePath, szFinish = "NETFAIL", szError = "";
	vector<string> vecFile;
	vector<string> vecFilePath;
	vector<_FileInfo> info;

	vecFile = cxt->vecFiles;
	info = cxt->fileInfo;
	
	UDTSOCKET client = cxt->sockAccept;

	// send file
	for (vector<string>::iterator it2 = vecFile.begin(); it2 != vecFile.end(); it2++)
	{
		for (vector<_FileInfo>::iterator it3 = info.begin(); it3 != info.end(); it3++)
		{
			szFilePath = it3->asciPath;
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
				nFileSize = it3->nFileSize;

				// send folder name
				if (szFolder.size() > 0)
				{
					if ('\\' == szFolder[szFolder.size()-1] || '/' == szFolder[szFolder.size()-1])
						szFolder = szFolder.substr(0, szFolder.size()-1);

#ifdef WIN32
					szTmp = WIN32_WC2CU(WIN32_C2WC(szFolder.c_str()));
#else
					szTmp = szFolder;
#endif
					nLen = szTmp.size();
					memset(Head, 0, 8);
					memcpy(Head, "DCR", 3);
					if (UDT::ERROR == UDT::send(client, (char*)Head, 3, 0))
						return 108;
					if (UDT::ERROR == UDT::send(client, (char*)&nLen, sizeof(int), 0))
						return 108;
					if (UDT::ERROR == UDT::send(client, szTmp.c_str(), nLen, 0))
						return 108;
				}

				// send filename size and filename
				memset(Head, 0, 8);
				memcpy(Head, "MCS", 3);
				if (UDT::ERROR == UDT::send(client, (char*)Head, 3, 0))
					return 108;
#ifdef WIN32
				szTmp = WIN32_WC2CU(WIN32_C2WC(szFileName.c_str()));
#else
				szTmp = szFileName;
#endif
				nLen = szTmp.size();
				if (UDT::ERROR == UDT::send(client, (char*)&nLen, sizeof(int), 0))
					return 108;
				if (UDT::ERROR == UDT::send(client, szTmp.c_str(), nLen, 0))
					return 108;

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

				CGuard::enterCS(m_Lock);
				cxt->fileName = szFileName;
				CGuard::leaveCS(m_Lock);

				// open the file
				try
				{
					fstream ifs(szFilePath.c_str(), ios::binary | ios::in);
					int64_t nOffset = 0, left = nFileSize;

					if (UDT::ERROR == UDT::send(client, (char*)&nFileSize, sizeof(nFileSize), 0))
						return 108;

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
							return 108;

						left -= send;
						nSendSize += send;

						CGuard::enterCS(m_Lock);
						cxt->iProgress = trace.mbpsSendRate / 8;
						CGuard::leaveCS(m_Lock);
					}
					ifs.close();
					m_pCallBack->onAcceptonFinish(cxt->strClientAddr.c_str(), szFileName.c_str(), 2, client);
				}
				catch (...)
				{
					return 108;
				}
			}
		}
	}

	memset(Head, 0, 8);
	memcpy(Head, "MSF", 3);
	if (UDT::ERROR == UDT::send(client, (char*)Head, 3, 0))
		return 108;

	return 0;
}

int CUdtCore::ProcessRecvFile(CLIENTCONEXT * cxt)
{
	char Head[8];
	int nReturnCode = 108, nLen = 0, nRet = 0, nPos = 0, nCount = 0;
	int64_t nFileSize = 0, nFileTotalSize = 0, nRecvSize = 0, iLastPercent = 0; 
	string szTmp, szTmp2, strReplyPath = "", szFinish = "NETFAIL", szSlash = "", szFilePath = "", szReFileName = "", szFolder = "";
	string szHostName, szFileName, szRecdevice, szRectype, szOwndevice, szOwntype, szSendType, szFileType;

	UDTSOCKET sock = cxt->sockAccept;
	UDTSOCKET sockCtrl = -1;

	while (true)
	{
		memset(Head, 0, 8);
		if (UDT::ERROR == UDT::recv(sock, (char *)Head, 3, 0))
			return 108;
		if (memcmp(Head,"TSR", 3) == 0)	// 1.	recv message response（TSR）
		{
			// recv hostname
			if (UDT::ERROR == UDT::recv(sock, (char*)&nLen, sizeof(nLen), 0))
				return 108;
			char * pstrHostName = new char[nLen+2];
			if (UDT::ERROR == UDT::recv(sock, pstrHostName, nLen, 0))
				return 108;
			pstrHostName[nLen] = '\0';

			// recv message
			if (UDT::ERROR == UDT::recv(sock, (char*)&nLen, sizeof(nLen), 0))
				return 108;
			char * pstrMsg = new char[nLen+2];
			if (UDT::ERROR == UDT::recv(sock, pstrMsg, nLen, 0))
				return 108;
			pstrMsg[nLen] = '\0';
#ifdef WIN32
			szTmp = WIN32_WC2C(WIN32_CU2WC(pstrMsg));
#else
			szTmp = pstrMsg;
#endif

			// notify to up
			m_pCallBack->onRecvMessage(szTmp.c_str(), cxt->strClientAddr.c_str(), pstrHostName);
			return 0;
		}
		else if (memcmp(Head,"MSR", 3) == 0 || memcmp(Head,"FSR", 3) == 0 || memcmp(Head,"DSR", 3) == 0)
		{
			// recv file total size, and file count
			if (memcmp(Head,"MSR", 3) == 0 || memcmp(Head,"DSR", 3) == 0)
			{
				if (UDT::ERROR == UDT::recv(sock, (char*)&nFileTotalSize, sizeof(nFileTotalSize), 0))
					return 108;
				if (memcmp(Head,"MSR", 3) == 0)
				{
					if (UDT::ERROR == UDT::recv(sock, (char*)&nCount, sizeof(nCount), 0))
						return 108;
				}
			}
			// recv filename hostname sendtype
			if (UDT::ERROR == UDT::recv(sock, (char*)&nLen, sizeof(nLen), 0))
				return 108;
			char * pstrFileName = new char[nLen+2];
			if (UDT::ERROR == UDT::recv(sock, pstrFileName, nLen, 0))
				return 108;
			pstrFileName[nLen] = '\0';
#ifdef WIN32
			szTmp = WIN32_WC2C(WIN32_CU2WC(pstrFileName));
#else
			szTmp = pstrFileName;
#endif
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
			cxt->fileName = szFileName;
			cxt->nFileTotalSize = nFileTotalSize;
			cxt->nFileCount = nCount;
			if (szFileType == "")
			{
				memset(Head, 0, 8);
				memcpy(Head, "FRR", 3);
				if (UDT::ERROR == UDT::send(sock, (char*)Head, 3, 0))
					return 108;
				return 112;
			}
			else
			{
				m_pCallBack->onAccept(cxt->strClientAddr.c_str(), szFileName.c_str(), nCount, nFileTotalSize, 
					szRecdevice.c_str(), szRectype.c_str(), szOwndevice.c_str(), szOwntype.c_str(), szSendType.c_str(), szFileType.c_str(), sock);
			}
		}
		else if (memcmp(Head, "MCS", 3) == 0)
		{
			if (UDT::ERROR == UDT::recv(sock, (char*)&nLen, sizeof(nLen), 0))
				return 108;
			char * pstrFileName = new char[nLen+2];
			if (UDT::ERROR == UDT::recv(sock, pstrFileName, nLen, 0))
				return 108;
			pstrFileName[nLen] = '\0';
			string szRcvFileName;
#ifdef WIN32
			szRcvFileName = WIN32_WC2C(WIN32_CU2WC(pstrFileName));
#else
			szRcvFileName = pstrFileName;
#endif

			// Recv file, need to get Recv CtrlSock
			if (sockCtrl <= 0)
			{
				CGuard::enterCS(m_Lock);
				vector<PCLIENTCONEXT>::iterator iter = VEC_CLIENT.begin();
				while (iter != VEC_CLIENT.end())
				{
					if ((*iter)->strClientAddr == cxt->strClientAddr)
					{
						nPos = szRcvFileName.find((*iter)->fileName, 0);
						if (nPos >= 0)
						{
							(*iter)->bTransfer = true;
							sockCtrl = (*iter)->sockAccept;
							nFileTotalSize = (*iter)->nFileTotalSize;
							nCount = (*iter)->nFileCount;
							cxt->fileSavePath = (*iter)->fileSavePath;
							break;
						}
					}
					else
						iter++;
				}
				CGuard::leaveCS(m_Lock);
			}

			// 单个文件上层已经合并路径跟名称
			if (nCount <= 1)
			{
				if (szRcvFileName.find("/") != string::npos ||
					szRcvFileName.find("\\") != string::npos)
				{
					szFilePath = cxt->fileSavePath + szRcvFileName;
				}
				else
				{
					szFilePath = cxt->fileSavePath;
				}
			}
			else
			{
				szFilePath = cxt->fileSavePath + szRcvFileName;
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

			if (UDT::ERROR == UDT::recv(sock, (char*)&nFileSize, sizeof(int64_t), 0))
				return 108;

			// receive the file
			fstream ofs(szFilePath.c_str(), ios::out | ios::binary | ios::trunc);
			int64_t offset = 0;
			int64_t left = nFileSize;
			int64_t nResponseSize = 0;
			while(left > 0)
			{
				int64_t recv = 0;
				if (left > TO_SND)
					recv = UDT::recvfile(sock, ofs, offset, TO_SND);
				else
					recv = UDT::recvfile(sock, ofs, offset, left);

				if (UDT::ERROR == recv)
					return 108;

				left -= recv;
				nRecvSize += recv;

				CGuard::enterCS(m_Lock);
				//if (ctrl->bTransfer)
				{
					if (nResponseSize == 0 || nRecvSize - nResponseSize > TO_SND*10 || nFileTotalSize == nRecvSize)
					{
						memset(Head, 0, 8);
						memcpy(Head, "FPR", 3);
						if (UDT::ERROR == UDT::send(sockCtrl, (char*)Head, 3, 0))
							return 108;
						if (UDT::ERROR == UDT::send(sockCtrl, (char*)&nRecvSize, sizeof(nRecvSize), 0))
							return 108;
					}
					m_pCallBack->onTransfer((long)nFileTotalSize, nRecvSize, 0.0, szRcvFileName.c_str(), 1, sockCtrl);
				}
				CGuard::leaveCS(m_Lock);
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
					szTmp = cxt->fileSavePath + szFolder;
					m_pCallBack->onAcceptonFinish(cxt->strClientAddr.c_str(), szTmp.c_str(), 1, sockCtrl);
				}
			}
			else
				m_pCallBack->onAcceptonFinish(cxt->strClientAddr.c_str(), szFilePath.c_str(), 1, sockCtrl);
		}
		else if (memcmp(Head,"DCR",3) == 0)
		{
			if (UDT::ERROR == UDT::recv(sock, (char*)&nLen, sizeof(nLen), 0))
				return 108;
			char * pstrFileName = new char[nLen+2];
			if (UDT::ERROR == UDT::recv(sock, pstrFileName, nLen, 0))
				return 108;
			pstrFileName[nLen] = '\0';
			string szRcvFileName;
#ifdef WIN32
			szRcvFileName = WIN32_WC2C(WIN32_CU2WC(pstrFileName));
#else
			szRcvFileName = pstrFileName;
#endif

			// Recv file, need to get Recv CtrlSock
			if (sockCtrl <= 0)
			{
				CGuard::enterCS(m_Lock);
				vector<PCLIENTCONEXT>::iterator iter = VEC_CLIENT.begin();
				while (iter != VEC_CLIENT.end())
				{
					if ((*iter)->strClientAddr == cxt->strClientAddr)
					{
						nPos = szRcvFileName.find((*iter)->fileName, 0);
						if (nPos >= 0)
						{
							(*iter)->bTransfer = true;
							sockCtrl = (*iter)->sockAccept;
							nFileTotalSize = (*iter)->nFileTotalSize;
							nCount = (*iter)->nFileCount;
							cxt->fileSavePath = (*iter)->fileSavePath;
							break;
						}
					}
					else
						iter++;
				}
				CGuard::leaveCS(m_Lock);
			}

			szTmp = cxt->fileSavePath + pstrFileName;
			//CreateDirectroy(szTmp);
			UdtFile::CreateDir(szTmp.c_str());
		}
		else if (memcmp(Head,"FSC",3) == 0)
		{
			memset(Head, 0, 8);
			memcpy(Head, "FSC", 3);
			if (UDT::ERROR == UDT::send(sock, (char*)Head, 3, 0))
				return 108;
			return 101;
		}
		else if (memcmp(Head,"FSS",3) == 0)
		{
			nReturnCode = 105;
			memset(Head, 0, 8);
			memcpy(Head, "FSS", 3);
			if (UDT::ERROR == UDT::send(sock, (char*)Head, 3, 0))
				return 108;
		}
		else if (memcmp(Head,"FRR",3) == 0)
		{
			return 102;
		}
		else if (memcmp(Head,"FRB",3) == 0)
		{
			return 113;
		}
		else if (memcmp(Head,"FRC",3) == 0)
		{
			return 102;
		}
		else if (memcmp(Head,"FRE",3) == 0)
		{
			return 106;
		}
		else if (memcmp(Head,"FSV",3) == 0)
		{
			return 112;
		}
		else if (memcmp(Head,"MSF",3) == 0)
		{
			return 110;
		}
		else if (memcmp(Head,"FSE",3) == 0)
		{
			if (nReturnCode != 105)
				return 0;
			return 105;
		}
	}

	return 0;
}


#ifndef WIN32
void * CUdtCore::_ListenThreadProc(void * pParam)
#else
DWORD WINAPI CUdtCore::_ListenThreadProc(LPVOID pParam)
#endif
{
	LISTENSOCKET * cxt = (LISTENSOCKET *)pParam;

	while (true)
	{
		// Process Accept, StopListen()，会改变m_bListenStatus的状态
		cxt->pThis->ProcessAccept(cxt);
		if (!cxt->bListen)
		{
			int nCount = 0;
			while (NONEXIST != UDT::getsockstate(cxt->sockListen))
			{
				cout << "Listen port:" << cxt->strServerPort << "states:" << UDT::getsockstate(cxt->sockListen) << endl;
				if (nCount > 8)
					break;
				nCount++;
				Sleep(1500);
			}
			cxt->pThis->m_bListenStatus = false;
			break;
		}
	}

	cout << "Exit _ListenThreadProc port:" << cxt->strServerPort << endl;

#ifndef WIN32
	return NULL;
#else
	return 0;
#endif
}

#ifndef WIN32
void * CUdtCore::_WorkThreadProc(void * pParam)
#else
DWORD WINAPI CUdtCore::_WorkThreadProc(LPVOID pParam)
#endif
{
	CLIENTCONEXT * pThis = (CLIENTCONEXT *)pParam;
	int nReturnCode = 0;

	if (pThis->Type == OP_SND_CTRL)
	{
		nReturnCode = pThis->pThis->ProcessSendCtrl(pThis);
		UDT::close(pThis->sockAccept);
		pThis->pThis->m_pCallBack->onFinished("RETURN", nReturnCode, pThis->sockAccept);
	}
	else if (pThis->Type == OP_SND_FILE)
	{
		nReturnCode = pThis->pThis->ProcessSendFile(pThis);
		UDT::close(pThis->sockAccept);
	}
	else if (pThis->Type == OP_RCV_CTRL)
	{
		nReturnCode = pThis->pThis->ProcessRecvFile(pThis);
		UDT::close(pThis->sockAccept);
		pThis->pThis->m_pCallBack->onFinished("RETURN", nReturnCode, pThis->sockAccept);
	}
	else if (pThis->Type == OP_RCV_FILE)
	{
		nReturnCode = pThis->pThis->ProcessRecvFile(pThis);
		UDT::close(pThis->sockAccept);
	}

	CGuard::enterCS(pThis->pThis->m_Lock);
	vector<PCLIENTCONEXT>::iterator iter = pThis->pThis->VEC_CLIENT.begin();
	while (iter != pThis->pThis->VEC_CLIENT.end())
	{
		if ((*iter)->sockAccept == pThis->sockAccept)
		{
			iter = pThis->pThis->VEC_CLIENT.erase(iter);
		}
		else
			iter++;
	}
	CGuard::leaveCS(pThis->pThis->m_Lock);

#ifndef WIN32
	return NULL;
#else
	return 0;
#endif
}