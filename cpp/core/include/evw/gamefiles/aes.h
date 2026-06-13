// Self-contained AES-256 (Rijndael, 128-bit block) ECB implementation.
// Replaces System.Security.Cryptography.Rijndael used by GTACrypto for the
// RAGE "AES" archive/resource encryption (256-bit key, 128-bit block, ECB, no padding).
#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace evw::crypto
{
    class Aes256Ecb
    {
    public:
        // key must be 32 bytes (256-bit).
        explicit Aes256Ecb(const std::vector<uint8_t>& key);

        // Encrypts/decrypts a single 16-byte block in place.
        void encryptBlock(uint8_t* block) const;
        void decryptBlock(uint8_t* block) const;

        // Processes only whole 16-byte blocks (length - length % 16); trailing
        // bytes are left untouched, matching the C# behavior. `rounds` repeats
        // the whole-buffer transform N times (GTA uses this for some data).
        std::vector<uint8_t> encrypt(const std::vector<uint8_t>& data, int rounds = 1) const;
        std::vector<uint8_t> decrypt(const std::vector<uint8_t>& data, int rounds = 1) const;

    private:
        static constexpr int kNk = 8;   // key length in 32-bit words (256-bit)
        static constexpr int kNb = 4;   // block size in 32-bit words (128-bit)
        static constexpr int kNr = 14;  // number of rounds for AES-256

        std::array<uint8_t, 16 * (kNr + 1)> roundKeys_{};

        void expandKey(const uint8_t* key);
    };
}
