// Port of DrawableBase / Drawable from Drawable.cs (legacy read path) — the
// top-level Ydr structure. Ties together the ShaderGroup (materials + embedded
// texture dictionary) and LOD model lists (High/Med/Low/VeryLow). Skeleton,
// Joints and collision Bounds are not parsed yet (their pointers are retained).
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "evw/gamefiles/model.h"
#include "evw/gamefiles/resource_data.h"
#include "evw/gamefiles/shader.h"
#include "evw/math/math.h"

namespace evw::gamefiles
{
    struct DrawableBase : public ResourceFileBase
    {
        uint64_t shaderGroupPointer = 0;
        uint64_t skeletonPointer = 0;
        math::Vector3 boundingCenter;
        float boundingSphereRadius = 0;
        math::Vector3 boundingBoxMin;
        math::Vector3 boundingBoxMax;
        uint64_t drawableModelsHighPointer = 0;
        uint64_t drawableModelsMediumPointer = 0;
        uint64_t drawableModelsLowPointer = 0;
        uint64_t drawableModelsVeryLowPointer = 0;
        float lodDistHigh = 0, lodDistMed = 0, lodDistLow = 0, lodDistVlow = 0;
        uint64_t jointsPointer = 0;
        uint64_t drawableModelsPointer = 0;

        std::shared_ptr<ShaderGroup> shaderGroup;
        std::shared_ptr<Skeleton> skeleton;
        std::vector<std::shared_ptr<DrawableModel>> modelsHigh;
        std::vector<std::shared_ptr<DrawableModel>> modelsMed;
        std::vector<std::shared_ptr<DrawableModel>> modelsLow;
        std::vector<std::shared_ptr<DrawableModel>> modelsVLow;

        // High + Med + Low + VLow, in order.
        std::vector<std::shared_ptr<DrawableModel>> allModels() const;

        void read(ResourceDataReader& r);

    protected:
        // Reads a LOD model list given the pointer to its ResourcePointerListHeader.
        static std::vector<std::shared_ptr<DrawableModel>> readModelList(ResourceDataReader& r, uint64_t lodPointer);
    };

    // YDR top-level drawable: a DrawableBase plus a name and collision bound pointer.
    struct Drawable : public DrawableBase
    {
        uint64_t namePointer = 0;
        uint64_t boundPointer = 0;
        std::string name;

        void read(ResourceDataReader& r);
    };

    // YDD: dictionary of drawables keyed by name hash.
    struct DrawableDictionary : public ResourceFileBase
    {
        uint64_t hashesPointer = 0;
        uint16_t hashesCount = 0;
        uint64_t drawablesPointer = 0;
        uint16_t drawablesCount = 0;

        std::vector<uint32_t> hashes;
        ResourcePointerArray64<Drawable> drawables;

        void read(ResourceDataReader& r);
    };
}