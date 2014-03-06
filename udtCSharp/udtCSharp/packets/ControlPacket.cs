using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using udtCSharp.UDT;

namespace udtCSharp.packets
{
    public abstract class ControlPacket : UDTPacket
    {
        protected int controlPacketType;

        protected long messageNumber;

        protected long timeStamp;

        protected long destinationID;

        protected byte[] controlInformation;

        private UDTSession session;

        public ControlPacket()
        {
        }

        public int getControlPacketType()
        {
            return controlPacketType;
        }

        public long getMessageNumber()
        {
            return messageNumber;
        }

        public void setMessageNumber(long messageNumber)
        {
            this.messageNumber = messageNumber;
        }

        public void setTimeStamp(long timeStamp)
        {
            this.timeStamp = timeStamp;
        }

        public long getTimeStamp()
        {
            return timeStamp;
        }

        public void setDestinationID(long destinationID)
        {
            this.destinationID = destinationID;
        }

        public long getDestinationID()
        {
            return destinationID;
        }

        public bool isControlPacket()
        {
            return true;
        }

        public virtual bool forSender()
        {
            return true;
        }

        public virtual bool isConnectionHandshake()
        {
            return true;
        }

        public UDTSession getSession()
        {
            return session;
        }

        public void setSession(UDTSession session)
        {
            this.session = session;
        }

        public long getPacketSequenceNumber()
        {
            return -1;
        }

        public int compareTo(UDTPacket other)
        {
            return (int)(getPacketSequenceNumber() - other.getPacketSequenceNumber());
        }

         /**
	     * return the header according to specification p.5
	     * @return
	     */
	    byte[] getHeader(){
		    byte[]res=new byte[16];
            Array.Copy(PacketUtil.encodeControlPacketType(controlPacketType), 0, res, 0, 4);
            Array.Copy(PacketUtil.encode(getAdditionalInfo()), 0, res, 4, 4);
            Array.Copy(PacketUtil.encode(timeStamp), 0, res, 8, 4);
            Array.Copy(PacketUtil.encode(destinationID), 0, res, 12, 4);
		    return res;
	    }

	    /**
	     * this method gets the "additional info" for this type of control packet
	     */
	    protected virtual long getAdditionalInfo()
        {
		    return 0L;
	    }
	
	    /**
	     * this method builds the control information
	     * from the control parameters
	     * @return
	     */
        public abstract byte[] encodeControlInformation(); 

	    /**
	     * complete header+ControlInformation packet for transmission
	     */	
	    public byte[] getEncoded()
        {
		    byte[] header=getHeader();
		    byte[] controlInfo=encodeControlInformation();
		    byte[] result=controlInfo!=null?
				    new byte[header.Length + controlInfo.Length]:
				    new byte[header.Length]; 
		    Array.Copy(header, 0, result, 0, header.Length);           
		    if(controlInfo!=null)
            {
                Array.Copy(controlInfo, 0, result, header.Length, controlInfo.Length);
		    }
		    return result;
	    }

	    public virtual bool equals(Object obj) 
        {
		    if (this == obj)
			    return true;
		    if (obj == null)
			    return false;
		    if (this.GetType() != obj.GetType())
			    return false;
		    ControlPacket other = (ControlPacket) obj;
		    if (controlPacketType != other.controlPacketType)
			    return false;
		    if (destinationID != other.destinationID)
			    return false;
		    if (timeStamp != other.timeStamp)
			    return false;
		    return true;
	    }

        public enum ControlPacketType
        {
            CONNECTION_HANDSHAKE,
            KEEP_ALIVE,
            ACK,
            NAK,
            UNUNSED_1,
            SHUTDOWN,
            ACK2,
            MESSAGE_DROP_REQUEST,
            UNUNSED_2,
            UNUNSED_3,
            UNUNSED_4,
            UNUNSED_5,
            UNUNSED_6,
            UNUNSED_7,
            UNUNSED_8,
            USER_DEFINED

        }
    }
}
