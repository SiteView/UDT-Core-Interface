// UDTTest.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include <vector>
#include "UDTProxy.h"

int _tmain(int argc, _TCHAR* argv[])
{
	printf("\t/***********************************\n");
	printf("\t 1 - �����ı���Ϣ   2 - ���͵����ļ�\n\t 3 - ���Ͷ���ļ�   4 - ����Ŀ¼�ļ�\n\t 5 - ֹͣ�����ļ�   6 - ֹͣ�����ļ�\n\t 7 - �Ƿ�����ļ�   0 - �˳�\n");
	printf("\t ***********************************/\n");

	CUdtProxy * pUdt = CUdtProxy::GetInStance();
	pUdt->listenFileSend(7777);

	char ip[32] = {0};
	char buf[256] = {0};
	while (gets_s(buf))
	{
		if (strcmp(buf, "1") == 0)
		{
			printf("����Ŀ��IP��");
			gets_s(ip);
			printf("������Ϣ���� :");
			gets_s(buf);
			pUdt->sendMessage(ip, 7777, buf, "PC", "GENIEMAP");
		}
		else if (strcmp(buf, "2") == 0)
		{
			printf("����Ŀ��IP:");
			gets_s(ip);
			printf("�����ļ���ȫ·�� :");
			char fileName[256];
			gets_s(fileName);
			pUdt->sendfile(ip, 7777, fileName, "PC", "GENIEMAP");
		}
		else if (strcmp(buf, "3") == 0)
		{
			printf("����Ŀ��IP��");
			gets_s(ip);
			printf("�����ļ���ȫ·�� , end ��������\n");
			std::vector<std::string> vecNames;
			char fileName[256];
			while (gets_s(fileName) != NULL)
			{
				if (strcmp("end", fileName) == 0)
					break;
				vecNames.push_back(fileName);
			}
			pUdt->sendMultiFiles(ip, 7777, vecNames, "PC", "GENIEMAP");
		}
		else if (strcmp(buf, "4") == 0)
		{
			printf("����Ŀ��IP��");
			gets_s(ip);
			printf("�����ļ���ȫ·�� :");
			char fileName[256];
			gets_s(fileName);
			pUdt->sendFolderFiles(ip, 7777, fileName, "PC", "GENIEMAP");
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
			printf("ȷ�����գ������ļ��洢·��:D:\\Genie\\���ܾ����գ�����:REJECT\n");
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

