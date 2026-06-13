using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Text;
using System.Linq;
using System.Windows.Forms;

namespace EvelentWalker
{
    /// <summary>
    /// A modern, Blender-style keyboard-shortcuts reference. Shows every binding
    /// grouped by category with searchable rows and key "chips". Read-only;
    /// bindings themselves are still configured from the Settings form.
    /// </summary>
    public class ShortcutsForm : Form
    {
        private readonly List<Section> sections = new List<Section>();
        private Panel listPanel;
        private TextBox searchBox;

        private sealed class Shortcut
        {
            public string Desc;
            public string Keys;
        }
        private sealed class Section
        {
            public string Title;
            public string Glyph;
            public List<Shortcut> Items = new List<Shortcut>();
        }

        public ShortcutsForm(KeyBindings kb)
        {
            if (kb == null) kb = new KeyBindings(Properties.Settings.Default.KeyBindings);
            BuildData(kb);
            BuildUi();
            try { EvelentTheme.Apply(this); } catch { }
            Populate(null);
        }

        private static string K(Keys k) => k.ToString();

        private void BuildData(KeyBindings kb)
        {
            var nav = new Section { Title = "Navigation", Glyph = "\uE7AA" };
            nav.Items.Add(new Shortcut { Desc = "Move forward", Keys = K(kb.MoveForward) });
            nav.Items.Add(new Shortcut { Desc = "Move backward", Keys = K(kb.MoveBackward) });
            nav.Items.Add(new Shortcut { Desc = "Move left", Keys = K(kb.MoveLeft) });
            nav.Items.Add(new Shortcut { Desc = "Move right", Keys = K(kb.MoveRight) });
            nav.Items.Add(new Shortcut { Desc = "Move up", Keys = K(kb.MoveUp) });
            nav.Items.Add(new Shortcut { Desc = "Move down", Keys = K(kb.MoveDown) });
            nav.Items.Add(new Shortcut { Desc = "Move slower / zoom in", Keys = K(kb.MoveSlowerZoomIn) });
            nav.Items.Add(new Shortcut { Desc = "Move faster / zoom out", Keys = K(kb.MoveFasterZoomOut) });
            nav.Items.Add(new Shortcut { Desc = "First-person / ped mode", Keys = K(kb.FirstPerson) });
            nav.Items.Add(new Shortcut { Desc = "Jump (ped mode)", Keys = K(kb.Jump) });
            nav.Items.Add(new Shortcut { Desc = "Exit first-person mode", Keys = "Esc" });
            sections.Add(nav);

            var edit = new Section { Title = "Editing", Glyph = "\uE70F" };
            edit.Items.Add(new Shortcut { Desc = "Toggle mouse select", Keys = K(kb.ToggleMouseSelect) });
            edit.Items.Add(new Shortcut { Desc = "Move (position widget)", Keys = K(kb.EditPosition) });
            edit.Items.Add(new Shortcut { Desc = "Rotate (rotation widget)", Keys = K(kb.EditRotation) });
            edit.Items.Add(new Shortcut { Desc = "Scale (scale widget)", Keys = K(kb.EditScale) });
            edit.Items.Add(new Shortcut { Desc = "Default mode / toggle widget space", Keys = K(kb.ExitEditMode) });
            edit.Items.Add(new Shortcut { Desc = "Delete selected item", Keys = "Delete" });
            edit.Items.Add(new Shortcut { Desc = "Create node link (path mode)", Keys = "J" });
            edit.Items.Add(new Shortcut { Desc = "Create node shortcut (path mode)", Keys = "K" });
            sections.Add(edit);

            var file = new Section { Title = "Project & files", Glyph = "\uE74E" };
            file.Items.Add(new Shortcut { Desc = "New", Keys = "Ctrl + N" });
            file.Items.Add(new Shortcut { Desc = "Open", Keys = "Ctrl + O" });
            file.Items.Add(new Shortcut { Desc = "Save", Keys = "Ctrl + S" });
            file.Items.Add(new Shortcut { Desc = "Save all", Keys = "Ctrl + Shift + S" });
            file.Items.Add(new Shortcut { Desc = "Undo", Keys = "Ctrl + Z" });
            file.Items.Add(new Shortcut { Desc = "Redo", Keys = "Ctrl + Y" });
            file.Items.Add(new Shortcut { Desc = "Copy", Keys = "Ctrl + C" });
            file.Items.Add(new Shortcut { Desc = "Paste", Keys = "Ctrl + V" });
            sections.Add(file);

            var view = new Section { Title = "Interface", Glyph = "\uE7B3" };
            view.Items.Add(new Shortcut { Desc = "Toggle toolbar", Keys = K(kb.ToggleToolbar) });
            view.Items.Add(new Shortcut { Desc = "This shortcuts page", Keys = "F1" });
            sections.Add(view);
        }

        private void BuildUi()
        {
            Text = "Keyboard Shortcuts";
            StartPosition = FormStartPosition.CenterParent;
            FormBorderStyle = FormBorderStyle.Sizable;
            MinimizeBox = false;
            MaximizeBox = true;
            ClientSize = new Size(560, 680);
            MinimumSize = new Size(440, 420);
            BackColor = EvelentTheme.WindowBack;
            Font = new Font("Segoe UI", 9f);
            KeyPreview = true;
            KeyDown += (s, e) => { if (e.KeyCode == Keys.Escape || e.KeyCode == Keys.F1) Close(); };

            var header = new HeaderPanel { Dock = DockStyle.Top, Height = 96 };

            var title = new Label
            {
                Text = "Keyboard Shortcuts",
                Font = new Font("Segoe UI Semibold", 17f, FontStyle.Bold),
                ForeColor = Color.White,
                BackColor = Color.Transparent,
                AutoSize = true,
                Location = new Point(24, 18)
            };
            var subtitle = new Label
            {
                Text = "Work fast, the Blender way. Press F1 anytime to open this page.",
                Font = new Font("Segoe UI", 9f),
                ForeColor = Color.FromArgb(220, 235, 245, 255),
                BackColor = Color.Transparent,
                AutoSize = true,
                Location = new Point(26, 52)
            };
            header.Controls.Add(title);
            header.Controls.Add(subtitle);

            var searchHost = new Panel { Dock = DockStyle.Top, Height = 52, Padding = new Padding(20, 10, 20, 8), BackColor = EvelentTheme.WindowBack };
            searchBox = new TextBox
            {
                Dock = DockStyle.Fill,
                BorderStyle = BorderStyle.FixedSingle,
                BackColor = EvelentTheme.InputBack,
                ForeColor = EvelentTheme.TextColor,
                Font = new Font("Segoe UI", 10f)
            };
            SetCue(searchBox, "Search shortcuts...");
            searchBox.TextChanged += (s, e) => Populate(searchBox.Text);
            searchHost.Controls.Add(searchBox);

            listPanel = new Panel
            {
                Dock = DockStyle.Fill,
                AutoScroll = true,
                BackColor = EvelentTheme.WindowBack,
                Padding = new Padding(20, 8, 20, 20)
            };

            Controls.Add(listPanel);
            Controls.Add(searchHost);
            Controls.Add(header);
        }

        private void Populate(string filter)
        {
            listPanel.SuspendLayout();
            listPanel.Controls.Clear();
            filter = (filter ?? "").Trim();
            bool hasFilter = filter.Length > 0;

            int width = listPanel.ClientSize.Width - listPanel.Padding.Horizontal;
            if (width < 200) width = 200;
            int y = 8;

            foreach (var sec in sections)
            {
                var items = hasFilter
                    ? sec.Items.Where(i => i.Desc.IndexOf(filter, StringComparison.OrdinalIgnoreCase) >= 0
                                        || i.Keys.IndexOf(filter, StringComparison.OrdinalIgnoreCase) >= 0).ToList()
                    : sec.Items;
                if (items.Count == 0) continue;

                var card = new EvelentCard
                {
                    Location = new Point(listPanel.Padding.Left, y),
                    Width = width,
                    Anchor = AnchorStyles.Left | AnchorStyles.Top | AnchorStyles.Right
                };

                var head = new EvelentSectionLabel
                {
                    Text = sec.Title,
                    Location = new Point(14, 12),
                    Size = new Size(width - 28, 22)
                };
                card.Controls.Add(head);

                int ry = 42;
                foreach (var it in items)
                {
                    var row = new ShortcutRow(it.Desc, it.Keys)
                    {
                        Location = new Point(12, ry),
                        Size = new Size(width - 24, 32),
                        Anchor = AnchorStyles.Left | AnchorStyles.Top | AnchorStyles.Right
                    };
                    card.Controls.Add(row);
                    ry += 34;
                }
                card.Height = ry + 8;
                listPanel.Controls.Add(card);
                y += card.Height + 14;
            }

            listPanel.ResumeLayout();
        }

        // --- cue banner (placeholder text) ---
        [System.Runtime.InteropServices.DllImport("user32.dll", CharSet = System.Runtime.InteropServices.CharSet.Unicode)]
        private static extern IntPtr SendMessage(IntPtr hWnd, int msg, IntPtr wParam, string lParam);
        private static void SetCue(TextBox tb, string cue)
        {
            void apply() { try { SendMessage(tb.Handle, 0x1501, (IntPtr)1, cue); } catch { } }
            if (tb.IsHandleCreated) apply(); else tb.HandleCreated += (s, e) => apply();
        }

        /// <summary>Gradient banner used at the top of the shortcuts window.</summary>
        private sealed class HeaderPanel : Panel
        {
            public HeaderPanel()
            {
                SetStyle(ControlStyles.AllPaintingInWmPaint | ControlStyles.OptimizedDoubleBuffer |
                         ControlStyles.UserPaint | ControlStyles.ResizeRedraw, true);
            }
            protected override void OnPaint(PaintEventArgs e)
            {
                var g = e.Graphics;
                using (var brush = new LinearGradientBrush(ClientRectangle,
                    Color.FromArgb(96, 132, 246), Color.FromArgb(124, 92, 220), 25f))
                    g.FillRectangle(brush, ClientRectangle);
            }
        }

        /// <summary>A single description + key-chip row.</summary>
        private sealed class ShortcutRow : Control
        {
            private readonly string desc;
            private readonly string keys;
            private bool hover;

            public ShortcutRow(string desc, string keys)
            {
                this.desc = desc;
                this.keys = keys;
                SetStyle(ControlStyles.AllPaintingInWmPaint | ControlStyles.OptimizedDoubleBuffer |
                         ControlStyles.UserPaint | ControlStyles.ResizeRedraw, true);
                Font = new Font("Segoe UI", 9.5f);
            }

            protected override void OnMouseEnter(EventArgs e) { hover = true; Invalidate(); base.OnMouseEnter(e); }
            protected override void OnMouseLeave(EventArgs e) { hover = false; Invalidate(); base.OnMouseLeave(e); }

            protected override void OnPaint(PaintEventArgs e)
            {
                var g = e.Graphics;
                g.SmoothingMode = SmoothingMode.AntiAlias;
                g.TextRenderingHint = TextRenderingHint.ClearTypeGridFit;

                var r = ClientRectangle;
                if (hover)
                {
                    using (var hb = new SolidBrush(EvelentTheme.AccentSoft))
                    using (var path = EvelentButton.Rounded(new Rectangle(r.X, r.Y, r.Width - 1, r.Height - 1), 6))
                        g.FillPath(hb, path);
                }

                // description on the left
                TextRenderer.DrawText(g, desc, Font, new Rectangle(10, 0, r.Width - 160, r.Height),
                    EvelentTheme.TextColor, TextFormatFlags.VerticalCenter | TextFormatFlags.Left | TextFormatFlags.EndEllipsis);

                // key chips on the right
                var parts = keys.Split(new[] { " + " }, StringSplitOptions.RemoveEmptyEntries);
                var chipFont = new Font("Segoe UI Semibold", 8.5f, FontStyle.Bold);
                int chipH = 22;
                int x = r.Right - 6;
                for (int i = parts.Length - 1; i >= 0; i--)
                {
                    string t = parts[i];
                    var sz = TextRenderer.MeasureText(g, t, chipFont);
                    int w = sz.Width + 16;
                    var chip = new Rectangle(x - w, (r.Height - chipH) / 2, w, chipH);
                    using (var path = EvelentButton.Rounded(chip, 5))
                    using (var b = new SolidBrush(EvelentTheme.InputBack))
                    using (var p = new Pen(EvelentTheme.BorderColor))
                    {
                        g.FillPath(b, path);
                        g.DrawPath(p, path);
                    }
                    TextRenderer.DrawText(g, t, chipFont, chip, EvelentTheme.TextColor,
                        TextFormatFlags.HorizontalCenter | TextFormatFlags.VerticalCenter);
                    x -= w + 6;
                }
                chipFont.Dispose();
            }
        }
    }
}
