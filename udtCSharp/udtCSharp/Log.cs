using System;
using System.Text;
using System.IO;
using System.Threading;

namespace udtCSharp
{
    public static class Log
    {
        private static StreamWriter m_log;
        //多线程操作日志,同步
        private static Mutex mutex = new Mutex();

        public static void Write(Exception ee)
        {
            string msg = "Source:" + ee.Source + "\r\n" +
                "Message:" + ee.Message + "\r\n" +
                "StackTrace:" + ee.StackTrace;

            Write(msg);
        }

        public static void Write(string message1, string message2)
        {
            string msg = message1 + "\r\n" + message2;

            Write(msg);
        }

        public static void Write(string message1, string message2, Exception ee)
        {
            string msg = message1 + "\r\n" + message2 + "Source:" + ee.Source + "\r\n" +
                "Message:" + ee.Message + "\r\n" + "StackTrace:" + ee.StackTrace; ;

            Write(msg);
        }

        public static void Write(string info, Exception ee)
        {
            string msg = info + "\r\n" + "Source:" + ee.Source + "\r\n" +
                "Message:" + ee.Message + "\r\n" +  "StackTrace:" + ee.StackTrace;

            Write(msg);
        }

        public static void Write(string msg)
        {
            mutex.WaitOne();
            try
            {
                if (!Directory.Exists(AppDomain.CurrentDomain.BaseDirectory + "\\udtCSharpLog"))
                {
                    Directory.CreateDirectory(AppDomain.CurrentDomain.BaseDirectory + "\\udtCSharpLog");
                }
                string filename = AppDomain.CurrentDomain.BaseDirectory + "\\udtCSharpLog\\" + DateTime.Now.ToString("yyyy-MM-dd") + ".txt";

                FileInfo fi = new FileInfo(filename);
                if (fi.Exists && fi.Length > 50000000)
                {
                    if (m_log != null)
                    {
                        m_log.Close();
                    }
                    m_log = new StreamWriter(filename, false, Encoding.UTF8);
                    m_log.AutoFlush = false;
                }
                else if (m_log == null)
                {
                    m_log = new StreamWriter(filename, true, Encoding.UTF8);
                    m_log.AutoFlush = false;
                }

                if (m_log != null)
                {
                    m_log.WriteLine("***" + DateTime.Now.ToString());
                    m_log.WriteLine(msg);

                    m_log.Flush();
                }
            }
            catch { }
            finally
            {
                mutex.ReleaseMutex();
            }
        }
    }
}
