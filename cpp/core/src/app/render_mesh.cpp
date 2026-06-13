#include "evw/app/render_mesh.h"

#include <unordered_map>

#include "evw/gamefiles/dxt_decode.h"
#include "evw/gamefiles/shader.h"
#include "evw/gamefiles/texture.h"

namespace evw::app
{
    using namespace evw::gamefiles;

    namespace
    {
        RenderMesh buildOne(const DrawableGeometry& geom)
        {
            RenderMesh m;
            m.shaderId = geom.shaderId;
            m.bbMin = geom.aabb.min.toVector3();
            m.bbMax = geom.aabb.max.toVector3();
            if (geom.vertexBuffer)
            {
                m.positions = geom.vertexBuffer->extractPositions();
                m.normals = geom.vertexBuffer->extractNormals();
                m.texCoords = geom.vertexBuffer->extractTexCoords0();
            }
            if (geom.indexBuffer)
                m.indices = geom.indexBuffer->indices;
            return m;
        }
    }

    std::vector<RenderMesh> buildMeshes(const DrawableModel& model)
    {
        std::vector<RenderMesh> meshes;
        for (const auto& geom : model.geometries)
            if (geom) meshes.push_back(buildOne(*geom));
        return meshes;
    }

    std::vector<RenderMesh> buildMeshes(const DrawableBase& drawable)
    {
        std::vector<RenderMesh> meshes;
        for (const auto& model : drawable.allModels())
        {
            if (!model) continue;
            auto sub = buildMeshes(*model);
            for (auto& m : sub) meshes.push_back(std::move(m));
        }
        return meshes;
    }

    RenderModel buildRenderModel(const DrawableBase& drawable)
    {
        RenderModel rm;
        rm.meshes = buildMeshes(drawable);

        auto sg = drawable.shaderGroup;
        if (!sg || !sg->textureDictionary) return rm;
        auto& texdict = *sg->textureDictionary;

        std::unordered_map<uint32_t, int> hashToIndex;

        for (auto& mesh : rm.meshes)
        {
            if (mesh.shaderId >= sg->shaders.items.size()) continue;
            auto shader = sg->shaders.items[mesh.shaderId];
            if (!shader || !shader->parametersList) continue;

            // Find the first texture parameter that resolves to a texture with data.
            for (const auto& param : shader->parametersList->parameters)
            {
                if (param.dataType != 0 || !param.texture) continue;
                uint32_t nameHash = param.texture->nameHash;
                auto tex = texdict.lookup(nameHash);
                if (!tex || !tex->data || tex->data->fullData.empty()) continue;

                auto it = hashToIndex.find(nameHash);
                if (it != hashToIndex.end()) { mesh.diffuseTexture = it->second; break; }

                auto rgba = evw::texconv::decodeToRGBA(*tex);
                if (rgba.empty()) continue;

                RenderTexture rt;
                rt.nameHash = nameHash;
                rt.width = tex->width;
                rt.height = tex->height;
                rt.rgba = std::move(rgba);
                int idx = static_cast<int>(rm.textures.size());
                rm.textures.push_back(std::move(rt));
                hashToIndex[nameHash] = idx;
                mesh.diffuseTexture = idx;
                break;
            }
        }
        return rm;
    }
}
