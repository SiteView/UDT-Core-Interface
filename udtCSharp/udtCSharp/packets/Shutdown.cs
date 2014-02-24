using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace udtCSharp.packets
{
    public class Shutdown : ControlPacket
    {

        public Shutdown()
        {
            this.controlPacketType = (int)ControlPacketType.SHUTDOWN;
        }


        public override byte[] encodeControlInformation()
        {
            return null;
        }


        public override bool forSender()
        {
            return false;
        }
    }
}
