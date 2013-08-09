package com.example.udt;

import com.dragonflow.FileTransfer;

import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.app.Activity;
import android.view.Menu;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

public class MainActivity extends Activity implements FileTransfer.Callback{

	public String szTmp="";
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		
		mCore.StartListen(7777, 7778);
		
        Button bt1 = (Button) this.findViewById(R.id.Btn1);
        Button bt2 = (Button) this.findViewById(R.id.Btn2);
        Button bt3 = (Button) this.findViewById(R.id.Btn3);
        Button bt4 = (Button) this.findViewById(R.id.Btn4);
        Button bt5 = (Button) this.findViewById(R.id.Btn5);
        Button bt6 = (Button) this.findViewById(R.id.Btn6);
        Button bt7 = (Button) this.findViewById(R.id.Btn7);
        Button bt8 = (Button) this.findViewById(R.id.Btn8);
        Text_ip = (EditText) this.findViewById(R.id.text_ip);
        Text_ip.setText("192.168.9.196");
        Text_msg = (EditText) this.findViewById(R.id.text_msg);
        Text_msg.setText("input message to send");
        Text_info = (TextView) this.findViewById(R.id.textview1);
        Text_info.setText("TurboTransfer");
        bt1.setOnClickListener(new OnClickListener() {
        	@Override
        	public void onClick(View arg0) {
        		// 发送文本消息
                strAddr = Text_ip.getText().toString();
                strMsg = Text_msg.getText().toString(); 
                mCore.SendMessage(strAddr, strMsg, "android");
			}});
        
        bt2.setOnClickListener(new OnClickListener() {
        	@Override
        	public void onClick(View arg0) {
        		// 发送单个文件
                strAddr = Text_ip.getText().toString();
                mCore.SendFiles(strAddr, new Object[]{"//mnt//sdcard//NetgearGenie//123.jpg"}, "HTC G18", "ANDROID", "zhujianwen", "WIN7", "GENIETURBO");
			}});
        
        bt3.setOnClickListener(new OnClickListener() {
        	@Override
        	public void onClick(View arg0) {
        		// 发送多个文件
        		strAddr = Text_ip.getText().toString();
        		mCore.SendFiles(strAddr, new Object[]{"//mnt//sdcard//NetgearGenie//123.jpg", "//mnt//sdcard//NetgearGenie//libjingle.zip"}, "HTC G18", "ANDROID", "zhujianwen", "WIN7", "GENIETURBO");
			}});
        
        bt4.setOnClickListener(new OnClickListener() {
        	@Override
        	public void onClick(View arg0) {
        		// 发送文件夹
        		strAddr = Text_ip.getText().toString();
   
        		mCore.SendFiles(strAddr, new Object[]{"//mnt//sdcard//NetgearGenie//tools"}, "HTC G18", "ANDROID", "zhujianwen", "WIN7", "GENIETURBO");
			}});
        
        bt5.setOnClickListener(new OnClickListener() {
        	@Override
        	public void onClick(View arg0) {
        		mCore.ReplyAccept("/mnt/sdcard/NetgearGenie/");
			}});
        
        bt6.setOnClickListener(new OnClickListener() {
        	@Override
        	public void onClick(View arg0) {
        		mCore.ReplyAccept("REJECT");
			}});
        
        bt7.setOnClickListener(new OnClickListener() {
        	@Override
        	public void onClick(View arg0) {
        		mCore.ReplyAccept("/mnt/sdcard/NetgearGenie/");
			}});
        
        bt8.setOnClickListener(new OnClickListener() {
        	@Override
        	public void onClick(View arg0) {
        		mCore.ReplyAccept("REJECT");
			}});
        
    
	}
	
	public void onAccept(String szIpAddr, String szHostName, String szSendType, String szFileName, int nFileCount)
	{
		//String szTmp = "Accept file name:" + szFileName;
		//Text_info.setText(szTmp);
	}
	
	public void onSendFinished(String szMsg)
	{
		//Text_info.setText(szMsg);
	}
	
	public void onRecvFinished(String szMsg)
	{
		//Text_info.setText(szMsg);
	}
	
	public void onSendTransfer(long nFileTotalSize, long nCurrent, String szFileName)
	{
//		String szTmp = "RecvTransfer:" + szFileName;
//		Text_info.setText(szTmp);
		szTmp = "Recv msg:" + szFileName;
		handler.sendEmptyMessage(1);
	}
	
	public void onRecvTransfer(long nFileTotalSize, long nCurrent, String szFileName)
	{
//		String szTmp = "RecvTransfer:" + szFileName;
//		Text_info.setText(szTmp);
		szTmp = "Recv msg:" + szFileName;
		handler.sendEmptyMessage(1);
	}
	
	public void onRecvMessage(String szIpAddr, String szHostName, String szMsg)
	{
		szTmp = "Recv msg:" + szMsg;
		handler.sendEmptyMessage(1);
//		Text_info.setText(szTmp);
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
//		 Inflate the menu; this adds items to the action bar if it is present.
//		getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}
	
	FileTransfer mCore = new FileTransfer(this);
	private String strAddr;
	private String strMsg;
	private TextView Text_info;
	private EditText Text_ip;
	private EditText Text_msg;
	
	private Handler handler = new Handler() {
		public void handleMessage(Message msg) {

			switch (msg.what) {

			case 1:
				Text_info.setText(szTmp);
				break;

			default:
				break;
			}
		};
	};
}
