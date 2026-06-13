#include "evw/gamefiles/dotnet_random.h"

namespace evw::compat
{
    namespace
    {
        constexpr int32_t MBIG = 2147483647;   // Int32.MaxValue
        constexpr int32_t MSEED = 161803398;
    }

    DotNetRandom::DotNetRandom(int32_t seed)
    {
        int32_t subtraction = (seed == (-2147483647 - 1)) ? 2147483647 : (seed < 0 ? -seed : seed);
        int32_t mj = MSEED - subtraction;
        seedArray_[55] = mj;
        int32_t mk = 1;
        for (int i = 1; i < 55; ++i)
        {
            int ii = (21 * i) % 55;
            seedArray_[ii] = mk;
            mk = mj - mk;
            if (mk < 0) mk += MBIG;
            mj = seedArray_[ii];
        }
        for (int k = 1; k < 5; ++k)
        {
            for (int i = 1; i < 56; ++i)
            {
                seedArray_[i] -= seedArray_[1 + (i + 30) % 55];
                if (seedArray_[i] < 0) seedArray_[i] += MBIG;
            }
        }
        inext_ = 0;
        inextp_ = 21;
    }

    int32_t DotNetRandom::internalSample()
    {
        int locINext = inext_;
        int locINextp = inextp_;
        if (++locINext >= 56) locINext = 1;
        if (++locINextp >= 56) locINextp = 1;

        int32_t retVal = seedArray_[locINext] - seedArray_[locINextp];
        if (retVal == MBIG) retVal--;
        if (retVal < 0) retVal += MBIG;
        seedArray_[locINext] = retVal;

        inext_ = locINext;
        inextp_ = locINextp;
        return retVal;
    }

    void DotNetRandom::nextBytes(uint8_t* buffer, size_t count)
    {
        for (size_t i = 0; i < count; ++i)
            buffer[i] = static_cast<uint8_t>(internalSample() % 256);
    }

    void DotNetRandom::nextBytes(std::vector<uint8_t>& buffer)
    {
        nextBytes(buffer.data(), buffer.size());
    }
}
