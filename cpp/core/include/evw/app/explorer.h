// UI-agnostic explorer model: wraps RpfManager and provides a searchable entry
// list plus typed previews of selected files. The ImGui front-end binds to this.
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "evw/gamefiles/gtacrypto.h"
#include "evw/gamefiles/rpf_manager.h"
#include "evw/app/render_mesh.h"

namespace evw::app
{
    enum class PreviewKind
    {
        None,
        Binary,
        TextureDictionary,
        Drawable,
        DrawableDictionary,
        Unknown,
    };

    struct FilePreview
    {
        PreviewKind kind = PreviewKind::None;
        std::string path;
        std::string summary;
        std::vector<std::string> items;   // e.g. texture names, model info
        size_t dataSize = 0;
        bool ok = false;
    };

    class ExplorerModel
    {
    public:
        ExplorerModel();

        // Scans a game folder for RPF archives and indexes entries.
        void init(const std::string& folder);
        bool isInited() const;

        size_t rpfCount() const;
        size_t entryCount() const;

        // All entry paths (sorted). Built once after init.
        const std::vector<std::string>& allPaths() const { return paths_; }

        // Returns up to `limit` entry paths containing `query` (case-insensitive).
        std::vector<std::string> search(const std::string& query, size_t limit = 200) const;

        // Opens a file by path and produces a typed preview.
        FilePreview openFile(const std::string& path);

        // Builds render meshes for a .ydr drawable at `path` (empty if not a drawable).
        std::vector<RenderMesh> buildDrawableMeshes(const std::string& path);

        // Builds a full render model (meshes + decoded textures) for a .ydr drawable.
        RenderModel buildDrawableRenderModel(const std::string& path);

        gamefiles::GTA5Keys& keys() { return keys_; }

    private:
        gamefiles::GTA5Keys keys_;
        std::unique_ptr<gamefiles::RpfManager> mgr_;
        std::vector<std::string> paths_;
    };
}
