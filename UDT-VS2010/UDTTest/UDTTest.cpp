// UDTTest.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include <vector>
#include "UDTProxy.h"

int _tmain(int argc, _TCHAR* argv[])
{
	printf("\t/***********************************\n");
	printf("\t 1 - �����ı���Ϣ   2 - ���͵����ļ�\n\t 3 - ֹͣ�����ļ�   4 - ֹͣ�����ļ�\n\t 5 - �Ƿ�����ļ�   0 - �˳�\n");
	printf("\t ***********************************/\n");

	CUdtProxy * pUdt = CUdtProxy::GetInStance();

	pUdt->core()->StartListen(7777, 7778);

	int sock;
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
			pUdt->core()->SendMsg(ip, buf, "zhujianwen");
		}
		else if (strcmp(buf, "2") == 0)
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
			sock = pUdt->core()->SendFiles(ip, vecNames, "HTC G18", "ANDROID", "zhujianwen", "WIN7", "GENIETURBO");
		}
		else if (strcmp(buf, "3") == 0)
		{
			pUdt->core()->StopTransfer(sock, 2);
			//pUdt->core()->StartListen(7777, 7778);
		}
		else if (strcmp(buf, "4") == 0)
		{
			pUdt->core()->StopTransfer(sock, 1);
			//pUdt->core()->StopListen();
		}
		else if (strcmp(buf, "5") == 0)
		{
			printf("ȷ�����գ������ļ��洢·��:D:\\Genie\\���ܾ����գ�����:REJECT\n");
			char filePath[256];
			gets_s(filePath);
			pUdt->core()->ReplyAccept(sock, filePath);
		}
		else if (strcmp(buf, "0") == 0)
		{
			printf("quit.....\n");
			break;
		}
	}

	pUdt->core()->StopListen();
	return 0;
}

