using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using udtCSharp.Common;

namespace udtCSharp.Receiver
{   
    /// <summary>
    /// the receiver loss list stores information about lost packets, 
    /// ordered by increasing sequence number.
    /// </summary>
    public class ReceiverLossList 
    {

	    private List<ReceiverLossListEntry> backingList;
	
	    public ReceiverLossList()
        {
		    backingList = new List<ReceiverLossListEntry>(32);
	    }
	
	    public void insert(ReceiverLossListEntry entry)
        {
		    lock (backingList)
            {
			    if(!backingList.Contains(entry))
                {
				    backingList.Add(entry);
			    }
		    }
	    }

	    public void remove(long seqNo)
        {
		    backingList.Remove(new ReceiverLossListEntry(seqNo));
	    }
	
	    public bool contains(ReceiverLossListEntry obj)
        {
		    return backingList.Contains(obj);
	    }
	
	    public bool isEmpty()
        {
            if(backingList.Count > 0)
            {
                return false;
            }
            else
            {
                return true;
            }		   
	    }	
  
        /// <summary>
        /// read (but NOT remove) the first entry in the loss list
        /// </summary>
        /// <returns></returns>
	    public ReceiverLossListEntry getFirstEntry()
        {
            if (!isEmpty())
            {
                return backingList[0];
            }
            else
            {
                return null;
            }
	    }
	
	    public int size()
        {
		    return backingList.Count();
	    }

        /// <summary>
        /// return all sequence numbers whose last feedback time is larger than k*RTT
        /// </summary>
        /// <param name="RTT">the current round trip time</param>
        /// <param name="doFeedback">true if the k parameter should be increased and the time should </param>
        ///  be reset (using {@link ReceiverLossListEntry#feedback()} )
        /// <returns></returns>
	    public List<long>getFilteredSequenceNumbers(long RTT, bool doFeedback)
        {
		    List<long> result = new List<long>();
           
            foreach (ReceiverLossListEntry e in backingList)
            {
			    if( (Util.getCurrentTime()-e.getLastFeedbackTime())>e.getK()*RTT)
                {
				    result.Add(e.getSequenceNumber());
				    if(doFeedback)e.feedback();
			    }
		    }
		    return result;
	    }
	
	    public string toString()
        {
            string svalue = "";
            foreach (ReceiverLossListEntry e in backingList)
            {
                if (svalue == "")
                {
                    svalue = "[" + e + "]";
                }
                else
                {
                    svalue = svalue + ",[" + e + "]";
                }
            }
            return svalue;
	    }	
    }
}
