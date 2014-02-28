using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Threading;

namespace udtCSharp.Common
{
    public class AtomicInteger
    {
        private int value;

        public AtomicInteger(int initialValue)
        {
            value = initialValue;
        }

        public AtomicInteger()
            : this(0)
        {
        }
        /// <summary>
        /// 获取当前值。
        /// </summary>
        /// <returns>当前值</returns>
        public int Get()
        {
            return value;
        }
        /// <summary>
        /// 设置为给定值。
        /// </summary>
        /// <param name="newValue">新值</param>
        public void Set(int newValue)
        {
            value = newValue;
        }
        /// <summary>
        /// 以原子方式设置为给定值，并返回旧值。
        /// </summary>
        /// <param name="newValue">新值</param>
        /// <returns>以前的值</returns>
        public int GetAndSet(int newValue)
        {
            for (; ; )
            {
                int current = Get();
                if (CompareAndSet(current, newValue))
                    return current;
            }
        }
        /// <summary>
        /// 如果当前值 == 预期值，则以原子方式将该值设置为给定的更新值。
        /// </summary>
        /// <param name="expect">预期值</param>
        /// <param name="update">新值</param>
        /// <returns>如果成功，则返回 true。返回 False 指示实际值与预期值不相等。</returns>
        public bool CompareAndSet(int expect, int update)
        {
            return Interlocked.CompareExchange(ref value, update, expect) == expect;
        }
        /// <summary>
        /// 以原子方式将当前值加 1。
        /// </summary>
        /// <returns>以前的值</returns>
        public int GetAndIncrement()
        {
            for (; ; )
            {
                int current = Get();
                int next = current + 1;
                if (CompareAndSet(current, next))
                    return current;
            }
        }
        /// <summary>
        /// 以原子方式将当前值减 1。
        /// </summary>
        /// <returns>以前的值</returns>
        public int GetAndDecrement()
        {
            for (; ; )
            {
                int current = Get();
                int next = current - 1;
                if (CompareAndSet(current, next))
                    return current;
            }
        }
        /// <summary>
        /// 以原子方式将给定值与当前值相加。
        /// </summary>
        /// <param name="delta">要加上的值</param>
        /// <returns>以前的值</returns>
        public int GetAndAdd(int delta)
        {
            for (; ; )
            {
                int current = Get();
                int next = current + delta;
                if (CompareAndSet(current, next))
                    return current;
            }
        }
        /// <summary>
        /// 以原子方式将当前值加 1。
        /// </summary>
        /// <returns>更新的值</returns>
        public int IncrementAndGet()
        {
            for (; ; )
            {
                int current = Get();
                int next = current + 1;
                if (CompareAndSet(current, next))
                    return next;
            }
        }
        /// <summary>
        /// 以原子方式将当前值减 1。
        /// </summary>
        /// <returns>更新的值</returns>
        public int DecrementAndGet()
        {
            for (; ; )
            {
                int current = Get();
                int next = current - 1;
                if (CompareAndSet(current, next))
                    return next;
            }
        }
        /// <summary>
        /// 以原子方式将给定值与当前值相加。
        /// </summary>
        /// <param name="delta">要加上的值</param>
        /// <returns>更新的值</returns>
        public int AddAndGet(int delta)
        {
            for (; ; )
            {
                int current = Get();
                int next = current + delta;
                if (CompareAndSet(current, next))
                    return next;
            }
        }
        /// <summary>
        /// 返回当前值的字符串表示形式。
        /// </summary>
        /// <returns>当前值的字符串表示形式。</returns>
        public override String ToString()
        {
            return Convert.ToString(Get());
        }
    }
}
