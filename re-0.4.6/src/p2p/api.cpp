#include "udt.h"
#include "core.h"
#include "p2p.h"


////////////////////////////////////////////////////////////////////////////////
// UDT_API
namespace UDT
{
	UDTSOCKET socket(int af, int type, int protocol)
	{
		//return CUDT::socket(af, type, protocol);
		return 0;
	}

	int bind(UDTSOCKET u, const struct sockaddr* name, int namelen)
	{
		//return CUDT::bind(u, name, namelen);
		return 0;
	}

	int listen(UDTSOCKET u, int backlog)
	{
		//return CUDT::listen(u, backlog);
		return 0;
	}

	UDTSOCKET accept(UDTSOCKET u, struct sockaddr* addr, int* addrlen)
	{
		//return CUDT::accept(u, addr, addrlen);
		return 0;
	}

	int connect(UDTSOCKET u, const struct sockaddr* name, int namelen)
	{
		return CUDT::connect(u, name, namelen);
	}

	int close(UDTSOCKET u)
	{
		//return CUDT::close(u);
		return 0;
	}

	int getpeername(UDTSOCKET u, struct sockaddr* name, int* namelen)
	{
		//return CUDT::getpeername(u, name, namelen);
		return 0;
	}

	int getsockname(UDTSOCKET u, struct sockaddr* name, int* namelen)
	{
		//return CUDT::getsockname(u, name, namelen);
		return 0;
	}

	int getsockopt(UDTSOCKET u, int level, SOCKOPT optname, void* optval, int* optlen)
	{
		//return CUDT::getsockopt(u, level, optname, optval, optlen);
		return 0;
	}

	int setsockopt(UDTSOCKET u, int level, SOCKOPT optname, const void* optval, int optlen)
	{
		//return CUDT::setsockopt(u, level, optname, optval, optlen);
		return 0;
	}

	int send(UDTSOCKET u, const char* buf, int len, int flags)
	{
		return CUDT::send(u, buf, len, flags);
		return 0;
	}

	int recv(UDTSOCKET u, char* buf, int len, int flags)
	{
		//return CUDT::recv(u, buf, len, flags);
		return 0;
	}

	int64_t sendfile(UDTSOCKET u, std::fstream& ifs, int64_t& offset, int64_t size, int block)
	{
		//return CUDT::sendfile(u, ifs, offset, size, block);
		return 0;
	}

	int64_t recvfile(UDTSOCKET u, std::fstream& ofs, int64_t& offset, int64_t size, int block)
	{
		//return CUDT::recvfile(u, ofs, offset, size, block);
		return 0;
	}


	int perfmon(UDTSOCKET u, TRACEINFO* perf, bool clear)
	{
		//return CUDT::perfmon(u, perf, clear);
		return 0;
	}

	//UDTSTATUS getsockstate(UDTSOCKET u)
	//{
		//return CUDT::getsockstate(u);
	//	return 0;
	//}

	int p2p_init(const char* ctlip, const uint16_t ctlport, const char* stunip, const uint16_t stunport, const char* name, const char* passwd)
	{
		return CUDT::p2p_init(ctlip, ctlport, stunip, stunport, name, passwd);
	}

	int p2p_connect(const char* peername)
	{
		return CUDT::p2p_connect(peername);
	}

	int p2p_disconnect(const char* peername)
	{
		return CUDT::p2p_disconnect(peername);
	}

	int p2p_send(const char* peername, const char* buf, int len)
	{
		return CUDT::p2p_send(peername, buf, len);
	}

	int p2p_close()
	{
		return 0;
	}

}  // namespace UDT

// init callback
void p2p_init_handler(int err, struct p2phandle* p2p)
{
	//p2p->usock = UDT::socket(AF_INET, SOCK_STREAM, 0);
	re_printf("init return:%d\n", err);
}

// connect callback
void p2p_request_handler(struct p2phandle *p2p, const char* peername, struct p2pconnection **ppconn)
{
	int err;
	err = p2p_accept(p2p, ppconn, peername, p2p_receive_handler, *ppconn);
	int fdaccept = udp_sock_fd((*ppconn)->ulsock, AF_INET);
	struct sa* lsa = NULL;
	lsa = (struct sa*)mem_zalloc(sizeof(struct sa),NULL);
	sa_cpy(lsa, p2p->sactl);
	int fdlisten = tcp_conn_fd(p2p->ltcp);
	UDPSOCKET udpsock = (UDPSOCKET)fdlisten;
	p2p->usock = CUDT::socket(AF_INET, SOCK_STREAM, 0);
	CUDT::bind(p2p->usock, &udpsock, &(p2p->sactl->u.sa), p2p->sactl->len);
	CUDT::listen(p2p->usock, 10);
	//CUDT::newConnection(p2p->usock, &(p2p->sactl->u.sa), NULL);
	if (err)
	{
		re_fprintf(stderr, "p2p accept error:%s\n", strerror(err));
		return ;
	}
}

// receive callback
void p2p_receive_handler(const char* data, uint32_t len, struct p2pconnection* p2pcon)
{
	//struct p2pconnection* p2pcon = (struct p2pconnection*)arg;
	int fd = udp_sock_fd(p2pcon->ulsock, AF_INET);
	CUDT::postRecv(p2pcon->p2p->usock, data, len);
	re_printf("p2p receive data:%s\n", data);
}

// tcp close callback
void tcp_close_handler(int err, void *arg)
{
	struct p2phandle *p2p = (struct p2phandle *)arg;
	if (!err && p2p->inith)
		p2p->inith(err, p2p);

	re_printf("tcp closed error:%s(%d)\n", strerror(err),err);
}


//////////////////////////////////////////////////////////////////////////
// API
int CUDT::p2p_init(const char* ctlip, const uint16_t ctlport, const char* stunip, const uint16_t stunport, const char* name, const char* passwd)
{
	try
	{
		return s_UDTUnited.p2p_init(ctlip, ctlport, stunip, stunport, name, passwd, p2p_init_handler, p2p_request_handler);
	}
	catch (...)
	{
		return INVALID_SOCK;
	}
}

int CUDT::p2p_connect(const char* peername)
{
	try
	{
		return s_UDTUnited.p2p_connect(peername);
	}
	catch (...)
	{
		return INVALID_SOCK;
	}
}

int CUDT::p2p_disconnect(const char* peername)
{
	try
	{
		return s_UDTUnited.p2p_disconnect(peername);
	}
	catch (...)
	{
		return INVALID_SOCK;
	}
}

int CUDT::p2p_send(const char* peername, const char* buf, int len)
{
	try
	{
		return s_UDTUnited.p2p_send(peername, buf, len);
	}
	catch (...)
	{
		return INVALID_SOCK;
	}
}

int CUDT::p2p_run()
{
	try
	{
		return s_UDTUnited.p2p_run();
	}
	catch (...)
	{
		return INVALID_SOCK;
	}
}

int CUDT::p2p_close()
{
	try
	{
		return s_UDTUnited.p2p_close();
	}
	catch (...)
	{
		return INVALID_SOCK;
	}
}

UDTSOCKET CUDT::socket(int af, int type, int protocol)
{
	try
	{
		return s_UDTUnited.newSocket(af, type);
	}
	catch (...)
	{
		return INVALID_SOCK;
	}
}

int CUDT::bind(UDTSOCKET u, const UDPSOCKET* udpsaock, const sockaddr* name, int namelen)
{
	try
	{
		return s_UDTUnited.bind(u, udpsaock, name, namelen);
	}
	catch (...)
	{
		return INVALID_SOCK;
	}
}

int CUDT::listen(UDTSOCKET u, int backlog)
{
	try
	{
		return s_UDTUnited.listen(u, backlog);
	}
	catch (...)
	{
		return INVALID_SOCK;
	}
}

int CUDT::newConnection(const UDTSOCKET listen, const sockaddr* peer, CHandShake* hs)
{
	try
	{
		return s_UDTUnited.newConnection(listen, peer, hs);
	}
	catch (...)
	{
		return INVALID_SOCK;
	}
}

int CUDT::connect(UDTSOCKET u, const sockaddr* name, int namelen)
{
	try
	{
		return s_UDTUnited.connect(u, name, namelen);
	}
	catch (...)
	{
		return INVALID_SOCK;
	}
}

int CUDT::postRecv(UDTSOCKET u, const char* buf, uint32_t len)
{
	try
	{
		return s_UDTUnited.postRecv(u, buf, len);
	}
	catch (...)
	{
		return INVALID_SOCK;
	}
}

int CUDT::send(UDTSOCKET u, const char* buf, int len, int flags)
{
	try
	{
		CUDT* udt = s_UDTUnited.lookup(u);
		return udt->send(buf, len);
	}
	catch (...)
	{
		return INVALID_SOCK;
	}
}

int CUDT::getpeername(UDTSOCKET u, sockaddr* name, int* namelen)
{
	return 0;
}

int CUDT::getsockname(UDTSOCKET u, sockaddr* name, int* namelen)
{
	return 0;
}

CUDT* CUDT::getUDTHandle(UDTSOCKET u)
{
	return NULL;
}

UDTSTATUS CUDT::getsockstate(UDTSOCKET u)
{
	return NONEXIST;
}