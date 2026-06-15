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
        Meta,
        Pso,
        Rbf,
        Unknown,
    };

    struct FilePreview
    {
        PreviewKind kind = PreviewKind::None;
        std::string path;
        std::string summary;
        std::vector<std::string> items;   // e.g. texture names, model info
        std::string text;                 // full XML / text dump when available
        size_t dataSize = 0;
        bool ok = false;
    };

    // One row in the folder/list view (a folder, an RPF archive, or a file).
    struct EntryInfo
    {
        std::string name;       // leaf name
        std::string path;       // full entry path (lowercase, '\\'-separated)
        bool isDirectory = false;
        std::string typeName;   // human-readable type
        long long size = 0;     // file size in bytes (0 for folders)
    };

    class ExplorerModel
    {
    public:
        ExplorerModel();

        // Scans a game folder for RPF archives and indexes entries.
        void init(const std::string& folder);
        bool isInited() const;

        // True if GTA5 decryption keys were loaded (real archives readable).
        bool keysLoaded() const { return keysLoaded_; }

        size_t rpfCount() const;
        size_t entryCount() const;

        // All entry paths (sorted). Built once after init.
        const std::vector<std::string>& allPaths() const { return paths_; }

        // Returns up to `limit` entry paths containing `query` (case-insensitive).
        std::vector<std::string> search(const std::string& query, size_t limit = 200) const;

        // ---- Folder navigation (virtual filesystem built from entry paths) ----
        // Immediate children of a folder path (empty path = top-level archives).
        // Folders are listed before files; both sorted by name.
        const std::vector<EntryInfo>& listChildren(const std::string& dir) const;
        // True if the path is a navigable folder/archive.
        bool isDirectory(const std::string& path) const;

        // Opens a file by path and produces a typed preview.
        FilePreview openFile(const std::string& path);

        // Extracts (decrypt + decompress) a file by path and writes it to disk.
        // Returns the number of bytes written, or 0 on failure.
        size_t extractToFile(const std::string& path, const std::string& outFile);

        // Produces an XML dump of a meta-like file (Meta/Ymap/Ytyp/Ymt, PSO, RBF)
        // by path, or an empty string if the file is not XML-convertible.
        std::string getXml(const std::string& path);

        // Builds render meshes for a .ydr drawable at `path` (empty if not a drawable).
        std::vector<RenderMesh> buildDrawableMeshes(const std::string& path);

        // Builds a full render model (meshes + decoded textures) for a .ydr drawable.
        RenderModel buildDrawableRenderModel(const std::string& path);

        // Builds a render model for a .yft fragment's main visual drawable.
        RenderModel buildFragmentRenderModel(const std::string& path);

        // Builds a render model for one drawable (by index) inside a .ydd dictionary.
        RenderModel buildDictionaryRenderModel(const std::string& path, size_t index = 0);

        // Number of drawables in a .ydd dictionary (0 if not a dictionary).
        size_t dictionaryDrawableCount(const std::string& path);

        // Decodes all textures of a .ytd into RGBA RenderTextures (empty otherwise).
        std::vector<RenderTexture> buildTextureList(const std::string& path);

        // Convenience: picks the right builder by file extension/type.
        RenderModel buildRenderModelForPath(const std::string& path);

        gamefiles::GTA5Keys& keys() { return keys_; }

    private:
        void buildTree();

        gamefiles::GTA5Keys keys_;
        bool keysLoaded_ = false;
        std::unique_ptr<gamefiles::RpfManager> mgr_;
        std::vector<std::string> paths_;
        std::map<std::string, std::vector<EntryInfo>> tree_;
    };
}
