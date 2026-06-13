// Port of EvelentWalker.Core/GameFiles/Utils/Jenk.cs
#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace evw::gamefiles
{
    enum class JenkHashInputEncoding
    {
        UTF8 = 0,
        ASCII = 1,
    };

    // Jenkins one-at-a-time hash, matching the C# implementation bit-for-bit.
    class JenkHash
    {
    public:
        // Hashes the raw bytes of the string. For ASCII/UTF8 inputs the byte
        // sequence is identical, so a single overload covers both encodings used
        // by the original code.
        static uint32_t GenHash(std::string_view text);
        static uint32_t GenHash(const std::vector<uint8_t>& data);
        static uint32_t GenHash(const uint8_t* data, size_t length);
    };

    // Thread-safe global mapping of hash -> original string (port of JenkIndex).
    class JenkIndex
    {
    public:
        static void Clear();
        // Returns true if the hash was already present (or empty), false if newly added.
        static bool Ensure(const std::string& str);
        // Returns the string, or the decimal representation of the hash if unknown.
        static std::string GetString(uint32_t hash);
        // Returns the string, or an empty string if unknown.
        static std::string TryGetString(uint32_t hash);
        static std::vector<std::string> GetAllStrings();

    private:
        static std::unordered_map<uint32_t, std::string>& index();
        static std::mutex& mutex();
    };
}
