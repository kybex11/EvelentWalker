// Port of EvelentWalker.Core/GameFiles/Resources/Texture.cs (legacy / non-gen9
// read path): TextureDictionary (YTD), Texture, TextureBase, TextureData.
// The gen9 (Enhanced) variant is not yet ported.
#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "evw/gamefiles/resource_data.h"

namespace evw::gamefiles
{
    enum class TextureFormat : uint32_t
    {
        D3DFMT_A8R8G8B8 = 21,
        D3DFMT_A1R5G5B5 = 25,
        D3DFMT_A8 = 28,
        D3DFMT_A8B8G8R8 = 32,
        D3DFMT_L8 = 50,
        D3DFMT_DXT1 = 0x31545844,
        D3DFMT_DXT3 = 0x33545844,
        D3DFMT_DXT5 = 0x35545844,
        D3DFMT_ATI1 = 0x31495441,
        D3DFMT_ATI2 = 0x32495441,
        D3DFMT_BC7 = 0x20374342,
    };

    // Human-readable name of a texture format.
    const char* textureFormatName(TextureFormat format);

    // True for block-compressed formats (DXT/ATI/BC7).
    bool isCompressedFormat(TextureFormat format);

    // Graphics-segment block holding the raw (mip-chained) pixel data.
    struct TextureData
    {
        std::vector<uint8_t> fullData;

        // Legacy size: sum over mip levels of (stride*height), each level /= 4.
        void read(ResourceDataReader& r, uint32_t format, int width, int height, int levels, int stride);
    };

    // Common header for textures and shader-param textures (80 bytes, legacy).
    struct TextureBase
    {
        uint32_t vft = 0;
        uint64_t namePointer = 0;
        uint32_t usageData = 0;
        std::string name;
        uint32_t nameHash = 0;

        void readBase(ResourceDataReader& r);
        // For use as a referenced block (shader texture parameters).
        void read(ResourceDataReader& r) { readBase(r); }
    };

    // A single texture (144 bytes, legacy).
    struct Texture : public TextureBase
    {
        uint16_t width = 0;
        uint16_t height = 0;
        uint16_t depth = 0;
        uint16_t stride = 0;
        TextureFormat format = TextureFormat::D3DFMT_A8R8G8B8;
        uint8_t levels = 0;
        uint64_t dataPointer = 0;
        std::shared_ptr<TextureData> data;

        void read(ResourceDataReader& r);

        long long memoryUsage() const { return data ? static_cast<long long>(data->fullData.size()) : 0; }
    };

    // YTD: dictionary of textures keyed by name hash.
    struct TextureDictionary : public ResourceFileBase
    {
        ResourceSimpleList64_uint textureNameHashes;
        ResourcePointerList64<Texture> textures;
        std::map<uint32_t, std::shared_ptr<Texture>> dict;

        void read(ResourceDataReader& r);

        std::shared_ptr<Texture> lookup(uint32_t hash) const;
        long long memoryUsage() const;

    private:
        void buildDict();
    };

    // Convenience: parse a YTD from extracted resource bytes + entry sizes.
    TextureDictionary loadTextureDictionary(uint32_t systemSize, uint32_t graphicsSize,
                                            const std::vector<uint8_t>& data);
}
