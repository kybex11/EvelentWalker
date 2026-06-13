#include "evw/gamefiles/texture.h"

#include "evw/gamefiles/jenk.h"

namespace evw::gamefiles
{
    namespace
    {
        std::string toLower(std::string s)
        {
            for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            return s;
        }
    }

    void TextureData::read(ResourceDataReader& r, uint32_t /*format*/, int /*width*/, int height,
                           int levels, int stride)
    {
        int fullLength = 0;
        int length = stride * height;
        for (int i = 0; i < levels; ++i)
        {
            fullLength += length;
            length /= 4;
        }
        fullData = r.ReadBytes(fullLength);
    }

    void TextureBase::readBase(ResourceDataReader& r)
    {
        // 10 x uint32 structure prefix (VFT + unknowns)
        vft = r.ReadUInt32();
        r.ReadUInt32(); // Unknown_4h
        r.ReadUInt32(); // Unknown_8h
        r.ReadUInt32(); // Unknown_Ch
        r.ReadUInt32(); // Unknown_10h
        r.ReadUInt32(); // Unknown_14h
        r.ReadUInt32(); // Unknown_18h
        r.ReadUInt32(); // Unknown_1Ch
        r.ReadUInt32(); // Unknown_20h
        r.ReadUInt32(); // Unknown_24h
        namePointer = r.ReadUInt64();
        r.ReadUInt16(); // Unknown_30h
        r.ReadUInt16(); // Unknown_32h
        r.ReadUInt32(); // Unknown_34h
        r.ReadUInt32(); // Unknown_38h
        r.ReadUInt32(); // Unknown_3Ch
        usageData = r.ReadUInt32();
        r.ReadUInt32(); // Unknown_44h
        r.ReadUInt32(); // ExtraFlags
        r.ReadUInt32(); // Unknown_4Ch

        name = r.ReadStringAt(namePointer);
        if (!name.empty())
            nameHash = JenkHash::GenHash(toLower(name));
    }

    void Texture::read(ResourceDataReader& r)
    {
        readBase(r);

        width = r.ReadUInt16();
        height = r.ReadUInt16();
        depth = r.ReadUInt16();
        stride = r.ReadUInt16();
        format = static_cast<TextureFormat>(r.ReadUInt32());
        r.ReadByte();              // Unknown_5Ch
        levels = r.ReadByte();
        r.ReadUInt16();            // Unknown_5Eh
        r.ReadUInt32();            // Unknown_60h
        r.ReadUInt32();            // Unknown_64h
        r.ReadUInt32();            // Unknown_68h
        r.ReadUInt32();            // Unknown_6Ch
        dataPointer = r.ReadUInt64();
        r.ReadUInt32();            // Unknown_78h
        r.ReadUInt32();            // Unknown_7Ch
        r.ReadUInt32();            // Unknown_80h
        r.ReadUInt32();            // Unknown_84h
        r.ReadUInt32();            // Unknown_88h
        r.ReadUInt32();            // Unknown_8Ch

        if (dataPointer != 0)
        {
            uint64_t backup = r.position();
            r.setPosition(dataPointer);
            data = std::make_shared<TextureData>();
            data->read(r, static_cast<uint32_t>(format), width, height, levels, stride);
            r.setPosition(backup);
        }
    }

    void TextureDictionary::read(ResourceDataReader& r)
    {
        ResourceFileBase::read(r);

        r.ReadUInt32(); // Unknown_10h
        r.ReadUInt32(); // Unknown_14h
        r.ReadUInt32(); // Unknown_18h
        r.ReadUInt32(); // Unknown_1Ch
        textureNameHashes.read(r);
        textures.read(r);

        buildDict();
    }

    void TextureDictionary::buildDict()
    {
        dict.clear();
        const auto& hashes = textureNameHashes.items;
        for (size_t i = 0; i < textures.items.size() && i < hashes.size(); ++i)
            dict[hashes[i]] = textures.items[i];
    }

    std::shared_ptr<Texture> TextureDictionary::lookup(uint32_t hash) const
    {
        auto it = dict.find(hash);
        return it != dict.end() ? it->second : nullptr;
    }

    long long TextureDictionary::memoryUsage() const
    {
        long long val = 0;
        for (const auto& tex : textures.items)
            if (tex) val += tex->memoryUsage();
        return val;
    }

    TextureDictionary loadTextureDictionary(uint32_t systemSize, uint32_t graphicsSize,
                                            const std::vector<uint8_t>& data)
    {
        ResourceDataReader reader(systemSize, graphicsSize, data);
        TextureDictionary td;
        td.read(reader);
        return td;
    }
}
