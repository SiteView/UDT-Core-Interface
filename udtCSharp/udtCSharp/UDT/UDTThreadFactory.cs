using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Threading;
using System.Runtime.CompilerServices;
using udtCSharp.Common;

namespace udtCSharp.UDT
{
    public class UDTThreadFactory
    {

        private static AtomicInteger num = new AtomicInteger(0);

        private static UDTThreadFactory theInstance = null;

        /// <summary>
        /// [MethodImpl(MethodImplOptions.Synchronized)]表示定义为同步方法
        /// [MethodImpl(MethodImplOptions.Synchronized)] = loc(this)
        /// </summary>
        /// <returns></returns>
        [MethodImpl(MethodImplOptions.Synchronized)]
        public static UDTThreadFactory get()
        {
            if (theInstance == null)
            {
                theInstance = new UDTThreadFactory();
            }
            return theInstance;
        }

        public Thread newThread(ThreadStart r)
        {
            Thread t = new Thread(r);
            t.Name = "UDT-Thread-" + num.IncrementAndGet();
            t.IsBackground = true;
            return t;
        }

    }
}
