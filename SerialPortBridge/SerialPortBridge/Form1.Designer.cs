
namespace Terminal
{
    partial class Form1
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.btn_scan = new System.Windows.Forms.Button();
            this.tb_rx = new System.Windows.Forms.RichTextBox();
            this.tb_tx = new System.Windows.Forms.RichTextBox();
            this.cb_ports = new System.Windows.Forms.ComboBox();
            this.btn_open = new System.Windows.Forms.Button();
            this.btn_clear_tx = new System.Windows.Forms.Button();
            this.btn_clear_rx = new System.Windows.Forms.Button();
            this.cb_ports2 = new System.Windows.Forms.ComboBox();
            this.btn_close_file = new System.Windows.Forms.Button();
            this.tb_file_handle = new System.Windows.Forms.TextBox();
            this.tb_rx_string = new System.Windows.Forms.RichTextBox();
            this.btn_clear_rx_string = new System.Windows.Forms.Button();
            this.btn_enter_dfu = new System.Windows.Forms.Button();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.btn_dwd_fota = new System.Windows.Forms.Button();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.label1 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.groupBox1.SuspendLayout();
            this.groupBox2.SuspendLayout();
            this.SuspendLayout();
            // 
            // btn_scan
            // 
            this.btn_scan.Location = new System.Drawing.Point(111, 34);
            this.btn_scan.Name = "btn_scan";
            this.btn_scan.Size = new System.Drawing.Size(75, 23);
            this.btn_scan.TabIndex = 0;
            this.btn_scan.Text = "Scan";
            this.btn_scan.UseVisualStyleBackColor = true;
            this.btn_scan.Click += new System.EventHandler(this.btn_scan_Click);
            // 
            // tb_rx
            // 
            this.tb_rx.Font = new System.Drawing.Font("Consolas", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.tb_rx.Location = new System.Drawing.Point(354, 125);
            this.tb_rx.Name = "tb_rx";
            this.tb_rx.Size = new System.Drawing.Size(320, 266);
            this.tb_rx.TabIndex = 1;
            this.tb_rx.Text = "";
            // 
            // tb_tx
            // 
            this.tb_tx.Font = new System.Drawing.Font("Consolas", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.tb_tx.Location = new System.Drawing.Point(16, 125);
            this.tb_tx.Name = "tb_tx";
            this.tb_tx.Size = new System.Drawing.Size(320, 266);
            this.tb_tx.TabIndex = 2;
            this.tb_tx.Text = "";
            // 
            // cb_ports
            // 
            this.cb_ports.FormattingEnabled = true;
            this.cb_ports.Location = new System.Drawing.Point(16, 34);
            this.cb_ports.Name = "cb_ports";
            this.cb_ports.Size = new System.Drawing.Size(78, 21);
            this.cb_ports.TabIndex = 3;
            // 
            // btn_open
            // 
            this.btn_open.Location = new System.Drawing.Point(111, 81);
            this.btn_open.Name = "btn_open";
            this.btn_open.Size = new System.Drawing.Size(75, 23);
            this.btn_open.TabIndex = 4;
            this.btn_open.Text = "Open";
            this.btn_open.UseVisualStyleBackColor = true;
            this.btn_open.Click += new System.EventHandler(this.btn_open_Click);
            // 
            // btn_clear_tx
            // 
            this.btn_clear_tx.Location = new System.Drawing.Point(261, 397);
            this.btn_clear_tx.Name = "btn_clear_tx";
            this.btn_clear_tx.Size = new System.Drawing.Size(75, 23);
            this.btn_clear_tx.TabIndex = 6;
            this.btn_clear_tx.Text = "Clear";
            this.btn_clear_tx.UseVisualStyleBackColor = true;
            this.btn_clear_tx.Click += new System.EventHandler(this.btn_clear_tx_Click);
            // 
            // btn_clear_rx
            // 
            this.btn_clear_rx.Location = new System.Drawing.Point(599, 397);
            this.btn_clear_rx.Name = "btn_clear_rx";
            this.btn_clear_rx.Size = new System.Drawing.Size(75, 23);
            this.btn_clear_rx.TabIndex = 7;
            this.btn_clear_rx.Text = "Clear";
            this.btn_clear_rx.UseVisualStyleBackColor = true;
            this.btn_clear_rx.Click += new System.EventHandler(this.btn_clear_rx_Click);
            // 
            // cb_ports2
            // 
            this.cb_ports2.FormattingEnabled = true;
            this.cb_ports2.Location = new System.Drawing.Point(16, 83);
            this.cb_ports2.Name = "cb_ports2";
            this.cb_ports2.Size = new System.Drawing.Size(78, 21);
            this.cb_ports2.TabIndex = 17;
            // 
            // btn_close_file
            // 
            this.btn_close_file.Location = new System.Drawing.Point(6, 19);
            this.btn_close_file.Name = "btn_close_file";
            this.btn_close_file.Size = new System.Drawing.Size(75, 23);
            this.btn_close_file.TabIndex = 24;
            this.btn_close_file.Text = "Close File";
            this.btn_close_file.UseVisualStyleBackColor = true;
            this.btn_close_file.Click += new System.EventHandler(this.btn_close_file_Click);
            // 
            // tb_file_handle
            // 
            this.tb_file_handle.Location = new System.Drawing.Point(94, 19);
            this.tb_file_handle.Name = "tb_file_handle";
            this.tb_file_handle.Size = new System.Drawing.Size(100, 20);
            this.tb_file_handle.TabIndex = 25;
            // 
            // tb_rx_string
            // 
            this.tb_rx_string.Font = new System.Drawing.Font("Consolas", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.tb_rx_string.Location = new System.Drawing.Point(695, 125);
            this.tb_rx_string.Name = "tb_rx_string";
            this.tb_rx_string.Size = new System.Drawing.Size(320, 266);
            this.tb_rx_string.TabIndex = 26;
            this.tb_rx_string.Text = "";
            // 
            // btn_clear_rx_string
            // 
            this.btn_clear_rx_string.Location = new System.Drawing.Point(940, 397);
            this.btn_clear_rx_string.Name = "btn_clear_rx_string";
            this.btn_clear_rx_string.Size = new System.Drawing.Size(75, 23);
            this.btn_clear_rx_string.TabIndex = 27;
            this.btn_clear_rx_string.Text = "Clear";
            this.btn_clear_rx_string.UseVisualStyleBackColor = true;
            this.btn_clear_rx_string.Click += new System.EventHandler(this.btn_clear_rx_string_Click);
            // 
            // btn_enter_dfu
            // 
            this.btn_enter_dfu.Location = new System.Drawing.Point(6, 24);
            this.btn_enter_dfu.Name = "btn_enter_dfu";
            this.btn_enter_dfu.Size = new System.Drawing.Size(75, 23);
            this.btn_enter_dfu.TabIndex = 28;
            this.btn_enter_dfu.Text = "Enter DFU Mode";
            this.btn_enter_dfu.UseVisualStyleBackColor = true;
            this.btn_enter_dfu.Click += new System.EventHandler(this.btn_enter_dfu_Click);
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.btn_dwd_fota);
            this.groupBox1.Controls.Add(this.btn_close_file);
            this.groupBox1.Controls.Add(this.tb_file_handle);
            this.groupBox1.Location = new System.Drawing.Point(354, 18);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(200, 100);
            this.groupBox1.TabIndex = 29;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Quectel Files commands";
            // 
            // btn_dwd_fota
            // 
            this.btn_dwd_fota.Location = new System.Drawing.Point(6, 59);
            this.btn_dwd_fota.Name = "btn_dwd_fota";
            this.btn_dwd_fota.Size = new System.Drawing.Size(75, 23);
            this.btn_dwd_fota.TabIndex = 26;
            this.btn_dwd_fota.Text = "Dwd FOTA";
            this.btn_dwd_fota.UseVisualStyleBackColor = true;
            this.btn_dwd_fota.Click += new System.EventHandler(this.btn_dwd_fota_Click);
            // 
            // groupBox2
            // 
            this.groupBox2.Controls.Add(this.btn_enter_dfu);
            this.groupBox2.Location = new System.Drawing.Point(695, 18);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(200, 100);
            this.groupBox2.TabIndex = 30;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "UART app commands";
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(13, 18);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(54, 13);
            this.label1.TabIndex = 31;
            this.label1.Text = "nRF COM";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(13, 68);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(71, 13);
            this.label2.TabIndex = 32;
            this.label2.Text = "Quectel COM";
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(1136, 450);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.groupBox2);
            this.Controls.Add(this.groupBox1);
            this.Controls.Add(this.btn_clear_rx_string);
            this.Controls.Add(this.tb_rx_string);
            this.Controls.Add(this.cb_ports2);
            this.Controls.Add(this.btn_clear_rx);
            this.Controls.Add(this.btn_clear_tx);
            this.Controls.Add(this.btn_open);
            this.Controls.Add(this.cb_ports);
            this.Controls.Add(this.tb_tx);
            this.Controls.Add(this.tb_rx);
            this.Controls.Add(this.btn_scan);
            this.Name = "Form1";
            this.Text = "Serial Port Bridge";
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.groupBox2.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button btn_scan;
        private System.Windows.Forms.RichTextBox tb_rx;
        private System.Windows.Forms.RichTextBox tb_tx;
        private System.Windows.Forms.ComboBox cb_ports;
        private System.Windows.Forms.Button btn_open;
        private System.Windows.Forms.Button btn_clear_tx;
        private System.Windows.Forms.Button btn_clear_rx;
        private System.Windows.Forms.ComboBox cb_ports2;
        private System.Windows.Forms.Button btn_close_file;
        private System.Windows.Forms.TextBox tb_file_handle;
        private System.Windows.Forms.RichTextBox tb_rx_string;
        private System.Windows.Forms.Button btn_clear_rx_string;
        private System.Windows.Forms.Button btn_enter_dfu;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Button btn_dwd_fota;
    }
}

