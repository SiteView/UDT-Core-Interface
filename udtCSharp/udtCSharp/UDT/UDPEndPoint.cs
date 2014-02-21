using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Threading;
using System.Net;
using System.Net.Sockets;
using System.Collections;
using udtCSharp.packets;

namespace udtCSharp.UDT
{
    public class UDPEndPoint
    {
	    private  int port;

	    private Socket dgSocket;

	    /// <summary>
	    /// active sessions keyed by socket ID 
	    /// </summary>
	    private  Dictionary<long,UDTSession> sessions = new Dictionary<long, UDTSession>();

        /// <summary>
        /// last received packet
        /// </summary>
        private UDTPacket lastPacket;
	    
	    /// <summary>
        /// if the endpoint is configured for a server socket,
	    /// this queue is used to handoff new UDTSessions to the application
	    /// </summary>
	    private  Queue<UDTSession> sessionHandoff=new Queue<UDTSession>();
	
	    private bool serverSocketMode=false;

	    /// <summary>
	    /// has the endpoint been stopped?
	    /// </summary>
        private volatile bool stopped = false;
        /// <summary>
        /// 数据长度
        /// </summary>
	    public static int DATAGRAM_SIZE=1400;

        /// <summary>
        /// 当前socket 的连接地址
        /// </summary>
        private IPEndPoint _ipep = new IPEndPoint(IPAddress.Any, 0);
        /// <summary>
        /// 运程主机的地址
        /// </summary>
        private EndPoint Remote = null;
         
        /// <summary>
        /// 在给定的套接字上创建UDPEndPoint
        /// </summary>
        /// <param name="_socket">a UDP socket</param>
        public UDPEndPoint(Socket _socket)
        {
            this.dgSocket = _socket;            
            port = ((IPEndPoint)dgSocket.LocalEndPoint).Port;

            _ipep = new IPEndPoint(((IPEndPoint)dgSocket.LocalEndPoint).Address, port);
            this.dgSocket.Bind(_ipep);
        }
      
        /// <summary>
        /// 绑定到给定的地址和端口上创建UDPEndPoint
        /// </summary>
        /// <param name="localAddress">IP地址</param>
        /// <param name="localPort">瑞口</param>
        public UDPEndPoint(IPAddress localAddress, int localPort)
        {
            try
            {
                if (localAddress == null)
                {
                    _ipep = new IPEndPoint(IPAddress.Any, localPort);
                    dgSocket = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, ProtocolType.Udp);
                }
                else
                {
                    _ipep = new IPEndPoint(localAddress, localPort);
                    dgSocket = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, ProtocolType.Udp);
                }
                if (localPort > 0)
                {
                    this.port = localPort;
                }
                else
                {
                    port = ((IPEndPoint)dgSocket.LocalEndPoint).Port;
                }

                //set a time out to avoid blocking in doReceive()
                dgSocket.ReceiveTimeout = 100000;
                //buffer size
                dgSocket.ReceiveBufferSize = 128 * 1024;

                dgSocket.Bind(_ipep);
            }
            catch (Exception exc)
            {
                Log.Write(exc);
            }
        }
       
        /// <summary>
        ///启动 endpoint. 如果serverSocketModeEnabled标志<code>真</ code>，
        ///一个新的连接可以传递给应用程序。该应用程序需要调用＃accept（）方法来获取socket
        /// </summary>
        /// <param name="serverSocketModeEnabled"></param>
        public void start(bool serverSocketModeEnabled)
        {
            this.serverSocketMode = serverSocketModeEnabled;

            Thread t = new Thread(new ThreadStart(receive));
            t.IsBackground = true;
            t.Start();
            Log.Write("UDTEndpoint started.");
        }

        private void receive()
        {
            try
            {
                doReceive();
            }
            catch (Exception ex)
            {
                Log.Write("WARNING", "", ex);
            }
        }

        /// <summary>
        /// 启动 endpoint.
        /// </summary>
        public void start()
        {
            start(false);
        }

        /// <summary>
        /// 暂停 endpoint.
        /// </summary>
        public void stop()
        {
            stopped = true;
            dgSocket.Close();
        }
       
        /// <summary>
        /// 返回socket的连接端口
        /// </summary>
        /// <returns></returns>
        public int getLocalPort() 
        {
            return ((IPEndPoint)this.dgSocket.LocalEndPoint).Port;
        }
       
        /// <summary>
        /// 返回socket的连接地址
        /// </summary>
        /// <returns></returns>
        public IPAddress getLocalAddress()
        {
            return ((IPEndPoint)this.dgSocket.LocalEndPoint).Address;
        }

        private Socket getSocket()
        {
            return dgSocket;
        }

        private UDTPacket getLastPacket()
        {
            return lastPacket;
        }

        /// <summary>
        /// 存储UDTSession会话
        /// </summary>
        /// <param name="destinationID"></param>
        /// <param name="session"></param>
        public void addSession(long destinationID,UDTSession session)
        {
            Log.Write("Storing session <"+destinationID+">");
            this.sessions.Add(destinationID, session);           
        }

        
        public UDTSession getSession(long destinationID)
        {
            UDTSession _udtsession = null;
            this.sessions.TryGetValue(destinationID,out _udtsession);

            return _udtsession;
        }

        public List<UDTSession> getSessions()
        {
            List<UDTSession> valueslist = new List<UDTSession>();
            Dictionary<long,UDTSession>.ValueCollection valueColl =  this.sessions.Values;
            foreach(UDTSession s in valueColl)
            {
               valueslist.Add(s);
            }
           
            return valueslist;
        }
      
        /// <summary>
        /// 从队列中取出一个UDTSession会话
        /// </summary>      
        /// <returns></returns>
        protected UDTSession accept()
        {
            try
            {
                return this.sessionHandoff.Dequeue();
            }
            catch
            {
                return null;
            }
        }

        byte[] dp = new byte[DATAGRAM_SIZE];//数据

        ///**
        // * single receive, run in the receiverThread, see {@link #start()}
        // * <ul>
        // * <li>Receives UDP packets from the network</li> 
        // * <li>Converts them to UDT packets</li>
        // * <li>dispatches the UDT packets according to their destination ID.</li>
        // * </ul> 
        // * @throws IOException
        // */
        private long lastDestID=-1;
        private UDTSession lastSession;       
	
        private int n=0;

        private object thisLock = new object();
	
        protected void doReceive()
        {
            while(!stopped){
                try{
                    try{
                        //will block until a packet is received or timeout has expired
                        IPEndPoint sender = new IPEndPoint(IPAddress.Any, 0);
                        EndPoint Remote = (EndPoint)sender;
                        //dgSocket.Bind(_ipep);
                        dgSocket.ReceiveFrom(dp,ref Remote);
					    
                        sender = (IPEndPoint)Remote;
                        Destination peer=new Destination(sender.Address,sender.Port);

                        int l=dp.Length;
                        UDTPacket packet=PacketFactory.createPacket(dp,l);
                        lastPacket=packet;

                        //handle connection handshake 
                        if(packet.isConnectionHandshake())
                        {
                            lock (thisLock)
                            {
                                long id=packet.getDestinationID();
                                UDTSession session;
                                sessions.TryGetValue(id, out session);
                                if(session==null)
                                {
                                    session = new ServerSession(dp, this);
                                    addSession(session.getSocketID(), session);
                                    //TODO need to check peer to avoid duplicate server session
                                    if (serverSocketMode)
                                    {
                                        Log.Write("Pooling new request.");
                                        sessionHandoff.Enqueue(session);
                                        Log.Write("Request taken for processing.");
                                    }
                                }
                                peer.setSocketID(((ConnectionHandshake)packet).getSocketID());
                                session.received(packet,peer);
                            }
                        }
                        else
                        {
                            //dispatch to existing session
                            long dest=packet.getDestinationID();
                            UDTSession session;
                            if(dest==lastDestID)
                            {
                                session=lastSession;
                            }
                            else
                            {
                                sessions.TryGetValue(dest, out session);
                                lastSession=session;
                                lastDestID=dest;
                            }
                            if(session==null){
                                n++;
                                if(n%100==1){
                                    Log.Write("Unknown session <"+dest+"> requested from <"+peer+"> packet type "+packet.getClass().getName());
                                }
                            }
                            else
                            {
                                session.received(packet,peer);
                            }
                        }
                    }
                    catch(SocketException ex)
                    {
                         Log.Write("INFO", "",ex);
                    }

                }catch(Exception ex){
                     Log.Write("WARNING", "",ex);
                }
            }
        }

        protected void doSend(UDTPacket packet)
        {
            try
            {
                byte[] data = packet.getEncoded();                
                byte[] dgp = packet.getSession().getDatagram();
                this.dgSocket.SendTo(dgp, Remote);
            }
            catch
            { }
        }

        public String toString()
        {
            return "UDPEndpoint port=" + port.ToString();
        }

        public void sendRaw(byte[] p)
        {
            this.dgSocket.Send(p);
        }
       
    }
}
