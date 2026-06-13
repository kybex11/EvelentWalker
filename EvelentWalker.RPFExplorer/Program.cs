using EvelentWalker.GameFiles;
using EvelentWalker.Properties;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace EvelentWalker.RPFExplorer
{
    static class Program
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main()
        {
            //Process.Start("EvelentWalker.exe", "explorer");

            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new ExploreForm());

            GTAFolder.UpdateSettings();
        }
    }
}
