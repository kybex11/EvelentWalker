#include "evw/gamefiles/dxt_decode.h"

namespace evw::texconv
{
    using gamefiles::Texture;
    using gamefiles::TextureFormat;

    namespace
    {
        struct Cursor
        {
            const uint8_t* p;
            size_t n;
            size_t pos = 0;
            uint8_t u8() { return pos < n ? p[pos++] : 0; }
            uint16_t u16() { uint16_t a = u8(); uint16_t b = u8(); return static_cast<uint16_t>(a | (b << 8)); }
            uint32_t u32()
            {
                uint32_t a = u8(), b = u8(), c = u8(), d = u8();
                return a | (b << 8) | (c << 16) | (d << 24);
            }
        };

        void rgb565(uint16_t color, uint8_t& r, uint8_t& g, uint8_t& b)
        {
            int temp;
            temp = (color >> 11) * 255 + 16;
            r = static_cast<uint8_t>((temp / 32 + temp) / 32);
            temp = ((color & 0x07E0) >> 5) * 255 + 32;
            g = static_cast<uint8_t>((temp / 64 + temp) / 64);
            temp = (color & 0x001F) * 255 + 16;
            b = static_cast<uint8_t>((temp / 32 + temp) / 32);
        }

        inline void putPixel(std::vector<uint8_t>& img, int width, int height, int px, int py,
                             uint8_t r, uint8_t g, uint8_t b, uint8_t a)
        {
            if (px < width && py < height)
            {
                size_t off = (static_cast<size_t>(py) * width + px) * 4;
                img[off] = r; img[off + 1] = g; img[off + 2] = b; img[off + 3] = a;
            }
        }
    }

    std::vector<uint8_t> decompressDxt1(const std::vector<uint8_t>& data, int width, int height)
    {
        std::vector<uint8_t> img(static_cast<size_t>(width) * height * 4, 0);
        Cursor c{data.data(), data.size()};
        int bx = (width + 3) / 4, by = (height + 3) / 4;
        for (int y = 0; y < by; ++y)
            for (int x = 0; x < bx; ++x)
            {
                uint16_t c0 = c.u16(), c1 = c.u16();
                uint8_t r0, g0, b0, r1, g1, b1;
                rgb565(c0, r0, g0, b0);
                rgb565(c1, r1, g1, b1);
                uint32_t lut = c.u32();
                for (int yy = 0; yy < 4; ++yy)
                    for (int xx = 0; xx < 4; ++xx)
                    {
                        uint8_t r = 0, g = 0, b = 0, a = 255;
                        uint32_t idx = (lut >> (2 * (4 * yy + xx))) & 0x3;
                        if (c0 > c1)
                        {
                            switch (idx)
                            {
                            case 0: r = r0; g = g0; b = b0; break;
                            case 1: r = r1; g = g1; b = b1; break;
                            case 2: r = (2 * r0 + r1) / 3; g = (2 * g0 + g1) / 3; b = (2 * b0 + b1) / 3; break;
                            case 3: r = (r0 + 2 * r1) / 3; g = (g0 + 2 * g1) / 3; b = (b0 + 2 * b1) / 3; break;
                            }
                        }
                        else
                        {
                            switch (idx)
                            {
                            case 0: r = r0; g = g0; b = b0; break;
                            case 1: r = r1; g = g1; b = b1; break;
                            case 2: r = (r0 + r1) / 2; g = (g0 + g1) / 2; b = (b0 + b1) / 2; break;
                            case 3: r = 0; g = 0; b = 0; a = 0; break;
                            }
                        }
                        putPixel(img, width, height, (x << 2) + xx, (y << 2) + yy, r, g, b, a);
                    }
            }
        return img;
    }

    std::vector<uint8_t> decompressDxt3(const std::vector<uint8_t>& data, int width, int height)
    {
        std::vector<uint8_t> img(static_cast<size_t>(width) * height * 4, 0);
        Cursor c{data.data(), data.size()};
        int bx = (width + 3) / 4, by = (height + 3) / 4;
        for (int y = 0; y < by; ++y)
            for (int x = 0; x < bx; ++x)
            {
                uint8_t av[8];
                for (int i = 0; i < 8; ++i) av[i] = c.u8();
                uint16_t c0 = c.u16(), c1 = c.u16();
                uint8_t r0, g0, b0, r1, g1, b1;
                rgb565(c0, r0, g0, b0);
                rgb565(c1, r1, g1, b1);
                uint32_t lut = c.u32();
                int ai = 0;
                for (int yy = 0; yy < 4; ++yy)
                    for (int xx = 0; xx < 4; ++xx)
                    {
                        uint8_t r = 0, g = 0, b = 0, a;
                        uint8_t ab = av[ai >> 1];
                        if ((ai & 1) == 0) a = static_cast<uint8_t>((ab & 0x0F) | ((ab & 0x0F) << 4));
                        else a = static_cast<uint8_t>((ab & 0xF0) | ((ab & 0xF0) >> 4));
                        ++ai;
                        uint32_t idx = (lut >> (2 * (4 * yy + xx))) & 0x3;
                        switch (idx)
                        {
                        case 0: r = r0; g = g0; b = b0; break;
                        case 1: r = r1; g = g1; b = b1; break;
                        case 2: r = (2 * r0 + r1) / 3; g = (2 * g0 + g1) / 3; b = (2 * b0 + b1) / 3; break;
                        case 3: r = (r0 + 2 * r1) / 3; g = (g0 + 2 * g1) / 3; b = (b0 + 2 * b1) / 3; break;
                        }
                        putPixel(img, width, height, (x << 2) + xx, (y << 2) + yy, r, g, b, a);
                    }
            }
        return img;
    }

    std::vector<uint8_t> decompressDxt5(const std::vector<uint8_t>& data, int width, int height)
    {
        std::vector<uint8_t> img(static_cast<size_t>(width) * height * 4, 0);
        Cursor c{data.data(), data.size()};
        int bx = (width + 3) / 4, by = (height + 3) / 4;
        for (int y = 0; y < by; ++y)
            for (int x = 0; x < bx; ++x)
            {
                uint8_t alpha0 = c.u8(), alpha1 = c.u8();
                uint64_t am = 0;
                am |= static_cast<uint64_t>(c.u8());
                am |= static_cast<uint64_t>(c.u8()) << 8;
                am |= static_cast<uint64_t>(c.u8()) << 16;
                am |= static_cast<uint64_t>(c.u8()) << 24;
                am |= static_cast<uint64_t>(c.u8()) << 32;
                am |= static_cast<uint64_t>(c.u8()) << 40;
                uint16_t c0 = c.u16(), c1 = c.u16();
                uint8_t r0, g0, b0, r1, g1, b1;
                rgb565(c0, r0, g0, b0);
                rgb565(c1, r1, g1, b1);
                uint32_t lut = c.u32();
                for (int yy = 0; yy < 4; ++yy)
                    for (int xx = 0; xx < 4; ++xx)
                    {
                        uint8_t r = 0, g = 0, b = 0, a;
                        uint32_t idx = (lut >> (2 * (4 * yy + xx))) & 0x3;
                        uint32_t aidx = static_cast<uint32_t>((am >> (3 * (4 * yy + xx))) & 0x7);
                        if (aidx == 0) a = alpha0;
                        else if (aidx == 1) a = alpha1;
                        else if (alpha0 > alpha1) a = static_cast<uint8_t>(((8 - aidx) * alpha0 + (aidx - 1) * alpha1) / 7);
                        else if (aidx == 6) a = 0;
                        else if (aidx == 7) a = 0xff;
                        else a = static_cast<uint8_t>(((6 - aidx) * alpha0 + (aidx - 1) * alpha1) / 5);
                        switch (idx)
                        {
                        case 0: r = r0; g = g0; b = b0; break;
                        case 1: r = r1; g = g1; b = b1; break;
                        case 2: r = (2 * r0 + r1) / 3; g = (2 * g0 + g1) / 3; b = (2 * b0 + b1) / 3; break;
                        case 3: r = (r0 + 2 * r1) / 3; g = (g0 + 2 * g1) / 3; b = (b0 + 2 * b1) / 3; break;
                        }
                        putPixel(img, width, height, (x << 2) + xx, (y << 2) + yy, r, g, b, a);
                    }
            }
        return img;
    }

    std::vector<uint8_t> decodeToRGBA(const Texture& texture)
    {
        if (!texture.data || texture.data->fullData.empty()) return {};
        const auto& d = texture.data->fullData;
        int w = texture.width, h = texture.height;
        switch (texture.format)
        {
        case TextureFormat::D3DFMT_DXT1: return decompressDxt1(d, w, h);
        case TextureFormat::D3DFMT_DXT3: return decompressDxt3(d, w, h);
        case TextureFormat::D3DFMT_DXT5: return decompressDxt5(d, w, h);
        case TextureFormat::D3DFMT_A8R8G8B8:
        case TextureFormat::D3DFMT_A8B8G8R8:
        {
            // Already 32-bit; copy the top-mip span.
            size_t need = static_cast<size_t>(w) * h * 4;
            if (d.size() >= need) return std::vector<uint8_t>(d.begin(), d.begin() + need);
            return d;
        }
        default:
            return {}; // unsupported here (ATI1/2, BC7, 16-bit)
        }
    }
}
