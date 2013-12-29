#include "p2p.h"


#define UNUSED(x) (void)x 

const uint16_t p2pflag = 0xc000;


void p2p_handle_destructor(void *arg);
void p2p_connection_destructor(void *arg);
void tcp_close_handler(int err, void *arg);
void tcp_estab_handler(void *arg);
void tcp_recv_handler(struct mbuf *mb, void* arg);
int p2p_sdp_encode(struct ice *ice, struct icem *icem, struct mbuf *mbuf);
int p2p_send_login(struct tcp_conn *tc, const char* user, const char* passwd);
int p2p_handle_alloc(struct p2p_handle **p2, const char* user, const char* passwd);
int p2p_connection_alloc(struct p2p_handle *p2p,struct p2p_connection **pconn,const char* name);
int p2p_request(struct p2p_connection *pconn, const char* myname, const char* peername);
int p2p_response(struct p2p_connection *pconn, const char* peername);
struct p2p_connection* find_p2pconnection(struct p2p_handle *p2p, const char* name);
static int print_handler(const char *p, size_t size, void *arg);

//////////////////////////////////////////////////////////////////////////
// p2p instance destructor handler
void p2p_destructor(void *arg)
{
	struct p2p_handle *p2p = (struct p2p_handle*)arg;
	mem_deref(p2p->ltcp);
	mem_deref(p2p->sactl);
	mem_deref(p2p->sastun);
}

//////////////////////////////////////////////////////////////////////////
// p2pconnection destructor handler
void p2p_connection_destructor(void *arg)
{
	struct p2p_connection *pc = (struct p2p_connection *)arg;
	
	list_unlink(&(pc->le));

	mem_deref(pc->p2ph);
	mem_deref(pc->ice);
	mem_deref(pc->peername);
}

//////////////////////////////////////////////////////////////////////////
// find p2p_connection
struct p2p_connection* find_p2pconnection(struct p2p_handle *p2p, const char* name)
{
	struct le *le;
	struct list *lstconn = &(p2p->lstconn);

	for(le = list_head(lstconn); le; le=le->next)
	{
		struct p2p_connection *pconn = (struct p2p_connection *)(le->data);
		if (str_cmp(pconn->peername,name)==0)
		{
			return pconn;
		}
	}
	return NULL;
}

//////////////////////////////////////////////////////////////////////////
//
void tcp_estab_handler(void *arg)
{
	int err;

	struct p2p_handle *p2p = (struct p2p_handle *)arg;

	err = p2p_send_login(p2p->ltcp, p2p->username, p2p->passwd);
	if (err)
	{
		re_fprintf(stderr, "p2p register to control server error:%s\n",strerror(err));
	}
}

//////////////////////////////////////////////////////////////////////////
//
int p2p_sdp_encode(struct ice *ice, struct icem *icem, struct mbuf *mbuf)
{
	struct le *le;
	struct re_printf pt;
	struct pl pl;
	char buf[1024];

	struct list *canlist;

	if (!ice || !icem)
		return -1;

	canlist = icem_lcandl(icem);

	for(le = canlist->head; le; le = le->next)
	{
		memset(buf,0,1024);
		pl.l = 1024;
		pl.p = buf;
		pt.vph=print_handler;
		pt.arg=&pl;
		ice_cand_encode(&pt,(struct cand *)le->data);
		mbuf_write_u32(mbuf,str_len(buf)+2);
		mbuf_write_str(mbuf,"CA");
		mbuf_write_str(mbuf,buf);
		re_printf("send sdp:%s\n",buf);	
	}

	mbuf_write_u32(mbuf,str_len(ice_ufrag(ice))+2);
	mbuf_write_str(mbuf, "UF");
	mbuf_write_str(mbuf, ice_ufrag(ice));
	mbuf_write_u32(mbuf, str_len(ice_pwd(ice))+2);
	mbuf_write_str(mbuf, "PW");
	mbuf_write_str(mbuf, ice_pwd(ice));
	return 0;
}

//////////////////////////////////////////////////////////////////////////
//
int p2p_sdp_decode(struct mbuf *mb, struct icem *icem)
{
	char buf[1024];
	char hd[8];
	uint16_t len;

	while(mb->end > mb->pos)
	{
		memset(buf,0,1024);
		memset(hd, 0, 8);
		len = mbuf_read_u32(mb);
		mbuf_read_str(mb,buf,len < 1024?len:1024);
		str_ncpy(hd,buf,3);
		re_printf("******recieve sdp:%s\n",buf);
		if (str_cmp(hd, "CA")==0)
			icem_sdp_decode(icem,ice_attr_cand,buf+2);
		else if (str_cmp(hd, "UF")==0)
			icem_sdp_decode(icem,ice_attr_ufrag,buf+2);
		else if (str_cmp(hd, "PW")==0)
			icem_sdp_decode(icem, ice_attr_pwd,buf+2);
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////
// receive tcp data handler
void tcp_recv_handler(struct mbuf *mb, void* arg)
{
	uint16_t flag;
	uint16_t type,len;
	struct p2p_handle *p2p = (struct p2p_handle *)arg;
	char peername[512], buf[512];
	struct p2p_connection *pconn;
	struct icem *icem;
	struct list *mlist;
	uint16_t uret;

	flag = mbuf_read_u16(mb);
	if (flag != p2pflag)
		return;

	type = mbuf_read_u16(mb);
	/* p2p connect request */
	if (type == P2P_CONNREQ || type == P2P_CONNREP)
	{
		memset(peername, 0, 512);
		len = mbuf_read_u16(mb);
		mbuf_read_str(mb, peername, len);
		len = mbuf_read_u16(mb);
		mbuf_read_str(mb, buf, len);

		pconn = find_p2pconnection(p2p, peername);
		if (type == P2P_CONNREQ)
		{
			if (!pconn)
			{
				if (p2p->requesth)
					p2p->requesth(p2p, peername, &pconn);
			}
		}
		if (pconn)
		{
			mlist = ice_medialist(pconn->ice);
			icem = (struct icem *)list_ledata(list_head(mlist));
			p2p_sdp_decode(mb, icem);


			if (type == P2P_CONNREQ && pconn->status != P2P_INIT && pconn->status != P2P_ACCEPT)
			{
				p2p_response(pconn, peername);
				pconn->status = P2P_CONNECTING;
				ice_conncheck_start(pconn->ice);
			}

			if (type == P2P_CONNREP)
			{
				/*start connect check */
				ice_conncheck_start(pconn->ice);
			}
		}
	}else if (type == P2P_LOGINREP)
	{
		/* login response message */
		uret = mbuf_read_u16(mb);
		if (p2p->status == P2P_INIT && p2p->inith)
		{
			p2p->inith(uret, p2p);
			if (!uret)
				p2p->status = P2P_CONNECTED;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// receive udp data handler
void udp_recv_handler(const struct sa *src, struct mbuf *mb, void *arg)
{
	char *name;
	uint16_t type,flag,len;
	struct p2p_connection *pconn = (struct p2p_connection *)arg;

	UNUSED(src);

	flag = mbuf_read_u16(mb);
	if (flag != p2pflag)
	{
		re_printf("not p2p package!\n");
		return ;
	}

	type = mbuf_read_u16(mb);
	if (type == P2P_DATA)
	{
		len = mbuf_read_u16(mb);
		name = (char *)mem_zalloc(len+1, NULL);
		mbuf_read_str(mb, name, len);
		if (pconn->rh)
			pconn->rh(name, len, pconn->arg);

		mem_deref(name);
	}else
	{
		re_printf("message type not process:%d\n", type);
	}
}


//////////////////////////////////////////////////////////////////////////
// tcp close handler
void tcp_close_handler(int err, void *arg)
{
	struct p2p_handle *p2p = (struct p2p_handle *)arg;
	if (!err && p2p->inith)
		p2p->inith(err, p2p);

	re_printf("tcp closed error:%s(%d)\n", strerror(err),err);
}

//////////////////////////////////////////////////////////////////////////
// gather infomation handler
void gather_handler(int err, uint16_t scode, const char* reason, void *arg)
{
	struct p2p_connection *p2pconn = (struct p2p_connection *)arg;

	UNUSED(scode);
	UNUSED(reason);

	re_printf("gather_handler:%s\n",strerror(err));
	if (p2pconn->status == P2P_INIT)
	{
		p2p_request(p2pconn, p2pconn->p2ph->username, p2pconn->peername);
	}
	else if (p2pconn->status == P2P_ACCEPT)
	{
		p2p_response(p2pconn, p2pconn->peername);
		p2pconn->status = P2P_CONNECTING;
		ice_conncheck_start(p2pconn->ice);
	}
}


//////////////////////////////////////////////////////////////////////////
// connect check handler
void connchk_handler(int err, bool update, void *arg)
{
	struct p2p_connection *p2pconn = (struct p2p_connection *)arg;

	UNUSED(update);

	re_printf("connchk_handler:%s", strerror(err));
	if (!err)
	{
		p2pconn->status = P2P_CONNECTED;
	}
	if (p2pconn->ch)
		p2pconn->ch(err,"", p2pconn->arg);
}

//////////////////////////////////////////////////////////////////////////
// re_printf_h
static int print_handler(const char *p, size_t size, void *arg)
{
	struct pl *pl = (struct pl*)arg;
	if (size > pl->l)
		return ENOMEM;
	memcpy((void *)pl->p, p, size);
	pl_advance(pl, size);
	return 0;
}


//////////////////////////////////////////////////////////////////////////
// alloc p2p instance
int p2p_handle_alloc(struct p2p_handle **p2p, const char* username, const char* passwd)
{
	struct p2p_handle *p;

	p = (struct p2p_handle *)mem_zalloc(sizeof(*p),p2p_destructor);
	if (!p)
		return ENOMEM;
	
	p->sactl = (struct sa*)mem_zalloc(sizeof(struct sa),NULL);
	p->sastun = (struct sa*)mem_zalloc(sizeof(struct sa), NULL);

	if (username)
	{
		p->username = (char *)mem_zalloc(str_len(username)+1, NULL);
		str_ncpy(p->username, username, str_len(username)+1);
	}

	if (passwd)
	{
		p->passwd = (char *)mem_zalloc(str_len(passwd)+1, NULL);
		str_ncpy(p->passwd, passwd, str_len(passwd)+1);
	}

	sa_init(p->sactl, AF_INET);
	sa_init(p->sastun, AF_INET);
	*p2p = p;
		
	return 0;
}

//////////////////////////////////////////////////////////////////////////
// alloc a p2pconnection
int p2p_connection_alloc(struct p2p_handle *p2p, struct p2p_connection **ppconn, const char* name)
{
	int err;
	struct p2p_connection *pconn;
	
	pconn = (struct p2p_connection *)mem_zalloc(sizeof(*pconn), p2p_connection_destructor);
	if (!pconn)
		return ENOMEM;
	
	pconn->peername = (char *)mem_zalloc(str_len(name)+1, NULL);
	str_ncpy(pconn->peername, name, str_len(name)+1);

	pconn->p2ph = (struct p2p_handle *)mem_ref(p2p);
	list_append(&(p2p->lstconn), &(pconn->le), pconn);

	err = ice_alloc(&(pconn->ice), ICE_MODE_FULL, false);
	if (err)
	{
		re_fprintf(stderr, "ice alloc error:%s\n", strerror(err));
		goto out;
	}
	*ppconn = pconn;
	return 0;
out:
	mem_deref(pconn->ice);
	mem_deref(pconn->p2ph);
	mem_deref(pconn);
	return err;
}


//////////////////////////////////////////////////////////////////////////
// send p2p login message to control server
int p2p_send_login(struct tcp_conn *tc, const char* user, const char* passwd)
{
	int err;

	struct mbuf *mb;

	mb = mbuf_alloc(512);
	mbuf_write_u16(mb, p2pflag);
	mbuf_write_u16(mb, P2P_LOGINREQ);
	mbuf_write_u16(mb, str_len(user));
	mbuf_write_str(mb, user);
	mbuf_write_u16(mb, str_len(passwd));
	mbuf_write_str(mb, passwd);

	mb->pos = 0;

	err = tcp_send(tc, mb);
	if (err)
	{
		re_fprintf(stderr, "send p2p login error:%s\n", strerror(err));
	}
	mem_deref(mb);
	return err;
}


//////////////////////////////////////////////////////////////////////////
// send p2p connect request to control server
int p2p_request(struct p2p_connection *pconn, const char* myname, const char* peername)
{
	int err;
	struct mbuf *mb;
	struct icem *icem;

	mb = mbuf_alloc(1024);
	mbuf_write_u16(mb, p2pflag);
	mbuf_write_u16(mb, P2P_CONNREQ);
	mbuf_write_u16(mb, str_len(myname));
	mbuf_write_str(mb, myname);
	mbuf_write_u16(mb, str_len(peername));
	mbuf_write_str(mb, peername);

	icem = (struct icem *)(list_ledata(list_head(ice_medialist(pconn->ice))));
	p2p_sdp_encode(pconn->ice, icem, mb);

	mb->pos = 0;
	err = tcp_send(pconn->p2ph->ltcp, mb);
	if (err)
	{
		re_fprintf(stderr, "send p2p request error:%s\n", strerror(err));	
	}
	mem_deref(mb);
	return err;
}

int p2p_response(struct p2p_connection *pconn, const char* peername)
{
	int err;
	struct mbuf *mb;
	struct icem *icem;

	mb = mbuf_alloc(1024);
	mbuf_write_u16(mb, p2pflag);
	mbuf_write_u16(mb, P2P_CONNREP);
	mbuf_write_u16(mb, str_len(pconn->p2ph->username));
	mbuf_write_str(mb, pconn->p2ph->username);
	mbuf_write_u16(mb, str_len(peername));
	mbuf_write_str(mb, peername);

	icem = (struct icem *)(list_ledata(list_head(ice_medialist(pconn->ice))));
	p2p_sdp_encode(pconn->ice, icem, mb);

	mb->pos = 0;
	err = tcp_send(pconn->p2ph->ltcp, mb);
	if (err)
	{		
		re_fprintf(stderr, "send p2p response error:%s\n", strerror(err));	
	}
	return err;
}

static void signal_handler(int sig)
{
	re_printf("terminating on signal %d...\n", sig);
	libre_close();
}

//////////////////////////////////////////////////////////////////////////
// call back
void p2p_init_handler(int err, struct p2p_handle* p2p)
{
	re_printf("init return:%d\n", err);
}
void p2p_receive_handler(const char* data, uint32_t len, void *arg)
{
	re_printf("receive data:%s\n", data);
}
void p2p_request_handler(struct p2p_handle *p2p, const char* peername, struct p2p_connection **ppconn)
{
	re_printf("p2p_request_handler data:%s\n", peername);
}


//////////////////////////////////////////////////////////////////////////
// CP2P

CP2PUnited CP2P::s_P2PUnited;

CP2P::CP2P()
{

}

CP2P::CP2P(const CP2P & other)
{

}

CP2P::~CP2P()
{

}

int CP2P::p2p_init(const char* ctlserv, const int ctlport, const char* stunsrv, const int stunport, const char* username, const char* passwd, p2p_init_h *inith, p2p_request_h *requesth)
{
	try
	{
		return s_P2PUnited.p2p_init(ctlserv, ctlport, stunsrv, stunport, username, passwd, inith, requesth);
	}
	catch (...)
	{
		return 0;
	}
	return 0;
}

int CP2P::p2p_run()
{
	try
	{
		return s_P2PUnited.p2p_run();
	}
	catch (...)
	{
		return 0;
	}
	return 0;
}

P2PSOCKET CP2P::newSocket()
{
	try
	{
		return s_P2PUnited.newSocket();
	}
	catch (...)
	{
		return 0;
	}
	return 0;
}

int CP2P::send(const P2PSOCKET u, const char* buf, int size, int flags)
{
	int err;
	//struct mbuf *mb;
	//const struct sa *sa;
	//struct list *list;
	//struct icem *icem;
	//uint8_t compid = ICE_COMPID_RTP;

	//mb = mbuf_alloc(1024);
	//mbuf_write_u16(mb, p2pflag);
	//mbuf_write_u16(mb, P2P_DATA);
	//mbuf_write_u16(mb, size);
	//mbuf_write_str(mb, buf);

	//mb->pos = 0;

	//list = ice_medialist(pconn->ice);
	//icem = (struct icem *)list_ledata(list_head(list));

	//sa = icem_selected_raddr((const struct icem*)icem, compid);
	//if (!sa){
	//	re_printf("select default cand.\n");
	//	sa = icem_cand_default(icem, compid);
	//}

	//if (sa)
	//{
	//	err = udp_send(pconn->ulsock, sa, mb);
	//}else
	//{
	//	err = ENOENT;

	//}

	//mem_deref(mb);
	try
	{
		CP2P * p2p = s_P2PUnited.lookup(u);
		return p2p->send(buf, size);
	}
	catch (...)
	{
	}
	return err;
}

int CP2P::recv(const P2PSOCKET u, const char* buf, int size, int flags)
{
	return 0;
}

uint64_t CP2P::sendfile(const P2PSOCKET u, const char* path, uint64_t* offset, int64_t size, int block)
{
	try
	{
		CP2P * p2p = s_P2PUnited.lookup(u);
		return p2p->sendfile(u, path, offset, size, block);
	}
	catch (...)
	{
	}
	
	return 0;
}

uint64_t CP2P::recvfile(const P2PSOCKET u, const char* path, uint64_t* offset, int64_t size, int block)
{
	try
	{
		CP2P * p2p = s_P2PUnited.lookup(u);
		return p2p->recvfile(u, path, offset, size, block);
	}
	catch (...)
	{
	}

	return 0;
}

int CP2P::send(const char* data, int size)
{
	if (size < 0)
		return 0;

	CGuard sendguard(m_SendLock);

	
	return size;
}

void CP2P::initSynch()
{
#ifndef WIN32
	pthread_mutex_init(&m_SendBlockLock, NULL);
	pthread_cond_init(&m_SendBlockCond, NULL);
	pthread_mutex_init(&m_RecvDataLock, NULL);
	pthread_cond_init(&m_RecvDataCond, NULL);
	pthread_mutex_init(&m_SendLock, NULL);
	pthread_mutex_init(&m_RecvLock, NULL);
	pthread_mutex_init(&m_AckLock, NULL);
	pthread_mutex_init(&m_ConnectionLock, NULL);
#else
	m_SendBlockLock = CreateMutex(NULL, false, NULL);
	m_SendBlockCond = CreateEvent(NULL, false, false, NULL);
	m_RecvDataLock = CreateMutex(NULL, false, NULL);
	m_RecvDataCond = CreateEvent(NULL, false, false, NULL);
	m_SendLock = CreateMutex(NULL, false, NULL);
	m_RecvLock = CreateMutex(NULL, false, NULL);
	m_AckLock = CreateMutex(NULL, false, NULL);
	m_ConnectionLock = CreateMutex(NULL, false, NULL);
#endif
}

void CP2P::destroySynch()
{
#ifndef WIN32
	pthread_mutex_destroy(&m_SendBlockLock);
	pthread_cond_destroy(&m_SendBlockCond);
	pthread_mutex_destroy(&m_RecvDataLock);
	pthread_cond_destroy(&m_RecvDataCond);
	pthread_mutex_destroy(&m_SendLock);
	pthread_mutex_destroy(&m_RecvLock);
	pthread_mutex_destroy(&m_AckLock);
	pthread_mutex_destroy(&m_ConnectionLock);
#else
	CloseHandle(m_SendBlockLock);
	CloseHandle(m_SendBlockCond);
	CloseHandle(m_RecvDataLock);
	CloseHandle(m_RecvDataCond);
	CloseHandle(m_SendLock);
	CloseHandle(m_RecvLock);
	CloseHandle(m_AckLock);
	CloseHandle(m_ConnectionLock);
#endif
}

void CP2P::releaseSynch()
{
#ifndef WIN32
	// wake up user calls
	pthread_mutex_lock(&m_SendBlockLock);
	pthread_cond_signal(&m_SendBlockCond);
	pthread_mutex_unlock(&m_SendBlockLock);

	pthread_mutex_lock(&m_SendLock);
	pthread_mutex_unlock(&m_SendLock);

	pthread_mutex_lock(&m_RecvDataLock);
	pthread_cond_signal(&m_RecvDataCond);
	pthread_mutex_unlock(&m_RecvDataLock);

	pthread_mutex_lock(&m_RecvLock);
	pthread_mutex_unlock(&m_RecvLock);
#else
	SetEvent(m_SendBlockCond);
	WaitForSingleObject(m_SendLock, INFINITE);
	ReleaseMutex(m_SendLock);
	SetEvent(m_RecvDataCond);
	WaitForSingleObject(m_RecvLock, INFINITE);
	ReleaseMutex(m_RecvLock);
#endif
}


//////////////////////////////////////////////////////////////////////////
// CP2PSocket
CP2PSocket::CP2PSocket()
	: m_pP2P(NULL)
	, m_SocketID(0)
{

}

CP2PSocket::~CP2PSocket()
{
	delete m_pP2P;
	m_pP2P = NULL;
}



//////////////////////////////////////////////////////////////////////////
// CP2PUnited
CP2PUnited::CP2PUnited()
	: m_SocketID(0)
	, m_iInstanceCount(0)
	, m_p2ph(NULL)
{
	// Socket ID MUST start from a random value
	srand((unsigned int)CTimer::getTime());
	m_SocketID = 1 + (int)((1 << 30) * (double(rand()) / RAND_MAX));

#ifndef WIN32
	pthread_mutex_init(&m_ControlLock, NULL);
	pthread_mutex_init(&m_IDLock, NULL);
	pthread_mutex_init(&m_InitLock, NULL);
#else
	m_ControlLock = CreateMutex(NULL, false, NULL);
	m_IDLock = CreateMutex(NULL, false, NULL);
	m_InitLock = CreateMutex(NULL, false, NULL);
#endif

#ifndef WIN32
	pthread_key_create(&m_TLSError, TLSDestroy);
#else
	m_TLSError = TlsAlloc();
	m_TLSLock = CreateMutex(NULL, false, NULL);
#endif

	//m_pCache = new CCache<CInfoBlock>;
}

CP2PUnited::~CP2PUnited()
{
#ifndef WIN32
	pthread_mutex_destroy(&m_ControlLock);
	pthread_mutex_destroy(&m_IDLock);
	pthread_mutex_destroy(&m_InitLock);
#else
	CloseHandle(m_ControlLock);
	CloseHandle(m_IDLock);
	CloseHandle(m_InitLock);
#endif

#ifndef WIN32
	pthread_key_delete(m_TLSError);
#else
	TlsFree(m_TLSError);
	CloseHandle(m_TLSLock);
#endif

	delete m_p2ph;
	//delete m_pCache;
}

int CP2PUnited::p2p_init(const char* ctlserv, const int ctlport, const char* stunsrv, const int stunport, const char* username, const char* passwd, p2p_init_h *inith, p2p_request_h *requesth)
{
	CGuard gcinit(m_InitLock);

	if (m_iInstanceCount++ > 0)
		return 0;

//	// Global initialization code
//#ifdef WIN32
//	WORD wVersionRequested;
//	WSADATA wsaData;
//	wVersionRequested = MAKEWORD(2, 2);
//
//	if (0 != WSAStartup(wVersionRequested, &wsaData))
//		throw CUDTException(1, 0,  WSAGetLastError());
//#endif
//
//	//init CTimer::EventLock
//
//	if (m_bGCStatus)
//		return true;
//
//	m_bClosing = false;
//#ifndef WIN32
//	pthread_mutex_init(&m_GCStopLock, NULL);
//	pthread_cond_init(&m_GCStopCond, NULL);
//	pthread_create(&m_GCThread, NULL, garbageCollect, this);
//#else
//	m_GCStopLock = CreateMutex(NULL, false, NULL);
//	m_GCStopCond = CreateEvent(NULL, false, false, NULL);
//	DWORD ThreadID;
//	m_GCThread = CreateThread(NULL, 0, garbageCollect, this, 0, &ThreadID);
//#endif

	int err = libre_init();
	if (err) 
	{
		re_fprintf(stderr, "re init failed: %s\n", strerror(err));
		goto out;
	}

	err = p2p_handle_alloc(&m_p2ph, username, passwd);
	if (!m_p2ph)
	{
		return ENOMEM;
	}
	
	m_p2ph->requesth = requesth;
	m_p2ph->inith = inith;
	m_p2ph->username = (char *)mem_zalloc(str_len(username)+1, NULL);
	str_ncpy(m_p2ph->username, username, str_len(username)+1);
	m_p2ph->passwd = (char *)mem_zalloc(str_len(passwd)+1, NULL);
	str_ncpy(m_p2ph->passwd, passwd, str_len(passwd)+1);
	// set init status
	m_p2ph->status = P2P_INIT;

	sa_set_str(m_p2ph->sactl, ctlserv, ctlport);
	sa_set_str(m_p2ph->sastun, stunsrv, stunport);

	err = tcp_conn_alloc(&(m_p2ph->ltcp), m_p2ph->sactl,tcp_estab_handler, tcp_recv_handler, tcp_close_handler, m_p2ph);
	if (err)
	{
		re_fprintf(stderr, "tcp connect alloc error:%s\n", strerror(err));
		goto out;

	}

	err = tcp_conn_connect(m_p2ph->ltcp, m_p2ph->sactl);
	if (err)
	{
		re_fprintf(stderr, "tcp connect error:%s\n", strerror(err));
		goto out;
	}

	m_p2ph->psock = CP2P::newSocket();
	m_bGCStatus = true;

	return 0;

out:
	mem_deref(m_p2ph);
	libre_close();
	return err;
}

int CP2PUnited::p2p_run()
{
	try
	{
		UNUSED(m_p2ph);
		re_main(signal_handler);
	}
	catch (...)
	{
	}

	return 0;
}

P2PSOCKET CP2PUnited::newSocket()
{
	CP2PSocket * ns =NULL;
	try
	{
		ns = new CP2PSocket;
		ns->m_pP2P = new CP2P;
	}
	catch (...)
	{
	}
	ns->m_SocketID = -- m_SocketID;
	VECP2P[ns->m_SocketID] = ns;
	return ns->m_SocketID;
}


CP2P* CP2PUnited::lookup(const P2PSOCKET u)
{
	std::map<P2PSOCKET, CP2PSocket*>::iterator it = VECP2P.find(u);
	if (it == VECP2P.end())
	{
		return NULL;
	}
	return it->second->m_pP2P;
}


//////////////////////////////////////////////////////////////////////////
// P2P_API
namespace P2P
{
	int p2p_init(const char* ctlserv, const int ctlport, const char* stunsrv, const int stunport, const char* username, const char* passwd)
	{
		return CP2P::p2p_init(ctlserv, ctlport, stunsrv, stunport, username, passwd, p2p_init_handler, p2p_request_handler);
	}

	int p2p_run()
	{
		return CP2P::p2p_run();
	}

	int p2p_send(const P2PSOCKET u, const char* buf, uint32_t size)
	{
		return CP2P::send(u, buf, size, 0);
	}

	int p2p_recv(const P2PSOCKET u, const char* buf, uint32_t size)
	{
		return CP2P::recv(u, buf, size, 0);
	}

	uint64_t p2p_sendfile(const P2PSOCKET u, const char* path, uint64_t* offset, uint64_t size, int block)
	{
		return CP2P::sendfile(u, path, offset, size, block);
	}

	uint64_t p2p_recvfile(const P2PSOCKET u, const char* path, uint64_t* offset, uint64_t size, int block)
	{
		return CP2P::recvfile(u, path, offset, size, block);
	}
}