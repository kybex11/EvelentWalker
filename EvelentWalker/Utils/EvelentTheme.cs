using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Text;
using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace EvelentWalker
{
    /// <summary>
    /// EvelentWalker UI theme helper.
    ///
    /// Applies a cohesive, modern dark colour scheme to WinForms controls, menus,
    /// toolbars and status bars across the whole application. In addition to simple
    /// recolouring it pulls in the native Windows dark-mode APIs so that the parts
    /// of the UI that can't be recoloured manually also look modern:
    ///   - dark window title bars (DWM immersive dark mode)
    ///   - dark, slim native scrollbars + modern selection on TreeView / ListView
    ///     (SetWindowTheme "DarkMode_Explorer")
    ///   - flat, owner-drawn tab strips
    ///
    /// Every form calls <see cref="Apply(Form)"/> from its constructor, so changing
    /// this single file restyles the entire app without touching individual forms.
    /// </summary>
    public static class EvelentTheme
    {
        // ---- Modern, premium dark palette (deep slate + soft indigo accent) ----
        public static readonly Color WindowBack = Color.FromArgb(26, 27, 38);    // window chrome
        public static readonly Color PanelBack = Color.FromArgb(33, 35, 54);     // panels / surfaces
        public static readonly Color InputBack = Color.FromArgb(41, 46, 66);     // editable surfaces
        public static readonly Color StripBack = Color.FromArgb(30, 31, 46);     // menus / toolbars
        public static readonly Color BorderColor = Color.FromArgb(53, 58, 84);   // subtle borders
        public static readonly Color TextColor = Color.FromArgb(198, 208, 230);  // primary text (readable)
        public static readonly Color DisabledText = Color.FromArgb(110, 116, 140);
        public static readonly Color Accent = Color.FromArgb(122, 162, 247);     // soft indigo/blue accent
        public static readonly Color AccentText = Color.FromArgb(240, 244, 255); // text on accent
        public static readonly Color AccentSoft = Color.FromArgb(45, 52, 78);    // gentle hover

        private static Icon appIcon;
        private static bool appIconTried;
        private static bool appModeInited;

        /// <summary>
        /// The application "E" logo icon, extracted from the executable.
        /// </summary>
        public static Icon AppIcon
        {
            get
            {
                if (!appIconTried)
                {
                    appIconTried = true;
                    try { appIcon = Icon.ExtractAssociatedIcon(Application.ExecutablePath); }
                    catch { appIcon = null; }
                }
                return appIcon;
            }
        }

        /// <summary>
        /// Apply the theme to a form and all of its child controls.
        /// </summary>
        public static void Apply(Form form)
        {
            if (form == null) return;

            EnsureAppDarkMode();

            form.BackColor = WindowBack;
            form.ForeColor = TextColor;

            var icon = AppIcon;
            if (icon != null)
            {
                try { form.Icon = icon; } catch { }
            }

            // Dark title bar. Must run once the native handle exists, and again if the
            // handle gets recreated, so wire it up as well as applying immediately.
            ApplyDarkTitleBar(form);
            form.HandleCreated -= FormHandleCreated;
            form.HandleCreated += FormHandleCreated;

            ApplyToControls(form);
            ApplyToComponents(form);
        }

        /// <summary>
        /// Theme components that live in the form's <c>components</c> container
        /// rather than its Controls tree - chiefly ContextMenuStrips, which would
        /// otherwise render with the default light Windows colours.
        /// </summary>
        private static void ApplyToComponents(Form form)
        {
            try
            {
                var field = form.GetType().GetField("components",
                    System.Reflection.BindingFlags.NonPublic | System.Reflection.BindingFlags.Instance);
                var container = field?.GetValue(form) as System.ComponentModel.IContainer;
                if (container == null) return;
                foreach (var comp in container.Components)
                {
                    if (comp is ToolStrip ts) // ContextMenuStrip / MenuStrip / ToolStrip
                    {
                        ts.Renderer = new EvelentToolStripRenderer();
                        ts.BackColor = StripBack;
                        ts.ForeColor = TextColor;
                    }
                }
            }
            catch { }
        }

        private static void FormHandleCreated(object sender, EventArgs e)
        {
            if (sender is Form f) ApplyDarkTitleBar(f);
        }

        /// <summary>
        /// Recursively style a control tree without changing the owning form's icon.
        /// </summary>
        public static void ApplyToControls(Control root)
        {
            foreach (Control c in root.Controls)
            {
                try { StyleControl(c); } catch { }
                ApplyToControls(c);
            }
        }

        /// <summary>
        /// Give a control modern rounded corners (re-applied on resize).
        /// </summary>
        public static void RoundCorners(Control c, int radius)
        {
            ApplyRound(c, radius);
            c.SizeChanged += (s, e) => ApplyRound(c, radius);
        }

        private static void ApplyRound(Control c, int radius)
        {
            if (c.Width <= 1 || c.Height <= 1) return;
            int d = radius * 2;
            var r = c.ClientRectangle;
            if (d > r.Width) d = r.Width;
            if (d > r.Height) d = r.Height;
            if (d < 2) { c.Region = null; return; }
            var path = new GraphicsPath();
            path.AddArc(r.X, r.Y, d, d, 180, 90);
            path.AddArc(r.Right - d, r.Y, d, d, 270, 90);
            path.AddArc(r.Right - d, r.Bottom - d, d, d, 0, 90);
            path.AddArc(r.X, r.Bottom - d, d, d, 90, 90);
            path.CloseFigure();
            c.Region = new Region(path);
        }

        private static void StyleControl(Control c)
        {
            // Custom controls paint themselves; don't let the generic styler touch them.
            if (c is EvelentButton || c is EvelentCard || c is EvelentSectionLabel) return;

            switch (c)
            {
                case MenuStrip ms:
                    ms.Renderer = new EvelentToolStripRenderer();
                    ms.BackColor = StripBack;
                    ms.ForeColor = TextColor;
                    ms.Padding = new Padding(6, 2, 0, 2);
                    break;
                case StatusStrip ss:
                    ss.Renderer = new EvelentToolStripRenderer();
                    ss.BackColor = StripBack;
                    ss.ForeColor = TextColor;
                    break;
                case ToolStrip ts:
                    ts.Renderer = new EvelentToolStripRenderer();
                    ts.BackColor = StripBack;
                    ts.ForeColor = TextColor;
                    break;
                case TextBox tb:
                    tb.ForeColor = tb.ReadOnly ? DisabledText : TextColor;
                    tb.BackColor = tb.ReadOnly ? PanelBack : InputBack;
                    if (tb.BorderStyle == BorderStyle.Fixed3D) tb.BorderStyle = BorderStyle.FixedSingle;
                    ApplyControlDarkMode(tb);
                    break;
                case RichTextBox rtb:
                    rtb.ForeColor = TextColor;
                    rtb.BackColor = InputBack;
                    rtb.BorderStyle = BorderStyle.None;
                    ApplyControlDarkMode(rtb);
                    break;
                case ComboBox cb:
                    cb.ForeColor = TextColor;
                    cb.BackColor = InputBack;
                    cb.FlatStyle = FlatStyle.Flat;
                    // Owner-draw so the closed box and the dropdown list are dark
                    // instead of the default white Windows list.
                    if (cb.DrawMode != DrawMode.OwnerDrawFixed)
                    {
                        cb.DrawMode = DrawMode.OwnerDrawFixed;
                        cb.DrawItem += ComboBox_DrawItem;
                    }
                    ApplyControlDarkMode(cb);
                    break;
                case Button btn:
                    StyleButton(btn);
                    break;
                case ListBox lb:
                    lb.ForeColor = TextColor;
                    lb.BackColor = InputBack;
                    lb.BorderStyle = BorderStyle.None;
                    ApplyControlDarkMode(lb);
                    break;
                case ListView lv:
                    lv.ForeColor = TextColor;
                    lv.BackColor = InputBack;
                    lv.BorderStyle = BorderStyle.None;
                    ApplyControlDarkMode(lv);
                    break;
                case TreeView tv:
                    tv.ForeColor = TextColor;
                    tv.BackColor = InputBack;
                    tv.BorderStyle = BorderStyle.None;
                    tv.LineColor = BorderColor;
                    ApplyControlDarkMode(tv);
                    break;
                case DataGridView dgv:
                    StyleDataGrid(dgv);
                    break;
                case CheckBox chk:
                    chk.ForeColor = TextColor;
                    break;
                case RadioButton rb:
                    rb.ForeColor = TextColor;
                    break;
                case LinkLabel ll:
                    ll.LinkColor = Accent;
                    ll.ActiveLinkColor = AccentText;
                    ll.VisitedLinkColor = Accent;
                    break;
                case Label lbl:
                    lbl.ForeColor = TextColor;
                    break;
                case GroupBox gb:
                    gb.ForeColor = TextColor;
                    gb.BackColor = WindowBack;
                    break;
                case TabControl tc:
                    StyleTabControl(tc);
                    break;
                case TabPage tp:
                    // UseVisualStyleBackColor makes the page ignore BackColor and
                    // paint white via the OS theme - turn it off so it goes dark.
                    tp.UseVisualStyleBackColor = false;
                    tp.ForeColor = TextColor;
                    tp.BackColor = WindowBack;
                    break;
                case Panel pnl:
                    pnl.BackColor = PanelBack;
                    pnl.ForeColor = TextColor;
                    break;
                case NumericUpDown nud:
                    nud.ForeColor = TextColor;
                    nud.BackColor = InputBack;
                    nud.BorderStyle = BorderStyle.FixedSingle;
                    break;
                case TrackBar trk:
                    // Match the parent surface and owner-draw a clean rounded
                    // slider, removing the light Windows-drawn track/groove.
                    trk.BackColor = trk.Parent != null ? trk.Parent.BackColor : PanelBack;
                    trk.ForeColor = TextColor;
                    SkinTrackBar(trk);
                    break;
                case ProgressBar pbar:
                    pbar.ForeColor = Accent;
                    pbar.BackColor = InputBack;
                    break;
                case PropertyGrid pg:
                    pg.ViewBackColor = InputBack;
                    pg.ViewForeColor = TextColor;
                    pg.ViewBorderColor = BorderColor;
                    pg.LineColor = BorderColor;
                    pg.CategoryForeColor = Accent;
                    pg.CategorySplitterColor = BorderColor;
                    pg.HelpBackColor = PanelBack;
                    pg.HelpForeColor = TextColor;
                    pg.HelpBorderColor = BorderColor;
                    pg.BackColor = PanelBack;
                    pg.CommandsBackColor = PanelBack;
                    pg.CommandsForeColor = TextColor;
                    break;
                case SplitContainer sc:
                    sc.BackColor = BorderColor;
                    sc.Panel1.BackColor = WindowBack;
                    sc.Panel2.BackColor = WindowBack;
                    break;
                default:
                    // Catch-all: any control still carrying a default light Windows
                    // system colour gets pulled into the dark palette so no white
                    // patches remain. Controls with an intentional custom colour
                    // (and the self-painting controls above) are left untouched.
                    if (IsLightSystemColor(c.BackColor))
                    {
                        c.BackColor = PanelBack;
                        c.ForeColor = TextColor;
                    }
                    break;
            }
        }

        private static void ComboBox_DrawItem(object sender, DrawItemEventArgs e)
        {
            var cb = (ComboBox)sender;
            var g = e.Graphics;
            bool selected = (e.State & DrawItemState.Selected) == DrawItemState.Selected;
            using (var b = new SolidBrush(selected ? Accent : InputBack))
                g.FillRectangle(b, e.Bounds);
            if (e.Index >= 0 && e.Index < cb.Items.Count)
            {
                string text = cb.GetItemText(cb.Items[e.Index]);
                TextRenderer.DrawText(g, text, cb.Font, e.Bounds,
                    selected ? AccentText : TextColor,
                    TextFormatFlags.Left | TextFormatFlags.VerticalCenter | TextFormatFlags.NoPrefix);
            }
        }

        /// <summary>
        /// True for the default light Windows surface colours that should be
        /// recoloured to the dark palette.
        /// </summary>
        private static bool IsLightSystemColor(Color c)
        {
            return c == SystemColors.Control
                || c == SystemColors.ControlLight
                || c == SystemColors.ControlLightLight
                || c == SystemColors.Window
                || c == SystemColors.ButtonFace
                || c == SystemColors.ButtonHighlight
                || c == SystemColors.AppWorkspace
                || c == SystemColors.InactiveBorder
                || c == Color.White
                || c == Color.WhiteSmoke;
        }

        private static void StyleButton(Button btn)
        {
            btn.ForeColor = TextColor;
            btn.BackColor = InputBack;
            btn.FlatStyle = FlatStyle.Flat;
            btn.FlatAppearance.BorderSize = 1;
            btn.FlatAppearance.BorderColor = BorderColor;
            btn.FlatAppearance.MouseOverBackColor = AccentSoft;
            btn.FlatAppearance.MouseDownBackColor = Accent;
            btn.UseVisualStyleBackColor = false;
            RoundCorners(btn, 6);
        }

        private static void StyleDataGrid(DataGridView dgv)
        {
            dgv.EnableHeadersVisualStyles = false;
            dgv.BackgroundColor = PanelBack;
            dgv.BorderStyle = BorderStyle.None;
            dgv.GridColor = BorderColor;
            dgv.ForeColor = TextColor;
            dgv.DefaultCellStyle.BackColor = InputBack;
            dgv.DefaultCellStyle.ForeColor = TextColor;
            dgv.DefaultCellStyle.SelectionBackColor = Accent;
            dgv.DefaultCellStyle.SelectionForeColor = AccentText;
            dgv.ColumnHeadersDefaultCellStyle.BackColor = StripBack;
            dgv.ColumnHeadersDefaultCellStyle.ForeColor = TextColor;
            dgv.ColumnHeadersDefaultCellStyle.SelectionBackColor = StripBack;
            dgv.RowHeadersDefaultCellStyle.BackColor = StripBack;
            dgv.RowHeadersDefaultCellStyle.ForeColor = TextColor;
            dgv.ColumnHeadersBorderStyle = DataGridViewHeaderBorderStyle.Single;
            dgv.RowHeadersBorderStyle = DataGridViewHeaderBorderStyle.Single;
        }

        private static void StyleTabControl(TabControl tc)
        {
            tc.ForeColor = TextColor;
            tc.BackColor = WindowBack;
            tc.SizeMode = TabSizeMode.Normal;
            tc.Appearance = TabAppearance.Normal;
            if (tc.DrawMode != TabDrawMode.OwnerDrawFixed)
            {
                tc.DrawMode = TabDrawMode.OwnerDrawFixed;
                tc.DrawItem += TabControl_DrawItem;
            }
        }

        private static void TabControl_DrawItem(object sender, DrawItemEventArgs e)
        {
            var tc = (TabControl)sender;
            var g = e.Graphics;
            g.SmoothingMode = SmoothingMode.AntiAlias;
            g.TextRenderingHint = TextRenderingHint.ClearTypeGridFit;

            int count = tc.TabCount;
            var rect = tc.GetTabRect(e.Index);
            int bandBottom = rect.Bottom + 2;
            bool selected = tc.SelectedIndex == e.Index;

            // Paint over the native (white) tab band around this tab so nothing
            // light shows through, including the edges before the first / after
            // the last tab.
            using (var bg = new SolidBrush(WindowBack))
            {
                int from = (e.Index == 0) ? 0 : rect.X - 2;
                int to = (e.Index == count - 1) ? tc.Width : rect.Right + 2;
                g.FillRectangle(bg, from, 0, to - from, bandBottom + 2);
            }

            // Rounded tab "pill".
            var tab = Rectangle.Inflate(rect, -2, -2);
            tab.Height += 2; // sit flush on the page
            using (var path = RoundedTop(tab, 7))
            using (var b = new SolidBrush(selected ? Accent : InputBack))
            using (var p = new Pen(selected ? Accent : BorderColor))
            {
                g.FillPath(b, path);
                g.DrawPath(p, path);
            }

            var flags = TextFormatFlags.HorizontalCenter | TextFormatFlags.VerticalCenter |
                        TextFormatFlags.EndEllipsis | TextFormatFlags.NoPrefix;
            TextRenderer.DrawText(g, tc.TabPages[e.Index].Text, tc.Font, tab,
                selected ? AccentText : TextColor, flags);
        }

        private static GraphicsPath RoundedTop(Rectangle r, int radius)
        {
            int d = radius * 2;
            var path = new GraphicsPath();
            if (d <= 0) { path.AddRectangle(r); return path; }
            path.AddArc(r.X, r.Y, d, d, 180, 90);
            path.AddArc(r.Right - d, r.Y, d, d, 270, 90);
            path.AddLine(r.Right, r.Bottom, r.X, r.Bottom);
            path.CloseFigure();
            return path;
        }

        // ---------------------------------------------------------------------
        //  Native dark-mode integration
        // ---------------------------------------------------------------------

        // Keep skins alive for the lifetime of their trackbars.
        private static readonly Dictionary<TrackBar, EvelentTrackBarSkin> trackSkins =
            new Dictionary<TrackBar, EvelentTrackBarSkin>();

        /// <summary>
        /// Owner-draw a trackbar so it shows a flat, rounded dark track and a
        /// round accent thumb instead of the light Windows-themed groove.
        /// </summary>
        private static void SkinTrackBar(TrackBar trk)
        {
            if (trackSkins.ContainsKey(trk)) return;
            var skin = new EvelentTrackBarSkin(trk);
            trackSkins[trk] = skin;
            trk.Disposed += (s, e) => trackSkins.Remove(trk);
        }


        /// <summary>
        /// Apply a slim, dark native theme (scrollbars, selection, hover) to a
        /// control once its handle exists. Safe no-op on unsupported systems.
        /// </summary>
        private static void ApplyControlDarkMode(Control c)
        {
            void Set()
            {
                try { SetWindowTheme(c.Handle, "DarkMode_Explorer", null); } catch { }
            }
            if (c.IsHandleCreated) Set();
            else c.HandleCreated += (s, e) => Set();
        }

        private static void ApplyDarkTitleBar(Form form)
        {
            try
            {
                if (!form.IsHandleCreated) return;
                int useDark = 1;
                // Windows 10 2004+ uses attribute 20; older builds used 19.
                if (DwmSetWindowAttribute(form.Handle, DWMWA_USE_IMMERSIVE_DARK_MODE, ref useDark, sizeof(int)) != 0)
                {
                    DwmSetWindowAttribute(form.Handle, DWMWA_USE_IMMERSIVE_DARK_MODE_OLD, ref useDark, sizeof(int));
                }
            }
            catch { }
        }

        private static void EnsureAppDarkMode()
        {
            if (appModeInited) return;
            appModeInited = true;
            try
            {
                // PreferredAppMode.ForceDark = 2 -> lets native controls (scrollbars,
                // menus, context menus) render dark across the process.
                SetPreferredAppMode(2);
                FlushMenuThemes();
            }
            catch { }
        }

        private const int DWMWA_USE_IMMERSIVE_DARK_MODE = 20;
        private const int DWMWA_USE_IMMERSIVE_DARK_MODE_OLD = 19;

        [DllImport("dwmapi.dll", PreserveSig = true)]
        private static extern int DwmSetWindowAttribute(IntPtr hwnd, int attr, ref int attrValue, int attrSize);

        [DllImport("uxtheme.dll", CharSet = CharSet.Unicode)]
        private static extern int SetWindowTheme(IntPtr hWnd, string pszSubAppName, string pszSubIdList);

        // Undocumented uxtheme ordinals used by Explorer / Terminal for app-wide dark mode.
        [DllImport("uxtheme.dll", EntryPoint = "#135", SetLastError = true)]
        private static extern int SetPreferredAppMode(int preferredAppMode);

        [DllImport("uxtheme.dll", EntryPoint = "#136")]
        private static extern void FlushMenuThemes();
    }

    /// <summary>
    /// Flat dark renderer for menus, toolbars and status strips, with a subtle
    /// accent hover highlight.
    /// </summary>
    public class EvelentToolStripRenderer : ToolStripProfessionalRenderer
    {
        public EvelentToolStripRenderer() : base(new EvelentColorTable()) { RoundedEdges = false; }

        protected override void OnRenderItemText(ToolStripItemTextRenderEventArgs e)
        {
            e.TextColor = e.Item.Enabled ? EvelentTheme.TextColor : EvelentTheme.DisabledText;
            base.OnRenderItemText(e);
        }

        protected override void OnRenderArrow(ToolStripArrowRenderEventArgs e)
        {
            e.ArrowColor = EvelentTheme.TextColor;
            base.OnRenderArrow(e);
        }

        protected override void OnRenderToolStripBorder(ToolStripRenderEventArgs e)
        {
            // Suppress the default raised/etched border for a cleaner flat look.
        }

        protected override void OnRenderSeparator(ToolStripSeparatorRenderEventArgs e)
        {
            var r = e.Item.ContentRectangle;
            using (var p = new Pen(EvelentTheme.BorderColor))
            {
                if (e.Vertical)
                {
                    int x = r.Left + r.Width / 2;
                    e.Graphics.DrawLine(p, x, r.Top + 3, x, r.Bottom - 3);
                }
                else
                {
                    int y = r.Top + r.Height / 2;
                    e.Graphics.DrawLine(p, r.Left + 3, y, r.Right - 3, y);
                }
            }
        }
    }

    /// <summary>
    /// Renderer for the bottom status bar that paints no background or border,
    /// so only the (white) status text shows over the window/3D area.
    /// </summary>
    public class EvelentTransparentStatusRenderer : ToolStripProfessionalRenderer
    {
        public EvelentTransparentStatusRenderer() : base(new EvelentColorTable()) { RoundedEdges = false; }
        protected override void OnRenderToolStripBackground(ToolStripRenderEventArgs e) { }
        protected override void OnRenderToolStripBorder(ToolStripRenderEventArgs e) { }
        protected override void OnRenderItemBackground(ToolStripItemRenderEventArgs e) { }
        protected override void OnRenderItemText(ToolStripItemTextRenderEventArgs e)
        {
            e.TextColor = Color.White;
            base.OnRenderItemText(e);
        }
    }

    public class EvelentColorTable : ProfessionalColorTable
    {
        public override Color MenuStripGradientBegin => EvelentTheme.StripBack;
        public override Color MenuStripGradientEnd => EvelentTheme.StripBack;
        public override Color ToolStripGradientBegin => EvelentTheme.StripBack;
        public override Color ToolStripGradientMiddle => EvelentTheme.StripBack;
        public override Color ToolStripGradientEnd => EvelentTheme.StripBack;
        public override Color ToolStripPanelGradientBegin => EvelentTheme.StripBack;
        public override Color ToolStripPanelGradientEnd => EvelentTheme.StripBack;
        public override Color ToolStripContentPanelGradientBegin => EvelentTheme.StripBack;
        public override Color ToolStripContentPanelGradientEnd => EvelentTheme.StripBack;
        public override Color StatusStripGradientBegin => EvelentTheme.StripBack;
        public override Color StatusStripGradientEnd => EvelentTheme.StripBack;
        public override Color ImageMarginGradientBegin => EvelentTheme.PanelBack;
        public override Color ImageMarginGradientMiddle => EvelentTheme.PanelBack;
        public override Color ImageMarginGradientEnd => EvelentTheme.PanelBack;
        public override Color ToolStripDropDownBackground => EvelentTheme.PanelBack;
        public override Color MenuBorder => EvelentTheme.BorderColor;
        public override Color MenuItemBorder => EvelentTheme.Accent;
        public override Color MenuItemSelected => EvelentTheme.AccentSoft;
        public override Color MenuItemSelectedGradientBegin => EvelentTheme.AccentSoft;
        public override Color MenuItemSelectedGradientEnd => EvelentTheme.AccentSoft;
        public override Color MenuItemPressedGradientBegin => EvelentTheme.PanelBack;
        public override Color MenuItemPressedGradientEnd => EvelentTheme.PanelBack;
        public override Color ButtonSelectedGradientBegin => EvelentTheme.AccentSoft;
        public override Color ButtonSelectedGradientMiddle => EvelentTheme.AccentSoft;
        public override Color ButtonSelectedGradientEnd => EvelentTheme.AccentSoft;
        public override Color ButtonSelectedHighlight => EvelentTheme.AccentSoft;
        public override Color ButtonSelectedBorder => EvelentTheme.Accent;
        public override Color ButtonPressedGradientBegin => EvelentTheme.Accent;
        public override Color ButtonPressedGradientMiddle => EvelentTheme.Accent;
        public override Color ButtonPressedGradientEnd => EvelentTheme.Accent;
        public override Color ButtonCheckedGradientBegin => EvelentTheme.AccentSoft;
        public override Color ButtonCheckedGradientMiddle => EvelentTheme.AccentSoft;
        public override Color ButtonCheckedGradientEnd => EvelentTheme.AccentSoft;
        public override Color CheckBackground => EvelentTheme.AccentSoft;
        public override Color CheckSelectedBackground => EvelentTheme.Accent;
        public override Color CheckPressedBackground => EvelentTheme.Accent;
        public override Color SeparatorDark => EvelentTheme.BorderColor;
        public override Color SeparatorLight => EvelentTheme.StripBack;
        public override Color GripDark => EvelentTheme.BorderColor;
        public override Color GripLight => EvelentTheme.StripBack;
    }

    /// <summary>
    /// Subclasses a native TrackBar's window so it can be owner-drawn with a
    /// flat, rounded dark track and a round accent thumb. The underlying control
    /// still handles all mouse/keyboard input and value/scroll events, so this
    /// is purely cosmetic and requires no changes to designer files.
    /// </summary>
    internal sealed class EvelentTrackBarSkin : NativeWindow
    {
        private readonly TrackBar tb;

        public EvelentTrackBarSkin(TrackBar trackBar)
        {
            tb = trackBar;
            if (tb.IsHandleCreated) AssignHandle(tb.Handle);
            tb.HandleCreated += (s, e) => { ReleaseHandle(); AssignHandle(tb.Handle); };
            tb.HandleDestroyed += (s, e) => ReleaseHandle();
            // The native control repaints itself on value changes, but make sure
            // any external value changes refresh the custom drawing too.
            tb.ValueChanged += (s, e) => { if (tb.IsHandleCreated) tb.Invalidate(); };
        }

        private const int WM_PAINT = 0x000F;
        private const int WM_ERASEBKGND = 0x0014;

        protected override void WndProc(ref Message m)
        {
            switch (m.Msg)
            {
                case WM_ERASEBKGND:
                    m.Result = (IntPtr)1; // we paint the whole surface ourselves
                    return;
                case WM_PAINT:
                    var ps = new PAINTSTRUCT();
                    IntPtr hdc = BeginPaint(m.HWnd, ref ps);
                    try { Paint(hdc); }
                    finally { EndPaint(m.HWnd, ref ps); }
                    m.Result = IntPtr.Zero;
                    return;
            }
            base.WndProc(ref m);
        }

        private void Paint(IntPtr hdc)
        {
            var rc = tb.ClientRectangle;
            if (rc.Width <= 0 || rc.Height <= 0) return;

            // Double-buffer to avoid flicker while dragging.
            using (var bmp = new Bitmap(rc.Width, rc.Height))
            {
                using (var g = Graphics.FromImage(bmp))
                {
                    g.SmoothingMode = SmoothingMode.AntiAlias;
                    using (var bg = new SolidBrush(tb.BackColor))
                        g.FillRectangle(bg, rc);

                    Draw(g, rc);
                }
                using (var dest = Graphics.FromHdc(hdc))
                    dest.DrawImageUnscaled(bmp, 0, 0);
            }
        }

        private void Draw(Graphics g, Rectangle rc)
        {
            int range = tb.Maximum - tb.Minimum;
            float frac = range > 0 ? (float)(tb.Value - tb.Minimum) / range : 0f;
            frac = Math.Max(0f, Math.Min(1f, frac));

            const int thumb = 14;   // thumb diameter
            const int track = 5;    // groove thickness
            int pad = thumb / 2 + 2;

            if (tb.Orientation == Orientation.Horizontal)
            {
                int x0 = rc.Left + pad;
                int x1 = rc.Right - pad;
                int cy = rc.Top + rc.Height / 2;
                if (x1 <= x0) return;

                var groove = new Rectangle(x0, cy - track / 2, x1 - x0, track);
                FillRound(g, groove, track / 2, EvelentTheme.InputBack);

                int thumbX = x0 + (int)(frac * (x1 - x0));
                var fill = new Rectangle(x0, cy - track / 2, thumbX - x0, track);
                if (fill.Width > 0) FillRound(g, fill, track / 2, EvelentTheme.Accent);

                DrawThumb(g, thumbX, cy, thumb);
            }
            else
            {
                int y0 = rc.Top + pad;
                int y1 = rc.Bottom - pad;
                int cx = rc.Left + rc.Width / 2;
                if (y1 <= y0) return;

                var groove = new Rectangle(cx - track / 2, y0, track, y1 - y0);
                FillRound(g, groove, track / 2, EvelentTheme.InputBack);

                // Vertical trackbars have their minimum at the bottom.
                int thumbY = y1 - (int)(frac * (y1 - y0));
                var fill = new Rectangle(cx - track / 2, thumbY, track, y1 - thumbY);
                if (fill.Height > 0) FillRound(g, fill, track / 2, EvelentTheme.Accent);

                DrawThumb(g, cx, thumbY, thumb);
            }
        }

        private static void DrawThumb(Graphics g, int cx, int cy, int diameter)
        {
            var r = new Rectangle(cx - diameter / 2, cy - diameter / 2, diameter, diameter);
            using (var b = new SolidBrush(EvelentTheme.Accent))
                g.FillEllipse(b, r);
            using (var p = new Pen(EvelentTheme.AccentText, 2f))
                g.DrawEllipse(p, r);
        }

        private static void FillRound(Graphics g, Rectangle r, int radius, Color color)
        {
            if (r.Width <= 0 || r.Height <= 0) return;
            int d = Math.Max(1, radius) * 2;
            if (d > r.Width) d = r.Width;
            if (d > r.Height) d = r.Height;
            using (var path = new GraphicsPath())
            {
                if (d < 2)
                {
                    path.AddRectangle(r);
                }
                else
                {
                    path.AddArc(r.X, r.Y, d, d, 180, 90);
                    path.AddArc(r.Right - d, r.Y, d, d, 270, 90);
                    path.AddArc(r.Right - d, r.Bottom - d, d, d, 0, 90);
                    path.AddArc(r.X, r.Bottom - d, d, d, 90, 90);
                    path.CloseFigure();
                }
                using (var b = new SolidBrush(color))
                    g.FillPath(b, path);
            }
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct RECT { public int Left, Top, Right, Bottom; }

        [StructLayout(LayoutKind.Sequential)]
        private struct PAINTSTRUCT
        {
            public IntPtr hdc;
            public bool fErase;
            public RECT rcPaint;
            public bool fRestore;
            public bool fIncUpdate;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 32)]
            public byte[] rgbReserved;
        }

        [DllImport("user32.dll")]
        private static extern IntPtr BeginPaint(IntPtr hWnd, ref PAINTSTRUCT lpPaint);

        [DllImport("user32.dll")]
        private static extern bool EndPaint(IntPtr hWnd, ref PAINTSTRUCT lpPaint);
    }
}
