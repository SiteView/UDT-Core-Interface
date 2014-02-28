using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace udtCSharp.packets
{
    public class MessageDropRequest : ControlPacket
    {
	    //Bits 35-64: Message number
	
	    private long msgFirstSeqNo;
	    private long msgLastSeqNo;
	
	    public MessageDropRequest()
        {
		    this.controlPacketType=(int)ControlPacketType.MESSAGE_DROP_REQUEST;
	    }
	
	    public MessageDropRequest(byte[]controlInformation):base()
        {
		    //this.controlInformation=controlInformation;
		    decode(controlInformation );
	    }
	
	    void decode(byte[]data)
        {
		    msgFirstSeqNo =PacketUtil.decode(data, 0);
		    msgLastSeqNo =PacketUtil.decode(data, 4);
	    }

	    public long getMsgFirstSeqNo() 
        {
		    return msgFirstSeqNo;
	    }

	    public void setMsgFirstSeqNo(long msgFirstSeqNo) 
        {
		    this.msgFirstSeqNo = msgFirstSeqNo;
	    }

	    public long getMsgLastSeqNo() 
        {
		    return msgLastSeqNo;
	    }

	    public void setMsgLastSeqNo(long msgLastSeqNo)
        {
		    this.msgLastSeqNo = msgLastSeqNo;
	    }


	    public override byte[] encodeControlInformation()
        {
		    try 
            {
                byte[] bmsgFirstSeqNo = PacketUtil.encode(msgFirstSeqNo);
                byte[] bmsgLastSeqNo = PacketUtil.encode(msgLastSeqNo);
                byte[] bos = new byte[bmsgFirstSeqNo.Length + bmsgLastSeqNo.Length];
                Array.Copy(bmsgFirstSeqNo, 0, bos, 0, bmsgFirstSeqNo.Length);
                Array.Copy(bmsgLastSeqNo, 0, bos, bmsgFirstSeqNo.Length, bmsgLastSeqNo.Length);
			   
			    return bos;
		    } 
            catch (Exception e) 
            {
			    // can't happen
                Log.Write(this.ToString(), "can't happen", e);
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
		    MessageDropRequest other = (MessageDropRequest) obj;
		    if (msgFirstSeqNo != other.msgFirstSeqNo)
			    return false;
		    if (msgLastSeqNo != other.msgLastSeqNo)
			    return false;
		    return true;
	    }

    }
}
