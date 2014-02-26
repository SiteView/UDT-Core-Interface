using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;

namespace udtCSharp.UDT
{
    /// <summary>
    /// UDTOutputStream provides a UDT version of {@link OutputStream}
    /// </summary>
    public class UDTOutputStream : Stream
    {
        private UDTSocket socket;

        private volatile bool closed;

        public UDTOutputStream(UDTSocket socket)
        {
            this.socket = socket;
        }
       
        public override void Write(byte[] b, int off, int len)
        {
            checkClosed();
            socket.doWrite(b, off, len);
        }

        public override void Flush()
        {
            try
            {
                checkClosed();
                socket.flush();
            }
            catch (Exception ie)
            {
                Log.Write(this.ToString(), ie);
            }
        }

        /**
         * This method signals the UDT sender that it can pause the 
         * sending thread. The UDT sender will resume when the next 
         * write() call is executed.<br/>
         * For example, one can use this method on the receiving end 
         * of a file transfer, to save some CPU time which would otherwise
         * be consumed by the sender thread.
         */
        public void pauseOutput()
        {
            socket.getSender().pause();
        }

        /// <summary>
        /// close this output stream
        /// </summary>
        public override void Close()
        {
            if (closed) return;
            closed = true;
        }

        private void checkClosed()
        {
            if (closed)
                Log.Write(this.ToString(), "Stream has been closed");
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

        public override int Read(byte[] buffer, int offset, int count)
        {
            throw new NotImplementedException();
        }

        public override long Seek(long offset, SeekOrigin origin)
        {
            throw new NotImplementedException();
        }

        public override void SetLength(long value)
        {
            throw new NotImplementedException();
        }

       
    }

}
