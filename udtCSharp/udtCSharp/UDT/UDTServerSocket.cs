using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Threading;
using System.Net;
using System.Net.Sockets;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

using udtCSharp.packets;
using udtCSharp.Common;

namespace udtCSharp.UDT
{
    public class UDTServerSocket 
    {	
	    private UDPEndPoint endpoint;
	
	    private bool started=false;
	
	    private volatile bool shutdown=false;	
      
        /// <summary>
        /// Create a UDT ServerSocket
        /// </summary>
        /// <param name="localAddress"></param>
        /// <param name="port">本地端口。如果为0，将选择任一个临时端口</param>
	    public UDTServerSocket(IPAddress localAddress, int port)
        {
            try
            {
		        endpoint=new UDPEndPoint(localAddress,port);
		        Log.Write(this.ToString(),"Created server endpoint on port "+endpoint.getLocalPort());
            }
            catch(Exception exc)
            {
                Log.Write(this.ToString(),"Create a UDT ServerSocket",exc);
            }
	    }

	    /// <summary>
	    /// starts a server on localhost
	    /// </summary>
	    /// <param name="port"></param>
	    public UDTServerSocket(int port):this(Dns.GetHostEntry(Dns.GetHostName()).AddressList[0],port)
        {
            ;
	    }	
      
        /// <summary>
        /// listens and blocks until a new client connects and returns a valid {@link UDTSocket} for the new connection
        /// 监听和阻塞，直到一个新的客户端连接，并返回一个有效的的{@ link UDTSocket}为新的连接
        /// </summary>
        /// [MethodImpl(MethodImplOptions.Synchronized)]表示定义为同步方法
        /// [MethodImpl(MethodImplOptions.Synchronized)] = loc(this)
        /// </summary>
        [MethodImpl(MethodImplOptions.Synchronized)]
	    public UDTSocket Accept()
        {
            try
            {
                if (!started)
                {
                    endpoint.start(true);
                    started = true;
                }
                while (!shutdown)
                {
                    try
                    {
                        //UDTSession session = endpoint.accept(10000, TimeUnit.MILLISECONDS);
                        UDTSession session = endpoint.accept();
                        if (session != null)
                        {
                            //wait for handshake to complete
                            while (!session.isReady() || session.getSocket() == null)
                            {
                                Thread.Sleep(100);
                            }
                            return session.getSocket();
                        }
                    }
                    catch (Exception exc)
                    {                        
                        Log.Write(this.ToString(), "listens and blocks until a new client connects and returns a valid {@link UDTSocket} for the new connection", exc);
                        return null;
                    }
                }
                return null;
            }
            catch (Exception exc)
            {
                Log.Write(this.ToString(), "listens and blocks until a new client connects and returns a valid {@link UDTSocket} for the new connection", exc);
                return null;
            }
	    } 
	
	    public void shutDown()
        {
		    this.shutdown=true;
            this.endpoint.stop();
	    }
	
	    public UDPEndPoint getEndpoint()
        {
		    return endpoint;
	    }
    }
}
