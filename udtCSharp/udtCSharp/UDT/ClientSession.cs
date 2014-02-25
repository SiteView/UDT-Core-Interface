using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Threading;
using System.Net;
using System.Net.Sockets;
using udtCSharp.packets;
using udtCSharp.Common;

namespace udtCSharp.UDT
{

     /// <summary>
     ///* Client side of a client-server UDT connection. 
     ///* Once established, the session provides a valid {@link UDTSocket}.
     /// </summary>
    public class ClientSession : UDTSession 
    {
	    private UDPEndPoint endPoint;

	    public ClientSession(UDPEndPoint endPoint, Destination dest):base("ClientSession localPort="+endPoint.getLocalPort(),dest)
        {		    
		    this.endPoint=endPoint;
		    Log.Write(this.ToString(),"Created "+toString());
	    }
       
        /// <summary>
        /// send connection handshake until a reply from server is received
        /// * TODO check for timeout
        /// * @throws InterruptedException
        /// * @throws IOException
        /// </summary>
	    public void connect()
        {
            try
            {
		        int n=0;
		        while(getState()!=ready)
                {
			        sendHandShake();
			        if(getState()==invalid)
                    {
                         Log.Write(this.ToString(),"Can't connect!");
                    }
			        n++;
			        if(getState()!=ready)Thread.Sleep(500);
		        }
		        cc.init();
		        Log.Write(this.ToString(),"Connected, "+n+" handshake packets sent");
            }
            catch(Exception exc)
            {
                 Log.Write(this.ToString(),exc);
            }
	    }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="packet"></param>
        /// <param name="peer"></param>
	    public override void received(UDTPacket packet, Destination peer) 
        {
		    lastPacket=packet;

		    if (packet is ConnectionHandshake) 
            {
			    ConnectionHandshake hs=(ConnectionHandshake)packet;

			    Log.Write(this.ToString(),"Received connection handshake from "+peer.getAddress()+" "+peer.getPort()+"\n"+hs.toString());

			    if (getState()!=ready)
                {
				    if(hs.getConnectionType()==1)
                    {
					    try
                        {
						    //TODO validate parameters sent by peer
						    long peerSocketID=hs.getSocketID();
						    destination.setSocketID(peerSocketID);
						    sendConfirmation(hs);
					    }
                        catch(Exception ex){
						     Log.Write(this.ToString(),"WARNING:Error creating socket",ex);
						    setState(invalid);
					    }
					    return;
				    }
				    else{
					    try{
						    //TODO validate parameters sent by peer
						    long peerSocketID=hs.getSocketID();
						    destination.setSocketID(peerSocketID);
						    setState(ready);
						    socket=new UDTSocket(endPoint,this);		
					    }catch(Exception ex){
						     Log.Write(this.ToString(),"WARNING:Error creating socket",ex);
						    setState(invalid);
					    }
					    return;
				    }
			    }
		    }

		    if(getState() == ready) 
            {
			    if(packet is Shutdown){
				    setState(shutdown);
				    active=false;
				    Log.Write(this.ToString(),"Connection shutdown initiated by the other side.");
				    return;
			    }
			    active = true;
			    try{
				    if(packet.forSender()){
					    socket.getSender().receive(lastPacket);
				    }else{
					    socket.getReceiver().receive(lastPacket);	
				    }
			    }catch(Exception ex){
				    //session is invalid
				    Log.Write(this.ToString(),"SEVERE:Error in "+toString(),ex);
				    setState(invalid);
			    }
			    return;
		    }
	    }


	    /// <summary>
	    /// handshake for connect
	    /// </summary>
	    protected void sendHandShake()
        {
            try
            {
		        ConnectionHandshake handshake = new ConnectionHandshake();
		        handshake.setConnectionType(ConnectionHandshake.CONNECTION_TYPE_REGULAR);
		        handshake.setSocketType(ConnectionHandshake.SOCKET_TYPE_DGRAM);
		        long initialSequenceNo=SequenceNumber.random();
		        setInitialSequenceNumber(initialSequenceNo);
		        handshake.setInitialSeqNo(initialSequenceNo);
		        handshake.setPacketSize(getDatagramSize());
		        handshake.setSocketID(mySocketID);
		        handshake.setMaxFlowWndSize(flowWindowSize);
		        handshake.setSession(this);
		        Log.Write(this.ToString(),"Sending "+handshake);
		        endPoint.doSend(handshake);
            }
            catch(Exception exc)
            {
                Log.Write(this.ToString(),exc);
            }
	    }

        /// <summary>
        /// 2nd handshake for connect
        /// </summary>
        /// <param name="hs"></param>
	    protected void sendConfirmation(ConnectionHandshake hs)
        {
            try
            {
                ConnectionHandshake handshake = new ConnectionHandshake();
                handshake.setConnectionType(-1);
                handshake.setSocketType(ConnectionHandshake.SOCKET_TYPE_DGRAM);
                handshake.setInitialSeqNo(hs.getInitialSeqNo());
                handshake.setPacketSize(hs.getPacketSize());
                handshake.setSocketID(mySocketID);
                handshake.setMaxFlowWndSize(flowWindowSize);
                handshake.setSession(this);
                Log.Write(this.ToString(), "Sending confirmation " + handshake.toString());
                endPoint.doSend(handshake);
            }
            catch (Exception exc)
            {
                Log.Write(this.ToString(), exc);
            }
	    }

	    public UDTPacket getLastPkt()
        {
		    return lastPacket;
	    }
    }
}
