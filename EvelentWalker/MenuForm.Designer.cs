namespace EvelentWalker
{
    partial class MenuForm
    {
        private System.ComponentModel.IContainer components = null;

        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(MenuForm));
            this.RPFExplorerButton = new EvelentWalker.EvelentButton();
            this.RPFBrowserButton = new EvelentWalker.EvelentButton();
            this.WorldButton = new EvelentWalker.EvelentButton();
            this.ProjectButton = new EvelentWalker.EvelentButton();
            this.ExtractKeysButton = new EvelentWalker.EvelentButton();
            this.ExtractScriptsButton = new EvelentWalker.EvelentButton();
            this.ExtractTexturesButton = new EvelentWalker.EvelentButton();
            this.ExtractRawFilesButton = new EvelentWalker.EvelentButton();
            this.ExtractShadersButton = new EvelentWalker.EvelentButton();
            this.BinarySearchButton = new EvelentWalker.EvelentButton();
            this.JenkGenButton = new EvelentWalker.EvelentButton();
            this.JenkIndButton = new EvelentWalker.EvelentButton();
            this.GCCollectButton = new EvelentWalker.EvelentButton();
            this.AboutButton = new EvelentWalker.EvelentButton();
            this.SuspendLayout();
            // 
            // button click handlers
            // 
            this.RPFExplorerButton.Click += new System.EventHandler(this.RPFExplorerButton_Click);
            this.RPFBrowserButton.Click += new System.EventHandler(this.RPFBrowserButton_Click);
            this.WorldButton.Click += new System.EventHandler(this.WorldButton_Click);
            this.ProjectButton.Click += new System.EventHandler(this.ProjectButton_Click);
            this.ExtractKeysButton.Click += new System.EventHandler(this.ExtractKeysButton_Click);
            this.ExtractScriptsButton.Click += new System.EventHandler(this.ExtractScriptsButton_Click);
            this.ExtractTexturesButton.Click += new System.EventHandler(this.ExtractTexturesButton_Click);
            this.ExtractRawFilesButton.Click += new System.EventHandler(this.ExtractRawFilesButton_Click);
            this.ExtractShadersButton.Click += new System.EventHandler(this.ExtractShadersButton_Click);
            this.BinarySearchButton.Click += new System.EventHandler(this.BinarySearchButton_Click);
            this.JenkGenButton.Click += new System.EventHandler(this.JenkGenButton_Click);
            this.JenkIndButton.Click += new System.EventHandler(this.JenkIndButton_Click);
            this.GCCollectButton.Click += new System.EventHandler(this.GCCollectButton_Click);
            this.AboutButton.Click += new System.EventHandler(this.AboutButton_Click);
            // 
            // MenuForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(700, 680);
            this.Font = new System.Drawing.Font("Segoe UI", 9F);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Name = "MenuForm";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "EvelentWalker";
            this.FormClosed += new System.Windows.Forms.FormClosedEventHandler(this.MainForm_FormClosed);
            this.Load += new System.EventHandler(this.MainForm_Load);
            this.ResumeLayout(false);
        }

        #endregion

        private EvelentWalker.EvelentButton ExtractScriptsButton;
        private EvelentWalker.EvelentButton BinarySearchButton;
        private EvelentWalker.EvelentButton RPFBrowserButton;
        private EvelentWalker.EvelentButton WorldButton;
        private EvelentWalker.EvelentButton ExtractTexturesButton;
        private EvelentWalker.EvelentButton GCCollectButton;
        private EvelentWalker.EvelentButton ExtractRawFilesButton;
        private EvelentWalker.EvelentButton ExtractShadersButton;
        private EvelentWalker.EvelentButton AboutButton;
        private EvelentWalker.EvelentButton JenkGenButton;
        private EvelentWalker.EvelentButton ExtractKeysButton;
        private EvelentWalker.EvelentButton ProjectButton;
        private EvelentWalker.EvelentButton JenkIndButton;
        private EvelentWalker.EvelentButton RPFExplorerButton;
    }
}
