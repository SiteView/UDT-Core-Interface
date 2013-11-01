#ifndef __UDTPROXY_H__
#define __UDTPROXY_H__

#include <vector>
#include "UdtCore.h"

class CUdtProxy : public CUDTCallBack
{
public:
	static CUdtProxy * GetInStance();
	CUdtCore *core() const;

	int GetSockID(const int nType);

private:
	CUdtProxy();
	~CUdtProxy();

protected:
	// Call back
	virtual void onAccept(const char* pstrAddr, const char* pstrFileName, int nFileCount, const int64_t nFileSize, const char* recdevice, const char* rectype, const char* owndevice, const char* owntype, const char* SendType, const char* FileType, int sock);
	virtual void onAcceptonFinish(const char* pstrAddr, const char* pFileName, int Type, int sock);
	virtual void onFinished(const char * pstrMsg, int Type, int sock);
	virtual void onTransfer(const int64_t nFileTotalSize, const int64_t nCurrent, const double iProgress, const char* pstrFileName, int Type, int sock);
	virtual void onRecvMessage(const char* pstrMsg, const char* pIpAddr, const char* pHostName);
	virtual void onTTSPing(const char* pstrIp, int Type);

private:
	static CUdtProxy * m_pUdtProxy;
	CUdtCore * m_pUdt;
	int m_sock;
};

#endif	// __UDTPROXY_H__
