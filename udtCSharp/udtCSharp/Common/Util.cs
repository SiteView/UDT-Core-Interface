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
        public static void copy(Stream source, Stream target)
        {
		    copy(source,target,-1, false);
	    }
	
	    /**
	        * copy input data from the source stream to the target stream
	        * @param source - input stream to read from
	        * @param target - output stream to write to
	        * @param size - how many bytes to copy (-1 for no limit)
	        * @param flush - whether to flush after each write
	        * @throws IOException
	        */
        public static void copy(Stream source, Stream target, long size, bool flush)
        {
            byte[] buf = new byte[source.Length];		   
            source.Read(buf, 0, buf.Length);
            target.Write(buf, 0, buf.Length);
            if (flush) target.Flush();
            source.Close();
            target.Close();
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
	
    }
}
