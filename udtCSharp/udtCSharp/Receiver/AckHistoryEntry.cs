using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using udtCSharp.Common;

namespace udtCSharp.Receiver
{
    /// <summary>
    /// store the Sent Acknowledge packet number
    /// and the time it is sent out
    /// </summary>
    public class AckHistoryEntry 
    {
	
	    private long ackSequenceNumber;
	    /// <summary>
	    /// the sequence number prior to which all the packets have been received
	    /// </summary>
	    private long ackNumber;
	    /// <summary>
	    /// time when the Acknowledgement entry was sent
	    /// </summary>
	    private long  sentTime;
	
	    public AckHistoryEntry(long ackSequenceNumber, long ackNumber, long sentTime)
        {
		    this.ackSequenceNumber= ackSequenceNumber;
		    this.ackNumber = ackNumber;
		    this.sentTime = sentTime;
	    }
	

	    public long getAckSequenceNumber()
        {
		    return ackSequenceNumber;
	    }

	    public long getAckNumber() {
		    return ackNumber;
	    }

	    public long getSentTime() {
		    return sentTime;
	    }
	
        /// <summary>
        /// get the age of this sent ack sequence number
        /// </summary>
        /// <returns></returns>
	    public long getAge() 
        {
		    return Util.getCurrentTime()-sentTime;
	    }
    }
}
