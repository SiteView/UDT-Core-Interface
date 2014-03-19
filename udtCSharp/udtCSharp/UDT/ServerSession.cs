using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using udtCSharp.packets;

namespace udtCSharp.UDT
{
    public class ServerSession : UDTSession 
    {
	    private UDPEndPoint endPoint;

	    //last received packet (for testing purposes)
	    new private UDTPacket lastPacket;

        public ServerSession(byte[] dp, UDPEndPoint endPoint)
            : base("ServerSession localPort=" + endPoint.getLocalPort() + " peer=" + endPoint.remoteIP.Address + ":" + endPoint.remoteIP.Port, new Destination(endPoint.remoteIP.Address, endPoint.remoteIP.Port))
        {
		    this.endPoint=endPoint;
            Log.Write(this.ToString(), "Created " + toString() + " talking to " + endPoint.remoteIP.Address + ":" + endPoint.remoteIP.Port);
	    }      

	    int n_handshake=0;

	    public override void received(UDTPacket packet, Destination peer)
        {
		    lastPacket=packet;

		    if(packet is ConnectionHandshake) 
            {
			    ConnectionHandshake connectionHandshake=(ConnectionHandshake)packet;
			    Log.Write(this.ToString(),"Received "+connectionHandshake);

			    if (getState()<=ready)
                {
				    destination.setSocketID(connectionHandshake.getSocketID());

				    if(getState()<=handshaking)
                    {
					    setState(handshaking);
				    }
				    try
                    {
					    handleHandShake(connectionHandshake);
					    n_handshake++;
					    try{						    
						    socket=new UDTSocket(endPoint, this);
                            setState(ready);
						    cc.init();
					    }catch(Exception uhe){
						    //session is invalid
						     Log.Write(this.ToString(),"SEVERE",uhe);
						    setState(invalid);
					    }
				    }catch(Exception ex){
					    //session invalid
					     Log.Write(this.ToString(),"WARNING:Error processing ConnectionHandshake",ex);
					    setState(invalid);
				    }
				    return;
			    }
		    }
            else if(packet is KeepAlive) 
            {
			    socket.getReceiver().resetEXPTimer();
			    active = true;
			    return;
		    }

		    if(getState()== ready) 
            {
			    active = true;

			    if (packet is KeepAlive)
                {
				    //nothing to do here
				    return;
			    }
                else if (packet is Shutdown)
                {
				    try
                    {
					    socket.getReceiver().stop();
				    }
                    catch(Exception ex)
                    {
					    Log.Write(this.ToString(),"WARNING",ex);
				    }
				    setState(shutdown);
				    Log.Write(this.ToString(),"SHUTDOWN ***");
				    active = false;
				    Log.Write(this.ToString(),"Connection shutdown initiated by the other side.");
				    return;
			    }
			    else
                {
				    try{
					    if(packet.forSender())
                        {
						    socket.getSender().receive(packet);
					    }
                        else
                        {
                            if (packet.getMessageNumber() == 9999)//作为返回确认数据包
                            {
                                //通知可以继续发送数据
                                socket.getSender().receive(packet);
                            }
                            else
                            {
                                socket.getReceiver().receive(packet);
                            }						    
					    }
				    }catch(Exception ex)
                    {
					    //session invalid
					    Log.Write(this.ToString(),"SEVERE",ex);
					    setState(invalid);
				    }
			    }
			    return;
		    }
	    }

	    /**
	     * for testing use only
	     */
	    UDTPacket getLastPacket()
        {
		    return lastPacket;
	    }

	    /**
	     * handle the connection handshake:<br/>
	     * <ul>
	     * <li>set initial sequence number</li>
	     * <li>send response handshake</li>
	     * </ul>
	     * @param handshake
	     * @param peer
	     * @throws IOException
	     */
	    protected void handleHandShake(ConnectionHandshake handshake)
        {
            try
            {
                ConnectionHandshake responseHandshake = new ConnectionHandshake();
                //compare the packet size and choose minimun
                long clientBufferSize = handshake.getPacketSize();
                long myBufferSize = getDatagramSize();
                long bufferSize = Math.Min(clientBufferSize, myBufferSize);
                long initialSequenceNumber = handshake.getInitialSeqNo();
                setInitialSequenceNumber(initialSequenceNumber);
                setDatagramSize((int)bufferSize);
                responseHandshake.setPacketSize(bufferSize);
                responseHandshake.setUdtVersion(4);
                responseHandshake.setInitialSeqNo(initialSequenceNumber);
                responseHandshake.setConnectionType(-1);
                responseHandshake.setMaxFlowWndSize(handshake.getMaxFlowWndSize());
                //tell peer what the socket ID on this side is 
                responseHandshake.setSocketID(mySocketID);
                responseHandshake.setDestinationID(this.getDestination().getSocketID());
                responseHandshake.setSession(this);
                Log.Write(this.ToString(),"Sending reply " + responseHandshake);
                endPoint.doSend(responseHandshake);
            }
            catch(Exception exc)
            {
                Log.Write(this.ToString(),exc);
            }
	    }
    }
}
