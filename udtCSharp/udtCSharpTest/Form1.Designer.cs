namespace udtCSharpTest
{
    partial class Form1
    {
        /// <summary>
        /// 必需的设计器变量。
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// 清理所有正在使用的资源。
        /// </summary>
        /// <param name="disposing">如果应释放托管资源，为 true；否则为 false。</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows 窗体设计器生成的代码

        /// <summary>
        /// 设计器支持所需的方法 - 不要
        /// 使用代码编辑器修改此方法的内容。
        /// </summary>
        private void InitializeComponent()
        {
            this.label1 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.label3 = new System.Windows.Forms.Label();
            this.label4 = new System.Windows.Forms.Label();
            this.textBox_CtrlPort = new System.Windows.Forms.TextBox();
            this.textBox_FilePort = new System.Windows.Forms.TextBox();
            this.richTextBox1 = new System.Windows.Forms.RichTextBox();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.button_StartListen = new System.Windows.Forms.Button();
            this.textBox_message = new System.Windows.Forms.TextBox();
            this.button_messageSend = new System.Windows.Forms.Button();
            this.richTextBox2 = new System.Windows.Forms.RichTextBox();
            this.groupBox3 = new System.Windows.Forms.GroupBox();
            this.richTextBox3 = new System.Windows.Forms.RichTextBox();
            this.button_fileSend = new System.Windows.Forms.Button();
            this.textBox_file = new System.Windows.Forms.TextBox();
            this.button_selectfile = new System.Windows.Forms.Button();
            this.groupBox1.SuspendLayout();
            this.groupBox2.SuspendLayout();
            this.groupBox3.SuspendLayout();
            this.SuspendLayout();
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(27, 32);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(65, 12);
            this.label1.TabIndex = 0;
            this.label1.Text = "文字信息：";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(15, 34);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(41, 12);
            this.label2.TabIndex = 1;
            this.label2.Text = "文件：";
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.button_StartListen);
            this.groupBox1.Controls.Add(this.richTextBox1);
            this.groupBox1.Controls.Add(this.textBox_FilePort);
            this.groupBox1.Controls.Add(this.textBox_CtrlPort);
            this.groupBox1.Controls.Add(this.label4);
            this.groupBox1.Controls.Add(this.label3);
            this.groupBox1.Location = new System.Drawing.Point(12, 5);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(641, 177);
            this.groupBox1.TabIndex = 2;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "监听";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(27, 25);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(155, 12);
            this.label3.TabIndex = 0;
            this.label3.Text = "监听端口1(命令收发端口)：";
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(275, 25);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(155, 12);
            this.label4.TabIndex = 1;
            this.label4.Text = "监听端口2(数据收发端口)：";
            // 
            // textBox_CtrlPort
            // 
            this.textBox_CtrlPort.Location = new System.Drawing.Point(184, 21);
            this.textBox_CtrlPort.Name = "textBox_CtrlPort";
            this.textBox_CtrlPort.Size = new System.Drawing.Size(71, 21);
            this.textBox_CtrlPort.TabIndex = 2;
            this.textBox_CtrlPort.Text = "7777";
            // 
            // textBox_FilePort
            // 
            this.textBox_FilePort.Location = new System.Drawing.Point(436, 21);
            this.textBox_FilePort.Name = "textBox_FilePort";
            this.textBox_FilePort.Size = new System.Drawing.Size(66, 21);
            this.textBox_FilePort.TabIndex = 3;
            this.textBox_FilePort.Text = "7778";
            // 
            // richTextBox1
            // 
            this.richTextBox1.Location = new System.Drawing.Point(22, 52);
            this.richTextBox1.Name = "richTextBox1";
            this.richTextBox1.Size = new System.Drawing.Size(597, 119);
            this.richTextBox1.TabIndex = 4;
            this.richTextBox1.Text = "";
            // 
            // groupBox2
            // 
            this.groupBox2.Controls.Add(this.richTextBox2);
            this.groupBox2.Controls.Add(this.button_messageSend);
            this.groupBox2.Controls.Add(this.textBox_message);
            this.groupBox2.Controls.Add(this.label1);
            this.groupBox2.Location = new System.Drawing.Point(12, 190);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(641, 178);
            this.groupBox2.TabIndex = 3;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "发送文字信息";
            // 
            // button_StartListen
            // 
            this.button_StartListen.Location = new System.Drawing.Point(526, 18);
            this.button_StartListen.Name = "button_StartListen";
            this.button_StartListen.Size = new System.Drawing.Size(93, 26);
            this.button_StartListen.TabIndex = 5;
            this.button_StartListen.Text = "启动监听";
            this.button_StartListen.UseVisualStyleBackColor = true;
            this.button_StartListen.Click += new System.EventHandler(this.button_StartListen_Click);
            // 
            // textBox_message
            // 
            this.textBox_message.Location = new System.Drawing.Point(98, 29);
            this.textBox_message.Name = "textBox_message";
            this.textBox_message.Size = new System.Drawing.Size(419, 21);
            this.textBox_message.TabIndex = 1;
            this.textBox_message.Text = "Relative bulk get method. ";
            // 
            // button_messageSend
            // 
            this.button_messageSend.Location = new System.Drawing.Point(526, 27);
            this.button_messageSend.Name = "button_messageSend";
            this.button_messageSend.Size = new System.Drawing.Size(93, 26);
            this.button_messageSend.TabIndex = 2;
            this.button_messageSend.Text = "启动发送";
            this.button_messageSend.UseVisualStyleBackColor = true;
            this.button_messageSend.Click += new System.EventHandler(this.button_messageSend_Click);
            // 
            // richTextBox2
            // 
            this.richTextBox2.Location = new System.Drawing.Point(22, 58);
            this.richTextBox2.Name = "richTextBox2";
            this.richTextBox2.Size = new System.Drawing.Size(597, 119);
            this.richTextBox2.TabIndex = 5;
            this.richTextBox2.Text = "";
            // 
            // groupBox3
            // 
            this.groupBox3.Controls.Add(this.button_selectfile);
            this.groupBox3.Controls.Add(this.richTextBox3);
            this.groupBox3.Controls.Add(this.button_fileSend);
            this.groupBox3.Controls.Add(this.textBox_file);
            this.groupBox3.Controls.Add(this.label2);
            this.groupBox3.Location = new System.Drawing.Point(12, 374);
            this.groupBox3.Name = "groupBox3";
            this.groupBox3.Size = new System.Drawing.Size(641, 242);
            this.groupBox3.TabIndex = 6;
            this.groupBox3.TabStop = false;
            this.groupBox3.Text = "发送文件";
            // 
            // richTextBox3
            // 
            this.richTextBox3.Location = new System.Drawing.Point(22, 58);
            this.richTextBox3.Name = "richTextBox3";
            this.richTextBox3.Size = new System.Drawing.Size(597, 178);
            this.richTextBox3.TabIndex = 5;
            this.richTextBox3.Text = "";
            // 
            // button_fileSend
            // 
            this.button_fileSend.Location = new System.Drawing.Point(526, 27);
            this.button_fileSend.Name = "button_fileSend";
            this.button_fileSend.Size = new System.Drawing.Size(93, 26);
            this.button_fileSend.TabIndex = 2;
            this.button_fileSend.Text = "启动发送";
            this.button_fileSend.UseVisualStyleBackColor = true;
            this.button_fileSend.Click += new System.EventHandler(this.button_fileSend_Click);
            // 
            // textBox_file
            // 
            this.textBox_file.Location = new System.Drawing.Point(63, 29);
            this.textBox_file.Name = "textBox_file";
            this.textBox_file.Size = new System.Drawing.Size(315, 21);
            this.textBox_file.TabIndex = 1;
            this.textBox_file.Text = "D:\\udt01.jpg";
            // 
            // button_selectfile
            // 
            this.button_selectfile.Location = new System.Drawing.Point(384, 27);
            this.button_selectfile.Name = "button_selectfile";
            this.button_selectfile.Size = new System.Drawing.Size(93, 26);
            this.button_selectfile.TabIndex = 6;
            this.button_selectfile.Text = "选择文件";
            this.button_selectfile.UseVisualStyleBackColor = true;
            this.button_selectfile.Click += new System.EventHandler(this.button_selectfile_Click);
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 12F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(665, 628);
            this.Controls.Add(this.groupBox3);
            this.Controls.Add(this.groupBox2);
            this.Controls.Add(this.groupBox1);
            this.Name = "Form1";
            this.Text = "Form1";
            this.Load += new System.EventHandler(this.Form1_Load);
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.groupBox2.ResumeLayout(false);
            this.groupBox2.PerformLayout();
            this.groupBox3.ResumeLayout(false);
            this.groupBox3.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.TextBox textBox_FilePort;
        private System.Windows.Forms.TextBox textBox_CtrlPort;
        private System.Windows.Forms.RichTextBox richTextBox1;
        private System.Windows.Forms.Button button_StartListen;
        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.TextBox textBox_message;
        private System.Windows.Forms.Button button_messageSend;
        private System.Windows.Forms.RichTextBox richTextBox2;
        private System.Windows.Forms.GroupBox groupBox3;
        private System.Windows.Forms.RichTextBox richTextBox3;
        private System.Windows.Forms.Button button_fileSend;
        private System.Windows.Forms.TextBox textBox_file;
        private System.Windows.Forms.Button button_selectfile;
    }
}

