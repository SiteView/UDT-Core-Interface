using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace udtCSharp.UDT
{
    public interface UDTPacket
    {
        long getMessageNumber();

        void setMessageNumber(long messageNumber);

        void setTimeStamp(long timeStamp);

        long getTimeStamp();

        void setDestinationID(long destinationID);

        long getDestinationID();

        bool isControlPacket();

        int getControlPacketType();

        byte[] getEncoded();

        /**
         * return <code>true</code> if this packet should be routed to
         * the {@link UDTSender} 
         * @return
         */
        bool forSender();

        bool isConnectionHandshake();

        UDTSession getSession();

        long getPacketSequenceNumber();
    }
}
