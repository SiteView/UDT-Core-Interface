using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace udtCSharp.Common
{
    /// <summary>
    /// Handle sequence numbers, taking the range of 0 - (2^31 - 1) into account
    /// </summary>
    public class SequenceNumber
    {
	    private const int maxOffset=0x3FFFFFFF;

	    private const long maxSequenceNo=0x7FFFFFFF;    

        /// <summary>
        ///  compare seq1 and seq2. Returns zero, if they are equal, a negative value if seq1 is smaller than
        ///  seq2, and a positive value if seq1 is larger than seq2.
        /// </summary>
        /// <param name="seq1"></param>
        /// <param name="seq2"></param>
        /// <returns></returns>
        public static long compare(long seq1, long seq2)
        {
    	    return (Math.Abs(seq1 - seq2) < maxOffset) ? (seq1 - seq2) : (seq2 - seq1);
        }
    
        /// <summary>
        /// length from the first to the second sequence number, including both
        /// </summary>
        /// <param name="seq1"></param>
        /// <param name="seq2"></param>
        /// <returns></returns>
        public static long length(long seq1, long seq2)
        {
            return (seq1 <= seq2) ? (seq2 - seq1 + 1) : (seq2 - seq1 + maxSequenceNo + 2);
        }
            
     
        /// <summary>
        /// compute the offset from seq2 to seq1
        /// </summary>
        /// <param name="seq1"></param>
        /// <param name="seq2"></param>
        /// <returns></returns>
	    public static long seqOffset(long seq1, long seq2)
        {
		    if (Math.Abs(seq1 - seq2) < maxOffset)
			    return seq2 - seq1;

		    if (seq1 < seq2)
			    return seq2 - seq1 - maxSequenceNo - 1;

		    return seq2 - seq1 + maxSequenceNo + 1;
	    }

      
        /// <summary>
        /// increment by one
        /// </summary>
        /// <param name="seq"></param>
        /// <returns></returns>
	    public static long increment(long seq)
        {
		    return (seq == maxSequenceNo) ? 0 : seq + 1;
	    }

       
        /// <summary>
        /// decrement by one
        /// </summary>
        /// <param name="seq"></param>
        /// <returns></returns>
	    public static long decrement(long seq)
        {
		    return (seq == 0) ? maxSequenceNo : seq - 1;
	    }	
      
        /// <summary>
        /// generates a random number between 1 and 0x3FFFFFFF (inclusive)
        /// </summary>
        /// <returns></returns>
	    public static long random()
        {
		    return new Random((int) DateTime.Now.Ticks & 0x0000FFFF).Next(1,maxOffset);
	    }
	
    }
}
