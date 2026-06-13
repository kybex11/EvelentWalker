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
#include <string>

#include "evw/gamefiles/ddsio.h"
#include "evw/gamefiles/gtacrypto.h"
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
            "  evw_cli ytd     <folder> <entryPath> [ddsOutFolder]\n");
        return 1;
    }
}

int main(int argc, char** argv)
{
    if (argc < 3) return usage();
    std::string cmd = argv[1];

    GTA5Keys keys; // empty: sufficient for unencrypted / OPEN archives

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

    return usage();
}
