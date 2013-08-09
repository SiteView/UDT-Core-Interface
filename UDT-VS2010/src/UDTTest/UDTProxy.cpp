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
	m_pUdt->SendFile(pstrAddr, nPort, pstrHostname, pstrSendtype, vecTexts, 1);
}

void CUdtProxy::sendfile(const char* pstrAddr, const int nPort, const char* pstrFileName, const char* pstrHostname, const char *pstrSendtype)
{
	std::vector<std::string> vecTexts;
	vecTexts.push_back(pstrFileName);
	m_pUdt->SendFile(pstrAddr, nPort, pstrHostname, pstrSendtype, vecTexts, 2);
}

void CUdtProxy::sendMultiFiles(const char* pstrAddr, const int nPort, const std::vector<std::string> strArray, const char* pstrHostName, const char* pstrSendtype)
{
	m_pUdt->SendFile(pstrAddr, nPort, pstrHostName, pstrSendtype, strArray, 3);
}

void CUdtProxy::sendFolderFiles(const char* pstrAddr, const int nPort, const char* pstrFolderName, const char* pstrHostName, const char* pstrSendtype)
{
	std::vector<std::string> vecTexts;
	vecTexts.push_back(pstrFolderName);
	m_pUdt->SendFile(pstrAddr, nPort, pstrHostName, pstrSendtype, vecTexts, 4);
}

void CUdtProxy::replyAccept(const char* pstrReply)
{
	m_pUdt->ReplyAccept(0, pstrReply);
}

void CUdtProxy::stopTransfer(const int nType)
{
	m_pUdt->StopTransfer(0, nType);
}

void CUdtProxy::stopListen()
{
	m_pUdt->StopListen();
}


//////////////////////////////////////////////////////////////////////////
// Call Back
void CUdtProxy::onAccept(const char* pstrIpAddr, const char* pstrHostName, const char* pstrSendType, const char* pstrFileName, int nFileCount)
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

	m_pUdt->ReplyAccept(0, path);
	std::cout<< "save file to:" << path << std::endl;
}

void CUdtProxy::onAcceptFolder(const char* pstrAddr, const char* pstrHostName, const char* pstrSendType, const char* pstrFileName, int nFileCount)
{
	std::cout<< "onAcceptFolder file name:" << pstrFileName << std::endl;

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

	m_pUdt->ReplyAccept(0, path);
	std::cout<< "save file to:" << path << std::endl;
}

void CUdtProxy::onAcceptonFinish(const char* pstrAddr, std::vector<std::string> vecFileName)
{
	std::cout<< "onAcceptonFinish file name:" << vecFileName[0] << std::endl;
}

void CUdtProxy::onSendFinished(const char * pstrMsg)
{
	std::cout<< "onSendFinished:" << pstrMsg << std::endl;
}

void CUdtProxy::onRecvFinished(const char * pstrMsg)
{
	std::cout<< "onRecvFinished:" << pstrMsg << std::endl;
}

void CUdtProxy::onSendTransfer(const int64_t nFileTotalSize, const int64_t nCurrent, const char* pstrFileName)
{
	double dTotal = (double)nFileTotalSize;
	double dPercent = nCurrent / dTotal;
	static char * Tmp = "%";
	printf("Send file name:%s, percent:%.2f%s\n", pstrFileName, dPercent*100, Tmp);
}

void CUdtProxy::onRecvTransfer(const int64_t nFileTotalSize, const int64_t nCurrent, const char* pstrFileName)
{
	double dTotal = (double)nFileTotalSize;
	double dPercent = nCurrent / dTotal;
	static char * Tmp = "%";
	printf("Recv file name:%s, percent:%.2f%s\n", pstrFileName, dPercent*100, Tmp);
}

void CUdtProxy::onRecvMessage(const char* pstrIpAddr, const char* pstrHostName, const char* pstrMsg)
{
	std::cout<< "Recv message:" << pstrMsg << std::endl;
}