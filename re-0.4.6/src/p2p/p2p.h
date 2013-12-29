#ifndef _P2P_H_
#define _P2P_H_

#include <re_types.h>
#include <re_mem.h>
#include <re_list.h>
#include <re_fmt.h>
#include <re_mbuf.h>
#include <re_sa.h>
#include <re_net.h>
#include <re_tcp.h>
#include <re_udp.h>
#include <re_ice.h>
#include <re_main.h>
#include <re_p2p.h>

#include <fstream>
#include <string>
#include <map>
#include <vector>

#include "common.h"

#define USERNAME_SIZE 512
#define PASSWD_SIZE 512
#define P2P_KEY_SIZE 64

struct ice;
struct udp_sock;
struct sa;
struct tcp_conn;
struct p2p_handle;
struct p2p_connection;

typedef void (p2p_init_h)(int err, struct p2p_handle* p2p);
typedef void (p2p_connect_h)(int err,const char* reason, void* arg);
typedef void (p2p_receive_h)(const char* data, uint32_t len, void *arg);
typedef void (p2p_request_h)(struct p2p_handle* p2p, const char* peername, struct p2p_connection **ppconn);

struct p2p_handle
{
	struct tcp_conn *ltcp; /* control tcp socket */
	char *username;
	char *passwd;
	struct sa* sactl;
	struct sa* sastun;
	struct list lstconn;   /* connection list */
	char key[P2P_KEY_SIZE];
	p2p_request_h *requesth;
	p2p_init_h *inith;
	uint16_t status;
	P2PSOCKET psock;
};


struct p2p_connection
{
	struct le le;
	char *peername;
	struct ice *ice;
	struct p2p_handle *p2ph;
	struct udp_sock *ulsock;
	uint16_t status;
	p2p_connect_h *ch;
	p2p_receive_h *rh;
	void *arg;
};


class CP2P
{
	friend class CP2PSocket;
	friend class CP2PUnited;
	friend class CRendezvousQueue;
	friend class CSndQueue;
	friend class CRcvQueue;
	friend class CSndUList;
	friend class CRcvUList;
private:
	CP2P();
	CP2P(const CP2P & other);
	const CP2P& operator=(const CP2P& other) {return * this;}
	~CP2P();

public:
	static int p2p_init(const char* ctlserv, const int ctlport, const char* stunsrv, const int stunport, const char* username, const char* passwd, p2p_init_h *inith, p2p_request_h *requesth);
	static int p2p_run();
	static P2PSOCKET newSocket();
	static int send(const P2PSOCKET u, const char* buf, int size, int flags);
	static int recv(const P2PSOCKET u, const char* buf, int size, int flags);
	static uint64_t sendfile(const P2PSOCKET u, const char* path, uint64_t* offset, int64_t size, int block = 364000);
	static uint64_t recvfile(const P2PSOCKET u, const char* path, uint64_t* offset, int64_t size, int block = 7280000);

private:
	int send(const char* data, int size);
	int recv(char* data, int size);
	uint64_t sendfile(const char* path, uint64_t* offset, int64_t size, int block = 364000);
	uint64_t recvfile(const char* path, uint64_t* offset, int64_t size, int block = 7280000);

private:
	static CP2PUnited s_P2PUnited;

private: // synchronization: mutexes and conditions
	pthread_mutex_t m_ConnectionLock;            // used to synchronize connection operation

	pthread_cond_t m_SendBlockCond;              // used to block "send" call
	pthread_mutex_t m_SendBlockLock;             // lock associated to m_SendBlockCond

	pthread_mutex_t m_AckLock;                   // used to protected sender's loss list when processing ACK

	pthread_cond_t m_RecvDataCond;               // used to block "recv" when there is no data
	pthread_mutex_t m_RecvDataLock;              // lock associated to m_RecvDataCond

	pthread_mutex_t m_SendLock;                  // used to synchronize "send" call
	pthread_mutex_t m_RecvLock;                  // used to synchronize "recv" call

	void initSynch();
	void destroySynch();
	void releaseSynch();
};


class CP2PSocket
{
public:
	CP2PSocket();
	~CP2PSocket();

	P2PSOCKET m_SocketID;
	CP2P* m_pP2P;

private:
	CP2PSocket(const CP2PSocket& other);
	const CP2PSocket& operator=(const CP2PSocket& other);
};

class CP2PUnited
{
	friend class CP2P;
public:
	CP2PUnited();
	~CP2PUnited();

public:
	int p2p_init(const char* ctlserv, const int ctlport, const char* stunsrv, const int stunport, const char* username, const char* passwd, p2p_init_h *inith, p2p_request_h *requesth);
	int p2p_run();
	P2PSOCKET newSocket();
	CP2P* lookup(const P2PSOCKET u);

private:
	std::map<P2PSOCKET, CP2PSocket*> VECP2P;
	P2PSOCKET m_SocketID;
	pthread_mutex_t m_ControlLock;                    // used to synchronize UDT API
	pthread_mutex_t m_IDLock;                         // used to synchronize ID generation
	std::map<int64_t, std::set<P2PSOCKET> > m_PeerRec;// record sockets from peers to avoid repeated connection request, int64_t = (socker_id << 30) + isn

private:
	pthread_key_t m_TLSError;                         // thread local error record (last error)
#ifndef WIN32
	static void TLSDestroy(void* e) {if (NULL != e) delete (CUDTException*)e;}
#else
	std::map<DWORD, CUDTException*> m_mTLSRecord;
	void checkTLSValue();
	pthread_mutex_t m_TLSLock;
#endif

private:
	volatile bool m_bClosing;
	pthread_mutex_t m_GCStopLock;
	pthread_cond_t m_GCStopCond;

	pthread_mutex_t m_InitLock;
	int m_iInstanceCount;				// number of startup() called by application
	bool m_bGCStatus;					// if the GC thread is working (true)

	pthread_t m_GCThread;
#ifndef WIN32
	static void* garbageCollect(void*);
#else
	static DWORD WINAPI garbageCollect(LPVOID);
#endif

	std::map<P2PSOCKET, CP2PSocket*> m_ClosedSockets;   // temporarily store closed sockets

	void checkBrokenSockets();
	void removeSocket(const P2PSOCKET u);

private:
	struct p2p_handle* m_p2ph;
};

#endif	// _P2P_H_
