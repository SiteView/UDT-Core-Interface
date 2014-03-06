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
using udtCSharp.Common;

namespace udtCSharp.Common
{
    /// <summary>
    /// 通过UDT发送单个文件助手应用程序 
    /// 拟与C + +版本中的UDT参考实现兼容
    /// 主要方法的用法：
    /// udtCSharp.Common.SendFile <server_port>
    /// </summary>
    public class SendFile
    {
	    private int serverPort;

        private bool verbose = false;
        /// <summary>
        /// 服务器IP
        /// </summary>
	    private String localIP="127.0.0.1";
	    private int localPort=-1;

	    //TODO configure pool size
        //private ExecutorService threadPool=Executors.newFixedThreadPool(3);

	    public SendFile(int serverPort)
        {
		    this.serverPort=serverPort;
	    }

	    public void RunSend()
        {
		    try
            {
			    UDTReceiver.connectionExpiryDisabled=true;
                IPAddress myHost = null;
                if (localIP != "")
                {
                    myHost = IPAddress.Parse(localIP);
                }
                else
                {
                    string hostname = Dns.GetHostName();
                    IPHostEntry hostip = Dns.GetHostEntry(hostname);
                    foreach (IPAddress ipaddress in hostip.AddressList)
                    {
                        if (ipaddress.ToString().IndexOf(':') < 0)//存在IPV6的地址，所以要判断
                        {
                            myHost = ipaddress;
                            break;
                        }
                    }
                }
                
			    UDTServerSocket server=new UDTServerSocket(myHost,serverPort);
                ThreadPool.SetMaxThreads(3, 3);
                
                while(true)
                {
				    UDTSocket socket = server.Accept();//
				    Thread.Sleep(1000);
                    TaskInfo ti = new TaskInfo(socket, verbose);
                    
                    ThreadPool.QueueUserWorkItem(new WaitCallback(run), ti);
                }
		    }
            catch(Exception ex)
            {
			    Log.Write(this.ToString(),ex);
		    }
	    }

        public void run(object stateInfo)
        {
            TaskInfo ti = (TaskInfo)stateInfo;
            bool memMapped = false;
			try
            {
				Log.Write("Handling request from "+ti.socket.getSession().getDestination());
				UDTInputStream inputStream=ti.socket.getInputStream();
				UDTOutputStream outputStream=ti.socket.getOutputStream();
				byte[]readBuf=new byte[32768];
				ByteBuffer bb= new ByteBuffer(readBuf);

				//read file name info 
                while (inputStream.Read(readBuf, 0, readBuf.Length) == 0)
                {
                    Thread.Sleep(100);
                }

				//how many bytes to read for the file name
                //byte[] len = new byte[4];
				byte[] len = bb.PopByteArray(4);
				if(ti.verbose)
                {
					StringBuilder sb=new StringBuilder();
					for(int i=0;i<len.Length;i++)
                    {
						sb.Append(len[i].ToString());
						sb.Append(" ");
					}
					Log.Write("[SendFile] name length data: "+sb.ToString());
				}
				long length=Util.decode(len, 0);
				if(verbose)
                    Log.Write("[SendFile] name length     : "+length);
				//byte[] fileName = new byte[(int)length];
				byte[] fileName = bb.PopByteArray((int)length);
                string sfileName = Encoding.Default.GetString(fileName);
                if(!File.Exists(sfileName))
                {
                    Log.Write("[SendFile] Send the file does not exist : " + sfileName);
                    return;
                }
				
                FileStream fis = new FileStream(sfileName, FileMode.Open, FileAccess.Read);
				Log.Write("[SendFile] File requested: " + sfileName);
				try
                {
					long size=fis.Length;
					Log.Write("[SendFile] File size: " + size);
					//send size info
					outputStream.Write(Util.encode64(size),0,8);
					outputStream.Flush();
					
                    //( System.DateTime.UtcNow.Ticks - new DateTime(1970, 1, 1, 0, 0, 0).Ticks)/10000;
                    //如果要得到Java中 System.currentTimeMillis() 一样的结果，就可以写成 上面那样，也可以这样写：
                    // TimeSpan ts=new TimeSpan( System.DateTime.UtcNow.Ticks - new DateTime(1970, 1, 1, 0, 0, 0).Ticks);
                    //(long)ts.TotalMilliseconds;
                    TimeSpan ts_start = new TimeSpan(System.DateTime.UtcNow.Ticks - new DateTime(1970, 1, 1, 0, 0, 0).Ticks);
					long start=(long)ts_start.TotalMilliseconds;

					//and send the file
					if(memMapped)
                    {
                        copyFile(fis, outputStream);
					}
                    else
                    {
                        Util.copySend(fis, outputStream, size, false);
					}
					Log.Write("[SendFile] Finished sending data.");

                    TimeSpan ts_end = new TimeSpan(System.DateTime.UtcNow.Ticks - new DateTime(1970, 1, 1, 0, 0, 0).Ticks);
					long end=(long)ts_end.TotalMilliseconds;

					Log.Write(ti.socket.getSession().getStatistics().toString());
					double rate=1000.0*size/1024/1024/(end-start);
					Log.Write("[SendFile] Rate: "+Math.Round(rate,3)+" MBytes/sec. "+Math.Round(8*rate,3)+" MBit/sec.");
				}
                finally
                {
					ti.socket.getSender().stop();
					if(fis!=null)fis.Close();
				}
				Log.Write("Finished request from "+ti.socket.getSession().getDestination());
			}
            catch(Exception ex)
            {
				Log.Write("[SendFile] File err",ex);
			}
		}

	    /**
	     * main() method for invoking as a commandline application
	     * @param args
	     * @throws Exception
	     */
	    public static bool Main_Send(String[] fullArgs)
        {
		    int serverPort=65321;
		    try
            {
			    serverPort= int.Parse(fullArgs[0]);

                SendFile sf=new SendFile(serverPort);
                //sf.RunSend();
                Thread _thread = new Thread(new ThreadStart(sf.RunSend));
                _thread.IsBackground = true;
                _thread.Start();
                return true;
		    }
            catch(Exception ex)
            {
			    usage();
                Log.Write("Main_Send", ex);
			    return false;
		    }
	    }

	    public static void usage()
        {
		    Log.Write("Usage: java -cp ... udt.util.SendFile <server_port>  [--verbose] [--localPort=<port>] [--localIP=<ip>]");
	    }

        private static void copyFile(FileStream file, UDTOutputStream os)
        {
            try
            {
                byte[] filebuf = new byte[file.Length];
                file.Read(filebuf, 0, (int)file.Length);
                               
                //可以读取的长度
                int numBytesToRead = (int)file.Length;
                //已经读取的长度
                int numBytesRead = 0;
                while (true)
                {
                    int len = 0;
                    len = Math.Min(1024 * 1024, numBytesToRead);
                    byte[] buf = new byte[len];
                    Array.Copy(filebuf, numBytesRead, buf, 0, len);
                    os.Write(buf, 0, len);
                   
                    numBytesRead += len;
                    numBytesToRead -= len;
                    if (numBytesToRead == 0)
                    {
                        break;
                    }
                }
                os.Flush();
                file.Close();
            }
            catch (Exception exc)
            {
                Log.Write("[SendFile] File err", exc);
            }
	    }
	
    }

    /// <summary>
    /// 传送参数的类
    /// </summary>
    public class TaskInfo
    {
        public UDTSocket socket;
        public bool verbose;

        public TaskInfo(UDTSocket _socket, bool _verbose)
        {
            socket = _socket;
            verbose = _verbose;
        }
    }
}
