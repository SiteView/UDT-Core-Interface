using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace udtCSharp.packets
{
       /**
     * Ack2 is sent by the {@link UDTSender} as immediate reply to an {@link Acknowledgement}
     */
    public class Acknowledgment2 : ControlPacket
    {
	    //the ack sequence number
	    private long ackSequenceNumber ;

	    public Acknowledgment2()
        {
		    this.controlPacketType=(int)ControlPacketType.ACK2;	
	    }

	    public Acknowledgment2(long ackSeqNo,byte[]controlInformation):base()
        {
		    this.ackSequenceNumber=ackSeqNo;
		    decode(controlInformation );
	    }

	    public long getAckSequenceNumber() 
        {
		    return ackSequenceNumber;
	    }

	    public void setAckSequenceNumber(long ackSequenceNumber) 
        {
		   this.ackSequenceNumber = ackSequenceNumber;
	    }
	 
	    void decode(byte[]data)
        {
	    }

	    public override bool forSender()
        {
		    return false;
	    }

	    private byte[]empty= new byte[0];

	    public override byte[] encodeControlInformation()
        {
		    return empty;
	    }
    }
}
