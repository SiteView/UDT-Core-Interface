using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Threading;

namespace udtCSharp.Common
{
    //public class AtomicReference<T> where T : class, new()
    public class AtomicReference<T> where T : class
    {
        private T value;

        public AtomicReference()
        {
           ;
        }
       
        public AtomicReference(T initialValue)
        {
            value = initialValue;
        }

        /// <summary>
        /// 获取当前值。
        /// </summary>
        /// <returns>当前值</returns>
        public T Get()
        {
            return value;
        }

        /// <summary>
        /// 设置为给定值。
        /// </summary>
        /// <param name="newValue">新值</param>
        public void Set(T newValue)
        {
            value = newValue;
        }

        /// <summary>
        /// 以原子方式设置为给定值，并返回旧值。
        /// </summary>
        /// <param name="newValue">新值</param>
        /// <returns>以前的值</returns>
        public T GetAndSet(T newValue)
        {
            for (; ; )
            {
                T current = Get();
                if (CompareAndSet(current, newValue))
                    return current;
            }
        }

        /// <summary>
        /// 如果当前值 == 预期值，则以原子方式将该值设置为给定的更新值。
        /// </summary>
        /// <param name="expect">预期值</param>
        /// <param name="update">新值</param>
        /// <returns>如果成功，则返回 true。返回 false 指示实际值与预期值不相等。</returns>
        public bool CompareAndSet(T expect, T update)
        {
            //return Interlocked.CompareExchange<T>(ref value, update, expect)== expect;
            return Interlocked.CompareExchange(ref value, update, expect) == expect;
        }
        /// <summary>
        /// 最终设置为给定值。
        /// </summary>
        /// <param name="newValue">新值</param>
        public void lazySet(T newValue)
        {
            T current = Get();
            Interlocked.Exchange(ref value, newValue);
        }
        /// <summary>
        /// 返回当前值的字符串表示形式。
        /// </summary>
        /// <returns>当前值的字符串表示形式。</returns>
        public String toString()
        {
            return value.ToString();
        }
    }
}
