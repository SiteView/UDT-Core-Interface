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

CUdtCore *CUdtProxy::core() const
{
	return m_pUdt;
}

int CUdtProxy::GetSockID(const int nType)
{
	return m_sock;
}

//////////////////////////////////////////////////////////////////////////
// Call Back
void CUdtProxy::onAccept(const char* pstrAddr, const char* pstrFileName, int nFileCount, const int64_t nFileSize, const char* recdevice, const char* rectype, const char* owndevice, const char* owntype, const char* SendType, const char* FileType, int sock)
{
	std::cout<< "Accept file name:" << pstrFileName << std::endl;
	m_sock = sock;
}

void CUdtProxy::onAcceptonFinish(const char* pstrAddr, const char* pFileName, const int64_t nFileSize, int Type, int sock)
{
	if (Type == 1)
		std::cout<< "Recv onAcceptonFinish file name:" << pFileName << std::endl;
	else
		std::cout<< "Send onAcceptonFinish file name:" << pFileName << std::endl;
}

void CUdtProxy::onFinished(const char * pstrMsg, int Type, int sock)
{
	if (Type == 1)
		std::cout<< "onRecvFinished:" << pstrMsg << std::endl;
	else
		std::cout<< "onSendFinished:" << pstrMsg << std::endl;
}

void CUdtProxy::onTransfer(const int64_t nFileTotalSize, const int64_t nCurrent, const double iProgress, const char* pstrFileName, int Type, int sock)
{
	double dTotal = (double)nFileTotalSize;
	double dPercent = nCurrent / dTotal;
	static char * Tmp = "%";
	if (Type == 1)
		printf("Recv file name:%s, percent:%.2f%s, %.2fMbits/sec\n", pstrFileName, dPercent*100, Tmp, iProgress);
	else
		printf("Send file name:%s, percent:%.2f%s, %.2fMbits/sec\n", pstrFileName, dPercent*100, Tmp, iProgress);
}

void CUdtProxy::onRecvMessage(const char* pstrMsg, const char* pIpAddr, const char* pHostName)
{
	std::cout<< "Recv message:" << pstrMsg << std::endl;
}

void CUdtProxy::onTTSPing(const char* pstrIp, int Type)
{

}