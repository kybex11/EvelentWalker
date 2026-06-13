// Reimplementation of System.Random (.NET Framework legacy subtractive RNG),
// required to reproduce the magic.dat de-scrambling in GTAKeys exactly.
#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace evw::compat
{
    class DotNetRandom
    {
    public:
        explicit DotNetRandom(int32_t seed);

        // Fills buffer with bytes, matching System.Random.NextBytes.
        void nextBytes(uint8_t* buffer, size_t count);
        void nextBytes(std::vector<uint8_t>& buffer);

    private:
        int32_t internalSample();

        std::array<int32_t, 56> seedArray_{};
        int inext_ = 0;
        int inextp_ = 0;
    };
}
