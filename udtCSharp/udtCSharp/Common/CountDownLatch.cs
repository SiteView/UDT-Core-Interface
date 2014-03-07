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
        private object lockobj = new object();
        private int counts;

        public CountDownLatch(int counts)
        {
            this.counts = counts;
        }
        /// <summary>
        /// 线程进入就绪队列之前等待。
        /// </summary>
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
        /// <summary>
        /// 线程进入就绪队列之前等待的毫秒数。
        /// </summary>
        /// <param name="timeout"></param>
        public void Await(int timeout)
        {
            lock (lockobj)
            {
                while (counts > 0)
                {
                    Monitor.Wait(lockobj,timeout);
                }
            }
        }

        /// <summary>
        /// 通知所有的等待线程对象状态的更改。
        /// </summary>
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
