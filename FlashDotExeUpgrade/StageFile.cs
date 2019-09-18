using System;
using System.Runtime.InteropServices;
using System.Text;

namespace FlashC167
{
    /// <summary>
    /// Responsible for initializing reading and providing access to the stage*.hex files
    /// </summary>
    internal class StageFile
    {
        [DllImport ("FlashSourcesDLL.dll")]
        public static extern void CopyStage1HexData (StringBuilder file, Int32 size);

        [DllImport ("FlashSourcesDLL.dll")]
        public static extern void CopyStage2HexData (StringBuilder file, Int32 size);

        [DllImport ("FlashSourcesDLL.dll")]
        public static extern void CopyStage3HexData (StringBuilder file, Int32 size);

        private String stage1Content;
        private String stage2Content;
        private String stage3Content;

        public void Init (Boolean aUseStage2MV, Boolean aUseStage2IP)
        {
            // The stage1, stage2 and stage3 hex files are now embedded resources
            stage1Content = Encoding.UTF8.GetString (Properties.Resources.STAGE1, 0, Properties.Resources.STAGE1.Length);
            if (aUseStage2MV == true)
            {
                stage2Content = Encoding.UTF8.GetString (Properties.Resources.STAGE2MV, 0, Properties.Resources.STAGE2MV.Length);
            }
            else if (aUseStage2IP == true)
            {
                stage2Content = Encoding.UTF8.GetString (Properties.Resources.STAGE2IP, 0, Properties.Resources.STAGE2IP.Length);
            }
            else
            {
                stage2Content = Encoding.UTF8.GetString (Properties.Resources.STAGE2, 0, Properties.Resources.STAGE2.Length);
            }
            stage3Content = Encoding.UTF8.GetString (Properties.Resources.STAGE3, 0, Properties.Resources.STAGE3.Length);

            StringBuilder sb1 = new StringBuilder (stage1Content);
            CopyStage1HexData (sb1, sb1.Length);

            StringBuilder sb2 = new StringBuilder (stage2Content);
            CopyStage2HexData (sb2, sb2.Length);

            StringBuilder sb3 = new StringBuilder (stage3Content);
            CopyStage3HexData (sb3, sb3.Length);
        }
    }
}