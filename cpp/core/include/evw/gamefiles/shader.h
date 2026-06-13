// Port of ShaderGroup / ShaderFX from Drawable.cs (legacy read path).
// ShaderGroup binds a drawable's embedded TextureDictionary and its list of
// shaders (materials). The detailed ShaderParametersBlock is not parsed yet
// (the parameters pointer is retained for a later pass).
#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "evw/gamefiles/resource_data.h"
#include "evw/gamefiles/texture.h"
#include "evw/math/math.h"

namespace evw::gamefiles
{
    // A single shader parameter: either a texture reference (dataType 0) or one
    // or more Vector4 values (dataType >= 1, count = dataType).
    struct ShaderParameter
    {
        uint8_t dataType = 0;
        uint64_t dataPointer = 0;
        std::shared_ptr<TextureBase> texture;   // when dataType == 0
        std::vector<math::Vector4> vectors;     // when dataType >= 1

        void readHeader(ResourceDataReader& r);
    };

    // The parameter block for a ShaderFX: a list of parameters followed by their
    // name hashes. Texture parameters reference TextureBase blocks by pointer.
    struct ShaderParametersBlock
    {
        std::vector<ShaderParameter> parameters;
        std::vector<uint32_t> hashes;

        // count comes from the owning ShaderFX (ParameterCount).
        void read(ResourceDataReader& r, int count);

        // Number of texture parameters (dataType == 0).
        size_t textureCount() const;
    };

    // A single material/shader (48-byte legacy structure).
    struct ShaderFX
    {
        uint64_t parametersPointer = 0;
        uint32_t name = 0;           // MetaHash, e.g. hash of "spec"
        uint8_t parameterCount = 0;
        uint8_t renderBucket = 0;
        uint16_t parameterSize = 0;
        uint16_t parameterDataSize = 0;
        uint32_t fileName = 0;       // MetaHash, e.g. hash of "spec.sps"
        uint32_t renderBucketMask = 0;
        uint8_t textureParametersCount = 0;

        std::shared_ptr<ShaderParametersBlock> parametersList;

        void read(ResourceDataReader& r);
    };

    struct ShaderGroup
    {
        uint64_t textureDictionaryPointer = 0;
        uint64_t shadersPointer = 0;
        uint16_t shadersCount = 0;

        std::shared_ptr<TextureDictionary> textureDictionary;
        ResourcePointerArray64<ShaderFX> shaders;

        void read(ResourceDataReader& r);
    };
}
