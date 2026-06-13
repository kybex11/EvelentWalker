#include "evw/gamefiles/rpf.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

#include "evw/gamefiles/data.h"
#include "evw/gamefiles/inflate.h"

namespace evw::gamefiles
{
    namespace
    {
        std::string toLower(std::string s)
        {
            std::transform(s.begin(), s.end(), s.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return s;
        }

        bool endsWith(const std::string& s, const std::string& suffix)
        {
            return s.size() >= suffix.size() &&
                   s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
        }
    }

    // ---- Data sources ----

    void MemoryDataSource::readAt(int64_t pos, uint8_t* out, size_t len)
    {
        if (pos < 0 || pos + static_cast<int64_t>(len) > static_cast<int64_t>(data_.size()))
            throw std::out_of_range("MemoryDataSource: read out of range");
        std::memcpy(out, data_.data() + pos, len);
    }

    FileDataSource::FileDataSource(const std::string& path)
    {
        FILE* f = std::fopen(path.c_str(), "rb");
        if (!f) throw std::runtime_error("Cannot open RPF file: " + path);
        file_ = std::shared_ptr<void>(f, [](void* p) { std::fclose(static_cast<FILE*>(p)); });
        std::fseek(f, 0, SEEK_END);
        size_ = std::ftell(f);
        std::fseek(f, 0, SEEK_SET);
    }

    void FileDataSource::readAt(int64_t pos, uint8_t* out, size_t len)
    {
        FILE* f = static_cast<FILE*>(file_.get());
        if (std::fseek(f, static_cast<long>(pos), SEEK_SET) != 0)
            throw std::runtime_error("RPF seek failed");
        if (std::fread(out, 1, len, f) != len)
            throw std::runtime_error("RPF read failed");
    }

    // ---- Entry helpers ----

    uint32_t getSizeFromFlags(uint32_t flags)
    {
        uint32_t s0 = ((flags >> 27) & 0x1) << 0;
        uint32_t s1 = ((flags >> 26) & 0x1) << 1;
        uint32_t s2 = ((flags >> 25) & 0x1) << 2;
        uint32_t s3 = ((flags >> 24) & 0x1) << 3;
        uint32_t s4 = ((flags >> 17) & 0x7F) << 4;
        uint32_t s5 = ((flags >> 11) & 0x3F) << 5;
        uint32_t s6 = ((flags >> 7) & 0xF) << 6;
        uint32_t s7 = ((flags >> 5) & 0x3) << 7;
        uint32_t s8 = ((flags >> 4) & 0x1) << 8;
        uint32_t ss = (flags >> 0) & 0xF;
        uint32_t baseSize = 0x200u << ss;
        return baseSize * (s0 + s1 + s2 + s3 + s4 + s5 + s6 + s7 + s8);
    }

    long long RpfEntry::getFileSize() const
    {        if (type == RpfEntryType::Binary)
            return (fileSize == 0) ? fileUncompressedSize : fileSize;
        if (type == RpfEntryType::Resource)
            return (fileSize == 0)
                       ? static_cast<long long>(getSizeFromFlags(systemFlags)) + getSizeFromFlags(graphicsFlags)
                       : fileSize;
        return 0;
    }

    uint32_t RpfEntry::systemSize() const { return getSizeFromFlags(systemFlags); }
    uint32_t RpfEntry::graphicsSize() const { return getSizeFromFlags(graphicsFlags); }

    // ---- RpfFile ----

    RpfFile::RpfFile(std::string name, std::string path, long long fileSize, long long startPos)
        : name_(std::move(name)), path_(std::move(path)), fileSize_(fileSize), startPos_(startPos)
    {
        nameLower_ = toLower(name_);
    }

    void RpfFile::readHeader(IDataSource& src, const GTA5Keys& keys)
    {
        // Read fixed 16-byte header.
        uint8_t hdr[16];
        src.readAt(startPos_, hdr, 16);
        DataReader hr(std::vector<uint8_t>(hdr, hdr + 16));
        version_ = hr.ReadUInt32();
        entryCount_ = hr.ReadUInt32();
        namesLength_ = hr.ReadUInt32();
        encryption_ = static_cast<RpfEncryption>(hr.ReadUInt32());

        if (version_ != 0x52504637u)
            throw std::runtime_error("Invalid RPF - not GTAV (bad version)");

        std::vector<uint8_t> entriesData(static_cast<size_t>(entryCount_) * 16);
        std::vector<uint8_t> namesData(namesLength_);
        src.readAt(startPos_ + 16, entriesData.data(), entriesData.size());
        src.readAt(startPos_ + 16 + static_cast<int64_t>(entriesData.size()), namesData.data(), namesData.size());

        switch (encryption_)
        {
        case RpfEncryption::NONE:
        case RpfEncryption::OPEN:
            break;
        case RpfEncryption::AES:
            entriesData = GTACrypto::DecryptAESData(entriesData, keys.PC_AES_KEY);
            namesData = GTACrypto::DecryptAESData(namesData, keys.PC_AES_KEY);
            isAESEncrypted_ = true;
            break;
        case RpfEncryption::NG:
        default:
            entriesData = GTACrypto::DecryptNG(entriesData, name_, static_cast<uint32_t>(fileSize_), keys);
            namesData = GTACrypto::DecryptNG(namesData, name_, static_cast<uint32_t>(fileSize_), keys);
            isNGEncrypted_ = (encryption_ == RpfEncryption::NG);
            break;
        }

        DataReader er(entriesData);
        allEntries_.clear();
        allEntries_.reserve(entryCount_);

        for (uint32_t i = 0; i < entryCount_; ++i)
        {
            uint32_t y = er.ReadUInt32();
            uint32_t x = er.ReadUInt32();
            er.setPosition(er.position() - 8);

            RpfEntry e;
            if (x == 0x7fffff00u)
            {
                e.type = RpfEntryType::Directory;
                e.nameOffset = er.ReadUInt32();
                er.ReadUInt32(); // ident (already validated via x)
                e.entriesIndex = er.ReadUInt32();
                e.entriesCount = er.ReadUInt32();
            }
            else if ((x & 0x80000000u) == 0)
            {
                e.type = RpfEntryType::Binary;
                uint64_t buf = er.ReadUInt64();
                e.nameOffset = static_cast<uint32_t>(buf & 0xFFFF);
                e.fileSize = static_cast<uint32_t>((buf >> 16) & 0xFFFFFF);
                e.fileOffset = static_cast<uint32_t>((buf >> 40) & 0xFFFFFF);
                e.fileUncompressedSize = er.ReadUInt32();
                e.encryptionType = er.ReadUInt32();
                e.isEncrypted = (e.encryptionType != 0);
            }
            else
            {
                e.type = RpfEntryType::Resource;
                e.nameOffset = er.ReadUInt16();
                auto b1 = er.ReadBytes(3);
                e.fileSize = b1[0] | (b1[1] << 8) | (b1[2] << 16);
                auto b2 = er.ReadBytes(3);
                e.fileOffset = (static_cast<uint32_t>(b2[0]) | (static_cast<uint32_t>(b2[1]) << 8) |
                                (static_cast<uint32_t>(b2[2]) << 16)) & 0x7FFFFF;
                e.systemFlags = er.ReadUInt32();
                e.graphicsFlags = er.ReadUInt32();

                if (e.fileSize == 0xFFFFFF)
                {
                    uint8_t buf[16];
                    src.readAt(startPos_ + static_cast<int64_t>(e.fileOffset) * 512, buf, 16);
                    e.fileSize = (static_cast<uint32_t>(buf[7]) << 0) | (static_cast<uint32_t>(buf[14]) << 8) |
                                 (static_cast<uint32_t>(buf[5]) << 16) | (static_cast<uint32_t>(buf[2]) << 24);
                }
            }

            e.h1 = y;
            e.h2 = x;

            // Resolve name from names block.
            size_t no = e.nameOffset;
            std::string nm;
            while (no < namesData.size() && namesData[no] != 0)
                nm.push_back(static_cast<char>(namesData[no++]));
            if (nm.size() > 256) nm = nm.substr(0, 256);
            e.name = nm;
            e.nameLower = toLower(nm);

            if (e.type == RpfEntryType::Resource)
                e.isEncrypted = endsWith(e.nameLower, ".ysc");

            allEntries_.push_back(std::move(e));
        }

        if (allEntries_.empty()) return;

        // Build the tree (assign parents and paths) via the directory entry ranges.
        allEntries_[0].path = path_;
        std::vector<int> stack{0};
        while (!stack.empty())
        {
            int idx = stack.back();
            stack.pop_back();
            const RpfEntry parent = allEntries_[idx]; // copy of fields we need
            int starti = static_cast<int>(parent.entriesIndex);
            int endi = static_cast<int>(parent.entriesIndex + parent.entriesCount);
            for (int i = starti; i < endi && i < static_cast<int>(allEntries_.size()); ++i)
            {
                RpfEntry& e = allEntries_[i];
                e.parent = idx;
                e.path = allEntries_[idx].path + "\\" + e.nameLower;
                if (e.type == RpfEntryType::Directory)
                    stack.push_back(i);
            }
        }
    }

    std::vector<uint8_t> RpfFile::decompressBytes(const std::vector<uint8_t>& bytes)
    {
        try
        {
            return compression::inflateRaw(bytes);
        }
        catch (const std::exception&)
        {
            return {}; // matches C# returning null on failure
        }
    }

    std::vector<uint8_t> RpfFile::extractBinary(IDataSource& src, const GTA5Keys& keys, const RpfEntry& e)
    {
        long long l = e.getFileSize();
        if (l <= 0) return {};

        std::vector<uint8_t> tbytes(static_cast<size_t>(l));
        src.readAt(startPos_ + static_cast<int64_t>(e.fileOffset) * 512, tbytes.data(), tbytes.size());

        std::vector<uint8_t> decr = tbytes;
        if (e.isEncrypted)
        {
            if (isAESEncrypted_)
                decr = GTACrypto::DecryptAESData(tbytes, keys.PC_AES_KEY);
            else
                decr = GTACrypto::DecryptNG(tbytes, e.name, e.fileUncompressedSize, keys);
        }

        if (e.fileSize > 0) // compressed
            return decompressBytes(decr);
        return decr;
    }

    std::vector<uint8_t> RpfFile::extractResource(IDataSource& src, const GTA5Keys& keys, const RpfEntry& e)
    {
        if (e.fileSize == 0) return {};

        const uint32_t offset = 0x10;
        uint32_t totlen = e.fileSize - offset;
        std::vector<uint8_t> tbytes(totlen);
        src.readAt(startPos_ + static_cast<int64_t>(e.fileOffset) * 512 + offset, tbytes.data(), tbytes.size());

        std::vector<uint8_t> decr = tbytes;
        if (e.isEncrypted)
        {
            if (isAESEncrypted_)
                decr = GTACrypto::DecryptAESData(tbytes, keys.PC_AES_KEY);
            else
                decr = GTACrypto::DecryptNG(tbytes, e.name, e.fileSize, keys);
        }

        std::vector<uint8_t> deflated = decompressBytes(decr);
        if (!deflated.empty()) return deflated;
        return decr;
    }

    std::vector<uint8_t> RpfFile::extractFile(IDataSource& src, const GTA5Keys& keys, size_t entryIndex)
    {
        if (entryIndex >= allEntries_.size()) throw std::out_of_range("RPF entry index");
        const RpfEntry& e = allEntries_[entryIndex];
        if (e.type == RpfEntryType::Binary) return extractBinary(src, keys, e);
        if (e.type == RpfEntryType::Resource) return extractResource(src, keys, e);
        return {};
    }

    std::vector<uint8_t> RpfFile::extractFile(const GTA5Keys& keys, size_t entryIndex)
    {
        if (!src_) throw std::runtime_error("RPF has no bound data source (call scanStructure first)");
        return extractFile(*src_, keys, entryIndex);
    }

    void RpfFile::scanStructure(IDataSource& src, const GTA5Keys& keys)
    {
        src_ = &src;
        readHeader(src, keys);

        children_.clear();
        for (const RpfEntry& e : allEntries_)
        {
            if (e.type != RpfEntryType::Binary) continue;
            if (!endsWith(e.nameLower, ".rpf")) continue;

            long long childStart = startPos_ + static_cast<long long>(e.fileOffset) * 512;
            long long childSize = e.getFileSize();
            auto child = std::make_unique<RpfFile>(e.name, e.path, childSize, childStart);
            try
            {
                child->scanStructure(src, keys);
                children_.push_back(std::move(child));
            }
            catch (const std::exception&)
            {
                // Skip corrupted / unreadable nested archive, matching C# behavior.
            }
        }
    }
}
