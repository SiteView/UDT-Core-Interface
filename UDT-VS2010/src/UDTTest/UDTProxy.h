#ifndef __UDTPROXY_H__
#define __UDTPROXY_H__

#include <vector>
#include "UdtCore.h"

class CUdtProxy : public CUDTCallBack
{
public:
	static CUdtProxy * GetInStance();

	// �������񡢼����˿�
	int Init(const int nCtrlPort, const int nRcvPort);
	// �����ı���Ϣ
	void sendMessage(const char* pstrAddr, const int nPort, const char* pstrMessage, const char* pstrHostname, const char* pstrSendtype);
	// ���͵����ļ�
	void sendfile(const char* pstrAddr, const int nPort, const char* pstrFileName, const char* pstrHostname, const char *pstrSendtype);
	// ���Ͷ���ļ�
	void sendMultiFiles(const char* pstrAddr, const int nPort, const std::vector<std::string> strArray, const char* pstrHostName, const char* pstrSendtype);
	// �����ļ���
	void sendFolderFiles(const char* pstrAddr, const int nPort, const char* pstrFolderName, const char* pstrHostName, const char* pstrSendtype);
	// �Ƿ����
	void replyAccept(const char* pstrReply);
	// ֹͣ���͡�ֹͣ����
	void stopTransfer(const int nType);
	// ֹͣ���񡢹رն˿�
	void stopListen();

private:
	CUdtProxy();
	~CUdtProxy();

protected:
	// Call back
	virtual void onAccept(const char* pstrAddr, const char* pstrFileName, int nFileCount, const char* recdevice, const char* rectype, const char* owndevice, const char* owntype, const char* SendType, int sock);
	virtual void onAcceptonFinish(const char* pstrAddr, const char* pFileName, int Type, int sock);
	virtual void onFinished(const char * pstrMsg, int Type, int sock);
	virtual void onTransfer(const int64_t nFileTotalSize, const int64_t nCurrent, const char* pstrFileName, int Type, int sock);
	virtual void onRecvMessage(const char* pstrMsg, const char* pIpAddr, const char* pHostName);

private:
	static CUdtProxy * m_pUdtProxy;
	CUdtCore * m_pUdt;
	int m_sock;
};

#endif	// __UDTPROXY_H__
