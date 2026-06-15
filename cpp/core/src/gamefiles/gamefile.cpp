#include "evw/gamefiles/gamefile.h"

#include <algorithm>

namespace evw::gamefiles
{
    namespace
    {
        std::string extOf(const std::string& filename)
        {
            auto dot = filename.find_last_of('.');
            if (dot == std::string::npos) return {};
            std::string e = filename.substr(dot + 1);
            std::transform(e.begin(), e.end(), e.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return e;
        }
    }

    GameFileType detectGameFileType(const std::string& filename)
    {
        std::string e = extOf(filename);
        if (e == "rpf") return GameFileType::Rpf;
        if (e == "ydr") return GameFileType::Ydr;
        if (e == "ydd") return GameFileType::Ydd;
        if (e == "yft") return GameFileType::Yft;
        if (e == "ytd") return GameFileType::Ytd;
        if (e == "ymap") return GameFileType::Ymap;
        if (e == "ytyp") return GameFileType::Ytyp;
        if (e == "ybn") return GameFileType::Ybn;
        if (e == "ybd") return GameFileType::Ybd;
        if (e == "ycd") return GameFileType::Ycd;
        if (e == "yed") return GameFileType::Yed;
        if (e == "yld") return GameFileType::Yld;
        if (e == "ypt") return GameFileType::Ypt;
        if (e == "ynv") return GameFileType::Ynv;
        if (e == "ynd") return GameFileType::Ynd;
        if (e == "ypdb") return GameFileType::Ypdb;
        if (e == "yvr") return GameFileType::Yvr;
        if (e == "ywr") return GameFileType::Ywr;
        if (e == "yfd") return GameFileType::Yfd;
        if (e == "ymf") return GameFileType::Ymf;
        if (e == "ymt") return GameFileType::Ymt;
        if (e == "meta") return GameFileType::Meta;
        if (e == "pso") return GameFileType::Pso;
        if (e == "rbf") return GameFileType::Rbf;
        if (e == "gxt2") return GameFileType::Gxt2;
        if (e == "rel") return GameFileType::Rel;
        if (e == "awc") return GameFileType::Awc;
        if (e == "fxc") return GameFileType::Fxc;
        if (e == "cut") return GameFileType::Cut;
        if (e == "mrf") return GameFileType::Mrf;
        if (e == "dat") return GameFileType::Heightmap;
        if (e == "dds") return GameFileType::Dds;
        if (e == "xml") return GameFileType::Xml;
        return GameFileType::Unknown;
    }

    const char* gameFileTypeName(GameFileType type)
    {
        switch (type)
        {
        case GameFileType::Rpf: return "RPF archive";
        case GameFileType::Ydr: return "Drawable (YDR)";
        case GameFileType::Ydd: return "Drawable Dictionary (YDD)";
        case GameFileType::Yft: return "Fragment (YFT)";
        case GameFileType::Ytd: return "Texture Dictionary (YTD)";
        case GameFileType::Ymap: return "Map Data (YMAP)";
        case GameFileType::Ytyp: return "Archetypes (YTYP)";
        case GameFileType::Ybn: return "Static Collisions (YBN)";
        case GameFileType::Ybd: return "Bounds Dictionary (YBD)";
        case GameFileType::Ycd: return "Clip Dictionary (YCD)";
        case GameFileType::Yed: return "Expression Dictionary (YED)";
        case GameFileType::Yld: return "Cloth Dictionary (YLD)";
        case GameFileType::Ypt: return "Particle Effects (YPT)";
        case GameFileType::Ynv: return "Nav Mesh (YNV)";
        case GameFileType::Ynd: return "Path Nodes (YND)";
        case GameFileType::Ypdb: return "Pose Matcher DB (YPDB)";
        case GameFileType::Yvr: return "Vehicle Record (YVR)";
        case GameFileType::Ywr: return "Waypoint Record (YWR)";
        case GameFileType::Yfd: return "Frame Filter Dictionary (YFD)";
        case GameFileType::Ymf: return "Manifest (YMF)";
        case GameFileType::Ymt: return "Meta (YMT)";
        case GameFileType::Meta: return "Meta";
        case GameFileType::Pso: return "PSO Metadata";
        case GameFileType::Rbf: return "RBF Metadata";
        case GameFileType::Gxt2: return "Global Text (GXT2)";
        case GameFileType::Rel: return "Audio Data (REL)";
        case GameFileType::Awc: return "Audio Wave Container (AWC)";
        case GameFileType::Fxc: return "Shaders (FXC)";
        case GameFileType::Cut: return "Cutscene (CUT)";
        case GameFileType::Mrf: return "Move Network (MRF)";
        case GameFileType::Gtxd: return "TXD Parent Mapping (GTXD)";
        case GameFileType::DistantLights: return "Distant Lights";
        case GameFileType::Watermap: return "Water Map (WMAP)";
        case GameFileType::Heightmap: return "Heightmap (DAT)";
        case GameFileType::Dds: return "DDS Texture";
        case GameFileType::Xml: return "XML";
        default: return "Unknown";
        }
    }
}
