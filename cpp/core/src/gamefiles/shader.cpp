#include "evw/gamefiles/shader.h"

namespace evw::gamefiles
{
    void ShaderParameter::readHeader(ResourceDataReader& r)
    {
        dataType = r.ReadByte();
        r.ReadByte();        // Unknown_1h
        r.ReadUInt16();      // Unknown_2h
        r.ReadUInt32();      // Unknown_4h
        dataPointer = r.ReadUInt64();
    }

    void ShaderParametersBlock::read(ResourceDataReader& r, int count)
    {
        parameters.clear();
        parameters.resize(count);
        for (int i = 0; i < count; ++i)
            parameters[i].readHeader(r);

        long long offset = 0;
        for (auto& p : parameters)
        {
            if (p.dataType == 0)
            {
                p.texture = r.ReadBlockAt<TextureBase>(p.dataPointer);
            }
            else if (p.dataType == 1)
            {
                offset += 16;
                auto v = r.ReadStructsAt<math::Vector4>(p.dataPointer, 1);
                if (!v.empty()) p.vectors = std::move(v);
            }
            else
            {
                offset += 16 * p.dataType;
                p.vectors = r.ReadStructsAt<math::Vector4>(p.dataPointer, p.dataType);
            }
        }

        r.setPosition(r.position() + static_cast<uint64_t>(offset));

        hashes.clear();
        hashes.reserve(count);
        for (int i = 0; i < count; ++i)
            hashes.push_back(r.ReadUInt32());
    }

    size_t ShaderParametersBlock::textureCount() const
    {
        size_t c = 0;
        for (const auto& p : parameters)
            if (p.dataType == 0) ++c;
        return c;
    }

    void ShaderFX::read(ResourceDataReader& r)
    {
        parametersPointer = r.ReadUInt64();
        name = r.ReadUInt32();
        r.ReadUInt32();                 // Unknown_Ch
        parameterCount = r.ReadByte();
        renderBucket = r.ReadByte();
        r.ReadUInt16();                 // Unknown_12h
        parameterSize = r.ReadUInt16();
        parameterDataSize = r.ReadUInt16();
        fileName = r.ReadUInt32();
        r.ReadUInt32();                 // Unknown_1Ch
        renderBucketMask = r.ReadUInt32();
        r.ReadUInt16();                 // Unknown_24h
        r.ReadByte();                   // Unknown_26h
        textureParametersCount = r.ReadByte();
        r.ReadUInt64();                 // Unknown_28h
        // ParametersList (ShaderParametersBlock): list of parameters + name hashes.
        if (parametersPointer != 0 && parameterCount > 0)
        {
            uint64_t backup = r.position();
            r.setPosition(parametersPointer);
            parametersList = std::make_shared<ShaderParametersBlock>();
            parametersList->read(r, parameterCount);
            r.setPosition(backup);
        }
    }

    void ShaderGroup::read(ResourceDataReader& r)
    {
        r.ReadUInt32();                 // VFT
        r.ReadUInt32();                 // Unknown_4h
        textureDictionaryPointer = r.ReadUInt64();
        shadersPointer = r.ReadUInt64();
        shadersCount = r.ReadUInt16();
        r.ReadUInt16();                 // ShadersCount2
        r.ReadUInt32();                 // Unknown_1Ch
        r.ReadUInt64();                 // Unknown_20h
        r.ReadUInt64();                 // Unknown_28h
        r.ReadUInt32();                 // ShaderGroupBlocksSize
        r.ReadUInt32();                 // Unknown_34h
        r.ReadUInt64();                 // Unknown_38h

        textureDictionary = r.ReadBlockAt<TextureDictionary>(textureDictionaryPointer);

        if (shadersPointer != 0 && shadersCount != 0)
        {
            uint64_t backup = r.position();
            r.setPosition(shadersPointer);
            shaders.read(r, shadersCount);
            r.setPosition(backup);
        }
    }
}
