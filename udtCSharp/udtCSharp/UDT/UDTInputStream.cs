using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;
using udtCSharp.Common;


namespace udtCSharp.UDT
{
    public class UDTInputStream : Stream
    {
        //the socket owning this inputstream
        private UDTSocket socket;

        private ReceiveBuffer receiveBuffer;

        //set to 'false' by the receiver when it gets a shutdown signal from the peer
        //see the noMoreData() method
        private bool expectMoreData = true;

        private volatile bool closed = false;

        private volatile bool blocking = true;

        /// <summary>
        /// create a new {@link UDTInputStream} connected to the given socket
        /// </summary>
        /// <param name="socket"></param>
        public UDTInputStream(UDTSocket socket)
        {
            try
            {
                this.socket = socket;
                int capacity = socket != null ? 2 * socket.getSession().getFlowWindowSize() : 128;
                long initialSequenceNum = socket != null ? socket.getSession().getInitialSequenceNumber() : 1;
                receiveBuffer = new ReceiveBuffer(capacity, initialSequenceNum);
            }
            catch (Exception ex)
            {
                Log.Write(this.ToString(), ex);
            }
        }

        //private byte[] single = new byte[1];
        //public override int read()
        //{
        //    int b = 0;
        //    while (b == 0)
        //        b = read(single);
        //    if (b > 0)
        //    {
        //        return single[0];
        //    }
        //    else
        //    {
        //        return b;
        //    }
        //}

        private AppData currentChunk = null;
        //offset into currentChunk
        int offset = 0;

      
        public override int Read(byte[] target, int offset, int count)
        {
            try
            {
                int read = 0;
                updateCurrentChunk(false);
                while (currentChunk != null)
                {
                    byte[] data = currentChunk.data;
                    int length = Math.Min(target.Length - read, data.Length - offset);
                    Array.Copy(data, offset, target, read, length);
                    read += length;
                    offset += length;
                    //check if chunk has been fully read
                    if (offset >= data.Length)
                    {
                        currentChunk = null;
                        offset = 0;
                    }

                    //if no more space left in target, exit now
                    if (read == target.Length)
                    {
                        return read;
                    }

                    updateCurrentChunk(blocking && read == 0);
                }

                if (read > 0) return read;
                if (closed) return -1;
                if (expectMoreData || !receiveBuffer.isEmpty()) return 0;
                //no more data
                return -1;
            }
            catch (Exception ex)
            {
                return -1;
                Log.Write(this.ToString(), ex);
            }
        }

        /// <summary>
        /// Reads the next valid chunk of application data from the queue<br/>
        /// In blocking mode,this method will block until data is available or the socket is closed, 
        ///  otherwise it will wait for at most 10 milliseconds.
        /// </summary>
        /// <param name="block"></param>
        private void updateCurrentChunk(bool block)
        {
            if (currentChunk != null) return;

            while (true)
            {
                try
                {
                    if (block)
                    {
                        currentChunk = receiveBuffer.poll(1);//秒
                        while (!closed && currentChunk == null)
                        {
                            currentChunk = receiveBuffer.poll(1000);//秒
                        }
                    }
                    else
                    {
                        currentChunk = receiveBuffer.poll(10);
                    }

                }
                catch (Exception ie)
                {
                    Log.Write(this.ToString(), ie);
                }
                return;
            }
        }

        /// <summary>
        /// new application data
        /// </summary>
        /// <param name="sequenceNumber"></param>
        /// <param name="data"></param>
        /// <returns></returns>
        protected bool haveNewData(long sequenceNumber, byte[] data)
        {
            return receiveBuffer.offer(new AppData(sequenceNumber, data));
        }


        public override void Close()
        {
            try
            {
                if (closed) return;
                closed = true;
                noMoreData();
            }
            catch (Exception ie)
            {
                Log.Write(this.ToString(), ie);
            }
        }

        public UDTSocket getSocket()
        {
            return socket;
        }

        /// <summary>
        /// sets the blocking mode
        /// </summary>
        /// <param name="block"></param>
        public void setBlocking(bool block)
        {
            this.blocking = block;
        }

        public int getReceiveBufferSize()
        {
            return receiveBuffer.getSize();
        }	

        /// <summary>
        /// notify the input stream that there is no more data
        /// </summary>
        protected void noMoreData()
        {
            expectMoreData = false;
        }
   

        public override bool CanRead
        {
            get { throw new NotImplementedException(); }
        }

        public override bool CanSeek
        {
            get { throw new NotImplementedException(); }
        }

        public override bool CanWrite
        {
            get { throw new NotImplementedException(); }
        }

        public override void Flush()
        {
            throw new NotImplementedException();
        }

        public override long Length
        {
            get { throw new NotImplementedException(); }
        }

        public override long Position
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }     

        public override long Seek(long offset, SeekOrigin origin)
        {
            throw new NotImplementedException();
        }

        public override void SetLength(long value)
        {
            throw new NotImplementedException();
        }

        public override void Write(byte[] buffer, int offset, int count)
        {
            throw new NotImplementedException();
        }
    }
    /// <summary>
    /// used for storing application data and the associated
    /// sequence number in the queue in ascending order
    /// </summary>
    public class AppData
    {
        public long sequenceNumber;
        public byte[] data;
        public AppData(long sequenceNumber, byte[] data)
        {
            this.sequenceNumber = sequenceNumber;
            this.data = data;
        }

        public int compareTo(AppData o)
        {
            return (int)(sequenceNumber - o.sequenceNumber);
        }

        public String toString()
        {
            return sequenceNumber + "[" + data.Length + "]";
        }

        public long getSequenceNumber()
        {
            return sequenceNumber;
        }

        //@Override
        public int hashCode()
        {
            int prime = 31;
            int result = 1;
            result = prime * result
            + (int)(sequenceNumber ^ (sequenceNumber >> 32));
            return result;
        }

        //@Override
        public bool equals(Object obj)
        {
            if (this == obj)
                return true;
            if (obj == null)
                return false;
            if (this.GetType() != obj.GetType())
                return false;
            AppData other = (AppData)obj;
            if (sequenceNumber != other.sequenceNumber)
                return false;
            return true;
        }
    }
}
