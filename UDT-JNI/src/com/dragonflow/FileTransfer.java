package com.dragonflow;

import android.os.Handler;

public class FileTransfer {
	
	public interface Callback
	{		
		public abstract void onAccept(String szIpAddr, String szHostName, String szSendType, String szFileName, int nFileCount);
		public abstract void onSendFinished(String szMsg);
		public abstract void onRecvFinished(String sztrMsg);
		public abstract void onSendTransfer(long nFileTotalSize, long nCurrent, String szFileName);
		public abstract void onRecvTransfer(long nFileTotalSize, long nCurrent, String szFileName);
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
		sendFiles(mCore, host, filelist, owndevice, owntype, recdevice, rectype, sendtype);
	}
	public void ReplyAccept(String strReply)
	{
		replyAccept(mCore, strReply, 0);
	}
	public void StopTransfer(int nType)
	{
		stopTransfer(mCore, nType, 0);
	}
	public void StopListen()
	{
		stopListen(mCore);
	}
	
	
	/**
	 *  * �����ļ���������ʱ���ص�
	 * @param host   IP ��ַ
	 * @param filename  �ļ���
	 * @param filecount   �ļ�����
	 * @param recdevice    �Է��豸����
	 * @param rectype      �����豸����
	 * @param owndevice    �Լ��豸����
	 * @param owntype      �����豸����
	 * @param sendtype     ��������    GENIEMAP /GENIETURBO
	 * @param sock       �׽�������
	 */
    public void onAccept(String host,String filename,int filecount,String recdevice,String rectype,String owndevice,String owntype,String sendtype,int sock)
    {
    	System.out.println("------onAccept:" + host + ":" + filename);
		//FileTransfer.this.hook_onAccept(host, DeviceName, SendType, filename, count);
    }
	void hook_onAccept(String host, String DeviceName, String SendType, String filename, int count)
	{
		if (mCallback != null) {
			mCallback.onAccept(host, DeviceName, SendType, filename, count);
		}
	}
	
    /**
     * �ļ��������ʱ�ص��ļ����� 
     * @param host     IP ��ַ
     * @param filename  ���ճɹ����ļ���
     * @param sock       �׽�������
     */
    public void onAcceptonFinish(String host, String filename,int sock)
    {
    	System.out.println("------onAccept:" + host + ":");
    	//FileTransfer.this.hook_onAcceptonFinish(host, fileNameList);
    }
    void hook_onAcceptonFinish(String host, Object[] fileNameList)
	{

	}
    
    /**
     * * ���ͽ���ʱ�ص�
     * @param msg  ����ʱ�ķ�����
     * @param type 1=�������ͷ�����  2=�������͵ķ�����
     */
    public void onFinished(String msg, int type)
    {
    	System.out.println("------onFinished:" + msg);
    	//FileTransfer.this.hook_onFinished(msg);
    }
    void hook_onFinished(String msg)
    {
    	if (mCallback != null)
    	{
    		mCallback.onRecvFinished(msg);
    	}
    }
    
    
    /**
     * * �����ļ��н��Ȼص�
     * @param sum    ���ļ���С
     * @param current	��ǰ������ɶ���
     * @param filename   ��ǰ���ļ���
     * @param type 1=�������ͽ���  2=�������ͽ���
     * @param sock      �׽�������
     */
    public void onTransfer(long sum,long current,String filename,int type ,int sock)
    {
    	System.out.println("------onTransfer:" + sum + ":" + current);
    	//FileTransfer.this.hook_onTransfer(sum, current, filename);
    }
    void hook_onTransfer(long sum,long current,String filename)
    {
    	if (mCallback != null)
    	{
    		mCallback.onRecvTransfer(sum, current, filename);
    	}
    }
    
    
    /**
     * ������Ϣʱ�ص�
     * @param msg  �ı���Ϣ
     * @param host  IP��ַ
     * @param hostname  ��������
     */
    public void onRecvMessage(String msg,String host,String hostname)
    {
    	System.out.println("------onRecvMessage:" + host + ":" + msg);
    	//FileTransfer.this.hook_onRecvMessage(host, hostname, msg);
    }
    void hook_onRecvMessage(String szIpAddr, String szHostName, String szMsg)
    {
		if (mCallback != null) {
			mCallback.onRecvMessage(szIpAddr, szHostName, szMsg);
		}
    }
    
    
	/**
	 *  ����ײ�� new()�����Ķ���ָ��
	 * @param delegate
	 * @return
	 */
	private static native long init(Object delegate);
	
	/**
	 * ɾ���ײ�� new()�����Ķ���ָ��
	 * @param core
	 */
	private static native void destroy(long core);
	
    /**
     * ��ʼ�����ļ�����
     * @param core �ײ��Ķ���
     * @param portctrl  �����İ�������˿�    7777
     * @param portfile  �������ļ�����˿�    7778
     * @return
     */
	private static native int startListen(long core, int portctrl, int portfile);
	
    /**
     * ֹͣ�����ļ�����
     * @param core �ײ��Ķ���
     * @return
     */
	private static native int stopListen(long core);
	
	   /**
	    * ������Ϣ 
	    * @param host  IP ��ַ
	    * @param message   �ı���Ϣ
	    * @param hostname   ������
	    * @param sendtype    
	    * @return
	    */
	private static native int sendMessage(long core, String host, String message, String hostname);
	
    /**
     * �����ļ�
     * @param core  �ײ��Ķ���
     * @param host  IP ��ַ 
     * @param filename  �ļ�����
     * @param hostname  ��������
     * @param owndevice    �Լ��豸����
     * @param owntype      �����豸����
     * @param recdevice    �Է��豸����
	 * @param rectype      �����豸����
	 * @param sendtype     ��������    GENIEMAP /GENIETURBO
     * @return
     */
	private static native int sendFiles(long core, String host, Object[] filename, String owndevice, String owntype, String recdevice, String rectype, String sendtype);
	
    /**
     * ֹͣ����
     * @param core  �ײ��Ķ���
     * @param type  1��ֹͣ����  2��ֹͣ����
     * @param sock      �׽�������
     */
	private static native int stopTransfer(long core, int type, int sock);
	
    /**
     * �Ƿ�����ļ� 
     * @param core  �ײ��Ķ���
     * @param strReply   "REJECT", "REJECTBUSY", "/mnt/sdcard/";
     * @param sock      �׽�������
     * @return
     */
	private static native int replyAccept(long core, String strReply, int sock);

    
	Callback mCallback = null;
	boolean mDisposed = false;
	boolean mStarted = false;
	Handler mDisp = new Handler();
	long mCore = 0;
	private String receiveFolder;
	private String response;

    static {
        System.loadLibrary("udt");
    }
}
