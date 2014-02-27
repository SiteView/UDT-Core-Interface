using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Threading;

namespace udtCSharp.Common
{
    public class CountDownLatch
    {
        private object lockobj;
        private int counts;

        public CountDownLatch(int counts)
        {
            this.counts = counts;
        }

        public void Await()
        {
            lock (lockobj)
            {
                while (counts > 0)
                {
                    Monitor.Wait(lockobj);
                }
            }
        }


        public void CountDown()
        {
            lock (lockobj)
            {
                counts--;
                Monitor.PulseAll(lockobj);
            }
        }
    }
}
