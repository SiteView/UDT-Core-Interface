using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using udtCSharp.Common;

namespace udtCSharp.Receiver
{
    /// <summary>
    /// a circular array that records time intervals between two probing data packets.
    /// It is used to determine the estimated link capacity.
    /// </summary>
    public class PacketPairWindow : CircularArray<long>
    {
	
	    /**
	     * construct a new packet pair window with the given size
	     * 
	     * @param size
	     */
        public PacketPairWindow(int size)
            : base(size)
        {
            //super(size);
	    }	
      
        /// <summary>
        /// compute the median packet pair interval of the last
        /// 16 packet pair intervals (PI).
        /// (see specification section 6.2, page 12) 
        /// </summary>
        /// <returns>@return time interval in microseconds</returns>
	    public double computeMedianTimeInterval()
        {
		    int num=haveOverflow?max:Math.Min(max, position);
		    double median=0;
		    for(int i=0; i<num;i++){
			    median+=(double)circularArray[i];	
		    }
		    median=median/num;
		
		    //median filtering
		    double upper=median*8;
		    double lower=median/8;
		    double total = 0;
		    double val=0;
		    int count=0;
		    for(int i=0; i<num;i++)
            {
                val = (double)circularArray[i];
			    if(val<upper && val>lower){
				    total+=val;
				    count++;
			    }
		    }
		    double res=total/count;
		    return res;
	    }	
       
        /// <summary>
        /// compute the estimated linK capacity using the values in packet pair window
        /// </summary>
        /// <returns>@return number of packets per second</returns>
	    public long getEstimatedLinkCapacity()
        {
		    long res=(long)Math.Ceiling(1000000/computeMedianTimeInterval());
		    return res;
	    }
    }
}
