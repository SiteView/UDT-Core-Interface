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
    public class UDPEndPoint : IDisposable
    {
	    private  int port;

        private UdpClient _udpClient;

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
        /// has the endpoint been started?
        /// </summary>
        private volatile bool _started = false;
        /// <summary>
        /// 数据长度
        /// </summary>
	    public const int DATAGRAM_SIZE=1400;
        /// <summary>
        /// 运程主机的地址
        /// </summary>
        public IPEndPoint remoteIP = new IPEndPoint(IPAddress.Any, 0);
        /// <summary>
        /// 在给定的套接字上创建UDPEndPoint
        /// </summary>
        /// <param name="_socket">a UDP socket</param>
        public UDPEndPoint(UdpClient _udpsocket)
        {
            this._udpClient = _udpsocket;

            this.port = ((IPEndPoint)this._udpClient.Client.LocalEndPoint).Port;
        }
      
        /// <summary>
        /// 绑定到给定的地址和端口上创建UDPEndPoint
        /// </summary>
        /// <param name="localAddress">IP地址</param>
        /// <param name="localPort">瑞口</param>
        public UDPEndPoint(int localPort)
        {
            try
            {
                this.port = localPort;
                _udpClient = new UdpClient(this.port);
                uint IOC_IN = 0x80000000;
                uint IOC_VENDOR = 0x18000000;
                uint SIO_UDP_CONNRESET = IOC_IN | IOC_VENDOR | 12;
                _udpClient.Client.IOControl(
                    (int)SIO_UDP_CONNRESET,
                    new byte[] { Convert.ToByte(false) },
                    null);
            }
            catch (Exception exc)
            {
                Log.Write(this.ToString(),"Create a UdpClient connection failure", exc);
            }
        }

        /// <summary>
        /// 启动 endpoint.
        /// </summary>
        public void start()
        {            
            if (!_started)
            {
                _started = true;
                start(_started);
            }
        }

        /// <summary>
        ///启动 endpoint 监听. 如果是服务端的监听 传入值为 true 否则是 false
        /// </summary>
        /// <param name="serverSocketModeEnabled"></param>
        public void start(bool serverSocketModeEnabled)
        {
            _started = true;
            this.serverSocketMode = serverSocketModeEnabled;
            ReceiveInternal();          
            Log.Write(this.ToString(), "UDTEndpoint started.");
        }

        protected void ReceiveInternal()
        {
            if (!_started)
            {
                return;
            }
            try
            {
                this._udpClient.BeginReceive(new AsyncCallback(ReceiveCallback),null);
            }
            catch (SocketException ex)
            {
                Log.Write(this.ToString(), "ReceiveInternal data", ex);
            }
        }
      
        /// <summary>
        /// single receive, run in the receiverThread, see {@link #start()}
        /// Receives UDP packets from the network
        /// Converts them to UDT packets
        /// dispatches the UDT packets according to their destination ID.
        /// </summary>
        /// <param name="result"></param>
        private void ReceiveCallback(IAsyncResult result)
        {
            if (!_started)
            {
                return;
            }
            //IPEndPoint remoteIP = new IPEndPoint(IPAddress.Any, 0);
            //byte[] dp = new byte[DATAGRAM_SIZE];//数据
            byte[] buffer = null;
            try
            {
                buffer = this._udpClient.EndReceive(result, ref remoteIP);
            }
            catch (SocketException ex)
            {
                Log.Write(this.ToString(), "ReceiveCallback data", ex);
            }
            finally
            {
                ReceiveInternal();
            }

            this.ReceiveData(buffer, remoteIP);
        }
       
        private long lastDestID = -1;
        private UDTSession lastSession;
        private int n = 0;

        private object thisLock = new object();
       /// <summary>
       /// 处理接收的数据
       /// </summary>
       /// <param name="dp"></param>
       /// <param name="_remoteIP"></param>
        private void ReceiveData(byte[] dp, IPEndPoint _remoteIP)
        {
            try
            {
                Destination peer = new Destination(_remoteIP.Address, _remoteIP.Port);

                int l = dp.Length;
                UDTPacket packet = PacketFactory.createPacket(dp, l);
                lastPacket = packet;

                //handle connection handshake  处理连接握手
                if (packet.isConnectionHandshake())
                {
                    lock (thisLock)
                    {
                        long id = packet.getDestinationID();
                        UDTSession session;
                        sessions.TryGetValue(id, out session);//ClientSession 或是 ServerSession
                        if (session == null)
                        {
                            session = new ServerSession(dp, this);
                            addSession(session.getSocketID(), session);
                            //TODO need to check peer to avoid duplicate server session
                            if (serverSocketMode)
                            {
                                sessionHandoff.Enqueue(session);
                                Log.Write(this.ToString(), "Pooling new request, request taken for processing.");
                            }
                        }
                        try
                        {
                            peer.setSocketID(((ConnectionHandshake)packet).getSocketID());
                            session.received(packet, peer);//ClientSession 或是 ServerSession
                        }
                        catch (Exception ex)
                        {
                            Log.Write(this.ToString(), "WARNING", ex);
                        }
                    }
                }
                else
                {
                    //dispatch to existing session
                    long dest = packet.getDestinationID();
                    UDTSession session;
                    if (dest == lastDestID)
                    {
                        session = lastSession;
                    }
                    else
                    {
                        sessions.TryGetValue(dest, out session);
                        lastSession = session;
                        lastDestID = dest;
                    }
                    if (session == null)
                    {
                        n++;
                        if (n % 100 == 1)
                        {
                            Log.Write(this.ToString(), "Unknown session <" + dest + "> requested from <" + peer + "> packet type " + packet.ToString());
                        }
                    }
                    else
                    {
                        session.received(packet, peer);
                    }
                }
            }
            catch (SocketException ex)
            {
                Log.Write(this.ToString(), "INFO", ex);
            }
        }       

        /// <summary>
        /// 暂停 endpoint.
        /// </summary>
        public void stop()
        {
            this.stopped = true;
            this.Dispose();
        }
       
        /// <summary>
        /// 返回UdpClient的连接端口
        /// </summary>
        /// <returns></returns>
        public int getLocalPort() 
        {
            return this.port;
        }

        private UdpClient getSocket()
        {
            return this._udpClient;
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
            try
            {
                this.sessions.Add(destinationID, session);
                Log.Write(this.ToString(), "Storing session <" + destinationID + ">");
            }
            catch (Exception exc)
            {
                Log.Write(this.ToString(), "Storing session",exc);
            }
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
        public UDTSession accept()
        {
            try
            {
                if (this.sessionHandoff.Count > 0)
                {
                    return this.sessionHandoff.Dequeue();
                }
                else
                {
                    return null;
                }
            }
            catch
            {
                return null;
            }
        }
        /// <summary>
        /// 发送udp数据包
        /// </summary>
        /// <param name="packet"></param>
        public void doSend(UDTPacket packet)
        {
            try
            {
                byte[] data = packet.getEncoded();
                IPEndPoint dgp = packet.getSession().getDatagram();
                this.SendInternal(data, dgp);
            }
            catch (Exception exc)
            {
                Log.Write(this.ToString(), "send data err", exc);
            }
        }

        protected void SendInternal(byte[] buffer, IPEndPoint remoteIP)
        {
            if (!_started)
            {
                Log.Write(this.ToString(), "UDP Closed.");
            }
            try
            {
                this._udpClient.BeginSend(
                   buffer,
                   buffer.Length,
                   remoteIP,
                   new AsyncCallback(SendCallback),
                   null);
            }
            catch (SocketException ex)
            {
                Log.Write(this.ToString(), " SendInternal UDP send data", ex);
            }
        }

        private void SendCallback(IAsyncResult result)
        {
            try
            {
                this._udpClient.EndSend(result);
            }
            catch (SocketException ex)
            {
                Log.Write(this.ToString(), "SendCallback UDP send data", ex);
            }
        }       

        public String toString()
        {
            return "UDPEndpoint port=" + this.port.ToString();
        }

        #region IDisposable 成员

        public void Dispose()
        {
            _started = false;
            if (_udpClient != null)
            {
                _udpClient.Close();
                _udpClient = null;
            }
        }
        #endregion
    }
}
