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
	if (m_bListenStatus)
		return 0;

	UDT::startup();

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
#ifndef WIN32
	pthread_t hHandle;
	pthread_create(&hHandle, NULL, _ListenThreadProc, pCtrl);
	pthread_detach(hHand);
#else
	HANDLE hHand = CreateThread(NULL, 0, _ListenThreadProc, pCtrl, 0, NULL);
#endif

	// Create listen file thread function
	LISTENSOCKET * pFile = new LISTENSOCKET();
	memset(strPort, 0, 32);
	sprintf(strPort, "%d", nFilePort);
	pFile->strServerPort = strPort;
	pFile->bListen = false;
	pFile->Type = OP_RCV_FILE;
	pFile->pThis = this;
#ifndef WIN32
	pthread_t hHandle;
	pthread_create(&hHand, NULL, _ListenThreadProc, pFile);
	pthread_detach(hHand);
#else
	HANDLE hHand = CreateThread(NULL, 0, _ListenThreadProc, pFile, 0, NULL);
#endif

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
	sprintf(strPort, "%d", m_nCtrlPort);
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
	pCtrl->sockAccept = client;
	pCtrl->nCtrlFileGroup = client;
	pCtrl->pThis = this;
	pCtrl->Type = OP_SND_CTRL;
	VEC_CLIENT.push_back(pCtrl);

#ifndef WIN32
	pthread_t hHandle;
	pthread_create(&hHandle, NULL,  _WorkThreadProc, pCtrl);
	pthread_detach(hHandle);
#else
	HWND hHandle = CreateThread(NULL, 0, _WorkThreadProc, pCtrl, 0, NULL);
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
			if (memcmp("REJECT", pstrReply, 6) == 0)
			{
				it = VEC_CLIENT.erase(it);
				memcpy(Head, "FRR", 3);
			}
			else if (memcmp("REJECTBUSY", pstrReply, 10) == 0 || strlen(pstrReply) <= 0)
			{
				it = VEC_CLIENT.erase(it);
				memcpy(Head, "FRB", 3);
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
			break;
		}
	}
	CGuard::leaveCS(m_Lock);
}

void CUdtCore::StopListen()
{
	m_bListenStatus = false;
	for (vector<PLISTENSOCKET>::iterator iter = VEC_LISTEN.begin(); iter != VEC_LISTEN.end(); iter++)
	{
#ifndef WIN32
		pthread_mutex_lock(&(*iter)->lock);
		UDT::close((*iter)->sockListen);
		pthread_cond_signal(&(*iter)->cond);
		pthread_mutex_unlock(&(*iter)->lock);
#else
		UDT::close((*iter)->sockListen);
		SetEvent((*iter)->cond);
		WaitForSingleObject((*iter)->hHand, INFINITE);
#endif
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
			return ;
		}
		cout << "Successful listening port: " << cxt->strServerPort << endl;
		cxt->bListen = true;
	}

	// accept port
	UDTSOCKET sockAccept;
	sockaddr_storage clientaddr;
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
	getnameinfo((sockaddr *)&clientaddr, sizeof(clientaddr), clienthost, sizeof(clienthost), clientservice, sizeof(clientservice), NI_NUMERICHOST|NI_NUMERICSERV);
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
	string szAddr, szPort;
	string szTmp, szFolderName, szFileName, szFilePath, szFinish = "NETFAIL", szError = "";
	vector<string> vecFileName;
	vector<string> vecDirName;
	vector<string> vecTmp;

	UDTSOCKET client = cxt->sockAccept;

	// send flags
	memset(Head, 0, 8);
	memcpy(Head, "MSR", 3);
	if (UDT::ERROR == UDT::send(client, (char*)Head, 3, 0))
		return 108;

	vecTmp = cxt->vecFiles;
	for (vector<string>::iterator it = vecTmp.begin(); it != vecTmp.end(); it++)
	{
		szTmp = *it;
		if ('\\' == szTmp[szTmp.size()-1] || '/' == szTmp[szTmp.size()-1])
			szTmp = szTmp.substr(0, szTmp.size()-1);

		SearchFileInDirectroy(szTmp, nFileTotalSize, vecDirName, vecFileName);
	}
	nFileCount = vecTmp.size();
	cxt->vecFilePath = vecFileName;

	// send file total size
	if (UDT::ERROR == UDT::send(client, (char*)&nFileTotalSize, sizeof(nFileTotalSize), 0))
		return 108;
	// send file count
	if (UDT::ERROR == UDT::send(client, (char*)&nFileCount, sizeof(nFileCount), 0))
		return 108;

	// send file name,(filename\hostname\sendtype)
	szFilePath = cxt->vecFiles[0];
	nPos = szFilePath.find_last_of('/');
	if (nPos < 0)
	{
		nPos = szFilePath.find_last_of("\\");
	}
	szFileName = szFilePath.substr(nPos+1);
	szTmp = szFileName + "\\" + cxt->ownDev + "\\" + cxt->ownType + "\\" + cxt->recvDev + "\\" + cxt->recvType + "\\" + cxt->sendType;
	if (vecDirName.empty())
		szTmp = szTmp + "\\F\\";
	else
		szTmp = szTmp + "\\D\\";
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
			UDTSOCKET sockFile;
			if (CreateUDTSocket(sockFile, "7778") < 0)
			{
				return 108;
			}
			if (UDT_Connect(sockFile, cxt->strServerAddr.c_str(), "7778") < 0)
			{
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
			pFile->vecFilePath = cxt->vecFilePath;
			pFile->sockAccept = sockFile;
			pFile->nCtrlFileGroup = cxt->sockAccept;
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
				m_pCallBack->onTransfer(nFileTotalSize, nLastSendSize, 0.0, szFileName.c_str(), 2, client);
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
	int64_t nFileTotalSize = 0, nSendSize = 0, iLastPercent = 0;
	string szTmp, szFolder, szAccept, szFileName, szFilePath, szFinish = "NETFAIL", szError = "";
	vector<string> vecFile;
	vector<string> vecFilePath;
	vector<string> vecDirName;

	vecFile = cxt->vecFiles;
	vecFilePath = cxt->vecFilePath;
	UDTSOCKET & client = cxt->sockAccept;

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
						return 108;
					if (UDT::ERROR == UDT::send(client, (char*)&nLen, sizeof(int), 0))
						return 108;
					if (UDT::ERROR == UDT::send(client, szFolder.c_str(), nLen, 0))
						return 108;
				}

				// send filename size and filename
				memset(Head, 0, 8);
				memcpy(Head, "MCS", 3);
				if (UDT::ERROR == UDT::send(client, (char*)Head, 3, 0))
					return 108;
				nLen = szFileName.size();
				if (UDT::ERROR == UDT::send(client, (char*)&nLen, sizeof(int), 0))
					return 108;
				if (UDT::ERROR == UDT::send(client, szFileName.c_str(), nLen, 0))
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
				int64_t nFileSize = 0, nOffset = 0, left = 0;
				fstream ifs(szFilePath.c_str(), ios::binary | ios::in);
				try
				{
					ifs.seekg(0, ios::end);
					nFileSize = ifs.tellg();
					ifs.seekg(0, ios::beg);

					left = nFileSize;
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
	vector<string> vecFileName;
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

			// notify to up
			m_pCallBack->onRecvMessage(pstrMsg, cxt->strClientAddr.c_str(), pstrHostName);
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
			const string szRcvFileName = pstrFileName;

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
				memset(Head, 0, 8);
				memcpy(Head, "FPR", 3);
				if (UDT::ERROR == UDT::send(sockCtrl, (char*)Head, 3, 0))
					return 108;
				if (UDT::ERROR == UDT::send(sockCtrl, (char*)&nRecvSize, sizeof(nRecvSize), 0))
					return 108;
				m_pCallBack->onTransfer((long)nFileTotalSize, nRecvSize, 0.0, szRcvFileName.c_str(), 1, sockCtrl);
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
			szTmp = cxt->fileSavePath + pstrFileName;
			CreateDirectroy(szTmp);
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
		else if (memcmp(Head,"FSE",3) == 0)
		{
			return 105;
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
	if (cxt)
	{
		cxt->pThis->VEC_LISTEN.push_back(cxt);
	}

	while (true)
	{
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
			cout << "_ListenThreadProc timeout" << endl;
			break;
		}
#else
		if (WAIT_TIMEOUT != WaitForSingleObject(cxt->cond, 1000))
		{
			std::cout << "_ListenThreadProc timeout" << endl;
			break;
		}
#endif

		// Process Accept, StopListen()，会改变m_bListenStatus的状态
		cxt->pThis->ProcessAccept(cxt);
		if (!cxt->bListen)
		{
			break;
		}
	}

	cout << "Exit _ListenThreadProc port:" << cxt->strServerPort << endl;
	UDT::close(cxt->sockListen);
	delete cxt;
	cxt = NULL;
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
		pThis->pThis->m_pCallBack->onFinished("RETURN", nReturnCode, pThis->sockAccept);
	}
	else if (pThis->Type == OP_SND_FILE)
	{
		nReturnCode = pThis->pThis->ProcessSendFile(pThis);
	}
	else if (pThis->Type == OP_RCV_CTRL)
	{
		pThis->pThis->m_bBusying = true;
		nReturnCode = pThis->pThis->ProcessRecvFile(pThis);
		pThis->pThis->m_bBusying = false;
		pThis->pThis->m_pCallBack->onFinished("RETURN", nReturnCode, pThis->sockAccept);
	}
	else if (pThis->Type == OP_RCV_FILE)
	{
		nReturnCode = pThis->pThis->ProcessRecvFile(pThis);
	}

	CGuard::enterCS(pThis->pThis->m_Lock);
	vector<PCLIENTCONEXT>::iterator iter = pThis->pThis->VEC_CLIENT.begin();
	while (iter != pThis->pThis->VEC_CLIENT.end())
	{
		if ((*iter)->sockAccept == pThis->sockAccept)
		{
			UDT::close(pThis->sockAccept);
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