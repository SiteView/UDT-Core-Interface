// p2ptest.cpp : 定义控制台应用程序的入口点。
//

#include <iostream>
#include <re.h>

uint64_t lastick = 0;

void p2p_connect_handler(int err,const char* reason, void* arg)
{
	DWORD dwNow = ::GetTickCount();

	re_printf("connect ok, spend(s):%d\n", (DWORD)(dwNow-lastick)/1000);
}

void p2p_init_handler(int err, struct p2p* p2p)
{
	re_printf("init return:%d\n", err);
}
void p2p_receive_handler(const char* data, uint32_t len, void *arg)
{
	re_printf("p2p receive data:%s\n", data);
}

void p2p_request_handler(struct p2p *p2p, const char* peername, struct p2pconnection **ppconn)
{
	int err;
	err = p2p_accept(p2p, ppconn, peername, p2p_receive_handler, *ppconn);
	if (err)
	{
		re_fprintf(stderr, "p2p accept error:%s\n", strerror(err));
	}
}

static int print_handler(const char *p, size_t size, void *arg)
{
	struct pl *pl = (struct pl*)arg;

	if (size > pl->l)
		return ENOMEM;

	memcpy((void *)pl->p, p, size);

	pl_advance(pl, size);

	return 0;
}


void print_icem(struct icem *icem)
{
	struct re_printf pt;
	struct pl pl;
	char *buf = new char[2048];

	memset(buf, 0, 2048);
	pl.l = 2048;
	pl.p = buf;
	pt.vph = print_handler;
	pt.arg = &pl;

	memset(buf, 0, 2048);
	pl.l = 2048;
	pl.p = buf;
	icem_debug(&pt, icem);
	re_printf("%s\n",buf);
}

#ifndef WIN32
void* monitor(void*);
#else
DWORD WINAPI monitor(LPVOID);
#endif

int main(int argc, char* argv[])
{
#ifdef WIN32
	char* filePath = argv[0];
	memset(argv, 0, sizeof(argv));
	argc = 2;
	argv[0] = filePath;
	argv[1] = "7777";
#endif

	//usage: sendfile [server_port]
	if ((2 < argc) || ((2 == argc) && (0 == atoi(argv[1]))))
	{
		cout << "usage: sendfile [server_port]" << endl;
		return 0;
	}

	int err = P2P::p2p_init("54.254.169.186", 3456, "54.254.169.186", 3478, "test4", "test4");
	if (err)
	{
		re_fprintf(stderr, "p2p init error:%s\n", strerror(err));
		goto out;
	}

	P2PSOCKET psock = 0;
	
#ifndef WIN32
	pthread_t filethread;
	pthread_create(&filethread, NULL, monitor, new P2PSOCKET(fhandle));
	pthread_detach(filethread);
#else
	CreateThread(NULL, 0, monitor, new P2PSOCKET(fhandle), 0, NULL);
#endif
	err = P2P::p2p_run();
	return 0;

out:
	P2P::p2p_close();
	return err;
}

#ifndef WIN32
void* monitor(void* usocket)
#else
DWORD WINAPI monitor(LPVOID usocket)
#endif
{
	P2PSOCKET fhandle = *(P2PSOCKET*)usocket;
	delete (P2PSOCKET*)usocket;
	bool bConnect = false;

	while(true){

		if (!bConnect)
		{
			lastick = ::GetTickCount();
			P2P::p2p_connect("android_test");
		}

		if (pconn && pconn->status == P2P_CONNECTED)
		{
			icem = (struct icem *)list_ledata(list_head(ice_medialist(pconn->ice)));
			icem_update(icem);
			print_icem(icem);
			//p2p_send(pconn,"hello!", 6);
		}

		Sleep(5000);
	}

#ifndef WIN32
	return NULL;
#else
	return 0;
#endif
}