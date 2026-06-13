// Port of EvelentWalker.Core/GameFiles/RpfManager.cs (core scan/index/lookup).
// Scans a game folder for *.rpf archives (recursively, including nested RPFs),
// indexes every entry by path, and provides file extraction by path.
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "evw/gamefiles/gtacrypto.h"
#include "evw/gamefiles/rpf.h"

namespace evw::gamefiles
{
    class RpfManager
    {
    public:
        struct EntryRef
        {
            RpfFile* rpf = nullptr;
            size_t index = 0;
        };

        // keys must remain valid for the lifetime of the manager (used on extract).
        explicit RpfManager(const GTA5Keys& keys) : keys_(keys) {}

        using StatusFn = std::function<void(const std::string&)>;
        using ErrorFn = std::function<void(const std::string&)>;

        // Scans `folder` recursively for *.rpf files and indexes their contents.
        void init(const std::string& folder, StatusFn updateStatus = nullptr, ErrorFn errorLog = nullptr);

        bool isInited() const { return isInited_; }

        // Total RPF archives indexed (including nested) and total entries.
        size_t rpfCount() const { return allRpfs_.size(); }
        size_t entryCount() const { return entryDict_.size(); }

        // Looks up an RPF archive by its (lowercased, backslash) path.
        RpfFile* findRpfFile(const std::string& path);

        // Looks up an entry by path. Returns nullptr if not found.
        const RpfEntry* getEntry(const std::string& path);

        // Extracts (decrypt + decompress) a file by path. Empty vector if missing.
        std::vector<uint8_t> getFileData(const std::string& path);

        const std::unordered_map<std::string, EntryRef>& entryDict() const { return entryDict_; }

    private:
        void addRpfFile(RpfFile* file);

        const GTA5Keys& keys_;
        bool isInited_ = false;

        std::vector<std::unique_ptr<IDataSource>> sources_;   // owns top-level file sources
        std::vector<std::unique_ptr<RpfFile>> topRpfs_;       // owns top-level archives
        std::vector<RpfFile*> allRpfs_;                       // flat list (incl. nested)
        std::unordered_map<std::string, RpfFile*> rpfDict_;
        std::unordered_map<std::string, EntryRef> entryDict_;
    };
}
