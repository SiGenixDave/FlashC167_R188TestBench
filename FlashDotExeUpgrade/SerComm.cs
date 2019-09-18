using System;
using System.IO.Ports;
using System.Runtime.InteropServices;

namespace FlashC167
{
    /// <summary>
    /// Responsible for initializing and writing to the user selected serial port
    /// </summary>
    internal class SerComm
    {
        private SerialPort serialPort;

        public delegate void InitDelegate (UInt16 aComPort, UInt16 aBaudRate);

        public delegate void TxCharDelegate (Byte aTxByte);

        public delegate Int32 RxCharDelegate ();

        private InitDelegate InitDG;
        private TxCharDelegate TxCharDG;
        private RxCharDelegate RxCharDG;

        [DllImport ("FlashSourcesDLL.dll")]
        public static extern void SetInitComCallback (InitDelegate func);

        [DllImport ("FlashSourcesDLL.dll")]
        public static extern void SetTxCharCallback (TxCharDelegate fn);

        [DllImport ("FlashSourcesDLL.dll")]
        public static extern void SetRxCharCallback (RxCharDelegate fn);

        // Call this from program.cs
        public void SerialDLLInit ()
        {
            InitDG = new InitDelegate (Init);
            SetInitComCallback (InitDG);
            TxCharDG = new TxCharDelegate (Putc);
            SetTxCharCallback (TxCharDG);
            RxCharDG = new RxCharDelegate (Getc);
            SetRxCharCallback (RxCharDG);
        }

        // This is called from the DLL after desired com port and baud rate is
        // decoded from command line arguments
        public void Init (UInt16 aComPort, UInt16 aBaudRate)
        {
            String comStr = "COM" + aComPort.ToString ();
            serialPort = new SerialPort (comStr, (Int32)aBaudRate, Parity.None, 8, StopBits.One);
            serialPort.Open ();
            serialPort.ReadTimeout = 20;
        }

        public void Close ()
        {
            serialPort.Close ();
        }

        public void Putc (Byte tx)
        {
            Byte[] txData = { 0 };
            txData[0] = tx;
            serialPort.Write (txData, 0, 1);
        }

        public Int32 Getc ()
        {
            Int32 rx;
            try
            {
                rx = serialPort.ReadByte ();
            }
            catch
            {
                rx = -1;
            }
            return rx;
        }
    }
}