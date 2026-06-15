// Port of EvelentWalker.Core/GameFiles/Utils/GTACrypto.cs (decryption path) and
// the key container from GTAKeys.cs.
//
// STATUS:
//  - AES-256 ECB encrypt/decrypt: ported (uses self-contained evw::crypto::Aes256Ecb).
//  - NG decryption (DecryptNG / block / rounds A,B): ported.
//  - NG encryption + key loading from magic.dat (Deflate + AES + RandomGauss LUT
//    generation): NOT ported yet (TODO) — depends on embedded resource & solver.
#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace evw::gamefiles
{
    // Holds the GTA5 decryption keys/tables once loaded.
    // Loading from magic.dat is not implemented yet; tables can be injected
    // directly (e.g. for tests or once a loader exists).
    struct GTA5Keys
    {
        std::vector<uint8_t> PC_AES_KEY;                                   // 32 bytes
        std::vector<std::vector<uint8_t>> PC_NG_KEYS;                      // [101][272]
        // [17][16][256] decryption tables.
        std::array<std::array<std::array<uint32_t, 256>, 16>, 17> PC_NG_DECRYPT_TABLES{};
        std::vector<uint8_t> PC_LUT;                                       // 256 bytes
        std::array<uint32_t, 4> PC_AWC_KEY{};

        bool ngTablesLoaded = false;

        // Selects an NG key by name+length, mirroring GTACrypto.GetNGKey.
        const std::vector<uint8_t>& getNGKey(const std::string& name, uint32_t length) const;

        // Loads NG keys/tables/LUT from a decrypted+inflated magic.dat blob.
        // Layout (bytes): [27472 NG keys][278528 decrypt tables][256 LUT][16 AWC key].
        // Mirrors GTAKeys.UseMagicData's unpacking step.
        void loadFromMagicBlob(const std::vector<uint8_t>& blob);

        // Full magic.dat pipeline: de-scramble with .NET Random(seed=Jenk(aesKey)),
        // AES-decrypt, raw-inflate, then unpack. `magic` is the raw resource bytes.
        void loadFromMagic(const std::vector<uint8_t>& magic, const std::vector<uint8_t>& aesKey);

        // Loads keys from a folder of CodeWalker-format .dat files:
        //   gtav_aes_key.dat (32), gtav_ng_key.dat (27472),
        //   gtav_ng_decrypt_tables.dat (278528), gtav_hash_lut.dat (256),
        //   gtav_awc_key.dat (16, optional). Returns true on success.
        bool loadFromKeysFolder(const std::string& folder);

        // Searches gta5.exe for the AES key by its known SHA-1 and stores it.
        // Returns true if found. NG keys/tables still need a Keys folder/magic.dat,
        // but this enables AES-encrypted archives and lays the exe-extraction base.
        bool loadAesKeyFromExe(const std::string& exePath);

        // True once NG keys + tables are available (real archives can be read).
        bool isLoaded() const { return ngTablesLoaded && !PC_NG_KEYS.empty(); }
    };

    namespace GTACrypto
    {
        // AES (256-bit, ECB, no padding) over whole 16-byte blocks.
        std::vector<uint8_t> DecryptAESData(const std::vector<uint8_t>& data,
                                            const std::vector<uint8_t>& key, int rounds = 1);
        std::vector<uint8_t> EncryptAESData(const std::vector<uint8_t>& data,
                                            const std::vector<uint8_t>& key, int rounds = 1);

        // NG decryption (the format used by most GTA5 PC RPF entries).
        std::vector<uint8_t> DecryptNG(const std::vector<uint8_t>& data,
                                       const std::vector<uint8_t>& key,
                                       const GTA5Keys& keys);
        std::vector<uint8_t> DecryptNG(const std::vector<uint8_t>& data,
                                       const std::string& name, uint32_t length,
                                       const GTA5Keys& keys);
    }
}
