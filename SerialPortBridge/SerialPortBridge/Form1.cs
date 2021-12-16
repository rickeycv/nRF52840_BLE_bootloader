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
using System.IO.Ports;
using System.Text.RegularExpressions;
using System.IO;

namespace Terminal
{
    public partial class Form1 : Form
    {
        
        SerialPort serialport = new SerialPort();
        SerialPort serialport2 = new SerialPort();
        byte[] bytes_rx = new byte[2048];
 
        public Form1()
        {
            InitializeComponent();
            serialport.BaudRate = 115200;
            serialport.DataReceived += new SerialDataReceivedEventHandler(DataReceivedHandler);
            serialport2.BaudRate = 115200;
            serialport2.DataReceived += new SerialDataReceivedEventHandler(DataReceivedHandler);
        }

        private void btn_scan_Click(object sender, EventArgs e)
        {
            string[] ports = SerialPort.GetPortNames();
            cb_ports.Items.Clear();
            cb_ports2.Items.Clear();
            foreach (string port in ports)
            {
                cb_ports.Items.Add(port);
                cb_ports2.Items.Add(port);
            }
            cb_ports.SelectedIndex = 0;
            cb_ports2.SelectedIndex = cb_ports2.Items.Count - 1;
        }

        private void btn_open_Click(object sender, EventArgs e)
        {
            if (btn_open.Text == "Open")
            {
                if (!serialport.IsOpen && !serialport2.IsOpen)
                {
                    serialport.PortName = cb_ports.SelectedItem.ToString();
                    serialport2.PortName = cb_ports2.SelectedItem.ToString();
                    try
                    {
                        serialport.Open();
                        serialport2.Open();
                        btn_open.Text = "Close";
                    }
                    catch (Exception ex)
                    {
                        if (serialport.IsOpen)
                        {
                            serialport.Close();
                        }
                        if (serialport2.IsOpen)
                        {
                            serialport2.Close();
                        }
                        MessageBox.Show(ex.Message + " Close serial port first.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    }
                }
                else
                {
                    MessageBox.Show("Serial Port Already Open!");
                }
            }
            else
            {
                serialport.Close();
                serialport2.Close();
                btn_open.Text = "Open";
            }
        }

        private void DataReceivedHandler(
                    object sender,
                    SerialDataReceivedEventArgs e)
        {
            int data_read;
            int bytes_to_read;

            SerialPort sp = (SerialPort)sender;


            if (sp.PortName == serialport.PortName)
            {
                bytes_to_read = sp.BytesToRead;
                data_read = sp.Read(bytes_rx, 0, bytes_to_read);
                serialport2.Write(bytes_rx, 0, bytes_to_read);
            }
            else
            {
                bytes_to_read = sp.BytesToRead;
                data_read = sp.Read(bytes_rx, 0, bytes_to_read);
                serialport.Write(bytes_rx, 0, bytes_to_read);
            }


            if (InvokeRequired)
            {
                this.Invoke(new MethodInvoker(delegate
                {

                    tb_rx.AppendText(BitConverter.ToString(bytes_rx, 0, data_read).Replace("-", ""));

                    tb_rx_string.AppendText(Encoding.UTF8.GetString(bytes_rx, 0, data_read));
                    
                    tb_rx.ScrollToCaret();
                    tb_rx_string.ScrollToCaret();
                }
                ));
            }
            else
            {

                tb_rx.AppendText(BitConverter.ToString(bytes_rx, 0/*data_rx_written_in_tb*/, data_read).Replace("-", ""));

                tb_rx_string.AppendText(Encoding.UTF8.GetString(bytes_rx, 0, data_read));

                tb_rx.ScrollToCaret();
                tb_rx_string.ScrollToCaret();
            }

        }

        public static byte[] StringToByteArrayFastest(string hex)
        {
            byte[] arr = new byte[hex.Length >> 1];

            for (int i = 0; i < hex.Length >> 1; ++i)
            {
                arr[i] = (byte)((GetHexVal(hex[i << 1]) << 4) + (GetHexVal(hex[(i << 1) + 1])));
            }

            return arr;
        }

        public static int GetHexVal(char hex)
        {
            int val = (int)hex;
            //For uppercase A-F letters:
            //return val - (val < 58 ? 48 : 55);
            //For lowercase a-f letters:
            //return val - (val < 58 ? 48 : 87);
            //Or the two combined, but a bit slower:
            return val - (val < 58 ? 48 : (val < 97 ? 55 : 87));
        }

        private void btn_clear_tx_Click(object sender, EventArgs e)
        {
            tb_tx.Text = "";
        }

        private void btn_clear_rx_Click(object sender, EventArgs e)
        {
            tb_rx.Text = "";
        }

        private void btn_clear_rx_string_Click(object sender, EventArgs e)
        {
            tb_rx_string.Text = "";
        }


        private void btn_close_file_Click(object sender, EventArgs e)
        {
            if (serialport2.IsOpen)
            {
                serialport2.Write("AT+QFCLOSE=" + tb_file_handle.Text + "\r");
            }
            else
            {
                MessageBox.Show("Open Quectel's serial port first!");
            }
        }

        private void btn_enter_dfu_Click(object sender, EventArgs e)
        {
            if (serialport.IsOpen)
            {
                serialport.Write("e\r");
            }
            else
            {
                MessageBox.Show("Open Quectel's serial port first!");
            }           
        }

        private void btn_dwd_fota_Click(object sender, EventArgs e)
        {
            if (serialport2.IsOpen)
            {
                serialport2.Write("AT+QFDWL=\"FOTA.txt\"\r");
            }
            else
            {
                MessageBox.Show("Open Quectel's serial port first!");
            }
        }
    }
}
