using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using udtCSharp.UDT;

namespace udtCSharp.packets
{
    public class DataPacket : UDTPacket
    {
	    private byte[] data ;
	    private long packetSequenceNumber;
	    private long messageNumber;
	    private long timeStamp;
	    private long destinationID;

	    private UDTSession session;
	
	    public DataPacket()
        {
	    }

	    /**
	     * create a DataPacket
	     * @param encodedData - network data
	     */
	    public DataPacket(byte[] encodedData)
        {
            decode(encodedData, encodedData.Length);
	    }

	    public DataPacket(byte[] encodedData, int length)
        {
		    decode(encodedData,length);
	    }
	
	    private void decode(byte[]encodedData,int length)
        {
		    packetSequenceNumber=PacketUtil.decode(encodedData, 0);
		    messageNumber=PacketUtil.decode(encodedData, 4);
		    timeStamp=PacketUtil.decode(encodedData, 8);
		    destinationID=PacketUtil.decode(encodedData, 12);
		    data=new byte[length-16];
            Array.Copy(encodedData, 16, data, 0, data.Length);
	    }


	    public byte[] getData() {
		    return this.data;
	    }

	    public double getLength(){
		    return data.Length;
	    }

	    /*
	     * aplivation data
	     * @param
	     */

	    public void setData(byte[] data) {
		    this.data = data;
	    }

	    public long getPacketSequenceNumber() {
		    return this.packetSequenceNumber;
	    }

	    public void setPacketSequenceNumber(long sequenceNumber) {
		    this.packetSequenceNumber = sequenceNumber;
	    }


	    public long getMessageNumber() {
		    return this.messageNumber;
	    }

	    public void setMessageNumber(long messageNumber) {
		    this.messageNumber = messageNumber;
	    }

	    public long getDestinationID() {
		    return this.destinationID;
	    }

	    public long getTimeStamp() {
		    return this.timeStamp;
	    }

	    public void setDestinationID(long destinationID) {
		    this.destinationID=destinationID;
	    }

	    public void setTimeStamp(long timeStamp) {
		    this.timeStamp=timeStamp;
	    }

	    /**
	     * complete header+data packet for transmission
	     */
	    public byte[] getEncoded(){
		    //header.length is 16
		    byte[] result=new byte[16+data.Length];
		    Array.Copy(PacketUtil.encode(packetSequenceNumber), 0, result, 0, 4);
            Array.Copy(PacketUtil.encode(messageNumber), 0, result, 4, 4);
            Array.Copy(PacketUtil.encode(timeStamp), 0, result, 8, 4);
            Array.Copy(PacketUtil.encode(destinationID), 0, result, 12, 4);
            Array.Copy(data, 0, result, 16, data.Length);
		    return result;
	    }

	    public bool isControlPacket(){
		    return false;
	    }

	    public bool forSender(){
		    return false;
	    }

	    public bool isConnectionHandshake(){
		    return false;
	    }
	
	    public int getControlPacketType(){
		    return -1;
	    }
	
	    public UDTSession getSession() {
		    return session;
	    }

	    public void setSession(UDTSession session) {
		    this.session = session;
	    }

	    public int compareTo(UDTPacket other){
		    return (int)(getPacketSequenceNumber()-other.getPacketSequenceNumber());
	    }
    }
}
