#include "p2p.h"


static void signal_handler(int sig)
{
	re_printf("terminating on signal %d...\n", sig);
	libre_close();
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

// struct p2phandle destructor
void p2phandle_destructor(void* arg)
{
	struct p2phandle* p2p = (struct p2phandle*)arg;
	mem_deref(p2p->ltcp);
	mem_deref(p2p->sactl);
	mem_deref(p2p->sastun);
}


// struct p2phandle alloc
int p2phandle_alloc(struct p2phandle** p2p, const char* username, const char* passwd)
{
	struct p2phandle* p;
	p = (struct p2phandle*)mem_zalloc(sizeof(*p), p2phandle_destructor);
	if (!p)
		return 1;

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

// p2pconnection intansce
void p2pconnection_destructor(void *arg)
{
	struct p2pconnection *pc = (struct p2pconnection *)arg;

	list_unlink(&(pc->le));

	mem_deref(pc->p2p);
	mem_deref(pc->ice);
	mem_deref(pc->peername);
}

// alloc a p2pconnection
int p2pconnection_alloc(struct p2phandle *p2p, struct p2pconnection **ppconn, const char* name)
{
	int err;
	struct p2pconnection *pconn;
	
	pconn = (struct p2pconnection *)mem_zalloc(sizeof(*pconn), p2pconnection_destructor);
	if (!pconn)
		return ENOMEM;
	
	pconn->peername = (char *)mem_zalloc(str_len(name)+1, NULL);
	str_ncpy(pconn->peername, name, str_len(name)+1);

	pconn->p2p = (struct p2phandle *)mem_ref(p2p);
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
	mem_deref(pconn->p2p);
	mem_deref(pconn);
	return err;
}

struct p2pconnection* find_p2pconnection(struct p2phandle *p2p, const char* name)
{
	struct le *le;
	struct list *lstconn = &(p2p->lstconn);

	for(le = list_head(lstconn); le; le=le->next)
	{
		struct p2pconnection *pconn = (struct p2pconnection *)(le->data);
		if (str_cmp(pconn->peername,name)==0)
		{
			return pconn;
		}
	}
	return NULL;
}

// gather infomation handler
void gather_handler(int err, uint16_t scode, const char* reason,
		void *arg)
{
	struct p2pconnection *p2pconn = (struct p2pconnection *)arg;
	
	UNUSED(scode);
	UNUSED(reason);
	
	re_printf("gather_handler:%s\n",strerror(err));
	if (p2pconn->status == P2P_INIT)
		p2p_request(p2pconn, p2pconn->p2p->username, p2pconn->peername);
	else if (p2pconn->status == P2P_ACCEPT)
	{
		p2p_response(p2pconn, p2pconn->peername);
		p2pconn->status = P2P_CONNECTING;
		ice_conncheck_start(p2pconn->ice);	 
	}
}

// connect check handler
void connchk_handler(int err, bool update, void *arg)
{
	struct p2pconnection *p2pconn = (struct p2pconnection *)arg;

	UNUSED(update);

	re_printf("connchk_handler:%s", strerror(err));
	if (!err)
	{
		p2pconn->status = P2P_CONNECTED;
	}
	if (p2pconn->ch)
		p2pconn->ch(err,"", p2pconn->arg);
}

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


// tcp connection established handler
void tcp_estab_handler(void *arg)
{
	int err;

	struct p2phandle *p2p = (struct p2phandle *)arg;
	
	err = p2p_send_login(p2p->ltcp, p2p->username, p2p->passwd);
	if (err)
	{
		re_fprintf(stderr, "p2p register to control server error:%s\n",strerror(err));
	}
}

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

// receive tcp data handler
void tcp_recv_handler(struct mbuf *mb, void* arg)
{
	uint16_t flag;
	uint16_t type,len;
	struct p2phandle *p2p = (struct p2phandle *)arg;
	char peername[512], buf[512];
	struct p2pconnection *pconn;
	struct icem *icem;
	struct list *mlist;
	uint16_t uret;

	flag = mbuf_read_u16(mb);
	if (flag != p2pflag)
		return;

	type = mbuf_read_u16(mb);
	// p2p connect request
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
				// start connect check
				ice_conncheck_start(pconn->ice);
			}
		}
	}
	else if (type == P2P_LOGINREP)
	{
		// login response message
		uret = mbuf_read_u16(mb);
		if (p2p->status == P2P_INIT && p2p->inith)
		{
			p2p->inith(uret, p2p);
			if (!uret)
				p2p->status = P2P_CONNECTED;
		}
	}
}

/*
 * receive udp data handler
 */
void udp_recv_handler(const struct sa *src, struct mbuf *mb, void *arg)
{
	char *name;
	uint16_t type,flag,len;
	struct p2pconnection *pconn = (struct p2pconnection *)arg;
	
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
			pconn->rh(name, len, pconn);

		mem_deref(name);
	}else
	{
		re_printf("message type not process:%d\n", type);
	}
}


/*
 * connect to anthor peer node
 */
int p2p_connect(struct p2phandle *p2p, struct p2pconnection **ppconn, const char* peername,
		p2p_connect_h *connecth, p2p_receive_h *receiveh, void *arg)
{
	int err;
	struct p2pconnection *pconn;
	struct sa laddr;
	struct icem *icem;

	err = p2pconnection_alloc(p2p, &pconn, peername);
	if (err)
	{
		re_fprintf(stderr, "p2pconnection alloc error:%s\n", strerror(err));
		goto out;
	}

	pconn->ch = connecth;
	pconn->rh = receiveh;
	pconn->arg = arg;
	
	err = icem_alloc(&(icem), pconn->ice, IPPROTO_UDP, 3, 
			gather_handler, connchk_handler, pconn);
	if (err)
	{

		re_fprintf(stderr, "p2pconnection alloc error:%s\n", strerror(err));
		goto out;
	}
  	err = net_default_source_addr_get(AF_INET, &laddr);
	if (err) 
	{
		re_fprintf(stderr, "local address error: %s\n", strerror(err));
		goto out;
	}
	 
	sa_set_port(&laddr, 0);
	err = udp_listen(&(pconn->ulsock), &laddr, udp_recv_handler, pconn);
	if (err)
	{
		re_fprintf(stderr, "udp listen error:%s\n", strerror(err));
		goto out;
	}

	err = icem_comp_add(icem,ICE_COMPID_RTP, pconn->ulsock);
	if (err)
	{
		re_fprintf(stderr, "icem compnent add error:%s\n", strerror(err));
		goto out;
	}
	err = icem_cand_add(icem,ICE_COMPID_RTP, 0, NULL, &laddr);
	if (err)
	{
		re_fprintf(stderr,"icem cand add error:%s\n", strerror(err));
		goto out;
	}
	
	/*set status to init */
	pconn->status = P2P_INIT;

	
	err = icem_gather_srflx(icem, p2p->sastun);
	if(err)
	{
		re_fprintf(stderr, "gather server filrex error:%s\n",strerror(err));
		goto out;
	}
	
	err = icem_gather_relay(icem, p2p->sastun, ice_ufrag(pconn->ice), ice_pwd(pconn->ice));
	if(err)
	{
		re_fprintf(stderr, "gather server relay error:%s\n",strerror(err));
		goto out;
	}
	
	*ppconn = pconn;
	return 0;
out:
	pconn->ch = NULL;
	pconn->rh = NULL;
	mem_deref(pconn);
	return err;
}

// p2p accept
int p2p_accept(struct p2phandle* p2p, struct p2pconnection** ppconn, const char* peername,
		p2p_receive_h *receiveh, void *arg)
{

	int err;
	struct p2pconnection *pconn;
	struct sa laddr;
	struct icem *icem;

	err = p2pconnection_alloc(p2p, &pconn, peername);
	if (err)
	{
		re_fprintf(stderr, "p2pconnection alloc error:%s\n", strerror(err));
		goto out;
	}

	pconn->rh = receiveh;
	pconn->arg = arg;
	
	err = icem_alloc(&(icem), pconn->ice, IPPROTO_UDP, 3, gather_handler, connchk_handler, pconn);
	if (err)
	{

		re_fprintf(stderr, "p2pconnection alloc error:%s\n", strerror(err));
		goto out;
	}
  	err = net_default_source_addr_get(AF_INET, &laddr);
	if (err) 
	{
		re_fprintf(stderr, "local address error: %s\n", strerror(err));
		goto out;
	}
	 
	sa_set_port(&laddr, 0);
	err = udp_listen(&(pconn->ulsock), &laddr, udp_recv_handler, pconn);
	if (err)
	{
		re_fprintf(stderr, "udp listen error:%s\n", strerror(err));
		goto out;
	}

	err = icem_comp_add(icem,ICE_COMPID_RTP, pconn->ulsock);
	if (err)
	{
		re_fprintf(stderr, "icem compnent add error:%s\n", strerror(err));
		goto out;
	}
	err = icem_cand_add(icem, ICE_COMPID_RTP, 0, NULL, &laddr);
	if (err)
	{
		re_fprintf(stderr,"icem cand add error:%s\n", strerror(err));
		goto out;
	}
	
	/*set status to ACCEPT */
	pconn->status = P2P_ACCEPT;
/*
	err = icem_gather_srflx(icem, p2p->sastun);
	if(err)
	{
		re_fprintf(stderr, "gather server filrex error:%s\n",strerror(err));
		goto out;
	}
*/		
	err = icem_gather_relay(icem, p2p->sastun, ice_ufrag(pconn->ice), ice_pwd(pconn->ice));
	if(err)
	{
		re_fprintf(stderr, "gather server relay error:%s\n",strerror(err));
		goto out;
	}
	
	*ppconn = pconn;
	return 0;
out:
	pconn->ch = NULL;
	pconn->rh = NULL;
	mem_deref(pconn);
	return err;
}

// send p2p connect request to control server
int p2p_request(struct p2pconnection *pconn, const char* myname, const char* peername)
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
	
	icem = (struct icem *)list_ledata(list_head(ice_medialist(pconn->ice)));
	p2p_sdp_encode(pconn->ice, icem, mb);

	mb->pos = 0;
	err = tcp_send(pconn->p2p->ltcp, mb);
	if (err)
	{
		re_fprintf(stderr, "send p2p request error:%s\n", strerror(err));	
	}
	mem_deref(mb);
	return err;
}


int p2p_response(struct p2pconnection *pconn, const char* peername)
{
	int err;
	struct mbuf *mb;
	struct icem *icem;

	mb = mbuf_alloc(1024);
	mbuf_write_u16(mb, p2pflag);
	mbuf_write_u16(mb, P2P_CONNREP);
	mbuf_write_u16(mb, str_len(pconn->p2p->username));
	mbuf_write_str(mb, pconn->p2p->username);
	mbuf_write_u16(mb, str_len(peername));
	mbuf_write_str(mb, peername);

	icem = (struct icem *)list_ledata(list_head(ice_medialist(pconn->ice)));
	p2p_sdp_encode(pconn->ice, icem, mb);

	mb->pos = 0;
	err = tcp_send(pconn->p2p->ltcp, mb);
	if (err)
	{		
		re_fprintf(stderr, "send p2p response error:%s\n", strerror(err));	
	}
	return err;

}


//////////////////////////////////////////////////////////////////////////
// CUDTSocket
CUDTSocket::CUDTSocket():
	m_Status(INIT), 
	m_TimeStamp(0), 
	m_iIPversion(0), 
	m_pSelfAddr(NULL), 
	m_pPeerAddr(NULL), 
	m_SocketID(0), 
	m_ListenSocket(0), 
	m_PeerID(0), 
	m_iISN(0), 
	m_pUDT(NULL), 
	m_pQueuedSockets(NULL), 
	m_pAcceptSockets(NULL), 
	m_AcceptCond(), 
	m_AcceptLock(), 
	m_uiBackLog(0), 
	m_iMuxID(-1)
{
#ifndef WIN32
	pthread_mutex_init(&m_AcceptLock, NULL);
	pthread_cond_init(&m_AcceptCond, NULL);
	pthread_mutex_init(&m_ControlLock, NULL);
#else
	m_AcceptLock = CreateMutex(NULL, false, NULL);
	m_AcceptCond = CreateEvent(NULL, false, false, NULL);
	m_ControlLock = CreateMutex(NULL, false, NULL);
#endif
}

CUDTSocket::~CUDTSocket()
{
	if (AF_INET == m_iIPversion)
	{
		delete (sockaddr_in*)m_pSelfAddr;
		delete (sockaddr_in*)m_pPeerAddr;
	}
	else
	{
		delete (sockaddr_in6*)m_pSelfAddr;
		delete (sockaddr_in6*)m_pPeerAddr;
	}

	delete m_pUDT;
	m_pUDT = NULL;

	delete m_pQueuedSockets;
	delete m_pAcceptSockets;

#ifndef WIN32
	pthread_mutex_destroy(&m_AcceptLock);
	pthread_cond_destroy(&m_AcceptCond);
	pthread_mutex_destroy(&m_ControlLock);
#else
	CloseHandle(m_AcceptLock);
	CloseHandle(m_AcceptCond);
	CloseHandle(m_ControlLock);
#endif
}


//////////////////////////////////////////////////////////////////////////
// CP2PUnited
CUDTUnited::CUDTUnited():
	m_Sockets(), 
	m_ControlLock(), 
	m_IDLock(), 
	m_SocketID(0),
	m_pCache(NULL),
	m_bClosing(false),
	m_GCStopLock(),
	m_GCStopCond(),
	m_InitLock(),
	m_iInstanceCount(0),
	m_bGCStatus(false),
	m_WorkerThread(),
	m_ClosedSockets()
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

	m_pCache = new CCache<CInfoBlock>;
}

CUDTUnited::~CUDTUnited()
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

	delete m_pCache;
}

int CUDTUnited::p2p_init(const char* ctlip, const uint16_t ctlport, const char* stunip, const uint16_t stunport, const char* name, const char* passwd, p2p_init_h* inith, p2p_request_h* requesth)
{
	CGuard gcinit(m_InitLock);

	if (m_iInstanceCount++ > 0)
		return 0;

	if (m_bGCStatus)
		return 0;

	// init p2p
	int err = 0;
	err = libre_init();
	if (err)
	{
		re_fprintf(stderr, "p2p init error:%s\n", strerror(err));
		return 1;
	}

	struct p2phandle* p;
	err = p2phandle_alloc(&p, name, passwd);
	if (!p)
		return 1;

	p->inith = inith;
	p->requesth = requesth;
	p->username = (char *)mem_zalloc(str_len(name)+1, NULL);
	p->passwd = (char *)mem_zalloc(str_len(passwd)+1, NULL);
	str_ncpy(p->username, name, str_len(name)+1);
	str_ncpy(p->passwd, passwd, str_len(passwd)+1);

	p->status = P2P_INIT;
	sa_set_str(p->sactl, ctlip, ctlport);
	sa_set_str(p->sastun, stunip, stunport);
	err = tcp_conn_alloc(&(p->ltcp), p->sactl, tcp_estab_handler, tcp_recv_handler, tcp_close_handler, p);
	// connect to tcp server
	err = tcp_conn_connect(p->ltcp, p->sactl);
	if (err)
	{
		re_fprintf(stderr, "p2p connect to tcp server error:%s\n", strerror(err));
		return 1;
	}

	m_bClosing = false;
#ifndef WIN32
	pthread_mutex_init(&m_GCStopLock, NULL);
	pthread_cond_init(&m_GCStopCond, NULL);
	pthread_create(&m_GCThread, NULL, worker, this);
#else
	m_GCStopLock = CreateMutex(NULL, false, NULL);
	m_GCStopCond = CreateEvent(NULL, false, false, NULL);
	m_WorkerThread = CreateThread(NULL, 0, worker, this, 0, NULL);
#endif

	m_p2ph = p;
	m_bGCStatus = true;

	return 0;
}

int CUDTUnited::p2p_run()
{
	int err = re_main(signal_handler);
	if (err)
	{
		re_printf("re_main failed.\n");
		return 1;
	}
	return 0;
}

int CUDTUnited::p2p_accept()
{
	return 0;
}

int CUDTUnited::p2p_connect(const char* peername)
{
	//err = p2p_connect(m_p2ph, &pconn, peername, connchk_handler, p2p_receive_handler);
	int err;
	struct p2pconnection *pconn;
	struct sa laddr;
	struct icem *icem;

	err = p2pconnection_alloc(m_p2ph, &pconn, peername);
	if (err)
	{
		re_fprintf(stderr, "p2pconnection alloc error:%s\n", strerror(err));
		goto out;
	}

	pconn->ch = p2p_connect_handler;
	pconn->rh = p2p_receive_handler;
	//pconn->arg = arg;

	err = icem_alloc(&(icem), pconn->ice, IPPROTO_UDP, 3, 
		gather_handler, connchk_handler, pconn);
	if (err)
	{

		re_fprintf(stderr, "p2pconnection alloc error:%s\n", strerror(err));
		goto out;
	}
	err = net_default_source_addr_get(AF_INET, &laddr);
	if (err) 
	{
		re_fprintf(stderr, "local address error: %s\n", strerror(err));
		goto out;
	}

	sa_set_port(&laddr, 0);
	err = udp_listen(&(pconn->ulsock), &laddr, udp_recv_handler, pconn);
	if (err)
	{
		re_fprintf(stderr, "udp listen error:%s\n", strerror(err));
		goto out;
	}

	err = icem_comp_add(icem,ICE_COMPID_RTP, pconn->ulsock);
	if (err)
	{
		re_fprintf(stderr, "icem compnent add error:%s\n", strerror(err));
		goto out;
	}
	err = icem_cand_add(icem,ICE_COMPID_RTP, 0, NULL, &laddr);
	if (err)
	{
		re_fprintf(stderr,"icem cand add error:%s\n", strerror(err));
		goto out;
	}

	/*set status to init */
	pconn->status = P2P_INIT;


	err = icem_gather_srflx(icem, p2p->sastun);
	if(err)
	{
		re_fprintf(stderr, "gather server filrex error:%s\n",strerror(err));
		goto out;
	}

	err = icem_gather_relay(icem, p2p->sastun, ice_ufrag(pconn->ice), ice_pwd(pconn->ice));
	if(err)
	{
		re_fprintf(stderr, "gather server relay error:%s\n",strerror(err));
		goto out;
	}

	*ppconn = pconn;
	return 0;
out:
	pconn->ch = NULL;
	pconn->rh = NULL;
	mem_deref(pconn);
	return err;
}

int CUDTUnited::p2p_send(const char* peername, const char* buf, int len)
{
	return;
}

int CUDTUnited::p2p_close()
{
	libre_close();
	return 0;
}

int CUDTUnited::bind(const UDTSOCKET u, const UDPSOCKET* udpsaock, const sockaddr* name, int namelen)
{
	CUDTSocket* s = locate(u);
	if (NULL == s)
		throw CUDTException(5, 4, 0);

	CGuard cg(s->m_ControlLock);

	// cannot bind a socket more than once
	if (INIT != s->m_Status)
		throw CUDTException(5, 0, 0);

	// check the size of SOCKADDR structure
	if (AF_INET == s->m_iIPversion)
	{
		if (namelen != sizeof(sockaddr_in))
			throw CUDTException(5, 3, 0);
	}
	else
	{
		if (namelen != sizeof(sockaddr_in6))
			throw CUDTException(5, 3, 0);
	}

	s->m_pUDT->open();
	updateMux(s, name, udpsaock);
	s->m_Status = OPENED;

	// copy address information of local node
	s->m_pUDT->m_pSndQueue->m_pChannel->getSockAddr(s->m_pSelfAddr);

	return 0;
}

int CUDTUnited::listen(const UDTSOCKET u, int backlog)
{
	CUDTSocket* s = locate(u);
	if (NULL == s)
		throw CUDTException(5, 4, 0);

	CGuard cg(s->m_ControlLock);

	// do nothing if the socket is already listening
	if (LISTENING == s->m_Status)
		return 0;

	// a socket can listen only if is in OPENED status
	if (OPENED != s->m_Status)
		throw CUDTException(5, 5, 0);

	// listen is not supported in rendezvous connection setup
	if (s->m_pUDT->m_bRendezvous)
		throw CUDTException(5, 7, 0);

	if (backlog <= 0)
		throw CUDTException(5, 3, 0);

	s->m_uiBackLog = backlog;

	try
	{
		s->m_pQueuedSockets = new std::set<UDTSOCKET>;
		s->m_pAcceptSockets = new std::set<UDTSOCKET>;
	}
	catch (...)
	{
		delete s->m_pQueuedSockets;
		delete s->m_pAcceptSockets;
		throw CUDTException(3, 2, 0);
	}

	s->m_pUDT->listen();

	s->m_Status = LISTENING;

	return 0;
}

UDTSOCKET CUDTUnited::newSocket(int af, int type)
{
	if ((type != SOCK_STREAM) && (type != SOCK_DGRAM))
		throw CUDTException(5, 3, 0);

	CUDTSocket* ns = NULL;

	try
	{
		ns = new CUDTSocket;
		ns->m_pUDT = new CUDT;
		if (AF_INET == af)
		{
			ns->m_pSelfAddr = (sockaddr*)(new sockaddr_in);
			((sockaddr_in*)(ns->m_pSelfAddr))->sin_port = 0;
		}
		else
		{
			ns->m_pSelfAddr = (sockaddr*)(new sockaddr_in6);
			((sockaddr_in6*)(ns->m_pSelfAddr))->sin6_port = 0;
		}
	}
	catch (...)
	{
		delete ns;
		throw CUDTException(3, 2, 0);
	}

	CGuard::enterCS(m_IDLock);
	ns->m_SocketID = -- m_SocketID;
	CGuard::leaveCS(m_IDLock);

	ns->m_Status = INIT;
	ns->m_ListenSocket = 0;
	ns->m_pUDT->m_SocketID = ns->m_SocketID;
	ns->m_pUDT->m_iSockType = (SOCK_STREAM == type) ? UDT_STREAM : UDT_DGRAM;
	ns->m_pUDT->m_iIPversion = ns->m_iIPversion = af;
	ns->m_pUDT->m_pCache = m_pCache;

	// protect the m_Sockets structure.
	CGuard::enterCS(m_ControlLock);
	try
	{
		m_Sockets[ns->m_SocketID] = ns;
	}
	catch (...)
	{
		//failure and rollback
		CGuard::leaveCS(m_ControlLock);
		delete ns;
		ns = NULL;
	}
	CGuard::leaveCS(m_ControlLock);

	if (NULL == ns)
		throw CUDTException(3, 2, 0);

	return ns->m_SocketID;
}

int CUDTUnited::newConnection(const UDTSOCKET listen, const sockaddr* peer, CHandShake* hs)
{
	CUDTSocket* ls = locate(listen);
	if (NULL == ls)
		return -1;

	CUDTSocket* ns = NULL;

	// if this connection has already been processed
	if (ns->m_pUDT->m_bBroken)
	{
		// last connection from the "peer" address has been broken
		ns->m_Status = CLOSED;
		ns->m_TimeStamp = CTimer::getTime();

		CGuard::enterCS(ls->m_AcceptLock);
		ls->m_pQueuedSockets->erase(ns->m_SocketID);
		ls->m_pAcceptSockets->erase(ns->m_SocketID);
		CGuard::leaveCS(ls->m_AcceptLock);
	}
	else
	{
		// connection already exist, this is a repeated connection request
		// respond with existing HS information

		hs->m_iISN = ns->m_pUDT->m_iISN;
		hs->m_iMSS = ns->m_pUDT->m_iMSS;
		hs->m_iFlightFlagSize = ns->m_pUDT->m_iFlightFlagSize;
		hs->m_iReqType = -1;
		hs->m_iID = ns->m_SocketID;

		return 0;

		//except for this situation a new connection should be started
	}

	// exceeding backlog, refuse the connection request
	if (ls->m_pQueuedSockets->size() >= ls->m_uiBackLog)
		return -1;

	try
	{
		ns = new CUDTSocket;
		ns->m_pUDT = new CUDT(*(ls->m_pUDT));
		if (AF_INET == ls->m_iIPversion)
		{
			ns->m_pSelfAddr = (sockaddr*)(new sockaddr_in);
			((sockaddr_in*)(ns->m_pSelfAddr))->sin_port = 0;
			ns->m_pPeerAddr = (sockaddr*)(new sockaddr_in);
			memcpy(ns->m_pPeerAddr, peer, sizeof(sockaddr_in));
		}
		else
		{
			ns->m_pSelfAddr = (sockaddr*)(new sockaddr_in6);
			((sockaddr_in6*)(ns->m_pSelfAddr))->sin6_port = 0;
			ns->m_pPeerAddr = (sockaddr*)(new sockaddr_in6);
			memcpy(ns->m_pPeerAddr, peer, sizeof(sockaddr_in6));
		}
	}
	catch (...)
	{
		delete ns;
		return -1;
	}

	CGuard::enterCS(m_IDLock);
	ns->m_SocketID = -- m_SocketID;
	CGuard::leaveCS(m_IDLock);

	ns->m_ListenSocket = listen;
	ns->m_iIPversion = ls->m_iIPversion;
	ns->m_pUDT->m_SocketID = ns->m_SocketID;
	ns->m_PeerID = hs->m_iID;
	ns->m_iISN = hs->m_iISN;

	int error = 0;

	try
	{
		// bind to the same addr of listening socket
		ns->m_pUDT->open();
		updateMux(ns, ls);
		ns->m_pUDT->connect(peer, hs);
	}
	catch (...)
	{
		error = 1;
		goto ERR_ROLLBACK;
	}

	ns->m_Status = CONNECTED;

	// copy address information of local node
	ns->m_pUDT->m_pSndQueue->m_pChannel->getSockAddr(ns->m_pSelfAddr);
	CIPAddress::pton(ns->m_pSelfAddr, ns->m_pUDT->m_piSelfIP, ns->m_iIPversion);

	// protect the m_Sockets structure.
	CGuard::enterCS(m_ControlLock);
	try
	{
		m_Sockets[ns->m_SocketID] = ns;
		m_PeerRec[(ns->m_PeerID << 30) + ns->m_iISN].insert(ns->m_SocketID);
	}
	catch (...)
	{
		error = 2;
	}
	CGuard::leaveCS(m_ControlLock);

	CGuard::enterCS(ls->m_AcceptLock);
	try
	{
		ls->m_pQueuedSockets->insert(ns->m_SocketID);
	}
	catch (...)
	{
		error = 3;
	}
	CGuard::leaveCS(ls->m_AcceptLock);

ERR_ROLLBACK:
	if (error > 0)
	{
		ns->m_pUDT->close();
		ns->m_Status = CLOSED;
		ns->m_TimeStamp = CTimer::getTime();

		return -1;
	}

	return 1;
}

int CUDTUnited::connect(const UDTSOCKET u, const sockaddr* name, int namelen)
{
	CUDTSocket* s = locate(u);
	if (NULL == s)
		throw CUDTException(5, 4, 0);

	CGuard cg(s->m_ControlLock);

	// check the size of SOCKADDR structure
	if (AF_INET == s->m_iIPversion)
	{
		if (namelen != sizeof(sockaddr_in))
			throw CUDTException(5, 3, 0);
	}
	else
	{
		if (namelen != sizeof(sockaddr_in6))
			throw CUDTException(5, 3, 0);
	}

	// a socket can "connect" only if it is in INIT or OPENED status
	if (INIT == s->m_Status)
	{
		if (!s->m_pUDT->m_bRendezvous)
		{
			s->m_pUDT->open();
			updateMux(s);
			s->m_Status = OPENED;
		}
		else
			throw CUDTException(5, 8, 0);
	}
	else if (OPENED != s->m_Status)
		throw CUDTException(5, 2, 0);

	// connect_complete() may be called before connect() returns.
	// So we need to update the status before connect() is called,
	// otherwise the status may be overwritten with wrong value (CONNECTED vs. CONNECTING).
	s->m_Status = CONNECTING;
	try
	{
		s->m_pUDT->connect(name);
	}
	catch (CUDTException e)
	{
		s->m_Status = OPENED;
		throw e;
	}

	// record peer address
	delete s->m_pPeerAddr;
	if (AF_INET == s->m_iIPversion)
	{
		s->m_pPeerAddr = (sockaddr*)(new sockaddr_in);
		memcpy(s->m_pPeerAddr, name, sizeof(sockaddr_in));
	}
	else
	{
		s->m_pPeerAddr = (sockaddr*)(new sockaddr_in6);
		memcpy(s->m_pPeerAddr, name, sizeof(sockaddr_in6));
	}

	return 0;
}

int CUDTUnited::postRecv(UDTSOCKET u, const char* buf, uint32_t len)
{
	CUDTSocket* s = locate(u);
	if (NULL == s)
		throw CUDTException(5, 4, 0);

	s->m_pUDT->m_pRcvQueue->postRecv(buf, len);
	return 0;
}

void CUDTUnited::connect_complete(const UDTSOCKET u)
{

}

CUDTSocket* CUDTUnited::locate(const UDTSOCKET u)
{
	CGuard cg(m_ControlLock);

	std::map<UDTSOCKET, CUDTSocket*>::iterator i = m_Sockets.find(u);

	if ((i == m_Sockets.end()) || (i->second->m_Status == CLOSED))
		return NULL;

	return i->second;
}

CUDTSocket* CUDTUnited::locate(const sockaddr* peer, const UDTSOCKET id, int32_t isn)
{
	CGuard cg(m_ControlLock);

	std::map<int64_t, std::set<UDTSOCKET> >::iterator i = m_PeerRec.find((id << 30) + isn);
	if (i == m_PeerRec.end())
		return NULL;

	for (std::set<UDTSOCKET>::iterator j = i->second.begin(); j != i->second.end(); ++ j)
	{
		std::map<UDTSOCKET, CUDTSocket*>::iterator k = m_Sockets.find(*j);
		// this socket might have been closed and moved m_ClosedSockets
		if (k == m_Sockets.end())
			continue;

		if (CIPAddress::ipcmp(peer, k->second->m_pPeerAddr, k->second->m_iIPversion))
			return k->second;
	}

	return NULL;
}

CUDT* CUDTUnited::lookup(const UDTSOCKET u)
{
	// protects the m_Sockets structure
	CGuard cg(m_ControlLock);

	std::map<UDTSOCKET, CUDTSocket*>::iterator i = m_Sockets.find(u);

	if ((i == m_Sockets.end()) || (i->second->m_Status == CLOSED))
		throw CUDTException(5, 4, 0);

	return i->second->m_pUDT;
}

UDTSTATUS CUDTUnited::getStatus(const UDTSOCKET u)
{
	return NONEXIST;
}

void CUDTUnited::updateMux(CUDTSocket* s, const sockaddr* addr, const UDPSOCKET* udpsock)
{
	CGuard cg(m_ControlLock);

	if ((s->m_pUDT->m_bReuseAddr) && (NULL != addr))
	{
		int port = (AF_INET == s->m_pUDT->m_iIPversion) ? ntohs(((sockaddr_in*)addr)->sin_port) : ntohs(((sockaddr_in6*)addr)->sin6_port);

		// find a reusable address
		for (std::map<int, CMultiplexer>::iterator i = m_mMultiplexer.begin(); i != m_mMultiplexer.end(); ++ i)
		{
			if ((i->second.m_iIPversion == s->m_pUDT->m_iIPversion) && (i->second.m_iMSS == s->m_pUDT->m_iMSS) && i->second.m_bReusable)
			{
				if (i->second.m_iPort == port)
				{
					// reuse the existing multiplexer
					++ i->second.m_iRefCount;
					s->m_pUDT->m_pSndQueue = i->second.m_pSndQueue;
					s->m_pUDT->m_pRcvQueue = i->second.m_pRcvQueue;
					s->m_iMuxID = i->second.m_iID;
					return;
				}
			}
		}
	}

	// a new multiplexer is needed
	CMultiplexer m;
	m.m_iMSS = s->m_pUDT->m_iMSS;
	m.m_iIPversion = s->m_pUDT->m_iIPversion;
	m.m_iRefCount = 1;
	m.m_bReusable = s->m_pUDT->m_bReuseAddr;
	m.m_iID = s->m_SocketID;

	m.m_pChannel = new CChannel(s->m_pUDT->m_iIPversion);
	m.m_pChannel->setSndBufSize(s->m_pUDT->m_iUDPSndBufSize);
	m.m_pChannel->setRcvBufSize(s->m_pUDT->m_iUDPRcvBufSize);

	try
	{
		if (NULL != udpsock)
			m.m_pChannel->open(*udpsock);
		else
			m.m_pChannel->open(addr);
	}
	catch (CUDTException& e)
	{
		m.m_pChannel->close();
		delete m.m_pChannel;
		throw e;
	}

	sockaddr* sa = (AF_INET == s->m_pUDT->m_iIPversion) ? (sockaddr*) new sockaddr_in : (sockaddr*) new sockaddr_in6;
	m.m_pChannel->getSockAddr(sa);
	m.m_iPort = (AF_INET == s->m_pUDT->m_iIPversion) ? ntohs(((sockaddr_in*)sa)->sin_port) : ntohs(((sockaddr_in6*)sa)->sin6_port);
	if (AF_INET == s->m_pUDT->m_iIPversion) delete (sockaddr_in*)sa; else delete (sockaddr_in6*)sa;

	m.m_pTimer = new CTimer;

	m.m_pSndQueue = new CSndQueue;
	m.m_pSndQueue->init(m.m_pChannel, m.m_pTimer);
	m.m_pRcvQueue = new CRcvQueue;
	m.m_pRcvQueue->init(32, s->m_pUDT->m_iPayloadSize, m.m_iIPversion, 1024, m.m_pChannel, m.m_pTimer);

	m_mMultiplexer[m.m_iID] = m;

	s->m_pUDT->m_pSndQueue = m.m_pSndQueue;
	s->m_pUDT->m_pRcvQueue = m.m_pRcvQueue;
	s->m_iMuxID = m.m_iID;
}

void CUDTUnited::updateMux(CUDTSocket* s, const CUDTSocket* ls)
{
	CGuard cg(m_ControlLock);

	int port = (AF_INET == ls->m_iIPversion) ? ntohs(((sockaddr_in*)ls->m_pSelfAddr)->sin_port) : ntohs(((sockaddr_in6*)ls->m_pSelfAddr)->sin6_port);

	// find the listener's address
	for (std::map<int, CMultiplexer>::iterator i = m_mMultiplexer.begin(); i != m_mMultiplexer.end(); ++ i)
	{
		if (i->second.m_iPort == port)
		{
			// reuse the existing multiplexer
			++ i->second.m_iRefCount;
			s->m_pUDT->m_pSndQueue = i->second.m_pSndQueue;
			s->m_pUDT->m_pRcvQueue = i->second.m_pRcvQueue;
			s->m_iMuxID = i->second.m_iID;
			return;
		}
	}
}

#ifndef WIN32
void* CUDTUnited::worker(void* p)
#else
DWORD WINAPI CUDTUnited::worker(LPVOID p)
#endif
{
	CUDTUnited* pThis = (CUDTUnited*)p;

	CGuard workguard(pThis->m_GCStopLock);

	while (!pThis->m_bClosing)
	{
		re_main(signal_handler);
	}

#ifndef WIN32
	return NULL;
#else
	return 0;
#endif
}