using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using udtCSharp;

namespace udtCSharpTest
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            //richTextBox1.Text = "fadsfsdafsdaf \n fsdafsdafsdaf";
            //udtCSharp.Log
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

        private void button_fileSend_Click(object sender, EventArgs e)
        {
            ;
        }
    }
}
