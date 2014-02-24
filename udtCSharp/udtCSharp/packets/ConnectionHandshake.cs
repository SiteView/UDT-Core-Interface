using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using udtCSharp.UDT;
using System.IO;

namespace udtCSharp.packets
{
   public class ConnectionHandshake : ControlPacket
   {
	    private long udtVersion=4;
	
	    public static long SOCKET_TYPE_STREAM=0;
	
	    public static long SOCKET_TYPE_DGRAM=1;
	
	    private long socketType= SOCKET_TYPE_DGRAM; //stream or dgram
	
	    private long initialSeqNo = 0;
	    private long packetSize;
	    private long maxFlowWndSize;
	
	    public static long CONNECTION_TYPE_REGULAR=1;
	
	    public static long CONNECTION_TYPE_RENDEZVOUS=0;
	
	    private long connectionType = CONNECTION_TYPE_REGULAR;//regular or rendezvous mode
	
	    private long socketID;
	
	    private long cookie=0;
	
	    public ConnectionHandshake()
        {
		    this.controlPacketType= (int)ControlPacketType.CONNECTION_HANDSHAKE;
	    }
	
	    public ConnectionHandshake(byte[]controlInformation):this()
        {
		    decode(controlInformation);
	    }
	
	    //faster than instanceof...
        public override bool isConnectionHandshake()
        {
		    return true;
	    }
	
	    void decode(byte[]data)
        {
		    udtVersion =PacketUtil.decode(data, 0);
		    socketType=PacketUtil.decode(data, 4);
		    initialSeqNo=PacketUtil.decode(data, 8);
		    packetSize=PacketUtil.decode(data, 12);
		    maxFlowWndSize=PacketUtil.decode(data, 16);
		    connectionType=PacketUtil.decode(data, 20);
		    socketID=PacketUtil.decode(data, 24);
		    if(data.Length>28)
            {
			    cookie=PacketUtil.decode(data, 28);
		    }
	    }

	    public long getUdtVersion() {
		    return udtVersion;
	    }
	    public void setUdtVersion(long udtVersion) {
		    this.udtVersion = udtVersion;
	    }
	
	    public long getSocketType() {
		    return socketType;
	    }
	    public void setSocketType(long socketType) {
		    this.socketType = socketType;
	    }
	
	    public long getInitialSeqNo() {
		    return initialSeqNo;
	    }
	    public void setInitialSeqNo(long initialSeqNo) {
		    this.initialSeqNo = initialSeqNo;
	    }
	
	    public long getPacketSize() {
		    return packetSize;
	    }
	    public void setPacketSize(long packetSize) {
		    this.packetSize = packetSize;
	    }
	
	    public long getMaxFlowWndSize() {
		    return maxFlowWndSize;
	    }
	    public void setMaxFlowWndSize(long maxFlowWndSize) {
		    this.maxFlowWndSize = maxFlowWndSize;
	    }
	
	    public long getConnectionType() {
		    return connectionType;
	    }
	    public void setConnectionType(long connectionType) {
		    this.connectionType = connectionType;
	    }
	
	    public long getSocketID() {
		    return socketID;
	    }
	    public void setSocketID(long socketID) {
		    this.socketID = socketID;
	    }
	
	    public override byte[] encodeControlInformation()
        {
		    try
            {
                byte[] budtVersion = PacketUtil.encode(udtVersion);
                byte[] bsocketType = PacketUtil.encode(socketType);
                byte[] binitialSeqNo = PacketUtil.encode(initialSeqNo);
                byte[] bpacketSize = PacketUtil.encode(packetSize);
                byte[] bmaxFlowWndSize = PacketUtil.encode(maxFlowWndSize);
                byte[] bconnectionType = PacketUtil.encode(connectionType);
                byte[] bsocketID = PacketUtil.encode(socketID);
                byte[] bvalue = new byte[budtVersion.Length + bsocketType.Length + binitialSeqNo.Length + bpacketSize.Length + bmaxFlowWndSize.Length + bconnectionType.Length + bsocketID.Length];
                Array.Copy(budtVersion, 0, bvalue, 0, budtVersion.Length);
                Array.Copy(bsocketType, 0, bvalue, budtVersion.Length, bsocketType.Length);
                Array.Copy(binitialSeqNo, 0, bvalue, budtVersion.Length+bsocketType.Length, binitialSeqNo.Length);
                Array.Copy(bpacketSize, 0, bvalue, budtVersion.Length + bsocketType.Length + binitialSeqNo.Length, bpacketSize.Length);
                Array.Copy(bmaxFlowWndSize, 0, bvalue, budtVersion.Length + bsocketType.Length + binitialSeqNo.Length + bpacketSize.Length, bmaxFlowWndSize.Length);
                Array.Copy(bconnectionType, 0, bvalue, budtVersion.Length + bsocketType.Length + binitialSeqNo.Length + bpacketSize.Length + bmaxFlowWndSize.Length, bconnectionType.Length);
                Array.Copy(bsocketID, 0, bvalue, budtVersion.Length + bsocketType.Length + binitialSeqNo.Length + bpacketSize.Length + bmaxFlowWndSize.Length + bconnectionType.Length, bsocketID.Length);

                return bvalue;
		    } 
            catch (Exception e) 
            {
			    // can't happen
			    return null;
		    }		
	    }


        public override bool equals(Object obj)
        {
		    if (this == obj)
			    return true;
		    if (!base.equals(obj))
			    return false;
		    if (this.GetType() != obj.GetType())
			    return false;
		    ConnectionHandshake other = (ConnectionHandshake) obj;
		    if (connectionType != other.connectionType)
			    return false;
		    if (initialSeqNo != other.initialSeqNo)
			    return false;
		    if (maxFlowWndSize != other.maxFlowWndSize)
			    return false;
		    if (packetSize != other.packetSize)
			    return false;
		    if (socketID != other.socketID)
			    return false;
		    if (socketType != other.socketType)
			    return false;
		    if (udtVersion != other.udtVersion)
			    return false;
		    return true;
	    }
	
	
	    public String toString()
        {
		    StringBuilder sb=new StringBuilder();
		    sb.Append("ConnectionHandshake [");
            sb.Append("connectionType=").Append(connectionType);
		    UDTSession session=getSession();
		    if(session!=null){
                sb.Append(", ");
                sb.Append(session.getDestination());
		    }
            sb.Append(", mySocketID=").Append(socketID);
            sb.Append(", initialSeqNo=").Append(initialSeqNo);
            sb.Append(", packetSize=").Append(packetSize);
            sb.Append(", maxFlowWndSize=").Append(maxFlowWndSize);
            sb.Append(", socketType=").Append(socketType);
            sb.Append(", destSocketID=").Append(destinationID);
            if (cookie > 0) sb.Append(", cookie=").Append(cookie);
            sb.Append("]");
		    return sb.ToString();
	    }
   }

}
