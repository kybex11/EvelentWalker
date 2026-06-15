// Port of Particle.cs header — the particle effects list (.ypt) resource
// (ptxFxList). This port parses the pgBase structure header (name + the various
// sub-dictionary pointers) and resolves the name string; the texture/drawable/
// rule sub-dictionaries are referenced by pointer but not fully parsed yet.
#pragma once

#include <cstdint>
#include <string>

#include "evw/gamefiles/resource_data.h"

namespace evw::gamefiles
{
    struct ParticleEffectsList : public ResourceFileBase
    {
        uint64_t namePointer = 0;
        uint64_t unknown18 = 0;
        uint64_t textureDictionaryPointer = 0;
        uint64_t unknown28 = 0;
        uint64_t drawableDictionaryPointer = 0;
        uint64_t particleRuleDictionaryPointer = 0;
        uint64_t unknown40 = 0;
        uint64_t emitterRuleDictionaryPointer = 0;
        uint64_t effectRuleDictionaryPointer = 0;
        uint64_t unknown58 = 0;

        // Resolved reference data.
        std::string name;

        void read(ResourceDataReader& r);
    };
}
