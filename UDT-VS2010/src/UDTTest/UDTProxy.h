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
	virtual void onAccept(const char* pstrIpAddr, const char* pstrHostName, const char* pstrSendType, const char* pstrFileName, int nFileCount);
	virtual void onAcceptFolder(const char* pstrAddr, const char* pstrHostName, const char* pstrSendType, const char* pstrFileName, int nFileCount);
	virtual void onAcceptonFinish(const char* pstrAddr, std::vector<std::string> vecFileName);
	virtual void onSendFinished(const char * pstrMsg);
	virtual void onRecvFinished(const char * pstrMsg);
	virtual void onSendTransfer(const int64_t nFileTotalSize, const int64_t nCurrent, const char* pstrFileName);
	virtual void onRecvTransfer(const int64_t nFileTotalSize, const int64_t nCurrent, const char* pstrFileName);
	virtual void onRecvMessage(const char* pstrIpAddr, const char* pstrHostName, const char* pstrMsg);

private:
	static CUdtProxy * m_pUdtProxy;
	CUdtCore * m_pUdt;
};

#endif	// __UDTPROXY_H__
