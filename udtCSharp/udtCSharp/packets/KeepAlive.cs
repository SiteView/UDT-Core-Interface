using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace udtCSharp.packets
{
    public class KeepAlive : ControlPacket
    {	
	    public KeepAlive()
        {
		    this.controlPacketType=(int)ControlPacketType.KEEP_ALIVE;	
	    }
	
	
	    public byte[] encodeControlInformation()
        {
		    return null;
	    }
    }
}
