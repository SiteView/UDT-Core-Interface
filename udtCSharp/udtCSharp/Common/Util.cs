using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;
using System.Net;
using System.Net.Sockets;
using udtCSharp.UDT;
using System.Security.Cryptography;


namespace udtCSharp.Common
{
    /**
    * helper methods 
    */
    public class Util 
    {
	    /**
	    * get the current timer value in microseconds
	    * @return
	    */
	    public static long getCurrentTime()
        {
            //1纳秒=1000 皮秒　
            //1纳秒 =0.001  微秒
            //1纳秒=0.000001 毫秒
            //1纳秒=0.00000 0001秒
		    return System.DateTime.Now.Millisecond * 100000 / 1000;
	    }
	
	
	    public static long SYN=10000;
	
	    public static double SYN_D=10000.0;
	
	    /**
	    * get the SYN time in microseconds. The SYN time is 0.01 seconds = 10000 microseconds
	    * @return
	    */
	    public static long getSYNTime()
        {
		    return 10000;
	    }
	
	    public static double getSYNTimeD(){
		    return 10000.0;
	    }
	
	    /**
	    * get the SYN time in seconds. The SYN time is 0.01 seconds = 10000 microseconds
	    * @return
	    */
	    public static double getSYNTimeSeconds(){
		    return 0.01;
	    }
	    /**
	    * read a line terminated by a new line '\n' character
	    * @param input - the input string to read from 
	    * @return the line read or <code>null</code> if end of input is reached
	    * @throws IOException
	    */
	    public static String readLine(StreamReader input)
        {
		    return readLine(input, '\n');
	    }
	
	    /**
	    * read a line from the given input stream
	    * @param input - the input stream
	    * @param terminatorChar - the character used to terminate lines
	    * @return the line read or <code>null</code> if end of input is reached
	    * @throws IOException
	    */
        public static String readLine(StreamReader input, char terminatorChar)
        {
            string svalue = input.ReadLine();
            input.Close();
            return svalue;           
	    }

	    /**
	    * copy input data from the source stream to the target stream
	    * @param source - input stream to read from
	    * @param target - output stream to write to
	    * @throws IOException
	    */
        public static void copy(UDTInputStream source, FileStream target)
        {
		    copy(source,target,-1, false);
	    }
	
        /// <summary>
        /// copy input data from the source stream to the target stream
        /// 用在接收文件
        /// </summary>
        /// <param name="source">input stream to read from</param>
        /// <param name="target">output stream to write to</param>
        /// <param name="size">要接收的总长度</param>
        /// <param name="flush">每次写入后是否清除此流的缓冲区</param>
        public static void copy(UDTInputStream source, FileStream target, long size, bool flush)
        {
            try
            {
                byte[] buf = new byte[8 * 65536];
                int c;
                int read = 0;
                while (true)
                {
                    c = source.Read(buf, 0, buf.Length);
                    if (c < 0) break;
                    read += c;
                    ///Log.Write("writing <"+c+"> bytes");
                    target.Write(buf, 0, c);
                    if (flush) target.Flush();
                    if (read >= size && size > -1) break;
                }
                if (!flush) target.Flush();
            }
            catch (Exception exc)
            {
                Log.Write("copy-ReceiveFile", exc);
            }
	    }

        /// <summary>
        /// copy input data from the source stream to the target stream
        /// 用在发送文件
        /// </summary>
        /// <param name="source">文件流</param>
        /// <param name="target">output stream to write to</param>
        /// <param name="size">要接收的总长度</param>
        /// <param name="flush">每次写入后是否清除此流的缓冲区</param>
        public static void copySend(FileStream source, UDTOutputStream target, long size, bool flush)
        {
            try
            {
                byte[] filebuf = new byte[source.Length];
                source.Read(filebuf, 0, (int)source.Length);

                //可以读取的长度
                int numBytesToRead = (int)source.Length;
                //已经读取的长度
                int numBytesRead = 0;

                while (true)
                {
                    int len = 0;
                    len = Math.Min(512 * 1024, numBytesToRead);
                    if (len == 0)
                        break;
                    byte[] buf = new byte[len];
                    Array.Copy(filebuf, numBytesRead, buf, 0, len);
                    //Log.Write("writing <"+len+"> bytes");
                    target.Write(buf, 0, len);

                    numBytesRead += len;
                    numBytesToRead -= len;

                    if (flush) 
                        target.Flush();
                }
                if (!flush)
                {
                    target.Flush();
                }
                source.Close();
            }
            catch (Exception exc)
            {
                source.Close();
                Log.Write("copySend-SendFile", exc);
            }
        }
	
	    /**
	    * perform UDP hole punching to the specified client by sending 
	    * a dummy packet. A local port will be chosen automatically.
	    * 
	    * @param client - client address
	    * @return the local port that can now be accessed by the client
	    * @throws IOException
	    */
        public static void doHolePunch(UDPEndPoint endpoint, IPAddress client, int clientPort)
        {
            byte[] p = new byte[1];
		    endpoint.sendRaw(p);
	    }
        /// <summary>
        /// MD5数据加密
        /// </summary>
        /// <param name="digest"></param>
        /// <returns></returns>
	    public static String hexString(MD5 digest)
        {
		    byte[] messageDigest = digest.Hash;
		    StringBuilder hexString = new StringBuilder();
		    for (int i=0;i<messageDigest.Length;i++) 
            {
                //String hex = Integer.toHexString(0xFF & messageDigest[i]); 
                //if(hex.Length==1)hexString.Append('0');
                //hexString.Append(hex);

                String hex = messageDigest[i].ToString();
                hexString.Append(hex);
		    }
		    return hexString.ToString();
	    }

        public static long decode(byte[] data, int start)
        {
            long result = (data[start + 3] & 0xFF) << 24
                         | (data[start + 2] & 0xFF) << 16
                         | (data[start + 1] & 0xFF) << 8
                         | (data[start] & 0xFF);
            return result;
        }

        public static byte[] encode(long value)
        {
            byte m4 = (byte)(value >> 24);
            byte m3 = (byte)(value >> 16);
            byte m2 = (byte)(value >> 8);
            byte m1 = (byte)(value);
            return new byte[] { m1, m2, m3, m4 };
        }

        public static byte[] encode64(long value)
        {
            byte m4 = (byte)(value >> 24);
            byte m3 = (byte)(value >> 16);
            byte m2 = (byte)(value >> 8);
            byte m1 = (byte)(value);
            return new byte[] { m1, m2, m3, m4, 0, 0, 0, 0 };
        }
	
    }
}
