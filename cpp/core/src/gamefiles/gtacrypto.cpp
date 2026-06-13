#include "evw/gamefiles/gtacrypto.h"

#include <cstring>
#include <stdexcept>

#include "evw/gamefiles/aes.h"
#include "evw/gamefiles/dotnet_random.h"
#include "evw/gamefiles/inflate.h"
#include "evw/gamefiles/jenk.h"

namespace evw::gamefiles
{
    namespace
    {
        // GTA5Hash.CalculateHash, used to select an NG key. Requires PC_LUT.
        uint32_t calculateHash(const std::string& text, const std::vector<uint8_t>& lut)
        {
            uint32_t result = 0;
            for (char ch : text)
            {
                uint8_t c = static_cast<uint8_t>(ch);
                uint32_t temp = 1025u * (static_cast<uint32_t>(lut[c]) + result);
                result = (temp >> 6) ^ temp;
            }
            return 32769u * (((9u * result) >> 11) ^ (9u * result));
        }

        // round 1, 2, 16
        void decryptRoundA(uint8_t* data,
                           const uint32_t* key,
                           const std::array<std::array<uint32_t, 256>, 16>& table)
        {
            uint32_t x1 = table[0][data[0]] ^ table[1][data[1]] ^ table[2][data[2]] ^ table[3][data[3]] ^ key[0];
            uint32_t x2 = table[4][data[4]] ^ table[5][data[5]] ^ table[6][data[6]] ^ table[7][data[7]] ^ key[1];
            uint32_t x3 = table[8][data[8]] ^ table[9][data[9]] ^ table[10][data[10]] ^ table[11][data[11]] ^ key[2];
            uint32_t x4 = table[12][data[12]] ^ table[13][data[13]] ^ table[14][data[14]] ^ table[15][data[15]] ^ key[3];
            std::memcpy(data + 0, &x1, 4);
            std::memcpy(data + 4, &x2, 4);
            std::memcpy(data + 8, &x3, 4);
            std::memcpy(data + 12, &x4, 4);
        }

        // round 3-15
        void decryptRoundB(uint8_t* data,
                           const uint32_t* key,
                           const std::array<std::array<uint32_t, 256>, 16>& table)
        {
            uint32_t x1 = table[0][data[0]] ^ table[7][data[7]] ^ table[10][data[10]] ^ table[13][data[13]] ^ key[0];
            uint32_t x2 = table[1][data[1]] ^ table[4][data[4]] ^ table[11][data[11]] ^ table[14][data[14]] ^ key[1];
            uint32_t x3 = table[2][data[2]] ^ table[5][data[5]] ^ table[8][data[8]] ^ table[15][data[15]] ^ key[2];
            uint32_t x4 = table[3][data[3]] ^ table[6][data[6]] ^ table[9][data[9]] ^ table[12][data[12]] ^ key[3];
            std::memcpy(data + 0, &x1, 4);
            std::memcpy(data + 4, &x2, 4);
            std::memcpy(data + 8, &x3, 4);
            std::memcpy(data + 12, &x4, 4);
        }

        void decryptNGBlock(uint8_t* block, const uint32_t* keyuints, const GTA5Keys& keys)
        {
            // subKeys[i] = keyuints + 4*i (17 sub-keys of 4 uints each)
            decryptRoundA(block, keyuints + 0 * 4, keys.PC_NG_DECRYPT_TABLES[0]);
            decryptRoundA(block, keyuints + 1 * 4, keys.PC_NG_DECRYPT_TABLES[1]);
            for (int k = 2; k <= 15; ++k)
                decryptRoundB(block, keyuints + k * 4, keys.PC_NG_DECRYPT_TABLES[k]);
            decryptRoundA(block, keyuints + 16 * 4, keys.PC_NG_DECRYPT_TABLES[16]);
        }
    }

    const std::vector<uint8_t>& GTA5Keys::getNGKey(const std::string& name, uint32_t length) const
    {
        uint32_t hash = calculateHash(name, PC_LUT);
        uint32_t keyidx = (hash + length + (101 - 40)) % 0x65;
        return PC_NG_KEYS[keyidx];
    }

    void GTA5Keys::loadFromMagicBlob(const std::vector<uint8_t>& b)
    {
        // [27472 NG keys][278528 decrypt tables][256 LUT][16 AWC key]
        constexpr size_t kKeysSize = 27472;   // 101 * 272
        constexpr size_t kTablesSize = 278528; // 17 * 16 * 256 * 4
        constexpr size_t kLutSize = 256;
        constexpr size_t kAwcSize = 16;
        if (b.size() < kKeysSize + kTablesSize + kLutSize + kAwcSize)
            throw std::runtime_error("magic blob too small");

        size_t bp = 0;

        // NG keys: 101 entries of 272 bytes.
        PC_NG_KEYS.assign(101, std::vector<uint8_t>(272));
        for (int i = 0; i < 101; ++i)
        {
            std::memcpy(PC_NG_KEYS[i].data(), b.data() + bp, 272);
            bp += 272;
        }

        // Decrypt tables: 17 x 16 x 256 little-endian uint32.
        for (int i = 0; i < 17; ++i)
            for (int j = 0; j < 16; ++j)
                for (int k = 0; k < 256; ++k)
                {
                    uint32_t v;
                    std::memcpy(&v, b.data() + bp, 4);
                    PC_NG_DECRYPT_TABLES[i][j][k] = v;
                    bp += 4;
                }
        ngTablesLoaded = true;

        // LUT.
        PC_LUT.assign(b.begin() + bp, b.begin() + bp + kLutSize);
        bp += kLutSize;

        // AWC key: 4 little-endian uint32.
        for (int i = 0; i < 4; ++i)
        {
            uint32_t v;
            std::memcpy(&v, b.data() + bp, 4);
            PC_AWC_KEY[i] = v;
            bp += 4;
        }
    }

    void GTA5Keys::loadFromMagic(const std::vector<uint8_t>& magic, const std::vector<uint8_t>& aesKey)
    {
        PC_AES_KEY = aesKey;

        // De-scramble: subtract four .NET Random byte streams seeded by Jenk(aesKey).
        int32_t seed = static_cast<int32_t>(JenkHash::GenHash(aesKey));
        compat::DotNetRandom rnd(seed);
        const size_t n = magic.size();
        std::vector<uint8_t> rb1(n), rb2(n), rb3(n), rb4(n);
        rnd.nextBytes(rb1);
        rnd.nextBytes(rb2);
        rnd.nextBytes(rb3);
        rnd.nextBytes(rb4);

        std::vector<uint8_t> db(n);
        for (size_t i = 0; i < n; ++i)
            db[i] = static_cast<uint8_t>((magic[i] - rb1[i] - rb2[i] - rb3[i] - rb4[i]) & 0xFF);

        // AES-decrypt, then raw-inflate.
        db = GTACrypto::DecryptAESData(db, aesKey);
        std::vector<uint8_t> blob = compression::inflateRaw(db);

        loadFromMagicBlob(blob);
    }

    namespace GTACrypto
    {
        std::vector<uint8_t> DecryptAESData(const std::vector<uint8_t>& data,
                                            const std::vector<uint8_t>& key, int rounds)
        {
            crypto::Aes256Ecb aes(key);
            return aes.decrypt(data, rounds);
        }

        std::vector<uint8_t> EncryptAESData(const std::vector<uint8_t>& data,
                                            const std::vector<uint8_t>& key, int rounds)
        {
            crypto::Aes256Ecb aes(key);
            return aes.encrypt(data, rounds);
        }

        std::vector<uint8_t> DecryptNG(const std::vector<uint8_t>& data,
                                       const std::vector<uint8_t>& key,
                                       const GTA5Keys& keys)
        {
            std::vector<uint8_t> out(data.size());

            // Reinterpret key bytes as little-endian uint32 array (key length / 4).
            std::vector<uint32_t> keyuints(key.size() / 4);
            std::memcpy(keyuints.data(), key.data(), keyuints.size() * 4);

            const size_t blocks = data.size() / 16;
            for (size_t b = 0; b < blocks; ++b)
            {
                uint8_t block[16];
                std::memcpy(block, data.data() + 16 * b, 16);
                decryptNGBlock(block, keyuints.data(), keys);
                std::memcpy(out.data() + 16 * b, block, 16);
            }

            // Copy the trailing (< 16) bytes unchanged.
            if (data.size() % 16 != 0)
            {
                size_t left = data.size() % 16;
                std::memcpy(out.data() + data.size() - left, data.data() + data.size() - left, left);
            }
            return out;
        }

        std::vector<uint8_t> DecryptNG(const std::vector<uint8_t>& data,
                                       const std::string& name, uint32_t length,
                                       const GTA5Keys& keys)
        {
            const auto& key = keys.getNGKey(name, length);
            return DecryptNG(data, key, keys);
        }
    }
}
