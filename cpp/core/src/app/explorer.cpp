#include "evw/app/explorer.h"

#include <algorithm>
#include <cstdio>

#include "evw/app/render_mesh.h"
#include "evw/gamefiles/awc.h"
#include "evw/gamefiles/bounds.h"
#include "evw/gamefiles/drawable.h"
#include "evw/gamefiles/frag.h"
#include "evw/gamefiles/gxt2.h"
#include "evw/gamefiles/navmesh.h"
#include "evw/gamefiles/node.h"
#include "evw/gamefiles/pso.h"
#include "evw/gamefiles/rbf.h"
#include "evw/gamefiles/texture.h"

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
        mgr_ = std::make_unique<RpfManager>(keys_);
        mgr_->init(folder);

        paths_.clear();
        paths_.reserve(mgr_->entryDict().size());
        for (const auto& kv : mgr_->entryDict())
            paths_.push_back(kv.first);
        std::sort(paths_.begin(), paths_.end());
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
            }
            else if (RbfFile::isRBF(data))
            {
                RbfFile rbf;
                auto root = rbf.load(data);
                char buf[128];
                std::snprintf(buf, sizeof(buf), "RBF metadata: root '%s', %zu children",
                              root ? root->name.c_str() : "?", root ? root->children.size() : 0);
                pv.summary = buf;
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
}