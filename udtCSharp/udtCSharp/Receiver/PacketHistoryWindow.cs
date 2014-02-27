using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using udtCSharp.Common;

namespace udtCSharp.Receiver
{
    /// <summary>
    ///  A circular array that records the packet arrival times 
    /// </summary>
    public class PacketHistoryWindow : CircularArray<long>
    {

	    /**
	     * create a new PacketHistoryWindow of the given size 
	     * @param size
	     */
	    public PacketHistoryWindow(int size):base(size)
        {
            //super(size);
	    }

        /// <summary>
        /// compute the packet arrival speed (see specification section 6.2, page 12)
        /// </summary>
        /// <returns></returns>
	    public long getPacketArrivalSpeed()
        {
		    if(!haveOverflow)return 0;
		    int num=max-1;
		    double AI;
		    double medianPacketArrivalSpeed;
		    double total=0;
		    int count=0;
		    long[]intervals=new long[num];
		    int pos=position-1;
		    if(pos<0)pos=num;
		    do{
			    long upper=circularArray[pos];
			    pos--;
			    if(pos<0)pos=num;
			    long lower=circularArray[pos];
			    long interval=upper-lower;
			    intervals[count]=interval;
			    total+=interval;
			    count++;
		    }while(count<num);
		    //compute median
		    AI=total / num;
		    //compute the actual value, filtering out intervals between AI/8 and AI*8
		    count=0;
		    total=0;
		    foreach(long l in intervals)
            {
			    if(l>AI/8 && l<AI*8)
                {
				    total+=l;
				    count++;
			    }
		    }
		    if(count>8){
			    medianPacketArrivalSpeed=1e6*count/total;
		    }
		    else{
			    medianPacketArrivalSpeed=0; 
		    }
		    return (long)Math.Ceiling(medianPacketArrivalSpeed);
	    }

    }

}
