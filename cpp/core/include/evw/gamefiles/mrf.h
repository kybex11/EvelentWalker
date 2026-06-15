// Port of MrfFile.cs header — the "MoVE" move-network resource (.mrf) used by
// the animation/state-machine system. This port parses the file header, the
// (usually empty) Unk1 block, and the move-network trigger/flag bit tables.
// Full node-graph parsing is not ported yet.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "evw/gamefiles/data.h"

namespace evw::gamefiles
{
    // A single move-network trigger/flag: a name hash mapped to a bit position.
    struct MrfMoveNetworkBit
    {
        uint32_t name = 0;        // text hash
        uint32_t bitPosition = 0;
    };

    class MrfFile
    {
    public:
        static constexpr uint32_t MAGIC = 0x45566F4D; // 'MoVE'

        // Parses an .mrf buffer. Returns false if the header is invalid.
        bool load(const std::vector<uint8_t>& data);

        bool isLoaded() const { return loaded_; }

        uint32_t magic() const { return magic_; }
        uint32_t version() const { return version_; }
        uint32_t dataLength() const { return dataLength_; }
        uint32_t unkBytesCount() const { return unkBytesCount_; }

        const std::vector<MrfMoveNetworkBit>& triggers() const { return triggers_; }
        const std::vector<MrfMoveNetworkBit>& flags() const { return flags_; }

    private:
        bool loaded_ = false;
        uint32_t magic_ = 0;
        uint32_t version_ = 0;
        uint32_t headerUnk1_ = 0;
        uint32_t headerUnk2_ = 0;
        uint32_t headerUnk3_ = 0;
        uint32_t dataLength_ = 0;
        uint32_t unkBytesCount_ = 0;
        uint32_t unk1Count_ = 0;
        std::vector<MrfMoveNetworkBit> triggers_;
        std::vector<MrfMoveNetworkBit> flags_;
    };
}
