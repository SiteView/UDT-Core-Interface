using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace udtCSharp.packets
{
    public class UserDefined : ControlPacket
    {	
	    public UserDefined()
        {
		    controlPacketType=(int)ControlPacketType.USER_DEFINED;
	    }
	
	    //Explained by bits 4-15,
	    //reserved for user defined Control Packet
	    public UserDefined(byte[]controlInformation)
        {
		    this.controlInformation=controlInformation;
	    }
	   
	    public override byte[] encodeControlInformation() 
        {
		    return null;
	    }
    }
}
