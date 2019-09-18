using System;
using System.Runtime.InteropServices;

namespace FlashC167
{
    internal class Program
    {
        [DllImport("FlashSourcesDLL.dll")]
        public static extern Int32 FlashMain(Int32 argc, String[] argv);

        // .NET came into play to support faster downloads with XP machines. All legacy code
        //  wrapped in a DLL. Serial ports were changed over to .NET to increase download speed.
        private static void Main(string[] args)
        {
            String resource1 = "FlashC167.Resources.FlashSourcesDLL.dll";
            EmbeddedAssembly.Load(resource1, "FlashSourcesDLL.dll");

            SerComm sc = new SerComm();
            StageFile sf = new StageFile();

            sc.SerialDLLInit();

            // MVB boards are designed differently such that a different access to RAM and FLASH are needed in the
            // C167; therefore stage2x is used for those type boards (such as the MVB)
            Boolean useStage2MV = false;
            Boolean useStage2IP = false;
            foreach (String s in args)
            {
                if (s.ToUpper() == "MVB")
                {
                    useStage2MV = true;
                    break;
                }
                else if (s.ToUpper() == "IPACK2")
                {
                    useStage2IP = true;
                }
            }

            sf.Init(useStage2MV, useStage2IP);

            // The last argument is always the Path to where Flash_Result.txt will be written
            string PathFileName = args[args.Length - 1];

            // Link to legacy code via DLL call
            // Since the last arg (Path) is exclusively used here, just subtract 1 from the args length so that the DLL
            // doesn't even know about it
            Int32 retVal = FlashMain((Int32)args.Length - 1, args);

            // Update error code file with the return value
            string retValText = "1[" + retVal.ToString("D3") + "]" + System.Environment.NewLine;

            System.IO.File.WriteAllText(PathFileName, retValText);

            sc.Close();

            Console.WriteLine("Press any key to continue...");
            Console.ReadKey();
        }
    }
}