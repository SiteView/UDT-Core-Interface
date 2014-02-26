using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace udtCSharp.Common
{
    /// <summary>
    ///  Circular array: the most recent value overwrites the oldest one if there is no more free space in the array
    /// </summary>
    /// <typeparam name="T"></typeparam>
    public class CircularArray<T>
    {
	    protected int position=0;
	
	    protected bool haveOverflow=false;
	
	    protected int max;
	
	    protected List<T> circularArray;
	
        /// <summary>
        /// Create a new circularArray of the given size
        /// </summary>
        /// <param name="size"></param>
	    public CircularArray(int size)
        {
		    max=size;
		    circularArray = new List<T>(size);	
	    }
	
        /// <summary>
        /// add an entry
        /// </summary>
        /// <param name="entry"></param>
	    public void add(T entry)
        {
		    if(position>=max)
            {
			    position=0;
			    haveOverflow=true;
		    }
		    if(circularArray.Count>position)
            {
			    circularArray.RemoveAt(position);
		    }
		    circularArray.Insert(position, entry);
		    position++;
	    }
	
        /// <summary>
        /// Returns the number of elements in this list 
        /// </summary>
        /// <returns></returns>
	    public int size()
        {
		    return circularArray.Count;
	    }
	
	    public String toString()
        {
		    return circularArray.ToString();
	    }

    }
}
