using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Threading;
using System.Windows.Forms;

using udtCSharp;
using udtCSharp.UDT;
using udtCSharp.Common;


namespace udtCSharpTest
{
    public partial class Form1 : Form
    {
        public bool serverStarted=false;
        public Form1()
        {
            InitializeComponent();
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            //richTextBox1.Text = "fadsfsdafsdaf \n fsdafsdafsdaf";
            //udtCSharp.Log

            //测试文件发和收
            main_Test();
        }

        //启动监听
        private void button_StartListen_Click(object sender, EventArgs e)
        {

        }
        //发送文字信息
        private void button_messageSend_Click(object sender, EventArgs e)
        {
            ;
        }

        private void button_selectfile_Click(object sender, EventArgs e)
        {
            OpenFileDialog fileDialog1 = new OpenFileDialog();
            fileDialog1.InitialDirectory = "D:\\";
            fileDialog1.Filter = "xls files (*.xls)|*.xls|All files (*.*)|*.*";
            fileDialog1.FilterIndex = 1;
            fileDialog1.RestoreDirectory = true;
            if (fileDialog1.ShowDialog() == DialogResult.OK)
            {
                if (textBox_file.Text != "")
                {
                    textBox_file.Text = textBox_file.Text + "|" + fileDialog1.FileName;
                }
                else
                {
                    textBox_file.Text = fileDialog1.FileName;
                }
            }
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void button_fileSend_Click(object sender, EventArgs e)
        {
            ;
        }

        private void main_Test()
        {
		    this.runServer();
		    do
            {
			    Thread.Sleep(500);
		    }while(!serverStarted);		
		   
            string pathSource = @"D:\XamlTest\MindSamples.rar";
            string pathNew = @"D:\XamlTest\MindSamplesBack.rar";
            //65321 是服务端的端口
            String[] args1 = new String[] { "127.0.0.1", "65321", pathSource, pathNew };
		    ReceiveFile.Main_Receive(args1);
	    }


	    private void runServer()
	    {
            ThreadStart threadDelegate = new ThreadStart(Runnable);
            Thread t = UDTThreadFactory.get().newThread(threadDelegate);
		    t.Start();
	    }

        private void Runnable()
		{
            //65321 是服务端的端口
			String []args=new String[]{"65321"};
			try
            {
				serverStarted=true;
				SendFile.Main_Send(args);
			}
            catch(Exception ex)
            {
				Log.Write(this.ToString(),ex);
			}
		}
    }
}
