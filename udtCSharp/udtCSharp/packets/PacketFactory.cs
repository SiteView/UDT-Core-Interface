using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using udtCSharp.UDT;

namespace udtCSharp.packets
{
    public class PacketFactory
    {
        /**
         * creates a Control or Data packet depending on the highest bit
         * of the first 32 bit of data
         * @param packetData
         * @return
         */
        public static UDTPacket createPacket(byte[] encodedData)
        {
            bool isControl = (encodedData[0] & 128) != 0;
            if (isControl) return createControlPacket(encodedData, encodedData.Length);
            return new DataPacket(encodedData);
        }

        public static UDTPacket createPacket(byte[] encodedData, int length)
        {
            bool isControl = (encodedData[0] & 128) != 0;
            if (isControl) return createControlPacket(encodedData, length);
            return new DataPacket(encodedData, length);
        }

        /**
         * create the right type of control packet based on the packet data 
         * @param packetData
         * @return
         */
        public static ControlPacket createControlPacket(byte[] encodedData, int length)
        {

            ControlPacket packet = null;

            int pktType = PacketUtil.decodeType(encodedData, 0);
            long additionalInfo = PacketUtil.decode(encodedData, 4);
            long timeStamp = PacketUtil.decode(encodedData, 8);
            long destID = PacketUtil.decode(encodedData, 12);
            byte[] controlInformation = new byte[length - 16];
            Array.Copy(encodedData, 16, controlInformation, 0, controlInformation.Length);

            //TYPE 0000:0
            if ((int)ControlPacket.ControlPacketType.CONNECTION_HANDSHAKE == pktType)
            {
                packet = new ConnectionHandshake(controlInformation);
            }
            //TYPE 0001:1
            else if ((int)ControlPacket.ControlPacketType.KEEP_ALIVE == pktType)
            {
                packet = new KeepAlive();
            }
            //TYPE 0010:2
            else if ((int)ControlPacket.ControlPacketType.ACK == pktType)
            {
                packet = new Acknowledgement(additionalInfo, controlInformation);
            }
            //TYPE 0011:3
            else if ((int)ControlPacket.ControlPacketType.NAK == pktType)
            {
                packet = new NegativeAcknowledgement(controlInformation);
            }
            //TYPE 0101:5
            else if ((int)ControlPacket.ControlPacketType.SHUTDOWN == pktType)
            {
                packet = new Shutdown();
            }
            //TYPE 0110:6
            else if ((int)ControlPacket.ControlPacketType.ACK2 == pktType)
            {
                packet = new Acknowledgment2(additionalInfo, controlInformation);
            }
            //TYPE 0111:7
            else if ((int)ControlPacket.ControlPacketType.MESSAGE_DROP_REQUEST == pktType)
            {
                packet = new MessageDropRequest(controlInformation);
            }
            //TYPE 1111:8
            else if ((int)ControlPacket.ControlPacketType.USER_DEFINED == pktType)
            {
                packet = new UserDefined(controlInformation);
            }

            if (packet != null)
            {
                packet.setTimeStamp(timeStamp);
                packet.setDestinationID(destID);
            }
            return packet;

        }
    }
}
