using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace EvelentWalker.Tools
{
    public partial class AboutForm : Form
    {
        public AboutForm()
        {
            InitializeComponent();
            try { EvelentTheme.Apply(this); } catch { }
        }

        private void OkButton_Click(object sender, EventArgs e)
        {
            Close();
        }
    }
}
