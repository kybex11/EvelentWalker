#include "evw/gamefiles/jenk.h"

namespace evw::gamefiles
{
    namespace
    {
        uint32_t computeHash(const uint8_t* data, size_t length)
        {
            uint32_t h = 0;
            for (size_t i = 0; i < length; ++i)
            {
                h += data[i];
                h += (h << 10);
                h ^= (h >> 6);
            }
            h += (h << 3);
            h ^= (h >> 11);
            h += (h << 15);
            return h;
        }
    }

    uint32_t JenkHash::GenHash(std::string_view text)
    {
        return computeHash(reinterpret_cast<const uint8_t*>(text.data()), text.size());
    }

    uint32_t JenkHash::GenHash(const std::vector<uint8_t>& data)
    {
        return computeHash(data.data(), data.size());
    }

    uint32_t JenkHash::GenHash(const uint8_t* data, size_t length)
    {
        return computeHash(data, length);
    }

    std::unordered_map<uint32_t, std::string>& JenkIndex::index()
    {
        static std::unordered_map<uint32_t, std::string> idx;
        return idx;
    }

    std::mutex& JenkIndex::mutex()
    {
        static std::mutex m;
        return m;
    }

    void JenkIndex::Clear()
    {
        std::lock_guard<std::mutex> lock(mutex());
        index().clear();
    }

    bool JenkIndex::Ensure(const std::string& str)
    {
        uint32_t hash = JenkHash::GenHash(str);
        if (hash == 0) return true;
        std::lock_guard<std::mutex> lock(mutex());
        auto& idx = index();
        if (idx.find(hash) == idx.end())
        {
            idx.emplace(hash, str);
            return false;
        }
        return true;
    }

    size_t JenkIndex::EnsureAll(const std::vector<std::string>& strings)
    {
        size_t added = 0;
        for (const auto& s : strings)
            if (!Ensure(s)) ++added;
        return added;
    }

    std::string JenkIndex::GetString(uint32_t hash)
    {
        std::lock_guard<std::mutex> lock(mutex());
        auto& idx = index();
        auto it = idx.find(hash);
        if (it != idx.end()) return it->second;
        return std::to_string(hash);
    }

    std::string JenkIndex::TryGetString(uint32_t hash)
    {
        std::lock_guard<std::mutex> lock(mutex());
        auto& idx = index();
        auto it = idx.find(hash);
        if (it != idx.end()) return it->second;
        return std::string();
    }

    std::vector<std::string> JenkIndex::GetAllStrings()
    {
        std::lock_guard<std::mutex> lock(mutex());
        std::vector<std::string> result;
        result.reserve(index().size());
        for (const auto& kv : index()) result.push_back(kv.second);
        return result;
    }
}
