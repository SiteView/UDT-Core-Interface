using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Net;
using System.Net.Sockets;

namespace udtCSharp.packets
{
    public class Destination
    {
        private  int port;

	    private IPAddress address;
	
	    //UDT socket ID of the peer
	    private long socketID;
	
	    public Destination(IPAddress address, int port){
		    this.address=address;
		    this.port=port;
	    }
	
	    public IPAddress getAddress(){
		    return address;
	    }
	
	    public int getPort(){
		    return port;
	    }
	
	    public long getSocketID() {
		    return socketID;
	    }

	    public void setSocketID(long socketID) {
		    this.socketID = socketID;
	    }

	    public String toString()
        {
		    return("Destination ["+address.ToString()+" port="+port+" socketID="+socketID)+"]";
	    } 

	   
	    public int hashCode() 
        {
		    int prime = 31;
		    int result = 1;
		    result = prime * result + ((address == null) ? 0 : address.GetHashCode());
		    result = prime * result + port;
            // C#是用<<(左移) 和 >>（右移） 运算符是用来执行移位运算。
            // ^ 是“异或”运算符，运算法则如下：
            //1^1=0
            //1^0=1
            //0^1=1
            //0^0=0
		    result = prime * result + (int) (socketID ^ (socketID >> 32));
		    return result;
	    }

	    
	    public bool equals(Object obj) 
        {
		    if (this == obj)
			    return true;
		    if (obj == null)
			    return false;
            if (this.GetType() != obj.GetType())
			    return false;

		    Destination other = (Destination) obj;
		    if (address == null) 
            {
			    if (other.address != null)
				    return false;
		    } 
            else if (!address.Equals(other.address))
			    return false;
		    if (port != other.port)
			    return false;
		    if (socketID != other.socketID)
			    return false;
		    return true;
	    }
    }
}
