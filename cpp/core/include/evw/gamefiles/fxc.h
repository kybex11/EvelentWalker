// Port of FxcFile.cs (header) — the compiled-shader container (.fxc, "rgxe").
// This port reads the magic, vertex type and the preset-parameter list; the full
// shader-group / cbuffer / technique tables are not parsed yet.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace evw::gamefiles
{
    struct FxcPresetParam
    {
        std::string name;   // e.g. "__rage_drawbucket"
        uint32_t value = 0;
    };

    class FxcFile
    {
    public:
        static constexpr uint32_t MAGIC = 1702389618u; // "rgxe"

        // Parses a .fxc buffer header. Returns false if the magic is wrong.
        bool load(const std::vector<uint8_t>& data);

        bool isLoaded() const { return loaded_; }
        uint32_t vertexType() const { return vertexType_; }
        const std::vector<FxcPresetParam>& presetParams() const { return presetParams_; }

    private:
        bool loaded_ = false;
        uint32_t vertexType_ = 0;
        std::vector<FxcPresetParam> presetParams_;
    };
}
