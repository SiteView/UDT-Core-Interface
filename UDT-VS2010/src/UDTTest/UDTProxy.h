#ifndef __UDTPROXY_H__
#define __UDTPROXY_H__

#include <vector>
#include "UdtCore.h"

class CUdtProxy : public CUDTCallBack
{
public:
	static CUdtProxy * GetInStance();

	// 开启服务、监听端口
	int Init(const int nCtrlPort, const int nRcvPort);
	// 发送文本消息
	void sendMessage(const char* pstrAddr, const int nPort, const char* pstrMessage, const char* pstrHostname, const char* pstrSendtype);
	// 发送单个文件
	void sendfile(const char* pstrAddr, const int nPort, const char* pstrFileName, const char* pstrHostname, const char *pstrSendtype);
	// 发送多个文件
	void sendMultiFiles(const char* pstrAddr, const int nPort, const std::vector<std::string> strArray, const char* pstrHostName, const char* pstrSendtype);
	// 发送文件夹
	void sendFolderFiles(const char* pstrAddr, const int nPort, const char* pstrFolderName, const char* pstrHostName, const char* pstrSendtype);
	// 是否接收
	void replyAccept(const char* pstrReply);
	// 停止发送、停止接收
	void stopTransfer(const int nType);
	// 停止服务、关闭端口
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
