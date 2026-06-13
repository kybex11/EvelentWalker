#include "evw/gamefiles/aes.h"

#include <stdexcept>

namespace evw::crypto
{
    namespace
    {
        // AES S-box and inverse S-box (FIPS-197).
        const uint8_t kSBox[256] = {
            0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
            0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
            0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
            0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
            0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
            0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
            0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
            0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
            0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
            0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
            0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
            0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
            0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
            0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
            0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
            0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16
        };

        uint8_t invSBox[256];
        const uint8_t kRcon[15] = {
            0x00,0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x1b,0x36,0x6c,0xd8,0xab,0x4d
        };

        struct InvSBoxInit
        {
            InvSBoxInit() { for (int i = 0; i < 256; ++i) invSBox[kSBox[i]] = static_cast<uint8_t>(i); }
        } invSBoxInit;

        // Multiplication in GF(2^8).
        uint8_t xtime(uint8_t x) { return static_cast<uint8_t>((x << 1) ^ ((x >> 7) * 0x1b)); }

        uint8_t gmul(uint8_t a, uint8_t b)
        {
            uint8_t p = 0;
            for (int i = 0; i < 8; ++i)
            {
                if (b & 1) p ^= a;
                uint8_t hi = a & 0x80;
                a <<= 1;
                if (hi) a ^= 0x1b;
                b >>= 1;
            }
            return p;
        }
    }

    Aes256Ecb::Aes256Ecb(const std::vector<uint8_t>& key)
    {
        if (key.size() != 32)
            throw std::invalid_argument("AES-256 requires a 32-byte key");
        expandKey(key.data());
    }

    void Aes256Ecb::expandKey(const uint8_t* key)
    {
        const int totalWords = kNb * (kNr + 1); // 60 words
        uint8_t w[60][4];

        for (int i = 0; i < kNk; ++i)
            for (int j = 0; j < 4; ++j)
                w[i][j] = key[i * 4 + j];

        for (int i = kNk; i < totalWords; ++i)
        {
            uint8_t temp[4] = {w[i - 1][0], w[i - 1][1], w[i - 1][2], w[i - 1][3]};
            if (i % kNk == 0)
            {
                // RotWord + SubWord + Rcon
                uint8_t t = temp[0];
                temp[0] = static_cast<uint8_t>(kSBox[temp[1]] ^ kRcon[i / kNk]);
                temp[1] = kSBox[temp[2]];
                temp[2] = kSBox[temp[3]];
                temp[3] = kSBox[t];
            }
            else if (i % kNk == 4)
            {
                // SubWord only (AES-256 specific)
                for (int j = 0; j < 4; ++j) temp[j] = kSBox[temp[j]];
            }
            for (int j = 0; j < 4; ++j)
                w[i][j] = static_cast<uint8_t>(w[i - kNk][j] ^ temp[j]);
        }

        for (int i = 0; i < totalWords; ++i)
            for (int j = 0; j < 4; ++j)
                roundKeys_[i * 4 + j] = w[i][j];
    }

    void Aes256Ecb::encryptBlock(uint8_t* s) const
    {
        auto addRoundKey = [&](int round) {
            for (int i = 0; i < 16; ++i) s[i] ^= roundKeys_[round * 16 + i];
        };

        addRoundKey(0);
        for (int round = 1; round <= kNr; ++round)
        {
            // SubBytes
            for (int i = 0; i < 16; ++i) s[i] = kSBox[s[i]];
            // ShiftRows (state is column-major: s[r + 4c])
            uint8_t t;
            t = s[1]; s[1] = s[5]; s[5] = s[9]; s[9] = s[13]; s[13] = t;
            t = s[2]; s[2] = s[10]; s[10] = t; t = s[6]; s[6] = s[14]; s[14] = t;
            t = s[15]; s[15] = s[11]; s[11] = s[7]; s[7] = s[3]; s[3] = t;
            // MixColumns (skip on final round)
            if (round != kNr)
            {
                for (int c = 0; c < 4; ++c)
                {
                    uint8_t* col = s + c * 4;
                    uint8_t a0 = col[0], a1 = col[1], a2 = col[2], a3 = col[3];
                    col[0] = static_cast<uint8_t>(xtime(a0) ^ (xtime(a1) ^ a1) ^ a2 ^ a3);
                    col[1] = static_cast<uint8_t>(a0 ^ xtime(a1) ^ (xtime(a2) ^ a2) ^ a3);
                    col[2] = static_cast<uint8_t>(a0 ^ a1 ^ xtime(a2) ^ (xtime(a3) ^ a3));
                    col[3] = static_cast<uint8_t>((xtime(a0) ^ a0) ^ a1 ^ a2 ^ xtime(a3));
                }
            }
            addRoundKey(round);
        }
    }

    void Aes256Ecb::decryptBlock(uint8_t* s) const
    {
        auto addRoundKey = [&](int round) {
            for (int i = 0; i < 16; ++i) s[i] ^= roundKeys_[round * 16 + i];
        };

        addRoundKey(kNr);
        for (int round = kNr - 1; round >= 0; --round)
        {
            // InvShiftRows
            uint8_t t;
            t = s[13]; s[13] = s[9]; s[9] = s[5]; s[5] = s[1]; s[1] = t;
            t = s[2]; s[2] = s[10]; s[10] = t; t = s[6]; s[6] = s[14]; s[14] = t;
            t = s[3]; s[3] = s[7]; s[7] = s[11]; s[11] = s[15]; s[15] = t;
            // InvSubBytes
            for (int i = 0; i < 16; ++i) s[i] = invSBox[s[i]];
            addRoundKey(round);
            // InvMixColumns (skip on what was the first round)
            if (round != 0)
            {
                for (int c = 0; c < 4; ++c)
                {
                    uint8_t* col = s + c * 4;
                    uint8_t a0 = col[0], a1 = col[1], a2 = col[2], a3 = col[3];
                    col[0] = static_cast<uint8_t>(gmul(a0, 14) ^ gmul(a1, 11) ^ gmul(a2, 13) ^ gmul(a3, 9));
                    col[1] = static_cast<uint8_t>(gmul(a0, 9) ^ gmul(a1, 14) ^ gmul(a2, 11) ^ gmul(a3, 13));
                    col[2] = static_cast<uint8_t>(gmul(a0, 13) ^ gmul(a1, 9) ^ gmul(a2, 14) ^ gmul(a3, 11));
                    col[3] = static_cast<uint8_t>(gmul(a0, 11) ^ gmul(a1, 13) ^ gmul(a2, 9) ^ gmul(a3, 14));
                }
            }
        }
    }

    std::vector<uint8_t> Aes256Ecb::encrypt(const std::vector<uint8_t>& data, int rounds) const
    {
        std::vector<uint8_t> buffer = data;
        const size_t length = data.size() - data.size() % 16;
        for (int r = 0; r < rounds; ++r)
            for (size_t off = 0; off < length; off += 16)
                encryptBlock(buffer.data() + off);
        return buffer;
    }

    std::vector<uint8_t> Aes256Ecb::decrypt(const std::vector<uint8_t>& data, int rounds) const
    {
        std::vector<uint8_t> buffer = data;
        const size_t length = data.size() - data.size() % 16;
        for (int r = 0; r < rounds; ++r)
            for (size_t off = 0; off < length; off += 16)
                decryptBlock(buffer.data() + off);
        return buffer;
    }
}
