#include "evw/app/explorer.h"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <map>
#include <set>

#include "evw/app/render_mesh.h"
#include "evw/gamefiles/awc.h"
#include "evw/gamefiles/bounds.h"
#include "evw/gamefiles/dictionaries.h"
#include "evw/gamefiles/drawable.h"
#include "evw/gamefiles/dxt_decode.h"
#include "evw/gamefiles/frag.h"
#include "evw/gamefiles/fxc.h"
#include "evw/gamefiles/gamefile.h"
#include "evw/gamefiles/gxt2.h"
#include "evw/gamefiles/gxt2.h"
#include "evw/gamefiles/meta.h"
#include "evw/gamefiles/meta_xml.h"
#include "evw/gamefiles/navmesh.h"
#include "evw/gamefiles/node.h"
#include "evw/gamefiles/pso.h"
#include "evw/gamefiles/pso_xml.h"
#include "evw/gamefiles/rbf.h"
#include "evw/gamefiles/rbf_xml.h"
#include "evw/gamefiles/texture.h"
#include "evw/gamefiles/ypdb.h"

namespace evw::app
{
    using namespace evw::gamefiles;

    namespace
    {
        std::string toLower(std::string s)
        {
            for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            return s;
        }
        bool endsWith(const std::string& s, const std::string& suf)
        {
            return s.size() >= suf.size() && s.compare(s.size() - suf.size(), suf.size(), suf) == 0;
        }
    }

    ExplorerModel::ExplorerModel() = default;

    void ExplorerModel::init(const std::string& folder)
    {
        // Try to load GTA5 decryption keys so real (encrypted) archives can be
        // read. Looks for a CodeWalker-style "Keys" folder next to the game or
        // inside it; silently continues with empty keys if none are found
        // (unencrypted / OPEN archives still work).
        keysLoaded_ = keys_.loadFromKeysFolder(folder + "\\Keys") ||
                      keys_.loadFromKeysFolder(folder) ||
                      keys_.loadFromKeysFolder("Keys");

        // If full keys weren't found, still try to recover the AES key directly
        // from gta5.exe (enables AES-encrypted archives; NG still needs full keys).
        if (!keysLoaded_ && keys_.PC_AES_KEY.size() != 32)
            keys_.loadAesKeyFromExe(folder + "\\gta5.exe");

        mgr_ = std::make_unique<RpfManager>(keys_);
        mgr_->init(folder);

        paths_.clear();
        paths_.reserve(mgr_->entryDict().size());
        for (const auto& kv : mgr_->entryDict())
            paths_.push_back(kv.first);
        std::sort(paths_.begin(), paths_.end());
        buildTree();
    }

    void ExplorerModel::buildTree()
    {
        tree_.clear();
        std::map<std::string, std::set<std::string>> seen; // parent -> child paths

        for (const auto& full : paths_)
        {
            size_t start = 0;
            std::string prefix;
            while (true)
            {
                size_t pos = full.find('\\', start);
                std::string seg = (pos == std::string::npos) ? full.substr(start)
                                                             : full.substr(start, pos - start);
                std::string childPath = prefix.empty() ? seg : prefix + "\\" + seg;
                if (!seg.empty() && seen[prefix].insert(childPath).second)
                {
                    EntryInfo info;
                    info.name = seg;
                    info.path = childPath;
                    tree_[prefix].push_back(info);
                }
                if (pos == std::string::npos) break;
                prefix = childPath;
                start = pos + 1;
            }
        }

        // Fill metadata and sort each folder (directories first, then by name).
        for (auto& kv : tree_)
        {
            for (auto& info : kv.second)
            {
                bool hasChildren = tree_.find(info.path) != tree_.end();
                const RpfEntry* e = mgr_->getEntry(info.path);
                if (hasChildren || endsWith(info.path, ".rpf") ||
                    (e && e->type == RpfEntryType::Directory))
                {
                    info.isDirectory = true;
                    info.typeName = endsWith(info.path, ".rpf") ? "RPF archive" : "Folder";
                }
                else if (e)
                {
                    info.size = e->getFileSize();
                    info.typeName = gamefiles::gameFileTypeName(
                        gamefiles::detectGameFileType(info.name));
                }
            }
            std::sort(kv.second.begin(), kv.second.end(),
                      [](const EntryInfo& a, const EntryInfo& b) {
                          if (a.isDirectory != b.isDirectory) return a.isDirectory > b.isDirectory;
                          return a.name < b.name;
                      });
        }
    }

    const std::vector<EntryInfo>& ExplorerModel::listChildren(const std::string& dir) const
    {
        static const std::vector<EntryInfo> empty;
        auto it = tree_.find(toLower(dir));
        return it != tree_.end() ? it->second : empty;
    }

    bool ExplorerModel::isDirectory(const std::string& path) const
    {
        return tree_.find(toLower(path)) != tree_.end();
    }

    size_t ExplorerModel::extractToFile(const std::string& path, const std::string& outFile)
    {
        if (!mgr_) return 0;
        auto data = mgr_->getFileData(path);
        if (data.empty()) return 0;
        std::ofstream out(outFile, std::ios::binary);
        if (!out) return 0;
        out.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
        return out ? data.size() : 0;
    }

    bool ExplorerModel::isInited() const { return mgr_ && mgr_->isInited(); }
    size_t ExplorerModel::rpfCount() const { return mgr_ ? mgr_->rpfCount() : 0; }
    size_t ExplorerModel::entryCount() const { return mgr_ ? mgr_->entryCount() : 0; }

    std::vector<std::string> ExplorerModel::search(const std::string& query, size_t limit) const
    {
        std::vector<std::string> result;
        std::string q = toLower(query);
        for (const auto& p : paths_)
        {
            if (q.empty() || p.find(q) != std::string::npos)
            {
                result.push_back(p);
                if (result.size() >= limit) break;
            }
        }
        return result;
    }

    FilePreview ExplorerModel::openFile(const std::string& path)
    {
        FilePreview pv;
        pv.path = path;
        if (!mgr_) return pv;

        const RpfEntry* e = mgr_->getEntry(path);
        if (!e) return pv;

        auto data = mgr_->getFileData(path);
        pv.dataSize = data.size();
        if (data.empty()) { pv.summary = "(empty or extraction failed)"; return pv; }

        std::string lower = toLower(path);

        if (endsWith(lower, ".ytd"))
        {
            pv.kind = PreviewKind::TextureDictionary;
            TextureDictionary td = loadTextureDictionary(e->systemSize(), e->graphicsSize(), data);
            char buf[128];
            std::snprintf(buf, sizeof(buf), "TextureDictionary: %zu textures", td.textures.size());
            pv.summary = buf;
            for (const auto& tex : td.textures.items)
            {
                if (!tex) continue;
                char line[256];
                std::snprintf(line, sizeof(line), "%s  %ux%u  mips=%u",
                              tex->name.c_str(), tex->width, tex->height, tex->levels);
                pv.items.emplace_back(line);
            }
            pv.ok = true;
        }
        else if (endsWith(lower, ".ydr"))
        {
            pv.kind = PreviewKind::Drawable;
            ResourceDataReader r(e->systemSize(), e->graphicsSize(), data);
            auto drw = r.ReadBlock<Drawable>();
            char buf[256];
            std::snprintf(buf, sizeof(buf),
                          "Drawable '%s': %zu models, %zu shaders, radius %.2f",
                          drw->name.c_str(), drw->allModels().size(),
                          drw->shaderGroup ? drw->shaderGroup->shaders.size() : 0,
                          drw->boundingSphereRadius);
            pv.summary = buf;
            pv.ok = true;
        }
        else if (endsWith(lower, ".ydd"))
        {
            pv.kind = PreviewKind::DrawableDictionary;
            ResourceDataReader r(e->systemSize(), e->graphicsSize(), data);
            auto ydd = r.ReadBlock<DrawableDictionary>();
            char buf[128];
            std::snprintf(buf, sizeof(buf), "DrawableDictionary: %u drawables", ydd->drawablesCount);
            pv.summary = buf;
            pv.ok = true;
        }
        else if (endsWith(lower, ".ybn"))
        {
            pv.kind = PreviewKind::Unknown;
            ResourceDataReader r(e->systemSize(), e->graphicsSize(), data);
            auto b = r.ReadBlock<Bounds>();
            char buf[160];
            std::snprintf(buf, sizeof(buf),
                          "Bounds type=%d  box=[%.1f %.1f %.1f]..[%.1f %.1f %.1f]  vol=%.1f",
                          static_cast<int>(b->type), b->boxMin.X, b->boxMin.Y, b->boxMin.Z,
                          b->boxMax.X, b->boxMax.Y, b->boxMax.Z, b->volume);
            pv.summary = buf;
            pv.ok = true;
        }
        else if (endsWith(lower, ".gxt2"))
        {
            pv.kind = PreviewKind::Unknown;
            Gxt2File gxt;
            if (gxt.load(data))
            {
                char buf[64];
                std::snprintf(buf, sizeof(buf), "GXT2: %zu text entries", gxt.entries().size());
                pv.summary = buf;
                size_t n = 0;
                for (const auto& en : gxt.entries())
                {
                    if (n++ >= 100) break;
                    char line[256];
                    std::snprintf(line, sizeof(line), "0x%08X = %s", en.hash, en.text.c_str());
                    pv.items.emplace_back(line);
                }
                pv.ok = true;
            }
        }
        else if (endsWith(lower, ".yft"))
        {
            pv.kind = PreviewKind::Drawable;
            ResourceDataReader r(e->systemSize(), e->graphicsSize(), data);
            auto frag = r.ReadBlock<FragType>();
            char buf[160];
            std::snprintf(buf, sizeof(buf), "Fragment: radius %.2f, gravity %.2f, glass %u%s",
                          frag->boundingSphereRadius, frag->gravityFactor, frag->glassWindowsCount,
                          frag->drawable ? ", has drawable" : "");
            pv.summary = buf;
            pv.ok = true;
        }
        else if (endsWith(lower, ".ynd"))
        {
            pv.kind = PreviewKind::Unknown;
            ResourceDataReader r(e->systemSize(), e->graphicsSize(), data);
            auto nd = r.ReadBlock<NodeDictionary>();
            char buf[128];
            std::snprintf(buf, sizeof(buf), "PathNodes: %u nodes (%u vehicle, %u ped)",
                          nd->nodesCount, nd->nodesCountVehicle, nd->nodesCountPed);
            pv.summary = buf;
            pv.ok = true;
        }
        else if (endsWith(lower, ".ynv"))
        {
            pv.kind = PreviewKind::Unknown;
            ResourceDataReader r(e->systemSize(), e->graphicsSize(), data);
            auto nav = r.ReadBlock<NavMesh>();
            char buf[160];
            std::snprintf(buf, sizeof(buf), "NavMesh: %u vertices, %u polys, area %u",
                          nav->verticesCount, nav->polysCount, nav->areaID);
            pv.summary = buf;
            pv.ok = true;
        }
        else if (endsWith(lower, ".awc"))
        {
            pv.kind = PreviewKind::Unknown;
            AwcFile awc;
            if (awc.load(data))
            {
                char buf[128];
                std::snprintf(buf, sizeof(buf), "AWC audio: %d streams (v%u)", awc.streamCount, awc.version);
                pv.summary = buf;
                pv.ok = true;
            }
        }
        else if (endsWith(lower, ".fxc"))
        {
            pv.kind = PreviewKind::Unknown;
            FxcFile fxc;
            if (fxc.load(data))
            {
                char buf[96];
                std::snprintf(buf, sizeof(buf), "FXC shader: vertexType=%u, %zu preset params",
                              fxc.vertexType(), fxc.presetParams().size());
                pv.summary = buf;
                for (const auto& p : fxc.presetParams())
                    pv.items.emplace_back(p.name + " = " + std::to_string(p.value));
                pv.ok = true;
            }
        }
        else if (endsWith(lower, ".ypdb"))
        {
            pv.kind = PreviewKind::Unknown;
            YpdbFile y;
            if (y.load(data))
            {
                char buf[96];
                std::snprintf(buf, sizeof(buf), "YPDB pose matcher: v%d, %d samples",
                              y.serializerVersion(), y.samplesCount());
                pv.summary = buf;
                pv.ok = true;
            }
        }
        else if (endsWith(lower, ".yed") || endsWith(lower, ".yfd") || endsWith(lower, ".yld"))
        {
            pv.kind = PreviewKind::Unknown;
            ResourceDataReader r(e->systemSize(), e->graphicsSize(), data);
            char buf[96];
            if (endsWith(lower, ".yed"))
            {
                auto d = r.ReadBlock<ExpressionDictionary>();
                std::snprintf(buf, sizeof(buf), "ExpressionDictionary: %zu names, %u items",
                              d->nameHashes.size(), d->itemCount);
            }
            else if (endsWith(lower, ".yfd"))
            {
                auto d = r.ReadBlock<FrameFilterDictionary>();
                std::snprintf(buf, sizeof(buf), "FrameFilterDictionary: %zu names, %u items",
                              d->nameHashes.size(), d->itemCount);
            }
            else
            {
                auto d = r.ReadBlock<ClothDictionary>();
                std::snprintf(buf, sizeof(buf), "ClothDictionary: %zu names, %u items",
                              d->nameHashes.size(), d->itemCount);
            }
            pv.summary = buf;
            pv.ok = true;
        }
        else if (endsWith(lower, ".ymap") || endsWith(lower, ".ytyp") ||
                 endsWith(lower, ".ymt") || (endsWith(lower, ".meta") && e->type == RpfEntryType::Resource))
        {
            pv.kind = PreviewKind::Meta;
            ResourceDataReader r(e->systemSize(), e->graphicsSize(), data);
            auto meta = r.ReadBlock<Meta>();
            char buf[160];
            std::snprintf(buf, sizeof(buf),
                          "Meta '%s': %zu structures, %zu enums, %zu data blocks",
                          meta->name.c_str(), meta->structureInfos.size(),
                          meta->enumInfos.size(), meta->dataBlocks.size());
            pv.summary = buf;
            pv.text = metaToXml(*meta);
            pv.ok = true;
        }
        else
        {
            pv.kind = (e->type == RpfEntryType::Resource) ? PreviewKind::Unknown : PreviewKind::Binary;
            if (PsoFile::isPSO(data))
            {
                PsoFile pso;
                pso.load(data);
                char buf[128];
                std::snprintf(buf, sizeof(buf), "PSO metadata: %zu blocks, %zu schema structs",
                              pso.entries().size(), pso.schema().size());
                pv.summary = buf;
                pv.kind = PreviewKind::Pso;
                pv.text = psoToXml(pso);
            }
            else if (RbfFile::isRBF(data))
            {
                RbfFile rbf;
                auto root = rbf.load(data);
                char buf[128];
                std::snprintf(buf, sizeof(buf), "RBF metadata: root '%s', %zu children",
                              root ? root->name.c_str() : "?", root ? root->children.size() : 0);
                pv.summary = buf;
                pv.kind = PreviewKind::Rbf;
                pv.text = rbfToXml(root);
            }
            else
            {
                char buf[64];
                std::snprintf(buf, sizeof(buf), "%zu bytes", data.size());
                pv.summary = buf;
            }
            pv.ok = true;
        }
        return pv;
    }

    std::string ExplorerModel::getXml(const std::string& path)
    {
        if (!mgr_) return {};
        const RpfEntry* e = mgr_->getEntry(path);
        if (!e) return {};
        auto data = mgr_->getFileData(path);
        if (data.empty()) return {};

        std::string lower = toLower(path);
        if (endsWith(lower, ".ymap") || endsWith(lower, ".ytyp") ||
            endsWith(lower, ".ymt") || (endsWith(lower, ".meta") && e->type == RpfEntryType::Resource))
        {
            ResourceDataReader r(e->systemSize(), e->graphicsSize(), data);
            auto meta = r.ReadBlock<Meta>();
            return metaToXml(*meta);
        }
        if (PsoFile::isPSO(data))
        {
            PsoFile pso;
            if (pso.load(data)) return psoToXml(pso);
        }
        if (RbfFile::isRBF(data))
        {
            RbfFile rbf;
            return rbfToXml(rbf.load(data));
        }
        return {};
    }

    std::vector<RenderMesh> ExplorerModel::buildDrawableMeshes(const std::string& path)
    {
        if (!mgr_) return {};
        std::string lower = toLower(path);
        if (!endsWith(lower, ".ydr")) return {};
        const RpfEntry* e = mgr_->getEntry(path);
        if (!e || e->type != RpfEntryType::Resource) return {};
        auto data = mgr_->getFileData(path);
        if (data.empty()) return {};
        ResourceDataReader r(e->systemSize(), e->graphicsSize(), data);
        auto drw = r.ReadBlock<Drawable>();
        return buildMeshes(*drw);
    }

    RenderModel ExplorerModel::buildDrawableRenderModel(const std::string& path)
    {
        if (!mgr_) return {};
        std::string lower = toLower(path);
        if (!endsWith(lower, ".ydr")) return {};
        const RpfEntry* e = mgr_->getEntry(path);
        if (!e || e->type != RpfEntryType::Resource) return {};
        auto data = mgr_->getFileData(path);
        if (data.empty()) return {};
        ResourceDataReader r(e->systemSize(), e->graphicsSize(), data);
        auto drw = r.ReadBlock<Drawable>();
        return buildRenderModel(*drw);
    }

    RenderModel ExplorerModel::buildFragmentRenderModel(const std::string& path)
    {
        if (!mgr_) return {};
        if (!endsWith(toLower(path), ".yft")) return {};
        const RpfEntry* e = mgr_->getEntry(path);
        if (!e || e->type != RpfEntryType::Resource) return {};
        auto data = mgr_->getFileData(path);
        if (data.empty()) return {};
        ResourceDataReader r(e->systemSize(), e->graphicsSize(), data);
        auto frag = r.ReadBlock<FragType>();
        if (!frag->drawable) return {};
        return buildRenderModel(*frag->drawable);
    }

    RenderModel ExplorerModel::buildDictionaryRenderModel(const std::string& path, size_t index)
    {
        if (!mgr_) return {};
        if (!endsWith(toLower(path), ".ydd")) return {};
        const RpfEntry* e = mgr_->getEntry(path);
        if (!e || e->type != RpfEntryType::Resource) return {};
        auto data = mgr_->getFileData(path);
        if (data.empty()) return {};
        ResourceDataReader r(e->systemSize(), e->graphicsSize(), data);
        auto ydd = r.ReadBlock<DrawableDictionary>();
        if (index >= ydd->drawables.items.size() || !ydd->drawables.items[index]) return {};
        return buildRenderModel(*ydd->drawables.items[index]);
    }

    RenderModel ExplorerModel::buildRenderModelForPath(const std::string& path)
    {
        std::string lower = toLower(path);
        if (endsWith(lower, ".ydr")) return buildDrawableRenderModel(path);
        if (endsWith(lower, ".yft")) return buildFragmentRenderModel(path);
        if (endsWith(lower, ".ydd")) return buildDictionaryRenderModel(path, 0);
        return {};
    }

    size_t ExplorerModel::dictionaryDrawableCount(const std::string& path)
    {
        if (!mgr_ || !endsWith(toLower(path), ".ydd")) return 0;
        const RpfEntry* e = mgr_->getEntry(path);
        if (!e || e->type != RpfEntryType::Resource) return 0;
        auto data = mgr_->getFileData(path);
        if (data.empty()) return 0;
        ResourceDataReader r(e->systemSize(), e->graphicsSize(), data);
        auto ydd = r.ReadBlock<DrawableDictionary>();
        return ydd->drawables.items.size();
    }

    std::vector<RenderTexture> ExplorerModel::buildTextureList(const std::string& path)
    {
        std::vector<RenderTexture> out;
        if (!mgr_ || !endsWith(toLower(path), ".ytd")) return out;
        const RpfEntry* e = mgr_->getEntry(path);
        if (!e || e->type != RpfEntryType::Resource) return out;
        auto data = mgr_->getFileData(path);
        if (data.empty()) return out;
        TextureDictionary td = loadTextureDictionary(e->systemSize(), e->graphicsSize(), data);
        for (const auto& tex : td.textures.items)
        {
            if (!tex) continue;
            auto rgba = evw::texconv::decodeToRGBA(*tex);
            if (rgba.empty()) continue;
            RenderTexture rt;
            rt.nameHash = tex->nameHash;
            rt.width = tex->width;
            rt.height = tex->height;
            rt.rgba = std::move(rgba);
            out.push_back(std::move(rt));
        }
        return out;
    }
}