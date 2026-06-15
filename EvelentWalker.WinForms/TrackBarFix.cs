using System;
using System.ComponentModel;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Windows.Forms;

namespace EvelentWalker.WinForms
{
    /// <summary>
    /// Flat, theme-aware replacement for <see cref="System.Windows.Forms.TrackBar"/>.
    /// The stock TrackBar is a native common control whose look (blue track, round
    /// grab, tick marks) cannot be themed and renders poorly on the dark VS2015
    /// theme. This owner-drawn control keeps the parts of the TrackBar API the
    /// forms actually use (Minimum/Maximum/Value/SmallChange/LargeChange/
    /// TickFrequency, the Scroll event and ISupportInitialize) and draws a clean
    /// flat slider that blends with the parent background.
    /// </summary>
    [DefaultEvent(nameof(Scroll))]
    [DefaultProperty(nameof(Value))]
    public class TrackBarFix : Control, ISupportInitialize
    {
        private int _minimum = 0;
        private int _maximum = 10;
        private int _value = 0;
        private int _smallChange = 1;
        private int _largeChange = 5;
        private int _tickFrequency = 1;
        private Orientation _orientation = Orientation.Horizontal;
        private TickStyle _tickStyle = TickStyle.None;

        private bool _dragging = false;
        private bool _hovered = false;

        public TrackBarFix()
        {
            SetStyle(ControlStyles.AllPaintingInWmPaint |
                     ControlStyles.UserPaint |
                     ControlStyles.OptimizedDoubleBuffer |
                     ControlStyles.ResizeRedraw |
                     ControlStyles.SupportsTransparentBackColor |
                     ControlStyles.Selectable, true);
            TabStop = true;
            Size = new Size(150, 30);
        }

        #region Public API (mirrors System.Windows.Forms.TrackBar)

        [DefaultValue(0)]
        public int Minimum
        {
            get => _minimum;
            set
            {
                _minimum = value;
                if (_maximum < _minimum) _maximum = _minimum;
                SetValueInternal(_value, false);
                Invalidate();
            }
        }

        [DefaultValue(10)]
        public int Maximum
        {
            get => _maximum;
            set
            {
                _maximum = value;
                if (_minimum > _maximum) _minimum = _maximum;
                SetValueInternal(_value, false);
                Invalidate();
            }
        }

        [DefaultValue(0)]
        public int Value
        {
            get => _value;
            set => SetValueInternal(value, false);
        }

        [DefaultValue(1)]
        public int SmallChange
        {
            get => _smallChange;
            set => _smallChange = Math.Max(1, value);
        }

        [DefaultValue(5)]
        public int LargeChange
        {
            get => _largeChange;
            set => _largeChange = Math.Max(1, value);
        }

        [DefaultValue(1)]
        public int TickFrequency
        {
            get => _tickFrequency;
            set { _tickFrequency = Math.Max(1, value); Invalidate(); }
        }

        [DefaultValue(typeof(Orientation), "Horizontal")]
        public Orientation Orientation
        {
            get => _orientation;
            set { _orientation = value; Invalidate(); }
        }

        [DefaultValue(typeof(TickStyle), "None")]
        public TickStyle TickStyle
        {
            get => _tickStyle;
            set { _tickStyle = value; Invalidate(); }
        }

        /// <summary>Raised when the user changes the value via mouse or keyboard.</summary>
        public event EventHandler Scroll;

        /// <summary>Raised whenever the value changes (programmatically or by the user).</summary>
        public event EventHandler ValueChanged;

        #endregion

        #region ISupportInitialize

        public void BeginInit() { }
        public void EndInit() { Invalidate(); }

        #endregion

        private void SetValueInternal(int newValue, bool fromUser)
        {
            if (newValue < _minimum) newValue = _minimum;
            if (newValue > _maximum) newValue = _maximum;
            if (newValue == _value)
            {
                if (fromUser) Invalidate();
                return;
            }
            _value = newValue;
            Invalidate();
            ValueChanged?.Invoke(this, EventArgs.Empty);
            if (fromUser) Scroll?.Invoke(this, EventArgs.Empty);
        }

        #region Theming helpers

        private Color EffectiveBackColor()
        {
            // Blend with the parent so the slider sits naturally on light or dark forms,
            // ignoring the stock ControlLightLight value the designer assigns.
            if (Parent != null && Parent.BackColor.A > 0) return Parent.BackColor;
            return BackColor;
        }

        private static bool IsDark(Color c) => (0.299 * c.R + 0.587 * c.G + 0.114 * c.B) < 128.0;

        private Color AccentColor => Color.FromArgb(0, 122, 204);          // VS blue
        private Color TrackUnfilledColor => IsDark(EffectiveBackColor())
            ? Color.FromArgb(78, 81, 88)
            : Color.FromArgb(196, 196, 196);
        private Color TickColor => IsDark(EffectiveBackColor())
            ? Color.FromArgb(110, 113, 120)
            : Color.FromArgb(160, 160, 160);
        private Color DisabledColor => IsDark(EffectiveBackColor())
            ? Color.FromArgb(95, 95, 100)
            : Color.FromArgb(170, 170, 170);

        #endregion

        #region Geometry

        private const int ThumbRadius = 7;
        private const int TrackThickness = 4;

        private int TrackStartX => ThumbRadius + 2;
        private int TrackEndX => Width - ThumbRadius - 2;
        private int TrackSpan => Math.Max(1, TrackEndX - TrackStartX);

        private float ValueRatio()
        {
            int range = _maximum - _minimum;
            if (range <= 0) return 0f;
            return (float)(_value - _minimum) / range;
        }

        private int ThumbCenterX() => TrackStartX + (int)Math.Round(ValueRatio() * TrackSpan);

        private int ValueFromX(int x)
        {
            float ratio = (float)(x - TrackStartX) / TrackSpan;
            if (ratio < 0f) ratio = 0f;
            if (ratio > 1f) ratio = 1f;
            return _minimum + (int)Math.Round(ratio * (_maximum - _minimum));
        }

        #endregion

        #region Painting

        protected override void OnPaint(PaintEventArgs e)
        {
            var g = e.Graphics;
            g.Clear(EffectiveBackColor());
            g.SmoothingMode = SmoothingMode.AntiAlias;
            g.PixelOffsetMode = PixelOffsetMode.HighQuality;

            int cy = Height / 2;
            int thumbX = ThumbCenterX();
            bool enabled = Enabled;

            Color fillColor = enabled ? AccentColor : DisabledColor;
            Color thumbColor = enabled ? (_dragging || _hovered ? ControlPaint.Light(AccentColor, 0.15f) : AccentColor) : DisabledColor;

            // Optional tick marks (kept subtle, below the track, never under the thumb area only).
            if (_tickStyle != TickStyle.None && _maximum > _minimum)
            {
                using (var tickPen = new Pen(TickColor))
                {
                    int freq = Math.Max(1, _tickFrequency);
                    int tickY = cy + TrackThickness / 2 + 4;
                    for (int v = _minimum; v <= _maximum; v += freq)
                    {
                        float r = (float)(v - _minimum) / (_maximum - _minimum);
                        int tx = TrackStartX + (int)Math.Round(r * TrackSpan);
                        g.DrawLine(tickPen, tx, tickY, tx, tickY + 4);
                    }
                }
            }

            // Unfilled track.
            using (var unfilled = new SolidBrush(TrackUnfilledColor))
                FillRoundedTrack(g, unfilled, TrackStartX, cy, TrackEndX, TrackThickness);

            // Filled portion (left of the thumb).
            using (var filled = new SolidBrush(fillColor))
                FillRoundedTrack(g, filled, TrackStartX, cy, thumbX, TrackThickness);

            // Thumb.
            var thumbRect = new Rectangle(thumbX - ThumbRadius, cy - ThumbRadius, ThumbRadius * 2, ThumbRadius * 2);
            using (var thumbBrush = new SolidBrush(thumbColor))
                g.FillEllipse(thumbBrush, thumbRect);
            // Subtle ring matching the background to lift the thumb off the track.
            using (var ringPen = new Pen(EffectiveBackColor(), 2f))
                g.DrawEllipse(ringPen, thumbRect);

            if (Focused && enabled)
            {
                var focusRect = new Rectangle(thumbX - ThumbRadius - 2, cy - ThumbRadius - 2,
                                              (ThumbRadius + 2) * 2, (ThumbRadius + 2) * 2);
                using (var focusPen = new Pen(ControlPaint.Light(AccentColor, 0.4f)))
                    g.DrawEllipse(focusPen, focusRect);
            }
        }

        private static void FillRoundedTrack(Graphics g, Brush brush, int x0, int cy, int x1, int thickness)
        {
            if (x1 <= x0) x1 = x0 + 1;
            int top = cy - thickness / 2;
            var rect = new Rectangle(x0, top, x1 - x0, thickness);
            int radius = thickness;
            using (var path = RoundedRect(rect, radius))
                g.FillPath(brush, path);
        }

        private static GraphicsPath RoundedRect(Rectangle rect, int radius)
        {
            int d = Math.Min(radius, Math.Min(rect.Width, rect.Height));
            if (d <= 0) d = 1;
            var path = new GraphicsPath();
            path.AddArc(rect.X, rect.Y, d, d, 90, 90);
            path.AddArc(rect.Right - d, rect.Y, d, d, 180, 90);
            path.AddArc(rect.Right - d, rect.Bottom - d, d, d, 270, 90);
            path.AddArc(rect.X, rect.Bottom - d, d, d, 0, 90);
            path.CloseFigure();
            return path;
        }

        protected override void OnPaintBackground(PaintEventArgs pevent)
        {
            // Background is painted in OnPaint to blend with the parent.
        }

        #endregion

        #region Interaction

        protected override void OnMouseDown(MouseEventArgs e)
        {
            base.OnMouseDown(e);
            if (!Enabled) return;
            if (e.Button == MouseButtons.Left)
            {
                Focus();
                _dragging = true;
                SetValueInternal(ValueFromX(e.X), true);
            }
        }

        protected override void OnMouseMove(MouseEventArgs e)
        {
            base.OnMouseMove(e);
            if (_dragging && Enabled)
                SetValueInternal(ValueFromX(e.X), true);
        }

        protected override void OnMouseUp(MouseEventArgs e)
        {
            _dragging = false;
            base.OnMouseUp(e);
        }

        protected override void OnMouseEnter(EventArgs e)
        {
            base.OnMouseEnter(e);
            _hovered = true;
            Invalidate();
        }

        protected override void OnMouseLeave(EventArgs e)
        {
            base.OnMouseLeave(e);
            _hovered = false;
            Invalidate();
        }

        protected override void OnMouseWheel(MouseEventArgs e)
        {
            base.OnMouseWheel(e);
            if (!Enabled) return;
            int steps = e.Delta / 120;
            if (steps != 0) SetValueInternal(_value + steps * _smallChange, true);
        }

        protected override bool IsInputKey(Keys keyData)
        {
            switch (keyData)
            {
                case Keys.Left:
                case Keys.Right:
                case Keys.Up:
                case Keys.Down:
                case Keys.PageUp:
                case Keys.PageDown:
                case Keys.Home:
                case Keys.End:
                    return true;
            }
            return base.IsInputKey(keyData);
        }

        protected override void OnKeyDown(KeyEventArgs e)
        {
            base.OnKeyDown(e);
            if (!Enabled) return;
            switch (e.KeyCode)
            {
                case Keys.Left:
                case Keys.Down:
                    SetValueInternal(_value - _smallChange, true); e.Handled = true; break;
                case Keys.Right:
                case Keys.Up:
                    SetValueInternal(_value + _smallChange, true); e.Handled = true; break;
                case Keys.PageDown:
                    SetValueInternal(_value - _largeChange, true); e.Handled = true; break;
                case Keys.PageUp:
                    SetValueInternal(_value + _largeChange, true); e.Handled = true; break;
                case Keys.Home:
                    SetValueInternal(_minimum, true); e.Handled = true; break;
                case Keys.End:
                    SetValueInternal(_maximum, true); e.Handled = true; break;
            }
        }

        protected override void OnGotFocus(EventArgs e) { base.OnGotFocus(e); Invalidate(); }
        protected override void OnLostFocus(EventArgs e) { base.OnLostFocus(e); Invalidate(); }
        protected override void OnEnabledChanged(EventArgs e) { base.OnEnabledChanged(e); Invalidate(); }

        #endregion
    }
}
