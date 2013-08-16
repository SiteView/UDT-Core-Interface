#include "UdtProxy.h"

CUdtProxy * CUdtProxy::m_pUdtProxy = nullptr;

CUdtProxy::CUdtProxy()
{
	m_pUdt = new CUdtCore(this);
}


CUdtProxy::~CUdtProxy()
{
}

CUdtProxy * CUdtProxy::GetInStance()
{
	if (m_pUdtProxy == nullptr)
	{
		m_pUdtProxy = new CUdtProxy;
	}

	return m_pUdtProxy;
}


int CUdtProxy::Init(const int nCtrlPort, const int nRcvPort)
{
	if (m_pUdt->StartListen(nCtrlPort, nRcvPort) < 0)
		return -1;

	return 0;
}

void CUdtProxy::sendMessage(const char* pstrAddr, const int nPort, const char* pstrMessage, const char* pstrHostname, const char* pstrSendtype)
{
	std::vector<std::string> vecTexts;
	vecTexts.push_back(pstrMessage);
	m_sock = m_pUdt->SendFiles(pstrAddr, vecTexts, pstrHostname, pstrHostname, pstrHostname, pstrHostname, pstrSendtype);
}

void CUdtProxy::sendfile(const char* pstrAddr, const int nPort, const char* pstrFileName, const char* pstrHostname, const char *pstrSendtype)
{
	std::vector<std::string> vecTexts;
	vecTexts.push_back(pstrFileName);
	m_sock = m_pUdt->SendFiles(pstrAddr, vecTexts, "HTC G18", "ANDROID", "zhujianwen", "WIN7", "GENIETURBO");
}

void CUdtProxy::sendMultiFiles(const char* pstrAddr, const int nPort, const std::vector<std::string> strArray, const char* pstrHostName, const char* pstrSendtype)
{
	m_sock = m_pUdt->SendFiles(pstrAddr, strArray, pstrHostName, pstrHostName, pstrHostName, pstrHostName, pstrSendtype);
}

void CUdtProxy::sendFolderFiles(const char* pstrAddr, const int nPort, const char* pstrFolderName, const char* pstrHostName, const char* pstrSendtype)
{
	std::vector<std::string> vecTexts;
	vecTexts.push_back(pstrFolderName);
	m_sock = m_pUdt->SendFiles(pstrAddr, vecTexts, pstrHostName, pstrHostName, pstrHostName, pstrHostName, pstrSendtype);
}

void CUdtProxy::replyAccept(const char* pstrReply)
{
	m_pUdt->ReplyAccept(m_sock, pstrReply);
}

void CUdtProxy::stopTransfer(const int nType)
{
	m_pUdt->StopTransfer(m_sock, nType);
}

void CUdtProxy::stopListen()
{
	m_pUdt->StopListen();
}


//////////////////////////////////////////////////////////////////////////
// Call Back
void CUdtProxy::onAccept(const char* pstrAddr, const char* pstrFileName, int nFileCount, const char* recdevice, const char* rectype, const char* owndevice, const char* owntype, const char* SendType, int sock)
{
	std::cout<< "Accept file name:" << pstrFileName << std::endl;

	char path[MAX_PATH];
#if WIN32
	GetCurrentDirectoryA(MAX_PATH, path);
#else
	if (NULL == getcwd(path, MAX_PATH))
		Error("Unable to get current path");
#endif

	if ('\\' != path[strlen(path)-1])
	{
		strcat(path, ("\\"));
	}

	m_sock = sock;
	m_pUdt->ReplyAccept(sock, path);
	std::cout<< "save file to:" << path << std::endl;
}

void CUdtProxy::onAcceptonFinish(const char* pstrAddr, const char* pFileName, int Type, int sock)
{
	std::cout<< "onAcceptonFinish file name:" << pFileName << std::endl;
}

void CUdtProxy::onFinished(const char * pstrMsg, int Type, int sock)
{
	std::cout<< "onSendFinished:" << pstrMsg << std::endl;
}

void CUdtProxy::onTransfer(const int64_t nFileTotalSize, const int64_t nCurrent, const char* pstrFileName, int Type, int sock)
{
	double dTotal = (double)nFileTotalSize;
	double dPercent = nCurrent / dTotal;
	static char * Tmp = "%";
	printf("Send file name:%s, percent:%.2f%s\n", pstrFileName, dPercent*100, Tmp);
}

void CUdtProxy::onRecvMessage(const char* pstrMsg, const char* pIpAddr, const char* pHostName)
{
	std::cout<< "Recv message:" << pstrMsg << std::endl;
}