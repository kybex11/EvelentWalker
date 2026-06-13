// Port of EvelentWalker.Core/GameFiles/RpfFile.cs (read path).
// Parses RPF7 (GTA V) archive table-of-contents and extracts entries
// (decryption: NONE/OPEN/AES/NG, decompression: raw DEFLATE).
#pragma once

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "evw/gamefiles/gtacrypto.h"

namespace evw::gamefiles
{
    enum class RpfEncryption : uint32_t
    {
        NONE = 0,
        OPEN = 0x4E45504F,
        AES = 0x0FFFFFF9,
        NG = 0x0FEFFFFF,
    };

    // Random-access byte source for the underlying archive file.
    class IDataSource
    {
    public:
        virtual ~IDataSource() = default;
        virtual void readAt(int64_t pos, uint8_t* out, size_t len) = 0;
        virtual int64_t size() const = 0;
    };

    // In-memory data source (useful for tests and nested RPFs).
    class MemoryDataSource : public IDataSource
    {
    public:
        explicit MemoryDataSource(std::vector<uint8_t> data) : data_(std::move(data)) {}
        void readAt(int64_t pos, uint8_t* out, size_t len) override;
        int64_t size() const override { return static_cast<int64_t>(data_.size()); }

    private:
        std::vector<uint8_t> data_;
    };

    // File-backed data source.
    class FileDataSource : public IDataSource
    {
    public:
        explicit FileDataSource(const std::string& path);
        void readAt(int64_t pos, uint8_t* out, size_t len) override;
        int64_t size() const override { return size_; }

    private:
        std::shared_ptr<void> file_; // FILE* held via shared_ptr with custom deleter
        int64_t size_ = 0;
    };

    enum class RpfEntryType { Directory, Binary, Resource };

    struct RpfEntry
    {
        RpfEntryType type = RpfEntryType::Binary;
        std::string name;
        std::string nameLower;
        std::string path;
        uint32_t nameOffset = 0;
        uint32_t h1 = 0;
        uint32_t h2 = 0;
        int parent = -1;

        // Directory
        uint32_t entriesIndex = 0;
        uint32_t entriesCount = 0;

        // File (binary/resource)
        uint32_t fileOffset = 0;
        uint32_t fileSize = 0;
        bool isEncrypted = false;

        // Binary
        uint32_t fileUncompressedSize = 0;
        uint32_t encryptionType = 0;

        // Resource
        uint32_t systemFlags = 0;
        uint32_t graphicsFlags = 0;

        bool isFile() const { return type != RpfEntryType::Directory; }
        long long getFileSize() const;

        // Resource segment sizes / version (valid for Resource entries).
        uint32_t systemSize() const;
        uint32_t graphicsSize() const;
        int resourceVersion() const
        {
            return static_cast<int>((((systemFlags >> 28) & 0xF) << 4) + ((graphicsFlags >> 28) & 0xF));
        }
    };

    // Computes a resource page size from RAGE page flags (RpfResourceFileEntry.GetSizeFromFlags).
    uint32_t getSizeFromFlags(uint32_t flags);

    class RpfFile
    {
    public:
        RpfFile(std::string name, std::string path, long long fileSize, long long startPos = 0);

        // Reads and decrypts the table of contents, builds the entry tree.
        void readHeader(IDataSource& src, const GTA5Keys& keys);

        // Reads this archive's header, then recursively scans nested *.rpf
        // entries (which share the same underlying data source). Stores the
        // data source pointer for later extraction via extractFile(keys, idx).
        void scanStructure(IDataSource& src, const GTA5Keys& keys);

        // Extracts (decrypt + decompress) a file entry by index in allEntries.
        std::vector<uint8_t> extractFile(IDataSource& src, const GTA5Keys& keys, size_t entryIndex);
        std::vector<uint8_t> extractFile(const GTA5Keys& keys, size_t entryIndex);

        const std::vector<RpfEntry>& entries() const { return allEntries_; }
        const std::string& name() const { return name_; }
        const std::string& nameLower() const { return nameLower_; }
        const std::string& path() const { return path_; }
        long long startPos() const { return startPos_; }
        long long fileSize() const { return fileSize_; }
        uint32_t entryCount() const { return entryCount_; }
        RpfEncryption encryption() const { return encryption_; }
        IDataSource* dataSource() const { return src_; }
        const std::vector<std::unique_ptr<RpfFile>>& children() const { return children_; }

        static std::vector<uint8_t> decompressBytes(const std::vector<uint8_t>& bytes);

    private:
        std::vector<uint8_t> extractBinary(IDataSource& src, const GTA5Keys& keys, const RpfEntry& e);
        std::vector<uint8_t> extractResource(IDataSource& src, const GTA5Keys& keys, const RpfEntry& e);

        std::string name_;
        std::string nameLower_;
        std::string path_;
        long long fileSize_ = 0;
        long long startPos_ = 0;
        IDataSource* src_ = nullptr;

        uint32_t version_ = 0;
        uint32_t entryCount_ = 0;
        uint32_t namesLength_ = 0;
        RpfEncryption encryption_ = RpfEncryption::NONE;
        bool isAESEncrypted_ = false;
        bool isNGEncrypted_ = false;

        std::vector<RpfEntry> allEntries_;
        std::vector<std::unique_ptr<RpfFile>> children_;
    };
}
