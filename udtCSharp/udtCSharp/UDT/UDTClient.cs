using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Threading;
using System.Net;
using System.Net.Sockets;
using udtCSharp.packets;

namespace udtCSharp.UDT
{
    public class UDTClient 
    {
	    private UDPEndPoint clientEndpoint;
	    private ClientSession clientSession;

	    public UDTClient(IPAddress address, int localport)
        {
            try
            {
		        //create endpoint
		        clientEndpoint=new UDPEndPoint(address,localport);//127.0.0.1 55321
		        Log.Write(this.ToString(),"Created client endpoint on port "+localport);
            }
            catch(Exception exc)
            {
                Log.Write(this.ToString(),exc);
            }
	    }

        public UDTClient(IPAddress address)
        {
		    //create endpoint
		    clientEndpoint = new UDPEndPoint(address);
            Log.Write(this.ToString(),"Created client endpoint on port " + clientEndpoint.getLocalPort());
	    }

	    public UDTClient(UDPEndPoint endpoint)
        {
		    clientEndpoint=endpoint;
	    }

      
        /// <summary>
        /// establishes a connection to the given server. 
        /// Starts the sender thread.
        /// </summary>
        /// <param name="host">127.0.0.1</param>
        /// <param name="port">65321</param>
	    public void connect(String host, int port)
        {
            try
            {
		        IPAddress address = IPAddress.Parse(host);
		        Destination destination=new Destination(address,port);
		        //create client session...
		        clientSession=new ClientSession(clientEndpoint,destination);
		        clientEndpoint.addSession(clientSession.getSocketID(), clientSession);

		        clientEndpoint.start();
		        clientSession.connect();
		        //wait for handshake
		        while(!clientSession.isReady())
                {
			        Thread.Sleep(500);
		        }
		        Log.Write(this.ToString(),"The UDTClient is connected");
		        Thread.Sleep(500);
            }
            catch(Exception exc)
            {
                Log.Write(this.ToString(),exc);
            }
	    }
      
        /// <summary>
        /// sends the given data asynchronously
        /// </summary>
        /// <param name="data"></param>
	    public void send(byte[]data)
        {
            try
            {
		        clientSession.getSocket().doWrite(data);
            }
            catch(Exception exc)
            {
                Log.Write(this.ToString(),exc);
            }
	    }

	    public void sendBlocking(byte[]data)
        {
            try
            {
		        clientSession.getSocket().doWriteBlocking(data);
            }
            catch(Exception exc)
            {
                Log.Write(this.ToString(),exc);
            }
	    }

	    public int read(byte[]data)
        {
            try
            {
                //return clientSession.getSocket().getInputStream().Read(data);
                return clientSession.getSocket().getInputStream().Read(data, 0, data.Length);
            }
            catch(Exception exc)
            {
                Log.Write(this.ToString(),exc);
                return 0;
            }
	    }

	    /**
	     * flush outstanding data (and make sure it is acknowledged)
	     * @throws IOException
	     * @throws InterruptedException
	     */
	    public void flush()
        {
            try
            {
		        clientSession.getSocket().flush();
            }
            catch(Exception exc)
            {                
                Log.Write(this.ToString(),exc);
            }
	    }


	    public void shutdown()
        {
		    if (clientSession.isReady()&& clientSession.active==true) 
		    {
			    Shutdown shutdown = new Shutdown();
			    shutdown.setDestinationID(clientSession.getDestination().getSocketID());
			    shutdown.setSession(clientSession);
			    try{
				    clientEndpoint.doSend(shutdown);
			    }
			    catch(Exception e)
			    {
				    Log.Write(this.ToString(),"SEVERE ERROR: Connection could not be stopped!",e);
			    }
			    clientSession.getSocket().getReceiver().stop();
			    clientEndpoint.stop();
		    }
	    }

	    public UDTInputStream getInputStream()
        {
		    return clientSession.getSocket().getInputStream();
	    }

	    public UDTOutputStream getOutputStream()
        {
		    return clientSession.getSocket().getOutputStream();
	    }

	    public UDPEndPoint getEndpoint()
        {
		    return clientEndpoint;
	    }

	    public UDTStatistics getStatistics()
        {
		    return clientSession.getStatistics();
	    }

    }
}
