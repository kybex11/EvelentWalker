// File-type identification by extension, mirroring the set of GTA V file kinds
// handled by CodeWalker. Used by the explorer/UI to choose a loader.
#pragma once

#include <string>

namespace evw::gamefiles
{
    enum class GameFileType
    {
        Unknown,
        Rpf,        // .rpf archive
        Ydr,        // drawable
        Ydd,        // drawable dictionary
        Yft,        // fragment
        Ytd,        // texture dictionary
        Ymap,       // map data
        Ytyp,       // archetype types
        Ybn,        // static collisions (bounds)
        Ybd,        // bounds dictionary
        Ycd,        // clip dictionary
        Yed,        // expression dictionary
        Yld,        // cloth dictionary
        Ypt,        // particle effects
        Ynv,        // nav mesh
        Ynd,        // path nodes
        Yvr,        // vehicle record
        Ywr,        // waypoint record
        Yfd,        // frame filter dictionary
        Ymf,        // manifest
        Ymt,        // meta (per-type)
        Meta,       // .meta / .ymt structured metadata
        Pso,        // pso metadata
        Rbf,        // rbf metadata
        Gxt2,       // global text
        Rel,        // audio dat
        Awc,        // audio wave container
        Fxc,        // shaders
        Cut,        // cutscene
        Heightmap,  // .dat heightmap
        Dds,        // texture
        Xml,        // xml
    };

    // Determines the file type from a filename/path extension (case-insensitive).
    GameFileType detectGameFileType(const std::string& filename);

    // Human-readable name for a file type.
    const char* gameFileTypeName(GameFileType type);
}
