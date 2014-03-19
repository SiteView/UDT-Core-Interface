using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Threading;
using udtCSharp.UDT;

namespace udtCSharp.Common
{
    /// <summary>
    /// The receive buffer stores data chunks to be read by the application
    /// </summary>
    public class ReceiveBuffer
    {
        private AppData[] buffer;

	    //the head of the buffer: contains the next chunk to be read by the application, 
	    //i.e. the one with the lowest sequence number
	    private volatile int readPosition=0;

	    //the lowest sequence number stored in this buffer
	    private long initialSequenceNumber;

	    //the highest sequence number already read by the application
	    private long highestReadSequenceNumber;

	    //number of chunks
        private AtomicInteger numValidChunks = new AtomicInteger(0);
       
	    //lock and condition for poll() with timeout	   
        
        /// <summary>
        /// 多线程操作同步
        /// </summary>
        private static Mutex mutex = new Mutex();
        private static Mutex mutex2 = new Mutex();

	    //the size of the buffer
	    private int size;

	    public ReceiveBuffer(int size, long initialSequenceNumber)
        {            
		    this.size=size;
		    this.buffer=new AppData[size];
		    this.initialSequenceNumber=initialSequenceNumber;         
		    highestReadSequenceNumber=SequenceNumber.decrement(initialSequenceNumber);
		    //Log.Write(this.ToString(),"SIZE: "+size);
	    }

	    public bool offer(AppData data)
        {
            if (numValidChunks.Get() == size)
            {
			    return false;
		    }
            mutex.WaitOne();
            try
            {
                
                long seq = data.getSequenceNumber();
                //if already have this chunk, discard it
                if (SequenceNumber.compare(seq, initialSequenceNumber) < 0) return true;
                //else compute insert position
                int offset = (int)SequenceNumber.seqOffset(initialSequenceNumber, seq);
                int insert = offset % size;
                buffer[insert] = data;
                numValidChunks.IncrementAndGet();
                if (insert == 498)
                {
                    int i = 630208;
                }
                return true;
            }
            catch
            {
                return false;
            }
            finally
            {
                mutex.ReleaseMutex();
		    }
	    }
       
        /// <summary>
        /// return a data chunk, guaranteed to be in-order, waiting up to the
        /// specified wait time if necessary for a chunk to become available.
        /// * @param timeout how long to wait before giving up, in units of
        /// *        <tt>unit</tt>
        /// * @param unit a <tt>TimeUnit</tt> determining how to interpret the
        /// *        <tt>timeout</tt> parameter
        /// * @return data chunk, or <tt>null</tt> if the
        /// *         specified waiting time elapses before an element is available
        /// * @throws InterruptedException if interrupted while waiting
        /// </summary>
        /// <param name="timeout"></param>
        /// <param name="units"></param>
        /// <returns></returns>
	    public AppData poll(int timeout)
        {		    
		    long nanos = timeout*1000;//毫秒
            mutex2.WaitOne((int)nanos);
		    try 
            {
			    for (;;) 
                {
                    if (numValidChunks.Get() != 0) 
                    {
					    return poll();
				    }
				    if (nanos <= 0)
                    {
					    return null;
                    }
                    nanos--;
                    Thread.Sleep(5);
			    }
		    }
            finally 
            {
                mutex2.ReleaseMutex();
		    }
	    }
      
        /// <summary>
        /// return a data chunk, guaranteed to be in-order. 
        /// </summary>
        /// <returns></returns>
	    public AppData poll()
        {
            if (numValidChunks.Get() == 0)
            {
			    return null;
		    }          
		    AppData r = buffer[readPosition];
		    if(r!=null)
            {
			    long thisSeq=r.getSequenceNumber();
                if (1 == SequenceNumber.seqOffset(highestReadSequenceNumber, thisSeq))
                {
                    increment();
                    highestReadSequenceNumber = thisSeq;
                }
                else
                {
                    return null;
                }
		    }
		    return r;
	    }

	    public int getSize()
        {
		    return size;
	    }

	    void increment()
        {
		    buffer[readPosition] = null;
		    readPosition++;
		    if(readPosition==size)readPosition=0;
            numValidChunks.DecrementAndGet();
	    }

	    public bool isEmpty()
        {
            return numValidChunks.Get() == 0; ;
	    }
    }
}
