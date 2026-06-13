using EvelentWalker.Properties;
using EvelentWalker.Tools;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Text;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace EvelentWalker
{
    public partial class MenuForm : Form
    {
        private volatile bool worldFormOpen = false;
        private WorldForm worldForm = null;

        public MenuForm()
        {
            InitializeComponent();
            BuildModernLayout();
            EvelentTheme.Apply(this);
        }

        /// <summary>
        /// Builds the launcher UI: a branded gradient sidebar plus a content area
        /// with grouped, icon-led action buttons. All buttons reuse the existing
        /// click handlers so behaviour is unchanged.
        /// </summary>
        private void BuildModernLayout()
        {
            MinimumSize = new Size(700, 600);
            BackColor = EvelentTheme.WindowBack;
            Font = new Font("Segoe UI", 9f);

            // ---- branded sidebar ----
            var sidebar = new SidebarPanel { Dock = DockStyle.Left, Width = 210 };

            var logo = new Label
            {
                Text = "E",
                Font = new Font("Segoe UI", 46f, FontStyle.Bold),
                ForeColor = Color.White,
                BackColor = Color.Transparent,
                AutoSize = false,
                TextAlign = ContentAlignment.MiddleCenter,
                Bounds = new Rectangle(30, 36, 150, 90)
            };
            var title = new Label
            {
                Text = "EvelentWalker",
                Font = new Font("Segoe UI Semibold", 15f, FontStyle.Bold),
                ForeColor = Color.White,
                BackColor = Color.Transparent,
                AutoSize = false,
                TextAlign = ContentAlignment.MiddleCenter,
                Bounds = new Rectangle(10, 132, 190, 26)
            };
            var subtitle = new Label
            {
                Text = "GTA V MODDING TOOLKIT",
                Font = new Font("Segoe UI", 7.5f, FontStyle.Bold),
                ForeColor = Color.FromArgb(200, 220, 235, 255),
                BackColor = Color.Transparent,
                AutoSize = false,
                TextAlign = ContentAlignment.MiddleCenter,
                Bounds = new Rectangle(10, 160, 190, 18)
            };
            var version = new Label
            {
                Text = "v1.0",
                Font = new Font("Segoe UI", 8f),
                ForeColor = Color.FromArgb(170, 230, 240, 255),
                BackColor = Color.Transparent,
                AutoSize = false,
                TextAlign = ContentAlignment.MiddleCenter,
                Dock = DockStyle.Bottom,
                Height = 36
            };
            sidebar.Controls.Add(logo);
            sidebar.Controls.Add(title);
            sidebar.Controls.Add(subtitle);
            sidebar.Controls.Add(version);

            // ---- content area ----
            var content = new Panel
            {
                Dock = DockStyle.Fill,
                BackColor = EvelentTheme.WindowBack,
                Padding = new Padding(28, 24, 28, 24),
                AutoScroll = true
            };

            var heading = new Label
            {
                Text = "Welcome back",
                Font = new Font("Segoe UI Semibold", 16f, FontStyle.Bold),
                ForeColor = EvelentTheme.TextColor,
                AutoSize = true,
                Location = new Point(28, 22)
            };
            var sub = new Label
            {
                Text = "Choose a tool to get started.",
                Font = new Font("Segoe UI", 9.5f),
                ForeColor = EvelentTheme.DisabledText,
                AutoSize = true,
                Location = new Point(30, 54)
            };
            content.Controls.Add(heading);
            content.Controls.Add(sub);

            int top = 92;
            top = AddSection(content, "Explore", top, new[]
            {
                (RPFExplorerButton, "\uEC50", "RPF Explorer", true),
                (RPFBrowserButton, "\uE71C", "RPF Browser", false),
                (WorldButton, "\uE909", "World View", false),
                (ProjectButton, "\uE8F4", "Project", false),
            });

            top = AddSection(content, "Extract", top, new[]
            {
                (ExtractKeysButton, "\uE192", "Extract Keys", false),
                (ExtractScriptsButton, "\uE943", "Extract Scripts", false),
                (ExtractTexturesButton, "\uEB9F", "Extract Textures", false),
                (ExtractRawFilesButton, "\uE7C3", "Extract Raw Files", false),
                (ExtractShadersButton, "\uE790", "Extract Shaders", false),
            });

            top = AddSection(content, "Tools", top, new[]
            {
                (BinarySearchButton, "\uE721", "Binary Search", false),
                (JenkGenButton, "\uE950", "JenkGen", false),
                (JenkIndButton, "\uE71D", "JenkInd", false),
                (GCCollectButton, "\uE74D", "GC Collect", false),
                (AboutButton, "\uE946", "About", false),
            });

            Controls.Add(content);
            Controls.Add(sidebar);
        }

        private int AddSection(Panel host, string title, int top,
            (EvelentButton btn, string glyph, string text, bool accent)[] items)
        {
            var label = new EvelentSectionLabel
            {
                Text = title,
                Location = new Point(28, top),
                Size = new Size(360, 22)
            };
            host.Controls.Add(label);
            top += 30;

            const int colW = 176, gap = 12, cols = 2, btnH = 40;
            for (int i = 0; i < items.Length; i++)
            {
                var it = items[i];
                int col = i % cols;
                int row = i / cols;
                var b = it.btn;
                b.Glyph = it.glyph;
                b.Text = it.text;
                b.Accent = it.accent;
                b.Size = new Size(colW, btnH);
                b.Location = new Point(28 + col * (colW + gap), top + row * (btnH + gap));
                b.TextAlign = ContentAlignment.MiddleLeft;
                host.Controls.Add(b);
                b.BringToFront();
            }
            int rows = (items.Length + cols - 1) / cols;
            return top + rows * (btnH + gap) + 14;
        }

        /// <summary>Gradient sidebar background used by the launcher.</summary>
        private sealed class SidebarPanel : Panel
        {
            public SidebarPanel()
            {
                SetStyle(ControlStyles.AllPaintingInWmPaint | ControlStyles.OptimizedDoubleBuffer |
                         ControlStyles.UserPaint | ControlStyles.ResizeRedraw, true);
            }
            protected override void OnPaint(PaintEventArgs e)
            {
                var g = e.Graphics;
                g.SmoothingMode = SmoothingMode.AntiAlias;
                using (var brush = new LinearGradientBrush(ClientRectangle,
                    Color.FromArgb(96, 132, 246), Color.FromArgb(124, 92, 220), 60f))
                {
                    g.FillRectangle(brush, ClientRectangle);
                }
                // logo plate
                var plate = new Rectangle(45, 34, 120, 96);
                using (var path = EvelentButton.Rounded(plate, 18))
                using (var b = new SolidBrush(Color.FromArgb(60, 255, 255, 255)))
                using (var p = new Pen(Color.FromArgb(120, 255, 255, 255)))
                {
                    g.FillPath(b, path);
                    g.DrawPath(p, path);
                }
            }
        }

        private void MainForm_Load(object sender, EventArgs e)
        {
        }

        private void MainForm_FormClosed(object sender, FormClosedEventArgs e)
        {
            Settings.Default.Save();
        }

        private void RPFExplorerButton_Click(object sender, EventArgs e)
        {
            ExploreForm f = new ExploreForm();
            f.Show(this);
        }

        private void RPFBrowserButton_Click(object sender, EventArgs e)
        {
            BrowseForm f = new BrowseForm();
            f.Show(this);
        }

        private void ExtractScriptsButton_Click(object sender, EventArgs e)
        {
            ExtractScriptsForm f = new ExtractScriptsForm();
            f.Show(this);
        }

        private void ExtractTexturesButton_Click(object sender, EventArgs e)
        {
            ExtractTexForm f = new ExtractTexForm();
            f.Show(this);
        }

        private void ExtractRawFilesButton_Click(object sender, EventArgs e)
        {
            ExtractRawForm f = new ExtractRawForm();
            f.Show(this);
        }

        private void ExtractShadersButton_Click(object sender, EventArgs e)
        {
            ExtractShadersForm f = new ExtractShadersForm();
            f.Show(this);
        }

        private void BinarySearchButton_Click(object sender, EventArgs e)
        {
            BinarySearchForm f = new BinarySearchForm();
            f.Show(this);
        }

        private void WorldButton_Click(object sender, EventArgs e)
        {
            if (worldFormOpen)
            {
                //MessageBox.Show("Can only open one world view at a time.");
                if (worldForm != null)
                {
                    worldForm.Invoke(new Action(() => { worldForm.Focus(); }));
                }
                return;
            }

            Thread thread = new Thread(new ThreadStart(() => {
                try
                {
                    worldFormOpen = true;
                    using (WorldForm f = new WorldForm())
                    {
                        worldForm = f;
                        f.ShowDialog();
                        worldForm = null;
                    }
                    worldFormOpen = false;
                }
                catch
                {
                    worldFormOpen = false;
                }
            }));
            thread.Start();
        }

        private void GCCollectButton_Click(object sender, EventArgs e)
        {
            GC.Collect();
        }

        private void AboutButton_Click(object sender, EventArgs e)
        {
            AboutForm f = new AboutForm();
            f.Show(this);
        }

        private void JenkGenButton_Click(object sender, EventArgs e)
        {
            JenkGenForm f = new JenkGenForm();
            f.Show(this);
        }

        private void JenkIndButton_Click(object sender, EventArgs e)
        {
            JenkIndForm f = new JenkIndForm();
            f.Show(this);
        }

        private void ExtractKeysButton_Click(object sender, EventArgs e)
        {
            ExtractKeysForm f = new ExtractKeysForm();
            f.Show(this);
        }

        private void ProjectButton_Click(object sender, EventArgs e)
        {
            Project.ProjectForm f = new Project.ProjectForm(null);
            f.Show(this);
        }
    }
}
