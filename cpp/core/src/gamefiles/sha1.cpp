#include "evw/gamefiles/sha1.h"

#include <cstring>

namespace evw::crypto
{
    namespace
    {
        inline uint32_t rol(uint32_t v, int b) { return (v << b) | (v >> (32 - b)); }
    }

    std::array<uint8_t, 20> sha1(const uint8_t* data, size_t length)
    {
        uint32_t h0 = 0x67452301, h1 = 0xEFCDAB89, h2 = 0x98BADCFE,
                 h3 = 0x10325476, h4 = 0xC3D2E1F0;

        // Pre-processing: append 0x80, pad to 56 mod 64, then 64-bit bit length.
        const uint64_t bitLen = static_cast<uint64_t>(length) * 8;
        std::vector<uint8_t> msg(data, data + length);
        msg.push_back(0x80);
        while (msg.size() % 64 != 56) msg.push_back(0x00);
        for (int i = 7; i >= 0; --i) msg.push_back(static_cast<uint8_t>((bitLen >> (i * 8)) & 0xFF));

        for (size_t chunk = 0; chunk < msg.size(); chunk += 64)
        {
            uint32_t w[80];
            for (int i = 0; i < 16; ++i)
            {
                const uint8_t* p = msg.data() + chunk + i * 4;
                w[i] = (static_cast<uint32_t>(p[0]) << 24) | (static_cast<uint32_t>(p[1]) << 16) |
                       (static_cast<uint32_t>(p[2]) << 8) | static_cast<uint32_t>(p[3]);
            }
            for (int i = 16; i < 80; ++i)
                w[i] = rol(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);

            uint32_t a = h0, b = h1, c = h2, d = h3, e = h4;
            for (int i = 0; i < 80; ++i)
            {
                uint32_t f, k;
                if (i < 20)      { f = (b & c) | ((~b) & d);            k = 0x5A827999; }
                else if (i < 40) { f = b ^ c ^ d;                       k = 0x6ED9EBA1; }
                else if (i < 60) { f = (b & c) | (b & d) | (c & d);     k = 0x8F1BBCDC; }
                else             { f = b ^ c ^ d;                       k = 0xCA62C1D6; }

                uint32_t tmp = rol(a, 5) + f + e + k + w[i];
                e = d; d = c; c = rol(b, 30); b = a; a = tmp;
            }
            h0 += a; h1 += b; h2 += c; h3 += d; h4 += e;
        }

        std::array<uint8_t, 20> out{};
        uint32_t hs[5] = {h0, h1, h2, h3, h4};
        for (int i = 0; i < 5; ++i)
        {
            out[i * 4 + 0] = static_cast<uint8_t>((hs[i] >> 24) & 0xFF);
            out[i * 4 + 1] = static_cast<uint8_t>((hs[i] >> 16) & 0xFF);
            out[i * 4 + 2] = static_cast<uint8_t>((hs[i] >> 8) & 0xFF);
            out[i * 4 + 3] = static_cast<uint8_t>(hs[i] & 0xFF);
        }
        return out;
    }

    std::array<uint8_t, 20> sha1(const std::vector<uint8_t>& data)
    {
        return sha1(data.data(), data.size());
    }
}
