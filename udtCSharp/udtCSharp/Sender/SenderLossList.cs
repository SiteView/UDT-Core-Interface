using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace udtCSharp.Sender
{
    /// <summary>
    /// stores the sequence number of the lost packets in increasing order
    /// </summary>
    public class SenderLossList 
    {
	    private LinkedList<long> backingList;

        /// <summary>
        /// create a new sender lost list
        /// </summary>
	    public SenderLossList()
        {
		    backingList = new LinkedList<long>();
	    }

	    public void insert(long obj)
        {
		    lock(backingList) 
            {
			    if(!backingList.Contains(obj))
                {
				    foreach(var value in backingList)
                    {
                        if (obj < value)
                        {
                            LinkedListNode<long> current = backingList.Find(value);
                            backingList.AddBefore(current, obj);
						    return;
					    }
				    }
				    backingList.AddLast(obj);
			    }
		    }
	    }

        /// <summary>
        /// retrieves the loss list entry with the lowest sequence number
        /// </summary>
        /// <returns></returns>
	    public long getFirstEntry()
        {
		    lock(backingList)
            {
                LinkedListNode<long> current = backingList.First;
                long value = current.Value;
                backingList.RemoveFirst();
                return value;
		    }
	    }

	    public bool isEmpty()
        {
            if (backingList.Count > 0)
            {
                return false;
            }
            else
            {
                return true;
            }
	    }

	    public int size()
        {
		    return backingList.Count;
	    }

	    public String toString()
        {
            string svalue = "";
            lock (backingList)
            {
                foreach (long e in backingList)
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
}
