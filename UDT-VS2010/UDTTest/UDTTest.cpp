// UDTTest.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <vector>
#include "UDTProxy.h"

int _tmain(int argc, _TCHAR* argv[])
{
	printf("\t/***********************************\n");
	printf("\t 1 - 发送文本消息   2 - 发送单个文件\n\t 3 - 发送多个文件   4 - 发送目录文件\n\t 5 - 停止发送文件   6 - 停止接收文件\n\t 7 - 是否接收文件   0 - 退出\n");
	printf("\t ***********************************/\n");

	CUdtProxy * pUdt = CUdtProxy::GetInStance();
	pUdt->Init(7777, 7778);

	char ip[32] = {0};
	char buf[256] = {0};
	while (gets_s(buf))
	{
		if (strcmp(buf, "1") == 0)
		{
			printf("输入目标IP：");
			gets_s(ip);
			printf("输入消息内容 :");
			gets_s(buf);
			pUdt->sendMessage(ip, 7000, buf, "PC", "GENIEMAP");
		}
		else if (strcmp(buf, "2") == 0)
		{
			printf("输入目标IP:");
			gets_s(ip);
			printf("输入文件名全路径 :");
			char fileName[256];
			gets_s(fileName);
			pUdt->sendfile(ip, 7000, fileName, "PC", "GENIEMAP");
		}
		else if (strcmp(buf, "3") == 0)
		{
			printf("输入目标IP：");
			gets_s(ip);
			printf("输入文件名全路径 , end 结束输入\n");
			std::vector<std::string> vecNames;
			char fileName[256];
			while (gets_s(fileName) != NULL)
			{
				if (strcmp("end", fileName) == 0)
					break;
				vecNames.push_back(fileName);
			}
			pUdt->sendMultiFiles(ip, 7000, vecNames, "PC", "GENIEMAP");
		}
		else if (strcmp(buf, "4") == 0)
		{
			printf("输入目标IP：");
			gets_s(ip);
			printf("输入文件名全路径 :");
			char fileName[256];
			gets_s(fileName);
			pUdt->sendFolderFiles(ip, 7000, fileName, "PC", "GENIEMAP");
		}
		else if (strcmp(buf, "5") == 0)
		{
			pUdt->stopTransfer(1);
		}
		else if (strcmp(buf, "6") == 0)
		{
			pUdt->stopTransfer(2);
		}
		else if (strcmp(buf, "7") == 0)
		{
			printf("确定接收，输入文件存储路径:D:\\Genie\\；拒绝接收，输入:REJECT\n");
			char filePath[256];
			gets_s(filePath);
			pUdt->replyAccept(filePath);
		}
		else if (strcmp(buf, "0") == 0)
		{
			printf("quit.....\n");
			break;
		}
	}

	pUdt->stopListen();
	return 0;
}

