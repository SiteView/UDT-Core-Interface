package com.dragonflow;

import android.os.Handler;

public class FileTransfer {
	
	public interface Callback
	{		
		public abstract void onAccept(String szIpAddr, String szHostName, String szSendType, String szFileName, int nFileCount);
		public abstract void onFinished(String szMsg, int Type);
		public abstract void onTransfer(long nFileTotalSize, long nCurrent, String szFileName, int Type);
		public abstract void onRecvMessage(String szIpAddr, String szHostName, String szMsg);
	}
	
	
	public FileTransfer(Callback callback)
	{
		mCallback = callback;
		mCore = init(this);
	}
	public void StartListen(int portctrl, int portfile)
	{
		if (startListen(mCore, portctrl, portfile) == 0)
		{
			System.out.println("------startListen success:" + portctrl + "--" + portfile);
		}
		
	}
	public void SendMessage(String address, String message, String hostname){
		sendMessage(mCore, address, message, hostname);
	}
	public void SendFiles(String host, Object[] filelist, String owndevice, String owntype, String recdevice, String rectype, String sendtype){
		sock = sendFiles(mCore, host, filelist, owndevice, owntype, recdevice, rectype, sendtype);
	}
	public void ReplyAccept(String strReply)
	{
		replyAccept(mCore, strReply, 0);
	}
	public void StopTransfer(int nType)
	{
		stopTransfer(mCore, nType, sock);
	}
	public void StopListen()
	{
		stopListen(mCore);
	}
	
	public void TestString()
	{
		String str = Test(mCore);
		System.out.println("------onAccept:" + str);
	}
	
	
	/**
	 *  * 接受文件传送请求时，回调
	 * @param host   IP 地址
	 * @param filename  文件名
	 * @param filecount   文件个数
	 * @param recdevice    对方设备名称
	 * @param rectype      对文设备类型
	 * @param owndevice    自己设备名称
	 * @param owntype      自已设备类型
	 * @param sendtype     发送类型    GENIEMAP /GENIETURBO
	 * @param sock       套接字索引
	 */
    public void onAccept(String host,String filename,int filecount, long fileSize, String recdevice,String rectype,String owndevice,String owntype,String sendtype, String fileType, int sock)
    {
    	System.out.println("------onAccept:" + host + ":" + filename);
    	if (filecount == 1 && fileType.equals("F"))
    	{
    		String filePath = "//mnt//sdcard//NetgearGenie//" + filename;
    		replyAccept(mCore, filePath, sock);
    	}
    	else
    	{
    		String filePath = "//mnt//sdcard//NetgearGenie//";
    		replyAccept(mCore, filePath, sock);
    	}
		//FileTransfer.this.hook_onAccept(host, DeviceName, SendType, filename, count);
    }
	void hook_onAccept(String host, String DeviceName, String SendType, String filename, int count)
	{
		//if (mCallback != null) {
		//	mCallback.onAccept(host, DeviceName, SendType, filename, count);
		//}
	}
	
    /**
     * 文件传送完毕时回调文件名称 
     * @param host     IP 地址
     * @param filename  接收成功的文件名
     * @param sock       套接字索引
     */
    public void onAcceptonFinish(String host, String filename, int Type, int sock)
    {
    	System.out.println("------onAccept:" + host + ":" + filename);
    	//FileTransfer.this.hook_onAcceptonFinish(host, fileNameList);
    }
    void hook_onAcceptonFinish(String host, Object[] fileNameList)
	{

	}
    
    /**
     * * 传送结束时回调
     * @param msg  结束时的返回码
     * @param type 1=接收类型返回码  2=发送类型的返回码
     */
    public void onFinished(String msg, int type)
    {
    	System.out.println("------onFinished:" + msg);
    	//FileTransfer.this.hook_onFinished(msg);
    }
    void hook_onFinished(String msg)
    {
    	//if (mCallback != null)
    	//{
    	//	mCallback.onRecvFinished(msg);
    	//}
    }
    
    
    /**
     * * 传送文件中进度回调
     * @param sum    总文件大小
     * @param current	当前传送完成多少
     * @param filename   当前的文件名
     * @param type 1=接收类型进度  2=发送类型进度
     * @param sock      套接字索引
     */
    public void onTransfer(long sum, long current, double iProgress, String filename, int type , int sock)
    {
    	System.out.println("------onTransfer:" + sum + ":" + current);
    	FileTransfer.this.hook_onTransfer(sum, current, filename, type);
    }
    void hook_onTransfer(long sum,long current,String filename, int type)
    {
    	if (mCallback != null)
    	{
    		mCallback.onTransfer(sum, current, filename, type);
    	}
    }
    
    
    /**
     * 接受消息时回调
     * @param msg  文本信息
     * @param host  IP地址
     * @param hostname  主机名称
     */
    public void onRecvMessage(String msg, String host, String hostname)
    {
    	System.out.println("------onRecvMessage:" + host + ":" + msg);
    	//FileTransfer.this.hook_onRecvMessage(host, hostname, msg);
    }
    void hook_onRecvMessage(String szIpAddr, String szHostName, String szMsg)
    {
		//if (mCallback != null) {
		//	mCallback.onRecvMessage(szIpAddr, szHostName, szMsg);
		//}
    }
    
	/**
	 * 已断线的IP
	 * @param ip
	 */
	public void heartAccept(String ip, int Type){
		
	}
    
    
	/**
	 *  保存底层库 new()出来的对象指针
	 * @param delegate
	 * @return
	 */
	private static native long init(Object delegate);
	
	/**
	 * 删除底层库 new()出来的对象指针
	 * @param core
	 */
	private static native void destroy(long core);
	
    /**
     * 开始接受文件传送
     * @param core 底层库的对象
     * @param portctrl  监听的按制命令端口    7777
     * @param portfile  监听的文件传输端口    7778
     * @return
     */
	private static native int startListen(long core, int portctrl, int portfile);
	
    /**
     * 停止接受文件传送
     * @param core 底层库的对象
     * @return
     */
	private static native int stopListen(long core);
	
	   /**
	    * 发送消息 
	    * @param host  IP 地址
	    * @param message   文本信息
	    * @param hostname   主机名
	    * @param sendtype    
	    * @return
	    */
	private static native int sendMessage(long core, String host, String message, String hostname);
	
    /**
     * 发送文件
     * @param core  底层库的对象
     * @param host  IP 地址 
     * @param filename  文件名称
     * @param hostname  主机名称
     * @param owndevice    自己设备名称
     * @param owntype      自已设备类型
     * @param recdevice    对方设备名称
	 * @param rectype      对文设备类型
	 * @param sendtype     发送类型    GENIEMAP /GENIETURBO
     * @return
     */
	private static native int sendFiles(long core, String host, Object[] filename, String owndevice, String owntype, String recdevice, String rectype, String sendtype);
	
    /**
     * 停止发送
     * @param core  底层库的对象
     * @param type  1：停止接收  2：停止发送
     * @param sock      套接字索引
     */
	private static native int stopTransfer(long core, int type, int sock);
	
    /**
     * 是否接受文件 
     * @param core  底层库的对象
     * @param strReply   "REJECT", "REJECTBUSY", "/mnt/sdcard/";
     * @param sock      套接字索引
     * @return
     */
	private static native int replyAccept(long core, String strReply, int sock);
	
	/**
	 * 2013-09-04  增加心跳  传送要测试的IP心跳
	 */
	private static native void heart(Object ips[]);
	private static native String Test(long core);

    
	Callback mCallback = null;
	boolean mDisposed = false;
	boolean mStarted = false;
	Handler mDisp = new Handler();
	long mCore = 0;
	int sock = 0;
	private String receiveFolder;
	private String response;

    static {
        System.loadLibrary("udt");
    }
}
