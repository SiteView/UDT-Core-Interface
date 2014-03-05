using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Threading;
using System.IO;
using System.Net;
using System.Net.Sockets;

using udtCSharp.UDT;


namespace udtCSharp.Common
{    
    /// <summary>
    ///  辅助类，用于接收通过UDT单个文件拟与C + +的版本在UDT参考实现兼容
    /// 
    /// 主要方法的用法：
    /// udtCSharp.Common.ReceiveFile <server_ip> <server_port> <remote_filename> <local_filename>
    /// </summary>
    public class ReceiveFile
    {
	    private int serverPort;
	    private string serverHost;
	    private string remoteFile;
	    private string localFile;
        //private NumberFormat format;

        private bool verbose = false;
	    private String localIP=null;
	    private int localPort=-1;
	
	    public ReceiveFile(String serverHost, int serverPort, String remoteFile, String localFile)
        {
		    this.serverHost=serverHost;
		    this.serverPort=serverPort;
		    this.remoteFile=remoteFile;
		    this.localFile=localFile;
		    //format=NumberFormat.getNumberInstance();//java 返回当前默认语言环境的通用数值格式。
		    //format.setMaximumFractionDigits(3);//设置数的小数部分所允许的最小位数
	    }
	
	    public void RunReceive()
        {
		    verbose=true;
		    try
            {
			    UDTReceiver.connectionExpiryDisabled=true;
			    IPAddress myHost = localIP !=null?IPAddress.Parse(localIP):Dns.GetHostEntry(Dns.GetHostName()).AddressList[0];
			    UDTClient client = new UDTClient(myHost,localPort);
			    client.connect(serverHost, serverPort);
			    UDTInputStream inputStream = client.getInputStream();
			    UDTOutputStream outputStream = client.getOutputStream();
			    
                Log.Write(this.ToString(),"[ReceiveFile] Requesting file "+remoteFile);

			    byte[] fName = Encoding.Default.GetBytes(remoteFile);
			
			    //send file name info
			    byte[]nameinfo=new byte[fName.Length + 4];
			    System.Array.Copy(Util.encode(fName.Length), 0, nameinfo, 0, 4);
			    System.Array.Copy(fName, 0, nameinfo, 4, fName.Length);
			
			    outputStream.Write(nameinfo,0,nameinfo.Length);
			    outputStream.Flush();
			    //pause the sender to save some CPU time
			    outputStream.pauseOutput();
			
			    //read size info (an 64 bit number) 
			    byte[]sizeInfo=new byte[8];
			
			    int total=0;
			    while(total < sizeInfo.Length)
                {
				    int r = inputStream.Read(sizeInfo,0,sizeInfo.Length);
				    if(r < 0)break;
				    total+=r;
			    }
			    long size = Util.decode(sizeInfo, 0);

                if (File.Exists(localFile))
                {
                    File.Delete(localFile);
                }

                Log.Write(this.ToString(), "[ReceiveFile] Write to local file <" + localFile + ">");

                FileStream os = new FileStream(localFile, FileMode.Create, FileAccess.Write);
			    try
                {
				    Log.Write(this.ToString(),"[ReceiveFile] Reading <"+size+"> bytes.");
                    //( System.DateTime.UtcNow.Ticks - new DateTime(1970, 1, 1, 0, 0, 0).Ticks)/10000;
                    //如果要得到Java中 System.currentTimeMillis() 一样的结果，就可以写成 上面那样，也可以这样写：
                    // TimeSpan ts=new TimeSpan( System.DateTime.UtcNow.Ticks - new DateTime(1970, 1, 1, 0, 0, 0).Ticks);
                    //(long)ts.TotalMilliseconds;
                    TimeSpan ts_start = new TimeSpan(System.DateTime.UtcNow.Ticks - new DateTime(1970, 1, 1, 0, 0, 0).Ticks);
                    long start = (long)ts_start.TotalMilliseconds;

			        //and read the file data
				    Util.copy(inputStream, os, size, false);

                    TimeSpan ts_end = new TimeSpan(System.DateTime.UtcNow.Ticks - new DateTime(1970, 1, 1, 0, 0, 0).Ticks);
                    long end = (long)ts_end.TotalMilliseconds;

				    double rate=1000.0*size/1024/1024/(end-start);//接收所用时间

                    Log.Write(this.ToString(), "[ReceiveFile] Rate: " + Math.Round(rate, 3) + " MBytes/sec. " + Math.Round(8 * rate,3) + " MBit/sec.");
			
				    client.shutdown();
				
				    if(verbose)
                         Log.Write(this.ToString(),client.getStatistics().toString());
				
			    }
                finally
                {
                    os.Close();
			    }		
		    }
            catch(Exception ex)
            {
			    Log.Write(this.ToString(),"RunReceive",ex);
		    }
	    }


        public static bool Main_Receive(String[] fullArgs)
        {
            int serverPort = 65321;
            String serverHost = "localhost";
            String remoteFile = "";
            String localFile = "";

            //String[] args = parseOptions(fullArgs);

            try
            {
                serverHost = fullArgs[0];
                serverPort = int.Parse(fullArgs[1]);
                remoteFile = fullArgs[2];
                localFile = fullArgs[3];

                ReceiveFile rf = new ReceiveFile(serverHost, serverPort, remoteFile, localFile);
                //rf.RunReceive();
                Thread _thread = new Thread(new ThreadStart(rf.RunReceive));
                _thread.IsBackground = true;
                _thread.Start();
                return true;
            }
            catch (Exception ex)
            {
                usage();
                Log.Write("Main_Receive", ex);
                return false;
            }
	    }
	
	    public static void usage()
        {
		    Log.Write("Usage: java -cp .. udt.util.ReceiveFile " +
				    "<server_ip> <server_port> <remote_filename> <local_filename> " +
				    "[--verbose] [--localPort=<port>] [--localIP=<ip>]");
	    }

       
	
    }

}
