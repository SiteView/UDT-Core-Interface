using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using udtCSharp.Common;

namespace udtCSharp.Receiver
{
    /// <summary>
    /// a circular array of each sent Ack and the time it is sent out
    /// </summary>
    public class AckHistoryWindow : CircularArray<AckHistoryEntry>
    {

        public AckHistoryWindow(int size)
            : base(size)
        {
            //super(size);
	    }
	
        /// <summary>
        /// return  the time for the given seq no, or <code>-1 </code> if not known
        /// </summary>
        /// <param name="ackNumber"></param>
        /// <returns></returns>
	    public long getTime(long ackNumber)
        {
		    foreach(AckHistoryEntry obj in circularArray)
            {
			    if(obj.getAckNumber()==ackNumber)
                {
				    return obj.getSentTime();
			    }
		    }
		    return -1;
	    }
	
	    public AckHistoryEntry getEntry(long ackNumber)
        {
		    foreach(AckHistoryEntry obj in circularArray)
            {
			    if(obj.getAckNumber()==ackNumber)
                {
				    return obj;
			    }
		    }
		    return null;
	    }

    }

}
