using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Text;
using System.Windows.Forms;

namespace EvelentWalker
{
    /// <summary>
    /// A flat, modern push button: rounded corners, smooth hover/press states,
    /// an optional Segoe MDL2 glyph and an accent variant. Pure GDI+ owner draw,
    /// so it looks identical on every Windows version.
    /// </summary>
    public class EvelentButton : Button
    {
        private bool hover;
        private bool pressed;

        /// <summary>A Segoe MDL2 Assets glyph (e.g. "\uE8B7") drawn left of the text.</summary>
        public string Glyph { get; set; }

        /// <summary>Paint the button filled with the accent colour (primary action).</summary>
        public bool Accent { get; set; }

        /// <summary>Corner radius in pixels.</summary>
        public int CornerRadius { get; set; } = 8;

        public EvelentButton()
        {
            SetStyle(ControlStyles.AllPaintingInWmPaint | ControlStyles.OptimizedDoubleBuffer |
                     ControlStyles.UserPaint | ControlStyles.ResizeRedraw | ControlStyles.SupportsTransparentBackColor, true);
            FlatStyle = FlatStyle.Flat;
            FlatAppearance.BorderSize = 0;
            BackColor = Color.Transparent;
            ForeColor = EvelentTheme.TextColor;
            Font = new Font("Segoe UI", 9.5f, FontStyle.Regular);
            Cursor = Cursors.Hand;
            Height = 38;
        }

        protected override void OnMouseEnter(EventArgs e) { hover = true; Invalidate(); base.OnMouseEnter(e); }
        protected override void OnMouseLeave(EventArgs e) { hover = false; pressed = false; Invalidate(); base.OnMouseLeave(e); }
        protected override void OnMouseDown(MouseEventArgs e) { pressed = true; Invalidate(); base.OnMouseDown(e); }
        protected override void OnMouseUp(MouseEventArgs e) { pressed = false; Invalidate(); base.OnMouseUp(e); }

        protected override void OnPaint(PaintEventArgs e)
        {
            var g = e.Graphics;
            g.SmoothingMode = SmoothingMode.AntiAlias;
            g.TextRenderingHint = TextRenderingHint.ClearTypeGridFit;

            var rect = ClientRectangle;
            rect.Width -= 1; rect.Height -= 1;

            Color fill;
            Color border;
            Color text;

            if (Accent)
            {
                fill = pressed ? Darken(EvelentTheme.Accent, 0.85f)
                     : hover ? Lighten(EvelentTheme.Accent, 0.12f)
                     : EvelentTheme.Accent;
                border = fill;
                text = EvelentTheme.AccentText;
            }
            else
            {
                fill = pressed ? EvelentTheme.AccentSoft
                     : hover ? Lighten(EvelentTheme.InputBack, 0.18f)
                     : EvelentTheme.InputBack;
                border = hover ? EvelentTheme.Accent : EvelentTheme.BorderColor;
                text = Enabled ? EvelentTheme.TextColor : EvelentTheme.DisabledText;
            }

            using (var path = Rounded(rect, CornerRadius))
            using (var b = new SolidBrush(fill))
            using (var p = new Pen(border))
            {
                g.FillPath(b, path);
                g.DrawPath(p, path);
            }

            // glyph + text, centred together
            var glyphFont = GlyphFont();
            string glyph = Glyph;
            var flags = TextFormatFlags.NoPrefix | TextFormatFlags.VerticalCenter | TextFormatFlags.Left;

            int padding = 14;
            var content = new Rectangle(padding, 0, Width - padding * 2, Height);

            if (!string.IsNullOrEmpty(glyph) && glyphFont != null)
            {
                var glyphSize = TextRenderer.MeasureText(g, glyph, glyphFont, content.Size, flags);
                TextRenderer.DrawText(g, glyph, glyphFont,
                    new Rectangle(content.Left, content.Top, glyphSize.Width, content.Height), text, flags);
                content = new Rectangle(content.Left + glyphSize.Width + 10, content.Top,
                    content.Width - glyphSize.Width - 10, content.Height);
            }

            TextRenderer.DrawText(g, Text, Font, content, text, flags);
        }

        private static Font glyphFontCache;
        private static bool glyphTried;
        private static Font GlyphFont()
        {
            if (!glyphTried)
            {
                glyphTried = true;
                try { glyphFontCache = new Font("Segoe MDL2 Assets", 11f); } catch { glyphFontCache = null; }
            }
            return glyphFontCache;
        }

        internal static GraphicsPath Rounded(Rectangle r, int radius)
        {
            int d = radius * 2;
            var path = new GraphicsPath();
            if (d <= 0) { path.AddRectangle(r); return path; }
            if (d > r.Width) d = r.Width;
            if (d > r.Height) d = r.Height;
            path.AddArc(r.X, r.Y, d, d, 180, 90);
            path.AddArc(r.Right - d, r.Y, d, d, 270, 90);
            path.AddArc(r.Right - d, r.Bottom - d, d, d, 0, 90);
            path.AddArc(r.X, r.Bottom - d, d, d, 90, 90);
            path.CloseFigure();
            return path;
        }

        internal static Color Lighten(Color c, float amount)
        {
            return Color.FromArgb(c.A,
                (int)Math.Min(255, c.R + (255 - c.R) * amount),
                (int)Math.Min(255, c.G + (255 - c.G) * amount),
                (int)Math.Min(255, c.B + (255 - c.B) * amount));
        }

        internal static Color Darken(Color c, float factor)
        {
            return Color.FromArgb(c.A, (int)(c.R * factor), (int)(c.G * factor), (int)(c.B * factor));
        }
    }

    /// <summary>
    /// A rounded "card" surface used to group content with a subtle border.
    /// </summary>
    public class EvelentCard : Panel
    {
        public int CornerRadius { get; set; } = 12;
        public Color BorderColorOverride { get; set; } = Color.Empty;

        public EvelentCard()
        {
            SetStyle(ControlStyles.AllPaintingInWmPaint | ControlStyles.OptimizedDoubleBuffer |
                     ControlStyles.UserPaint | ControlStyles.ResizeRedraw, true);
            BackColor = EvelentTheme.PanelBack;
            ForeColor = EvelentTheme.TextColor;
        }

        protected override void OnPaintBackground(PaintEventArgs e) { /* custom paint */ }

        protected override void OnPaint(PaintEventArgs e)
        {
            var g = e.Graphics;
            g.SmoothingMode = SmoothingMode.AntiAlias;
            var r = ClientRectangle;
            r.Width -= 1; r.Height -= 1;
            var border = BorderColorOverride == Color.Empty ? EvelentTheme.BorderColor : BorderColorOverride;
            using (var path = EvelentButton.Rounded(r, CornerRadius))
            using (var b = new SolidBrush(BackColor))
            using (var p = new Pen(border))
            {
                g.FillPath(b, path);
                g.DrawPath(p, path);
            }
        }
    }

    /// <summary>
    /// A small uppercase section caption with an accent tick, for grouping.
    /// </summary>
    public class EvelentSectionLabel : Label
    {
        public EvelentSectionLabel()
        {
            SetStyle(ControlStyles.AllPaintingInWmPaint | ControlStyles.OptimizedDoubleBuffer |
                     ControlStyles.UserPaint | ControlStyles.ResizeRedraw, true);
            ForeColor = EvelentTheme.DisabledText;
            Font = new Font("Segoe UI", 8.25f, FontStyle.Bold);
            AutoSize = false;
            Height = 22;
        }

        protected override void OnPaint(PaintEventArgs e)
        {
            var g = e.Graphics;
            g.SmoothingMode = SmoothingMode.AntiAlias;
            using (var accent = new SolidBrush(EvelentTheme.Accent))
                g.FillRectangle(accent, 0, (Height - 12) / 2, 3, 12);
            var flags = TextFormatFlags.VerticalCenter | TextFormatFlags.Left | TextFormatFlags.NoPrefix;
            TextRenderer.DrawText(g, (Text ?? "").ToUpperInvariant(), Font,
                new Rectangle(10, 0, Width - 10, Height), ForeColor, flags);
        }
    }
}
