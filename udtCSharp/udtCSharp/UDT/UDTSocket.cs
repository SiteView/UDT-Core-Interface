using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Globalization;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Security.Permissions;
using System.Text;
using System.Xml.Linq;
using System.Threading;
using udtCSharp.packets;
using udtCSharp.Common;

namespace udtCSharp.UDT
{
    /// <summary>
    /// UDTSocket is analogous to a normal java.net.Socket, it provides input and output streams for the application
    /// udtsocket类似于一个正常的 net.socket，它提供对应用程序的输入和输出流
    /// </summary>
    public class UDTSocket
    {
        //endpoint
	    private UDPEndPoint endpoint;

        private volatile bool active;

        //processing received data
        private UDTReceiver receiver;
        private UDTSender sender;
	
        private UDTSession session;

        private UDTInputStream inputStream;
        private UDTOutputStream outputStream;
     
        /// <summary>
        /// * @param host
        /// * @param port
        /// * @param endpoint
        /// * @throws SocketException,UnknownHostException
        /// </summary>
        /// <param name="endpoint"></param>
        /// <param name="session"></param>
	    public UDTSocket(UDPEndPoint endpoint, UDTSession session)
        {
		    this.endpoint=endpoint;
		    this.session=session;
            this.receiver = new UDTReceiver(session, endpoint);
            this.sender = new UDTSender(session, endpoint);
	    }

        public UDTReceiver getReceiver() 
        {
		    return receiver;
	    }

	    public void setReceiver(UDTReceiver receiver)
        {
		    this.receiver = receiver;
	    }

	    public UDTSender getSender() 
        {
		    return sender;
	    }

	    public void setSender(UDTSender sender) 
        {
		    this.sender = sender;
	    }

	    public void setActive(bool active) 
        {
		    this.active = active;
	    }

	    public bool isActive() 
        {
		    return active;
	    }

	    public UDPEndPoint getEndpoint()
        {
		    return endpoint;
	    }
      
        /// <summary>
        /// get the input stream for reading from this socket
        /// 从这个socket中读取输入流
        /// [MethodImpl(MethodImplOptions.Synchronized)]表示定义为同步方法
        /// [MethodImpl(MethodImplOptions.Synchronized)] = loc(this)
        /// </summary>
        [MethodImpl(MethodImplOptions.Synchronized)]
	    public UDTInputStream getInputStream()
        {
            try
            {
		        if(inputStream==null)
                {
			        inputStream=new UDTInputStream(this);
		        }
		        return inputStream;
            }
            catch(Exception exc)
            {
                Log.Write(this.ToString(),"get the input stream for reading from this socket",exc);
                return null;
            }
	    }
    
       
        /// <summary>
        /// get the output stream for writing to this socket
        /// 将输出流写到到socket中
        /// [MethodImpl(MethodImplOptions.Synchronized)]表示定义为同步方法
        /// [MethodImpl(MethodImplOptions.Synchronized)] = loc(this)
        /// </summary>
        [MethodImpl(MethodImplOptions.Synchronized)]
	    public UDTOutputStream getOutputStream()
        {
            try
            {
		        if(outputStream==null)
                {
			        outputStream=new UDTOutputStream(this);
		        }
		        return outputStream;
            }
            catch(Exception exc)
            {
                Log.Write(this.ToString(),"get the output stream for writing to this socket",exc);
                return null;
            }
	    }
	
	    public UDTSession getSession()
        {
		    return session;
	    }	
      
        /// <summary>
        /// write single block of data without waiting for any acknowledgement
        /// 不等待任何确认,写单块数据
        /// </summary>
        /// <param name="data"></param>
	    public void doWrite(byte[]data)
        {           
		    doWrite(data, 0, data.Length);
	    }	
     
        /// <summary>
        /// write the given data 
        /// 写入给定的数据
        /// </summary>
        /// <param name="data">the data array</param>
        /// <param name="offset">the offset into the array</param>
        /// <param name="length">the number of bytes to write</param>
	    public void doWrite(byte[]data, int offset, int length)
        {
		    try
            {
                //doWrite(data, offset, length, Integer.MAX_VALUE, TimeUnit.MILLISECONDS);
                int itimeout = 2147483647;
                doWrite(data, offset, length, itimeout);
		    }
            catch(Exception exc)
            {
			    Log.Write(this.ToString(),"write the given data ",exc);
		    }
	    }
	
	    /**
	     * write the given data, waiting at most for the specified time if the queue is full
	     * @param data
	     * @param offset
	     * @param length
	     * @param timeout
	     * @param units
	     * @throws IOException - if data cannot be sent
	     * @throws InterruptedException
	     */
        //protected void doWrite(byte[]data, int offset, int length, int timeout, TimeUnit units)
        /// <summary>
        /// write the given data, waiting at most for the specified time if the queue is full
        /// 写入给定数据，如果队列已满，将最多等待指定的超时时间
        /// </summary>
        /// <param name="data"></param>
        /// <param name="offset"></param>
        /// <param name="length"></param>
        /// <param name="timeout"></param>
        public void doWrite(byte[]data, int offset, int length, int timeout)
        {
            int chunksize = session.getDatagramSize() - 24;//need some bytes for the header  1400-24=1376 每一次发送的最大字节数
            ByteBuffer bb = new ByteBuffer(data, offset, length);
		    long seqNo=0;
		    while(bb.Remaining()>0)
            {
                try
                {
			        int len = Math.Min(bb.Remaining(),chunksize);
			        byte[]chunk=new byte[len];
			        bb.Get(ref chunk);
			        DataPacket packet=new DataPacket();
			        seqNo=sender.getNextSequenceNumber();
			        packet.setPacketSequenceNumber(seqNo);
			        packet.setSession(session);
			        packet.setDestinationID(session.getDestination().getSocketID());
			        packet.setData(chunk);
			        //put the packet into the send queue
			        if(!sender.sendUdtPacket(packet, timeout))
                    {				       
                        Log.Write(this.ToString(),"Queue full");
			        }
                    Thread.Sleep(50);
                }
                catch(Exception exc)
                {
			         Log.Write(this.ToString(),"write the given data, waiting at most for the specified time if the queue is full ",exc);
		        }     
		    }
		    if(length>0)active=true;
	    }

        /// <summary>
        /// will block until the outstanding packets have really been sent out and acknowledged
        /// 直到要发送的数据包都真的被发送出去和确认之前将被阻塞
        /// </summary>
	    public void flush() 
        {
            try
            {
		        if(!active)return;
		        long seqNo=sender.getCurrentSequenceNumber();
		        if(seqNo<0)
                {
                    //在非法或不适当的时间调用方法时产生的信号。
                    //Log.Write(this.ToString(),"");
                }
		        while(!sender.isSentOut(seqNo))
                {
			        Thread.Sleep(5);
		        }
		        if(seqNo>-1)
                {
			        //wait until data has been sent out and acknowledged
			        while(active && !sender.haveAcknowledgementFor(seqNo))
                    {
				        sender.waitForAck(seqNo);
			        }
		        }
            }
            catch(Exception exc)
            {
			        Log.Write(this.ToString(),"Will block until the outstanding packets have really been sent out and acknowledged",exc);
		    }
             //TODO need to check if we can pause the sender...
		     //sender.pause();
	    }
	
        /// <summary>
        /// writes and wait for ack
        /// 写和等待ack
        /// </summary>
        /// <param name="data"></param>
	    public void doWriteBlocking(byte[]data)
        {
            try
            {
                doWrite(data);
                flush();
            }
            catch (Exception exc)
            {
                Log.Write(this.ToString(), "Writes and wait for ack", exc);
            }
	    }

        /// <summary>
        /// close the connection
        /// 关闭连接
        /// </summary>
	    public void close()
        {
            try
            {
                if (inputStream != null)
                    inputStream.Close();
                if (outputStream != null)
                    outputStream.Close();
                active = false;
            }
            catch (Exception exc)
            {
                Log.Write(this.ToString(), "Close the connection", exc);
            }
	    }

    }

}
