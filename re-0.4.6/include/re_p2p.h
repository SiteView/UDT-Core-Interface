
#ifndef _RE_P2P_H_
#define _RE_P2P_H_

#include <set>

#ifdef WIN32
	#ifndef __MINGW__
		//#ifdef UDT_EXPORTS
		//	#define P2P_API __declspec(dllexport)
		//#else
		//	#define P2P_API __declspec(dllimport)
		//#endif
		#define P2P_API
	#else
		#define P2P_API
	#endif
#else
	#define P2P_API __attribute__ ((visibility("default")))
#endif

typedef int P2PSOCKET;
typedef std::set<P2PSOCKET> ud_set;
#define UD_CLR(u, uset) ((uset)->erase(u))
#define UD_ISSET(u, uset) ((uset)->find(u) != (uset)->end())
#define UD_SET(u, uset) ((uset)->insert(u))
#define UD_ZERO(uset) ((uset)->clear())


//////////////////////////////////////////////////////////////////////////
enum
{
	P2P_LOGINREQ = 1,
	P2P_LOGINREP,
	P2P_CONNREQ, 
	P2P_CONNREP, 
	P2P_DATA
};

enum
{
	P2P_INIT = 1,
	P2P_ACCEPT,
	P2P_CONNECTING,
	P2P_CONNECTED,
	P2P_CONNECTERR
};

enum EPOLLOpt
{
	// this values are defined same as linux epoll.h
	// so that if system values are used by mistake, they should have the same effect
	UDT_EPOLL_IN = 0x1,
	UDT_EPOLL_OUT = 0x4,
	UDT_EPOLL_ERR = 0x8
};

enum UDTSTATUS
{
	INIT = 1, 
	OPENED, 
	LISTENING, 
	CONNECTING, 
	CONNECTED, 
	BROKEN, 
	CLOSING, 
	CLOSED, 
	NONEXIST
};

enum UDTOpt
{
	UDT_MSS,             // the Maximum Transfer Unit
	UDT_SNDSYN,          // if sending is blocking
	UDT_RCVSYN,          // if receiving is blocking
	UDT_CC,              // custom congestion control algorithm
	UDT_FC,		// Flight flag size (window size)
	UDT_SNDBUF,          // maximum buffer in sending queue
	UDT_RCVBUF,          // UDT receiving buffer size
	UDT_LINGER,          // waiting for unsent data when closing
	UDP_SNDBUF,          // UDP sending buffer size
	UDP_RCVBUF,          // UDP receiving buffer size
	UDT_MAXMSG,          // maximum datagram message size
	UDT_MSGTTL,          // time-to-live of a datagram message
	UDT_RENDEZVOUS,      // rendezvous connection mode
	UDT_SNDTIMEO,        // send() timeout
	UDT_RCVTIMEO,        // recv() timeout
	UDT_REUSEADDR,	// reuse an existing port or create a new one
	UDT_MAXBW,		// maximum bandwidth (bytes per second) that the connection can use
	UDT_STATE,		// current socket state, see UDTSTATUS, read only
	UDT_EVENT,		// current avalable events associated with the socket
	UDT_SNDDATA,		// size of data in the sending buffer
	UDT_RCVDATA		// size of data available for recv
};


////////////////////////////////////////////////////////////////////////////////
// struct CPerfMon
struct CPerfMon
{
	// global measurements
	int64_t msTimeStamp;                 // time since the UDT entity is started, in milliseconds
	int64_t pktSentTotal;                // total number of sent data packets, including retransmissions
	int64_t pktRecvTotal;                // total number of received packets
	int pktSndLossTotal;                 // total number of lost packets (sender side)
	int pktRcvLossTotal;                 // total number of lost packets (receiver side)
	int pktRetransTotal;                 // total number of retransmitted packets
	int pktSentACKTotal;                 // total number of sent ACK packets
	int pktRecvACKTotal;                 // total number of received ACK packets
	int pktSentNAKTotal;                 // total number of sent NAK packets
	int pktRecvNAKTotal;                 // total number of received NAK packets
	int64_t usSndDurationTotal;		// total time duration when UDT is sending data (idle time exclusive)

	// local measurements
	int64_t pktSent;                     // number of sent data packets, including retransmissions
	int64_t pktRecv;                     // number of received packets
	int pktSndLoss;                      // number of lost packets (sender side)
	int pktRcvLoss;                      // number of lost packets (receiver side)
	int pktRetrans;                      // number of retransmitted packets
	int pktSentACK;                      // number of sent ACK packets
	int pktRecvACK;                      // number of received ACK packets
	int pktSentNAK;                      // number of sent NAK packets
	int pktRecvNAK;                      // number of received NAK packets
	double mbpsSendRate;                 // sending rate in Mb/s
	double mbpsRecvRate;                 // receiving rate in Mb/s
	int64_t usSndDuration;		// busy sending time (i.e., idle time exclusive)

	// instant measurements
	double usPktSndPeriod;               // packet sending period, in microseconds
	int pktFlowWindow;                   // flow window size, in number of packets
	int pktCongestionWindow;             // congestion window size, in number of packets
	int pktFlightSize;                   // number of packets on flight
	double msRTT;                        // RTT, in milliseconds
	double mbpsBandwidth;                // estimated bandwidth, in Mb/s
	int byteAvailSndBuf;                 // available UDT sender buffer size
	int byteAvailRcvBuf;                 // available UDT receiver buffer size
};

namespace P2P
{
	P2P_API int p2p_init(const char* ctlserv, const int ctlport, const char* stunsrv, const int stunport, const char* username, const char* passwd);
	P2P_API int p2p_connect(const char* peername);
	P2P_API int p2p_accept(const char* peername);
	P2P_API int p2p_disconnect(const char* peername);
	P2P_API int p2p_close();
	P2P_API int p2p_run();
	P2P_API int p2p_send(const P2PSOCKET u, const char* buf, uint32_t size);
	P2P_API int p2p_recv(const P2PSOCKET u, const char* buf, uint32_t size);
	P2P_API uint64_t p2p_sendfile(const P2PSOCKET u, const char* path, uint64_t* offset, uint64_t size, int block);
	P2P_API uint64_t p2p_recvfile(const P2PSOCKET u, const char* path, uint64_t* offset, uint64_t size, int block);
}

#endif