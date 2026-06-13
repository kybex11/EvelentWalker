#include "evw/gamefiles/rpf_manager.h"

#include <algorithm>
#include <filesystem>

#include "evw/gamefiles/jenk.h"

namespace fs = std::filesystem;

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

    void RpfManager::addRpfFile(RpfFile* file)
    {
        allRpfs_.push_back(file);
        rpfDict_[file->path()] = file;

        const auto& entries = file->entries();
        for (size_t i = 0; i < entries.size(); ++i)
        {
            const RpfEntry& e = entries[i];
            if (e.name.empty()) continue;
            entryDict_[e.path] = EntryRef{file, i};

            // Compute name hashes for file entries (parity with C# AddRpfFile).
            if (e.isFile())
            {
                // NameHash / ShortNameHash are informational here; the JenkIndex
                // is populated for reverse lookups.
                JenkIndex::Ensure(e.nameLower);
            }
        }

        for (const auto& child : file->children())
            addRpfFile(child.get());
    }

    void RpfManager::init(const std::string& folder, StatusFn updateStatus, ErrorFn errorLog)
    {
        std::string base = folder;
        if (!base.empty() && base.back() != '\\' && base.back() != '/')
            base += '\\';

        std::vector<std::string> rpfPaths;
        std::error_code ec;
        for (auto it = fs::recursive_directory_iterator(folder, ec);
             it != fs::recursive_directory_iterator(); it.increment(ec))
        {
            if (ec) break;
            if (!it->is_regular_file(ec)) continue;
            std::string p = it->path().string();
            if (endsWith(toLower(p), ".rpf"))
                rpfPaths.push_back(p);
        }
        std::sort(rpfPaths.begin(), rpfPaths.end());

        for (const std::string& rpfpath : rpfPaths)
        {
            try
            {
                // Relative path (lowercased, backslash separators) like the C# version.
                std::string rel = rpfpath;
                if (rel.rfind(base, 0) == 0) rel = rel.substr(base.size());
                std::replace(rel.begin(), rel.end(), '/', '\\');
                rel = toLower(rel);

                auto src = std::make_unique<FileDataSource>(rpfpath);
                long long fsize = src->size();
                auto rpf = std::make_unique<RpfFile>(
                    fs::path(rpfpath).filename().string(), rel, fsize);

                if (updateStatus) updateStatus("Scanning " + rel + "...");
                rpf->scanStructure(*src, keys_);

                addRpfFile(rpf.get());
                topRpfs_.push_back(std::move(rpf));
                sources_.push_back(std::move(src));
            }
            catch (const std::exception& ex)
            {
                if (errorLog) errorLog(rpfpath + ": " + ex.what());
            }
        }

        if (updateStatus) updateStatus("Scan complete");
        isInited_ = true;
    }

    RpfFile* RpfManager::findRpfFile(const std::string& path)
    {
        auto it = rpfDict_.find(toLower(path));
        return it != rpfDict_.end() ? it->second : nullptr;
    }

    const RpfEntry* RpfManager::getEntry(const std::string& path)
    {
        auto it = entryDict_.find(toLower(path));
        if (it == entryDict_.end()) return nullptr;
        return &it->second.rpf->entries()[it->second.index];
    }

    std::vector<uint8_t> RpfManager::getFileData(const std::string& path)
    {
        auto it = entryDict_.find(toLower(path));
        if (it == entryDict_.end()) return {};
        return it->second.rpf->extractFile(keys_, it->second.index);
    }
}
