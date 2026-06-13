#include "evw/gamefiles/ddsio.h"

#include <cstring>
#include <fstream>
#include <stdexcept>

#include "evw/gamefiles/data.h"

namespace evw::texconv
{
    using gamefiles::Texture;
    using gamefiles::TextureFormat;

    namespace
    {
        constexpr uint32_t DDS_MAGIC = 0x20534444; // "DDS "

        // dwFlags
        constexpr uint32_t DDSD_CAPS = 0x1;
        constexpr uint32_t DDSD_HEIGHT = 0x2;
        constexpr uint32_t DDSD_WIDTH = 0x4;
        constexpr uint32_t DDSD_PIXELFORMAT = 0x1000;
        constexpr uint32_t DDSD_MIPMAPCOUNT = 0x20000;
        constexpr uint32_t DDSD_LINEARSIZE = 0x80000;
        constexpr uint32_t DDSD_PITCH = 0x8;

        // pixelformat flags
        constexpr uint32_t DDPF_ALPHAPIXELS = 0x1;
        constexpr uint32_t DDPF_ALPHA = 0x2;
        constexpr uint32_t DDPF_FOURCC = 0x4;
        constexpr uint32_t DDPF_RGB = 0x40;
        constexpr uint32_t DDPF_LUMINANCE = 0x20000;

        // caps
        constexpr uint32_t DDSCAPS_COMPLEX = 0x8;
        constexpr uint32_t DDSCAPS_TEXTURE = 0x1000;
        constexpr uint32_t DDSCAPS_MIPMAP = 0x400000;

        constexpr uint32_t fourcc(char a, char b, char c, char d)
        {
            return static_cast<uint32_t>(a) | (static_cast<uint32_t>(b) << 8) |
                   (static_cast<uint32_t>(c) << 16) | (static_cast<uint32_t>(d) << 24);
        }

        struct FormatInfo
        {
            bool compressed = false;
            uint32_t blockBytes = 0;   // bytes per 4x4 block (compressed)
            uint32_t fourCC = 0;       // for FOURCC formats
            bool dx10 = false;         // requires DX10 header extension
            uint32_t dxgiFormat = 0;   // when dx10
            // uncompressed pixel format:
            uint32_t pfFlags = 0;
            uint32_t bitCount = 0;
            uint32_t rMask = 0, gMask = 0, bMask = 0, aMask = 0;
        };

        FormatInfo describe(TextureFormat fmt)
        {
            FormatInfo f;
            switch (fmt)
            {
            case TextureFormat::D3DFMT_DXT1:
                f.compressed = true; f.blockBytes = 8; f.fourCC = fourcc('D','X','T','1'); break;
            case TextureFormat::D3DFMT_DXT3:
                f.compressed = true; f.blockBytes = 16; f.fourCC = fourcc('D','X','T','3'); break;
            case TextureFormat::D3DFMT_DXT5:
                f.compressed = true; f.blockBytes = 16; f.fourCC = fourcc('D','X','T','5'); break;
            case TextureFormat::D3DFMT_ATI1:
                f.compressed = true; f.blockBytes = 8; f.fourCC = fourcc('A','T','I','1'); break;
            case TextureFormat::D3DFMT_ATI2:
                f.compressed = true; f.blockBytes = 16; f.fourCC = fourcc('A','T','I','2'); break;
            case TextureFormat::D3DFMT_BC7:
                f.compressed = true; f.blockBytes = 16; f.dx10 = true; f.dxgiFormat = 98; break; // DXGI_FORMAT_BC7_UNORM
            case TextureFormat::D3DFMT_A8R8G8B8:
                f.pfFlags = DDPF_RGB | DDPF_ALPHAPIXELS; f.bitCount = 32;
                f.rMask = 0x00ff0000; f.gMask = 0x0000ff00; f.bMask = 0x000000ff; f.aMask = 0xff000000; break;
            case TextureFormat::D3DFMT_A8B8G8R8:
                f.pfFlags = DDPF_RGB | DDPF_ALPHAPIXELS; f.bitCount = 32;
                f.rMask = 0x000000ff; f.gMask = 0x0000ff00; f.bMask = 0x00ff0000; f.aMask = 0xff000000; break;
            case TextureFormat::D3DFMT_A1R5G5B5:
                f.pfFlags = DDPF_RGB | DDPF_ALPHAPIXELS; f.bitCount = 16;
                f.rMask = 0x7c00; f.gMask = 0x03e0; f.bMask = 0x001f; f.aMask = 0x8000; break;
            case TextureFormat::D3DFMT_A8:
                f.pfFlags = DDPF_ALPHA; f.bitCount = 8; f.aMask = 0xff; break;
            case TextureFormat::D3DFMT_L8:
                f.pfFlags = DDPF_LUMINANCE; f.bitCount = 8; f.rMask = 0xff; break;
            default:
                // Fallback: treat as 32-bit BGRA.
                f.pfFlags = DDPF_RGB | DDPF_ALPHAPIXELS; f.bitCount = 32;
                f.rMask = 0x00ff0000; f.gMask = 0x0000ff00; f.bMask = 0x000000ff; f.aMask = 0xff000000; break;
            }
            return f;
        }

        uint32_t topLevelSize(const FormatInfo& f, uint32_t width, uint32_t height, uint16_t stride)
        {
            if (f.compressed)
            {
                uint32_t bw = (width + 3) / 4; if (bw == 0) bw = 1;
                uint32_t bh = (height + 3) / 4; if (bh == 0) bh = 1;
                return bw * bh * f.blockBytes;
            }
            // Uncompressed: row pitch.
            return stride ? stride : (width * (f.bitCount / 8));
        }
    }

    std::vector<uint8_t> getDDSFile(const Texture& texture)
    {
        if (!texture.data || texture.data->fullData.empty())
            throw std::runtime_error("Texture has no pixel data to export");

        FormatInfo f = describe(texture.format);
        uint32_t width = texture.width;
        uint32_t height = texture.height;
        uint32_t mips = texture.levels ? texture.levels : 1;

        gamefiles::DataWriter w;
        w.Write(static_cast<uint32_t>(DDS_MAGIC));

        // DDS_HEADER (124 bytes)
        uint32_t flags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
        if (mips > 1) flags |= DDSD_MIPMAPCOUNT;
        flags |= f.compressed ? DDSD_LINEARSIZE : DDSD_PITCH;

        w.Write(static_cast<uint32_t>(124));                  // dwSize
        w.Write(flags);                                       // dwFlags
        w.Write(height);                                      // dwHeight
        w.Write(width);                                       // dwWidth
        w.Write(topLevelSize(f, width, height, texture.stride)); // dwPitchOrLinearSize
        w.Write(static_cast<uint32_t>(0));                    // dwDepth
        w.Write(mips);                                        // dwMipMapCount
        for (int i = 0; i < 11; ++i) w.Write(static_cast<uint32_t>(0)); // dwReserved1[11]

        // DDS_PIXELFORMAT (32 bytes)
        w.Write(static_cast<uint32_t>(32));                   // dwSize
        if (f.compressed && !f.dx10)
        {
            w.Write(static_cast<uint32_t>(DDPF_FOURCC));
            w.Write(f.fourCC);
            w.Write(static_cast<uint32_t>(0)); // bitcount
            w.Write(static_cast<uint32_t>(0)); // rmask
            w.Write(static_cast<uint32_t>(0)); // gmask
            w.Write(static_cast<uint32_t>(0)); // bmask
            w.Write(static_cast<uint32_t>(0)); // amask
        }
        else if (f.dx10)
        {
            w.Write(static_cast<uint32_t>(DDPF_FOURCC));
            w.Write(fourcc('D','X','1','0'));
            w.Write(static_cast<uint32_t>(0));
            w.Write(static_cast<uint32_t>(0));
            w.Write(static_cast<uint32_t>(0));
            w.Write(static_cast<uint32_t>(0));
            w.Write(static_cast<uint32_t>(0));
        }
        else
        {
            w.Write(f.pfFlags);
            w.Write(static_cast<uint32_t>(0)); // fourCC
            w.Write(f.bitCount);
            w.Write(f.rMask);
            w.Write(f.gMask);
            w.Write(f.bMask);
            w.Write(f.aMask);
        }

        // caps
        uint32_t caps = DDSCAPS_TEXTURE;
        if (mips > 1) caps |= DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
        w.Write(caps);                                        // dwCaps
        w.Write(static_cast<uint32_t>(0));                    // dwCaps2
        w.Write(static_cast<uint32_t>(0));                    // dwCaps3
        w.Write(static_cast<uint32_t>(0));                    // dwCaps4
        w.Write(static_cast<uint32_t>(0));                    // dwReserved2

        // DX10 extension header (20 bytes) when needed.
        if (f.dx10)
        {
            w.Write(f.dxgiFormat);                            // dxgiFormat
            w.Write(static_cast<uint32_t>(3));                // resourceDimension = TEXTURE2D
            w.Write(static_cast<uint32_t>(0));                // miscFlag
            w.Write(static_cast<uint32_t>(1));                // arraySize
            w.Write(static_cast<uint32_t>(0));                // miscFlags2
        }

        // Pixel data (full mip chain, already laid out).
        std::vector<uint8_t> out = w.buffer();
        out.insert(out.end(), texture.data->fullData.begin(), texture.data->fullData.end());
        return out;
    }

    bool writeDDSToFile(const Texture& texture, const std::string& path)
    {
        try
        {
            auto dds = getDDSFile(texture);
            std::ofstream f(path, std::ios::binary);
            if (!f) return false;
            f.write(reinterpret_cast<const char*>(dds.data()), static_cast<std::streamsize>(dds.size()));
            return static_cast<bool>(f);
        }
        catch (const std::exception&)
        {
            return false;
        }
    }
}
