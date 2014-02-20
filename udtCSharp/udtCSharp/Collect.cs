using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;

namespace udtCSharp
{
    public abstract class Collect : ICollect
    {
        ~Collect()
        {
            this.Dispose();
        }

        private long dispoing = 0;

        public bool IsDisposed
        {
            get
            {
                return Interlocked.Read(ref dispoing) == 0 ? false : true;
            }
        }

        protected abstract void SingleDispose();

        public void Dispose()
        {
            if (Interlocked.Exchange(ref dispoing, 1) == 0)
            {
                try
                {
                    this.SingleDispose();
                }
                catch (Exception err)
                {
                    string errmsg = string.Format("{0} Dispose Error!\r\nErorMessage:{1}", this.GetType(), err);
                    Log.Write(errmsg);
                }
            }
            GC.SuppressFinalize(this);
        }
    }
}
