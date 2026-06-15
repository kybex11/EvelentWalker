// evw_cli — command-line front end for the EvelentWalker C++ core.
// Ties together the ported layers: RPF scanning, file extraction, YTD parsing.
//
// Commands:
//   evw_cli scan    <folder> [maxList]      Scan a game folder, list RPF/entry counts.
//   evw_cli extract <folder> <entryPath> <outFile>   Extract a file by path.
//   evw_cli ytd     <folder> <entryPath>    Parse a .ytd and list its textures.
//   evw_cli keys    <gta5.exe> <aesKeyB64>  (info) keys come from magic.dat (TODO loader)
//
// NOTE: AES/NG-encrypted archives require GTA5 keys (magic.dat). Loading the AES
// key from gta5.exe (SHA1 hash search) is not yet ported, so encrypted archives
// need the key supplied externally. Unencrypted / OpenIV (OPEN) archives work now.
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "evw/app/explorer.h"
#include "evw/gamefiles/ddsio.h"
#include "evw/gamefiles/gamefile.h"
#include "evw/gamefiles/heightmap.h"
#include "evw/gamefiles/jenk.h"
#include "evw/gamefiles/gtacrypto.h"
#include "evw/gamefiles/meta.h"
#include "evw/gamefiles/meta_xml.h"
#include "evw/gamefiles/pso.h"
#include "evw/gamefiles/pso_xml.h"
#include "evw/gamefiles/rbf.h"
#include "evw/gamefiles/rbf_xml.h"
#include "evw/gamefiles/resource_data.h"
#include "evw/gamefiles/rpf_manager.h"
#include "evw/gamefiles/texture.h"

using namespace evw::gamefiles;

namespace
{
    const char* formatName(TextureFormat f)
    {
        switch (f)
        {
        case TextureFormat::D3DFMT_A8R8G8B8: return "A8R8G8B8";
        case TextureFormat::D3DFMT_A8B8G8R8: return "A8B8G8R8";
        case TextureFormat::D3DFMT_A1R5G5B5: return "A1R5G5B5";
        case TextureFormat::D3DFMT_A8: return "A8";
        case TextureFormat::D3DFMT_L8: return "L8";
        case TextureFormat::D3DFMT_DXT1: return "DXT1";
        case TextureFormat::D3DFMT_DXT3: return "DXT3";
        case TextureFormat::D3DFMT_DXT5: return "DXT5";
        case TextureFormat::D3DFMT_ATI1: return "ATI1";
        case TextureFormat::D3DFMT_ATI2: return "ATI2";
        case TextureFormat::D3DFMT_BC7: return "BC7";
        default: return "UNKNOWN";
        }
    }

    int usage()
    {
        std::printf(
            "evw_cli - EvelentWalker C++ core CLI\n"
            "Usage:\n"
            "  evw_cli scan    <folder> [maxList]\n"
            "  evw_cli extract <folder> <entryPath> <outFile>\n"
            "  evw_cli ytd     <folder> <entryPath> [ddsOutFolder]\n"
            "  evw_cli xml     <folder> <entryPath> [outFile]\n"
            "  evw_cli meta    <folder> <entryPath>\n"
            "  evw_cli tree    <folder> [subPath]\n"
            "  evw_cli types   <folder>\n"
            "  evw_cli hash    <string>\n"
            "  evw_cli hmap    <folder> <entryPath> [out.pgm]\n");
        return 1;
    }

    // Detects type and converts an extracted file to XML; empty if unsupported.
    std::string toXml(const RpfEntry* e, const std::string& path, const std::vector<uint8_t>& data)
    {
        std::string lower = path;
        for (char& c : lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        auto ends = [&](const char* s) {
            std::string suf = s;
            return lower.size() >= suf.size() && lower.compare(lower.size() - suf.size(), suf.size(), suf) == 0;
        };
        if (ends(".ymap") || ends(".ytyp") || ends(".ymt") ||
            (ends(".meta") && e->type == RpfEntryType::Resource))
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
}

int main(int argc, char** argv)
{
    if (argc < 3) return usage();
    std::string cmd = argv[1];

    GTA5Keys keys; // empty: sufficient for unencrypted / OPEN archives

    // Auto-load CodeWalker-format keys if a "Keys" folder is present next to the
    // scanned folder (or in the working dir), enabling encrypted archive reads.
    if (argc >= 3)
    {
        std::string folder = argv[2];
        if (keys.loadFromKeysFolder(folder + "\\Keys") || keys.loadFromKeysFolder(folder) ||
            keys.loadFromKeysFolder("Keys"))
            std::fprintf(stderr, "[keys] GTA5 decryption keys loaded\n");
        else if (keys.loadAesKeyFromExe(folder + "\\gta5.exe"))
            std::fprintf(stderr, "[keys] AES key recovered from gta5.exe (NG keys still need a Keys folder)\n");
    }

    if (cmd == "scan")
    {
        std::string folder = argv[2];
        int maxList = (argc > 3) ? std::atoi(argv[3]) : 20;

        RpfManager mgr(keys);
        mgr.init(folder,
                 nullptr,
                 [](const std::string& e) { std::fprintf(stderr, "  [err] %s\n", e.c_str()); });

        std::printf("Scanned: %zu RPF archives, %zu entries\n", mgr.rpfCount(), mgr.entryCount());
        int n = 0;
        for (const auto& kv : mgr.entryDict())
        {
            if (n++ >= maxList) break;
            std::printf("  %s\n", kv.first.c_str());
        }
        return 0;
    }

    if (cmd == "extract")
    {
        if (argc < 5) return usage();
        std::string folder = argv[2], entryPath = argv[3], outFile = argv[4];
        RpfManager mgr(keys);
        mgr.init(folder);
        auto data = mgr.getFileData(entryPath);
        if (data.empty())
        {
            std::fprintf(stderr, "Entry not found or empty: %s\n", entryPath.c_str());
            return 2;
        }
        std::ofstream out(outFile, std::ios::binary);
        out.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
        std::printf("Wrote %zu bytes to %s\n", data.size(), outFile.c_str());
        return 0;
    }

    if (cmd == "ytd")
    {
        if (argc < 4) return usage();
        std::string folder = argv[2], entryPath = argv[3];
        std::string ddsFolder = (argc > 4) ? argv[4] : "";
        RpfManager mgr(keys);
        mgr.init(folder);
        const RpfEntry* e = mgr.getEntry(entryPath);
        if (!e || e->type != RpfEntryType::Resource)
        {
            std::fprintf(stderr, "Not a resource entry: %s\n", entryPath.c_str());
            return 2;
        }
        auto data = mgr.getFileData(entryPath);
        if (data.empty())
        {
            std::fprintf(stderr, "Failed to extract: %s\n", entryPath.c_str());
            return 2;
        }
        TextureDictionary td = loadTextureDictionary(e->systemSize(), e->graphicsSize(), data);
        std::printf("TextureDictionary: %zu textures\n", td.textures.size());
        for (const auto& tex : td.textures.items)
        {
            if (!tex) continue;
            std::printf("  %-32s %4ux%-4u %-6s mips=%u  data=%lld bytes\n",
                        tex->name.c_str(), tex->width, tex->height,
                        formatName(tex->format), tex->levels, tex->memoryUsage());
            if (!ddsFolder.empty())
            {
                std::string out = ddsFolder + "/" + (tex->name.empty() ? "tex" : tex->name) + ".dds";
                if (evw::texconv::writeDDSToFile(*tex, out))
                    std::printf("    -> %s\n", out.c_str());
                else
                    std::fprintf(stderr, "    failed to write %s\n", out.c_str());
            }
        }
        return 0;
    }

    if (cmd == "xml")
    {
        if (argc < 4) return usage();
        std::string folder = argv[2], entryPath = argv[3];
        std::string outFile = (argc > 4) ? argv[4] : "";
        RpfManager mgr(keys);
        mgr.init(folder);
        const RpfEntry* e = mgr.getEntry(entryPath);
        if (!e) { std::fprintf(stderr, "Entry not found: %s\n", entryPath.c_str()); return 2; }
        auto data = mgr.getFileData(entryPath);
        if (data.empty()) { std::fprintf(stderr, "Failed to extract: %s\n", entryPath.c_str()); return 2; }
        std::string xml = toXml(e, entryPath, data);
        if (xml.empty()) { std::fprintf(stderr, "Not XML-convertible: %s\n", entryPath.c_str()); return 3; }
        if (outFile.empty())
        {
            std::fwrite(xml.data(), 1, xml.size(), stdout);
        }
        else
        {
            std::ofstream out(outFile, std::ios::binary);
            out.write(xml.data(), static_cast<std::streamsize>(xml.size()));
            std::printf("Wrote %zu bytes of XML to %s\n", xml.size(), outFile.c_str());
        }
        return 0;
    }

    if (cmd == "meta")
    {
        if (argc < 4) return usage();
        std::string folder = argv[2], entryPath = argv[3];
        RpfManager mgr(keys);
        mgr.init(folder);
        const RpfEntry* e = mgr.getEntry(entryPath);
        if (!e || e->type != RpfEntryType::Resource)
        {
            std::fprintf(stderr, "Not a resource entry: %s\n", entryPath.c_str());
            return 2;
        }
        auto data = mgr.getFileData(entryPath);
        if (data.empty()) { std::fprintf(stderr, "Failed to extract: %s\n", entryPath.c_str()); return 2; }
        ResourceDataReader r(e->systemSize(), e->graphicsSize(), data);
        auto meta = r.ReadBlock<Meta>();
        std::printf("Meta '%s'\n  %zu structures, %zu enums, %zu data blocks (root block %d)\n",
                    meta->name.c_str(), meta->structureInfos.size(), meta->enumInfos.size(),
                    meta->dataBlocks.size(), meta->rootBlockIndex);
        for (const auto& s : meta->structureInfos.data)
            std::printf("  struct %s  size=%d  entries=%zu\n",
                        metaHashName(s.structureNameHash).c_str(), s.structureSize, s.entries.size());
        return 0;
    }

    if (cmd == "tree")
    {
        std::string folder = argv[2];
        std::string sub = (argc > 3) ? argv[3] : "";
        evw::app::ExplorerModel model;
        model.init(folder);
        std::printf("Keys: %s\n", model.keysLoaded() ? "loaded" : "not loaded");
        const auto& children = model.listChildren(sub);
        std::printf("%zu entries under '%s':\n", children.size(), sub.empty() ? "(root)" : sub.c_str());
        for (const auto& e : children)
            std::printf("  %s %-22s %s\n", e.isDirectory ? "[D]" : "   ",
                        e.typeName.c_str(), e.name.c_str());
        return 0;
    }

    if (cmd == "hash")
    {
        std::string s = argv[2];
        std::printf("Jenkins hash of \"%s\": %u (0x%08X)\n", s.c_str(),
                    JenkHash::GenHash(std::string_view(s)), JenkHash::GenHash(std::string_view(s)));
        return 0;
    }

    if (cmd == "types")
    {
        std::string folder = argv[2];
        RpfManager mgr(keys);
        mgr.init(folder);
        std::map<std::string, size_t> counts;
        for (const auto& kv : mgr.entryDict())
        {
            const std::string& path = kv.first;
            size_t slash = path.find_last_of('\\');
            std::string name = slash == std::string::npos ? path : path.substr(slash + 1);
            counts[gameFileTypeName(detectGameFileType(name))]++;
        }
        std::printf("Entry types in %zu entries:\n", mgr.entryCount());
        for (const auto& c : counts)
            std::printf("  %-28s %zu\n", c.first.c_str(), c.second);
        return 0;
    }

    if (cmd == "hmap")
    {
        if (argc < 4) return usage();
        std::string folder = argv[2], entryPath = argv[3];
        std::string outFile = (argc > 4) ? argv[4] : "";
        RpfManager mgr(keys);
        mgr.init(folder);
        auto data = mgr.getFileData(entryPath);
        if (data.empty()) { std::fprintf(stderr, "Failed to extract: %s\n", entryPath.c_str()); return 2; }
        HeightmapFile hm;
        if (!hm.load(data)) { std::fprintf(stderr, "Not a heightmap: %s\n", entryPath.c_str()); return 3; }
        std::string pgm = hm.toPGM();
        std::printf("Heightmap %ux%u\n", hm.width, hm.height);
        if (outFile.empty()) outFile = "heightmap.pgm";
        std::ofstream out(outFile, std::ios::binary);
        out.write(pgm.data(), static_cast<std::streamsize>(pgm.size()));
        std::printf("Wrote PGM (%zu bytes) to %s\n", pgm.size(), outFile.c_str());
        return 0;
    }

    return usage();
}