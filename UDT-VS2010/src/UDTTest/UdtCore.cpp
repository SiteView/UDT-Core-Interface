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
	pthread_t hCHandle;
	pthread_create(&hCHandle, NULL, _ListenThreadProc, pCtrl);
	pthread_detach(hCHandle);
#else
	HANDLE hCHand = CreateThread(NULL, 0, _ListenThreadProc, pCtrl, 0, NULL);
#endif

	return 0;
}

int CUdtCore::SendMsg(const char* pstrAddr, const char* pstrMsg, const char* pstrHostName)
{
	UDT::startup();
	int nLen = 0, nReturnCode = 108;
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

	// send login user name
	char cmd[CMD_SIZE];
	memset(cmd, 0, CMD_SIZE);
	sprintf(cmd, "USER %s", pstrHostName);
	if (UDT::ERROR == UDT::send(client, (char*)cmd, CMD_SIZE, 0))
		return 108;

	// send flags
	memset(cmd, 0, 8);
	memcpy(cmd, "TEXT", 3);
	if (UDT::ERROR == UDT::send(client, (char*)cmd, CMD_SIZE, 0))
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

	if (UDT::getsockstate(m_sockListen) != LISTENING)
	{
		cout << "Port 7777  is not in listening states!" << endl;
		m_pCallBack->onFinished("Create fail!", 108, m_sockListen);
		return 0;
	}

	int64_t nFileTotalSize = 0;
	string szTmp;
	vector<string> vecDirs;
	vector<_FileInfo> info;
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
	sprintf(strCtrlPort, "%d", 7777);
	sprintf(strFilePort, "%d", 7778);

	// connect to CtrlPort
	if (CreateUDTSocket(client, strCtrlPort) < 0)
	{
		m_pCallBack->onFinished("Create fail!", 108, client);
		return 0;
	}
	//if (UDT_Connect(client, pstrAddr, strCtrlPort) < 0)
	//{
	//	m_pCallBack->onFinished("Connect fail!", 108, client);
	//	return 0;
	//}
	CLIENTCONEXT * pCtrl = new CLIENTCONEXT;
	pCtrl->strServerAddr = pstrAddr;
	pCtrl->strServerPort = strCtrlPort;
	pCtrl->ownDev = owndevice;
	pCtrl->ownType = owntype;
	pCtrl->recvDev = recdevice;
	pCtrl->recvType = rectype;
	pCtrl->sendType = pstrSendtype;
	pCtrl->vecFiles = vecFiles;
	pCtrl->vecDirs = vecDirs;
	pCtrl->fileInfo = info;
	pCtrl->nFileTotalSize = nFileTotalSize;
	pCtrl->nFileCount = vecFiles.size();
	pCtrl->sockAccept = client;
	pCtrl->bTransfer = false;
	pCtrl->pThis = this;
	pCtrl->Type = OP_SND_CTRL;
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
	char cmd[CMD_SIZE];
	if (memcmp("REJECTBUSY", pstrReply, 10) == 0 || strlen(pstrReply) <= 0)
	{
		memset(cmd, 0, CMD_SIZE);
		sprintf(cmd, "202 %s", "device reject busy");
		m_pCallBack->onFinished("device reject busy", 113, sock);
	}
	else if (memcmp("REJECT", pstrReply, 6) == 0)
	{
		memset(cmd, 0, CMD_SIZE);
		sprintf(cmd, "202 %s", "device reject");
		m_pCallBack->onFinished("device reject", 102, sock);
	}
	else
	{
		m_szFileSavePath = pstrReply;
		memset(cmd, 0, CMD_SIZE);
		sprintf(cmd, "200 %s", "accept receive file");
	}

	if (UDT::ERROR == UDT::send(sock, cmd, CMD_SIZE, 0))
		return ;
}

void CUdtCore::StopTransfer(const UDTSOCKET sock, const int nType)
{
	char cmd[CMD_SIZE];
	if (nType == 1)
	{
		// stop send file
		memset(cmd, 0, CMD_SIZE);
		sprintf(cmd, "%s", "ABOR");
		if (UDT::ERROR == UDT::send(sock, (char*)cmd, CMD_SIZE, 0))
			return ;
		CGuard::enterCS(m_Lock);
		m_bTransfer = false;
		CGuard::leaveCS(m_Lock);
	}
	else
	{
		// stop recv file
		memset(cmd, 0, CMD_SIZE);
		sprintf(cmd, "350 %s", "stop receive file");
		if (UDT::ERROR == UDT::send(sock, cmd, CMD_SIZE, 0))
			return ;
	}
}

void CUdtCore::StopListen()
{
	UDT::close(m_sockListen);
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
		m_sockListen = cxt->sockListen;
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
	char cmd[CMD_SIZE];
	int nReturnCode = 108, nLen = 0, nPos = 0, nFileCount = 0;
	int64_t nFileTotalSize = 0, nSendSize = 0, nLastSendSize = 0;
	string szTmp, szPercent, szFolderName, szFileName, szFilePath, szFinish = "NETFAIL", szError = "";
	nFileCount = cxt->nFileCount;
	nFileTotalSize = cxt->nFileTotalSize;
	szFilePath = cxt->vecFiles[0];

	// connect to data port
	while (true)
	{
		if (UDT_Connect(cxt->sockAccept, cxt->strServerAddr.c_str(), cxt->strServerPort.c_str()) < 0)
		{
			const char * error = UDT::getlasterror_desc();
			m_pCallBack->onFinished(error,119, cxt->sockAccept);
			nLen++;
#ifdef WIN32
			Sleep(1000);
#else
			sleep(1);
#endif
		}
		else
			break;
		if (nLen >= 3)
		{
			m_pCallBack->onFinished("Connect fail", 119, cxt->sockAccept);
			return -1;
		}
	}
	UDTSOCKET sockCtrl = cxt->sockAccept;
	// get sockaddr info
	struct sockaddr_in addr_c;
	nLen = sizeof(addr_c);
	UDT::getsockname(sockCtrl, (sockaddr*)&addr_c, &nLen);
	char clientHost[NI_MAXHOST];
	char clientService[NI_MAXSERV];
	sprintf(clientHost, "%s", inet_ntoa(addr_c.sin_addr));
	sprintf(clientService, "%d", addr_c.sin_port);

	while (true)
	{
		memset(cmd, 0, CMD_SIZE);
		if (UDT::ERROR == UDT::recv(sockCtrl, cmd, CMD_SIZE, 0))
		{
			const char * error = UDT::getlasterror_desc();
			m_pCallBack->onFinished(error, 108, sockCtrl);
			break;
		}
		if (memcmp(cmd, "120", 3) == 0)
		{
			// 服务器准备就绪的时间分钟数
		}
		else if (memcmp(cmd, "125", 3) == 0)
		{
			// 打开数据连接开始传输
		}
		else if (memcmp(cmd, "150", 3) == 0)
		{
			// 打开连接
		}
		else if (memcmp(cmd, "200", 3) == 0)
		{
			// 成功
			memset(cmd, 0, CMD_SIZE);
			sprintf(cmd, "%s%s%s%s", "PORT ", clientHost, ",7000,", clientService);
			if (UDT::ERROR == UDT::send(sockCtrl, (char*)cmd, CMD_SIZE, 0))
				return 108;
			//szTmp = cmd;
			//cxt->uuid = szTmp.substr(5);
			//VEC_CLIENT.push_back(cxt);
		}
		else if (memcmp(cmd, "202", 3) == 0)
		{
			// 命令没有执行
			szTmp = cmd;
			szTmp = szTmp.substr(4);
			// send quit
			memset(cmd, 0, CMD_SIZE);
			sprintf(cmd, "%s", "QUIT");
			if (UDT::ERROR == UDT::send(sockCtrl, (char*)cmd, CMD_SIZE, 0))
				return 108;

			if (szTmp == "device reject")
				return 103;
			else if (szTmp == "device reject busy")
				return 114;
			else
				return 0;
		}
		else if (memcmp(cmd, "214", 3) == 0)
		{
			szTmp = cmd;
			string szCmd = szTmp.substr(4);
			nPos = szCmd.find_first_of(" ");
			szTmp = szCmd.substr(0, nPos);
			nSendSize = atoi(szTmp.c_str());
			szCmd = szCmd.substr(nPos+1);
			nPos = szCmd.find_first_of(" ");
			szTmp = szCmd.substr(0, nPos);
			double dRate = atoi(szTmp.c_str()) / 100.0;
			szTmp = szCmd.substr(nPos+1);
			m_pCallBack->onTransfer(nFileTotalSize, nSendSize, dRate, szTmp.c_str(), 2, sockCtrl);
		}
		else if (memcmp(cmd, "220", 3) == 0)
		{
			// 服务就绪 
			// send login user name
			memset(cmd, 0, CMD_SIZE);
			sprintf(cmd, "USER %s", "SiteView");
			if (UDT::ERROR == UDT::send(sockCtrl, (char*)cmd, CMD_SIZE, 0))
				return 108;
		}
		else if (memcmp(cmd, "221", 3) == 0)
		{
			// 退出网络 
		}
		else if (memcmp(cmd, "225", 3) == 0)
		{
			// 打开数据连接
		}
		else if (memcmp(cmd, "226", 3) == 0)
		{
			// 结束数据连接
			memset(cmd, 0, CMD_SIZE);
			sprintf(cmd, "%s", "QUIT");
			if (UDT::ERROR == UDT::send(sockCtrl, (char*)cmd, CMD_SIZE, 0))
				return 108;
			return 109;
		}
		else if (memcmp(cmd, "227", 3) == 0)
		{
			// 进入被动模式
		}
		else if (memcmp(cmd, "230", 3) == 0)
		{
			// user logged in, proceed
			// send directory
			for (vector<string>::iterator iter = cxt->vecDirs.begin(); iter != cxt->vecDirs.end(); iter++)
			{
				memset(cmd, 0, CMD_SIZE);
				sprintf(cmd, "NLST %s", (*iter).c_str());
				if (UDT::ERROR == UDT::send(sockCtrl, (char*)cmd, CMD_SIZE, 0))
					return 108;
			}
			// send file path name
			for (vector<_FileInfo>::iterator iter = cxt->fileInfo.begin(); iter != cxt->fileInfo.end(); iter++)
			{
				memset(cmd, 0, CMD_SIZE);
				sprintf(cmd, "LIST %s", (*iter).asciPath.c_str());
				if (UDT::ERROR == UDT::send(sockCtrl, (char*)cmd, CMD_SIZE, 0))
					return 108;
			}
			// send file total size
			memset(cmd, 0, CMD_SIZE);
			sprintf(cmd, "%s%d", "ALLO ", cxt->nFileTotalSize);
			if (UDT::ERROR == UDT::send(sockCtrl, (char*)cmd, CMD_SIZE, 0))
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
			//#ifdef WIN32
			//	szTmp = WIN32_WC2CU(WIN32_C2WC(szTmp.c_str()));
			//#endif
			memset(cmd, 0, CMD_SIZE);
			sprintf(cmd, "%s%s", "SITE ", szTmp.c_str());
			if (UDT::ERROR == UDT::send(sockCtrl, cmd, CMD_SIZE, 0))
				return 108;
		}
		else if (memcmp(cmd, "421", 3) == 0)
		{
			// 服务关闭
		}
		else if (memcmp(cmd, "350", 3) == 0)
		{
			// 文件行为暂停
			memset(cmd, 0, CMD_SIZE);
			sprintf(cmd, "%s", "ABOR");
			if (UDT::ERROR == UDT::send(sockCtrl, (char*)cmd, CMD_SIZE, 0))
				return 108;

			CGuard::enterCS(m_Lock);
			m_bTransfer = false;
			CGuard::leaveCS(m_Lock);
		}
		else if (memcmp(cmd, "426", 3) == 0)
		{
			// send quit
			memset(cmd, 0, CMD_SIZE);
			sprintf(cmd, "%s", "QUIT");
			if (UDT::ERROR == UDT::send(sockCtrl, (char*)cmd, CMD_SIZE, 0))
				return 108;
			return 107;
		}
		else if (memcmp(cmd, "800", 3) == 0)
		{
			return 112;
		}
	}

	return 0;
}


int CUdtCore::ProcessRecvCtrl(CLIENTCONEXT * cxt)
{
	char cmd[CMD_SIZE];
	int nReturnCode = 108, nLen = 0, nRet = 0, nPos = 0, nCount = 0;
	int64_t nFileSize = 0, nFileTotalSize = 0, nRecvSize = 0, nSendSize = 0, iLastPercent = 0; 
	string szTmp, szTmp2, strReplyPath = "", szFinish = "NETFAIL", szSlash = "", szFilePath = "", szReFileName = "", szFolder = "";
	string szUser, szPort, szHostName, szFileName, szRecdevice, szRectype, szOwndevice, szOwntype, szSendType, szFileType;
	std::vector<std::string> vecDirName;
	std::vector<std::string> vecFileName;
	UDTSOCKET sockCtrl = cxt->sockAccept;

	// send login user name
	memset(cmd, 0, CMD_SIZE);
	sprintf(cmd, "220 %s", "Welcome to siteView turboTranfser");
	if (UDT::ERROR == UDT::send(sockCtrl, (char*)cmd, CMD_SIZE, 0))
	{
		const char * error = UDT::getlasterror_desc();
		m_pCallBack->onFinished(error, 108, sockCtrl);
		UDT::close(sockCtrl);
		return -1;
	}

	while (true)
	{
		memset(cmd, 0, CMD_SIZE);
		if (UDT::ERROR == UDT::recv(sockCtrl, cmd, CMD_SIZE, 0))
		{
			const char * error = UDT::getlasterror_desc();
			m_pCallBack->onFinished(error, 108, sockCtrl);
			break;
		}
		if (memcmp(cmd,"TEXT", 4) == 0)	// 1.	recv message response（TSR）
		{
			// recv message
			if (UDT::ERROR == UDT::recv(sockCtrl, (char*)&nLen, sizeof(nLen), 0))
			{
				const char * error = UDT::getlasterror_desc();
				m_pCallBack->onFinished(error, 108, sockCtrl);
				break;
			}
			char * pstrMsg = new char[nLen+2];
			if (UDT::ERROR == UDT::recv(sockCtrl, pstrMsg, nLen, 0))
			{
				const char * error = UDT::getlasterror_desc();
				m_pCallBack->onFinished(error, 108, sockCtrl);
				break;
			}
			pstrMsg[nLen] = '\0';
#ifdef WIN32
			szTmp = WIN32_WC2C(WIN32_CU2WC(pstrMsg));
#else
			szTmp = pstrMsg;
#endif
			// notify to up
			m_pCallBack->onRecvMessage(szTmp.c_str(), cxt->strClientAddr.c_str(), szHostName.c_str());
			break;
		}
		else if (memcmp(cmd, "ACCT", 4) == 0)
		{
			// ACCT系统特权帐号就是控制端口的PORT命令内容、用来匹配PORT-DATA
			szFilePath = "";
			szTmp = cmd;
			szPort = szTmp.substr(5);
		}
		else if (memcmp(cmd, "USER", 4) == 0)
		{
			szUser = cmd;
			szUser = szUser.substr(5);
			memset(cmd, 0, CMD_SIZE);
			sprintf(cmd, "230 %s", "User logged in, proceed");
			if (UDT::ERROR == UDT::send(sockCtrl, cmd, CMD_SIZE, 0))
			{
				const char * error = UDT::getlasterror_desc();
				m_pCallBack->onFinished(error, 108, sockCtrl);
				break;
			}
		}
		else if (memcmp(cmd, "SITE", 4) == 0)
		{
			// 由服务器提供的特殊参数
			szHostName = "";
			szTmp = cmd;
			szTmp = szTmp.substr(5);
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
			m_pCallBack->onAccept(cxt->strClientAddr.c_str(), szFileName.c_str(), nCount, nFileTotalSize, 
				szRecdevice.c_str(), szRectype.c_str(), szOwndevice.c_str(), szOwntype.c_str(), szSendType.c_str(), szFileType.c_str(), sockCtrl);
		}
		else if (memcmp(cmd, "NLST", 4) == 0)
		{
			szFolder = "";
			szTmp = cmd;
			szFolder = szTmp.substr(5);
			if ('\\' == szFolder[szFolder.size()-1] || '/' == szFolder[szFolder.size()-1])
				szFolder = szFolder.substr(0, szFolder.size()-1);
			if (!szFolder.empty())
			{
				vecDirName.push_back(szFolder);
			}
		}
		else if (memcmp(cmd, "LIST", 4) == 0)
		{
			szFilePath = "";
			szTmp = cmd;
			szFilePath = szTmp.substr(5);
			if (!szFilePath.empty())
			{
				vecFileName.push_back(szFilePath);
			}
		}
		else if (memcmp(cmd, "ALLO", 4) == 0)
		{
			szFilePath = "";
			szTmp = cmd;
			szTmp = szTmp.substr(5);
			if (!szTmp.empty())
			{
				nFileTotalSize = atoi(szTmp.c_str());
			}
		}
		else if (memcmp(cmd, "MKD", 3) == 0)
		{
			szFolder = "";
			szTmp = cmd;
			szFolder = szTmp.substr(4);
			if (!szFolder.empty())
			{
				UdtFile::CreateDir(szFolder.c_str());
			}
		}
		else if (memcmp(cmd, "PORT", 4) == 0)
		{
			szTmp = cmd;
			szPort = szTmp.substr(5);
			CLIENTCONEXT * pData = new CLIENTCONEXT();
			nPos = szPort.find_last_of(",");
			if (nPos >= 0)
			{
				pData->strServerPort = szPort.substr(nPos+1);
				szTmp = szPort.substr(0, nPos);
			}
			nPos = szTmp.find_last_of(",");
			if (nPos >= 0)
			{
				pData->strServerAddr = szTmp.substr(0, nPos);
			}
			if (!pData->strServerAddr.empty() && !pData->strServerPort.empty())
			{
				CGuard::enterCS(m_Lock);
				m_bTransfer = true;
				CGuard::leaveCS(m_Lock);
				pData->sockListen = sockCtrl;
				pData->vecFiles = vecFileName;
				pData->vecDirs = vecDirName;
				pData->szPort = szPort;
				pData->nFileTotalSize = nFileTotalSize;
				pData->pThis = this;
				pData->Type = OP_RCV_FILE;
#ifndef WIN32
				pthread_t hHandle;
				pthread_create(&hHandle, NULL, _WorkThreadProc, pData);
				pthread_detach(hHandle);
#else
				HANDLE hHandle = CreateThread(NULL, 0, _WorkThreadProc, pData, 0, NULL);
#endif
			}
		}
		else if (memcmp(cmd, "STOR", 4) == 0)
		{

		}
		else if (memcmp(cmd, "RETR", 4) == 0)
		{
			szTmp = cmd;
			nPos = szTmp.find_first_of(" ");
			if (nPos >= 0)
			{
				szFilePath = szTmp.substr(nPos+1);
			}
			// open the file
			try
			{
				_FileInfo info;
				UdtFile::GetInfo(szFilePath.c_str(), info);
				nFileSize = info.nFileSize;
				memset(cmd, 0, CMD_SIZE);
				sprintf(cmd, "213 %d", nFileSize);
				if (UDT::ERROR == UDT::send(sockCtrl, cmd, CMD_SIZE, 0))
				{
					const char * error = UDT::getlasterror_desc();
					m_pCallBack->onFinished(error, 108, sockCtrl);
					break;
				}

				fstream ifs(szFilePath.c_str(), ios::in | ios::binary);
				int64_t nOffset = 0, left = nFileSize;

				while (left > 0)
				{
					CGuard::enterCS(m_Lock);
					if (!m_bTransfer)
					{
						CGuard::leaveCS(m_Lock);
						break;
					}
					CGuard::leaveCS(m_Lock);

					int64_t send = 0;
					if (left > TO_SND)
						send = UDT::sendfile(sockCtrl, ifs, nOffset, TO_SND);
					else
						send = UDT::sendfile(sockCtrl, ifs, nOffset, left);
					if (UDT::ERROR == send)
					{
						const char * error = UDT::getlasterror_desc();
						m_pCallBack->onFinished(error, 108, sockCtrl);
						UDT::close(sockCtrl);
						return -1;
					}
					left -= send;
					nSendSize += send;
				}
				ifs.close();
				m_pCallBack->onAcceptonFinish(cxt->strClientAddr.c_str(), szFileName.c_str(), nFileSize, 2, sockCtrl);
			}
			catch (...)
			{
				m_pCallBack->onFinished("open the file faild", 108, sockCtrl);
				break;
			}
		}
		else if (memcmp(cmd, "ABOR", 4) == 0)
		{
			CGuard::enterCS(m_Lock);
			if (m_bTransfer)
				m_pCallBack->onFinished("other stop send file", 105, sockCtrl);
			else
				m_pCallBack->onFinished("owner stop recv file", 106, sockCtrl);
			m_bTransfer = false;
			CGuard::leaveCS(m_Lock);

			memset(cmd, 0, CMD_SIZE);
			sprintf(cmd, "426 %s", "stop receive file");
			if (UDT::ERROR == UDT::send(sockCtrl, cmd, CMD_SIZE, 0))
			{
				const char * error = UDT::getlasterror_desc();
				m_pCallBack->onFinished(error, 108, sockCtrl);
				break;
			}
		}
		else if (memcmp(cmd, "MSR", 3) == 0)
		{
			memset(cmd, 0, CMD_SIZE);
			memcpy(cmd, "FSV", 3);
			if (UDT::ERROR == UDT::send(sockCtrl, (char*)cmd, 3, 0))
			{
				const char * error = UDT::getlasterror_desc();
				m_pCallBack->onFinished(error, 108, sockCtrl);
				break;
			}
			m_pCallBack->onFinished("other version is low", 112, sockCtrl);
			break;
		}
		else if (memcmp(cmd, "QUIT", 4) == 0)
		{
			m_pCallBack->onFinished("server exit", 120, sockCtrl);
			break;
		}
	}

	UDT::close(sockCtrl);
	return 0;
}

int CUdtCore::ProcessRecvFile(CLIENTCONEXT * cxt)
{
	char cmd[CMD_SIZE];
	int nReturnCode = 108, nLen = 0, nRet = 0, nPos = 0, nCount = 0;
	int64_t nFileSize = 0, nFileTotalSize = 0, nRecvSize = 0, iLastPercent = 0; 
	string szTmp, szTmp2, szFinish = "NETFAIL", szSlash = "", szFilePath = "", szReFileName = "", szFolder = "";
	string szHostName, szFileName, szRecdevice, szRectype, szOwndevice, szOwntype, szSendType, szFileType;
	const string szReplyPath = m_szFileSavePath;
	std::vector<std::string> vecFileName;
	nFileTotalSize = cxt->nFileTotalSize;

	if (CreateUDTSocket(cxt->sockAccept, "7777") < 0)
	{
		return 108;
	}
	// connect to data port
	while (true)
	{
		if (UDT_Connect(cxt->sockAccept, cxt->strServerAddr.c_str(), "7777") < 0)
		{
			const char * error = UDT::getlasterror_desc();
			m_pCallBack->onFinished(error,119, cxt->sockAccept);
			nLen++;
#ifdef WIN32
			Sleep(1000);
#else
			sleep(1);
#endif
		}
		else
			break;
		if (nLen >= 3)
		{
			m_pCallBack->onFinished("Connect fail", 119, cxt->sockAccept);
			return -1;
		}
	}
	UDTSOCKET sockData = cxt->sockAccept;
	UDTSOCKET sockCtrl = cxt->sockListen;
	// receive welcome info
	memset(cmd, 0, CMD_SIZE);
	if (UDT::ERROR == UDT::recv(sockData, cmd, CMD_SIZE, 0))
	{
		const char * error = UDT::getlasterror_desc();
		m_pCallBack->onFinished(error, 108, sockData);
		UDT::close(sockData);
		return -1;
	}

	// send login user name
	memset(cmd, 0, CMD_SIZE);
	sprintf(cmd, "ACCT %s", cxt->szPort.c_str());
	if (UDT::ERROR == UDT::send(sockData, (char*)cmd, CMD_SIZE, 0))
	{
		const char * error = UDT::getlasterror_desc();
		m_pCallBack->onFinished(error, 108, sockData);
		UDT::close(sockData);
		return -1;
	}

	for (vector<string>::iterator iter = cxt->vecFiles.begin(); iter != cxt->vecFiles.end(); iter++)
	{
		const string szRemoteFileName = *iter;
		// send login user name
		memset(cmd, 0, CMD_SIZE);
		sprintf(cmd, "RETR %s", szRemoteFileName.c_str());
		if (UDT::ERROR == UDT::send(sockData, cmd, CMD_SIZE, 0))
		{
			const char * error = UDT::getlasterror_desc();
			m_pCallBack->onFinished(error, 108, sockData);
			UDT::close(sockData);
			return -1;
		}
		if (UDT::ERROR == UDT::recv(sockData, (char*)&nFileSize, sizeof(nFileSize), 0))
		{
			const char * error = UDT::getlasterror_desc();
			m_pCallBack->onFinished(error, 108, sockData);
			UDT::close(sockData);
			return -1;
		}

		szFileName = "";
		szFilePath = szRemoteFileName;
		for (vector<string>::iterator iter2 = cxt->vecDirs.begin(); iter2 != cxt->vecDirs.end(); iter2++)
		{
			szTmp = *iter2;
			nPos = szFilePath.find(szTmp.c_str());
			if (nPos >= 0)
			{
				nPos = szTmp.find_last_of("\\");
				if (nPos < 0)
				{
					nPos = szTmp.find_last_of("/");
				}
				if (nPos >= 0)
				{
					szTmp = szTmp.substr(0, nPos);
					szFileName = szFilePath.substr(szTmp.size()+1);
					if (cxt->vecFiles.size() <= 1)
					{
						szFilePath = szReplyPath;
					}
					else
						szFilePath = szReplyPath + szFileName;
					nPos = szFilePath.find_last_of("\\");
					if (nPos < 0)
					{
						nPos = szFilePath.find_last_of("/");
					}
					if (nPos >= 0)
					{
						szTmp = szFilePath.substr(0, nPos);
						UdtFile::CreateDir(szTmp.c_str());
					}
				}
				break;
			}
		}
		if (szFileName == "")
		{
			nPos = szFilePath.find_last_of("\\");
			if (nPos < 0)
			{
				nPos = szFilePath.find_last_of("/");
			}
			szFileName = szFilePath.substr(nPos+1);
			if (cxt->vecFiles.size() <= 1)
			{
				szFilePath = szReplyPath;
			}
			else
				szFilePath = szReplyPath + szFileName;
		}

		// receive the file
		fstream ofs(szFilePath.c_str(), ios::out | ios::binary | ios::trunc);
		int64_t offset = 0;
		int64_t left = nFileSize;
		while(left > 0)
		{
			CGuard::enterCS(m_Lock);
			if (!m_bTransfer)
			{
				// send quit
				memset(cmd, 0, CMD_SIZE);
				sprintf(cmd, "%s", "QUIT");
				if (UDT::ERROR == UDT::send(sockData, cmd, CMD_SIZE, 0))
					return 108;
				CGuard::leaveCS(m_Lock);
				return 106;
			}
			CGuard::leaveCS(m_Lock);
			// receive file
			UDT::TRACEINFO trace;
			UDT::perfmon(sockData, &trace);
			int64_t recv = 0;
			if (left > TO_SND)
				recv = UDT::recvfile(sockData, ofs, offset, TO_SND);
			else
				recv = UDT::recvfile(sockData, ofs, offset, left);
			if (UDT::ERROR == recv)
				return 108;
			UDT::perfmon(sockData, &trace);

			left -= recv;
			nRecvSize += recv;
			// send recv rate
			nReturnCode = (int)(trace.mbpsRecvRate * 100);
			memset(cmd, 0, CMD_SIZE);
			sprintf(cmd, "%d", nRecvSize);
			szTmp = cmd;
			memset(cmd, 0, CMD_SIZE);
			sprintf(cmd, "%d", nReturnCode);
			szTmp2 = cmd;
			memset(cmd, 0, CMD_SIZE);
			sprintf(cmd, "%s%s%s%s%s%s", "214 ", szTmp.c_str(), " ", szTmp2.c_str(), " ", szRemoteFileName.c_str());
			if (UDT::ERROR == UDT::send(sockCtrl, cmd, CMD_SIZE, 0))
				return 108;
			m_pCallBack->onTransfer(nFileTotalSize, nRecvSize, trace.mbpsRecvRate, szFilePath.c_str(), 1, sockData);
		}
		ofs.close();

		// 文件夹，只回调一次
		szTmp = szFilePath.substr(szReplyPath.size());
		nPos = szTmp.find_first_of("/");
		if (nPos < 0)
		{
			nPos = szTmp.find_first_of("\\");
		}
		if (nPos >= 0)
		{
			szTmp = szTmp.substr(0, nPos);
			if (szTmp != szFolder)
			{
				szFolder = szTmp;
				szTmp = szReplyPath + szFolder;
				m_pCallBack->onAcceptonFinish(cxt->strClientAddr.c_str(), szTmp.c_str(), nFileSize, 1, sockCtrl);
			}
		}
		else
			m_pCallBack->onAcceptonFinish(cxt->strClientAddr.c_str(), szFilePath.c_str(), nFileSize, 1, sockCtrl);
	}

	// send quit
	memset(cmd, 0, CMD_SIZE);
	sprintf(cmd, "%s", "QUIT");
	if (UDT::ERROR == UDT::send(sockData, (char*)cmd, CMD_SIZE, 0))
		return 108;
	memset(cmd, 0, CMD_SIZE);
	sprintf(cmd, "226 %s", "transfer ok");
	if (UDT::ERROR == UDT::send(sockCtrl, cmd, CMD_SIZE, 0))
		return 108;

	m_pCallBack->onFinished("recv file sucess", 110, sockCtrl);
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
	CLIENTCONEXT * cxt = (CLIENTCONEXT *)pParam;
	int nReturnCode = 0;

	if (cxt->Type == OP_SND_CTRL)
	{
		cxt->pThis->ProcessSendCtrl(cxt);
	}
	else if (cxt->Type == OP_SND_FILE)
	{
	}
	else if (cxt->Type == OP_RCV_CTRL)
	{
		cxt->pThis->ProcessRecvCtrl(cxt);
	}
	else if (cxt->Type == OP_RCV_FILE)
	{
		nReturnCode = cxt->pThis->ProcessRecvFile(cxt);
		if (nReturnCode == 119)
		{
			UDT::close(cxt->sockAccept);
		}
	}

#ifndef WIN32
	return NULL;
#else
	return 0;
#endif
}