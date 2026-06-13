#include "evw/gamefiles/drawable.h"

namespace evw::gamefiles
{
    std::vector<std::shared_ptr<DrawableModel>> DrawableBase::readModelList(ResourceDataReader& r, uint64_t lodPointer)
    {
        std::vector<std::shared_ptr<DrawableModel>> models;
        if (lodPointer == 0) return models;

        // Read ResourcePointerListHeader at lodPointer: {ptr u64, count u16, cap u16, pad u32}
        uint64_t backup = r.position();
        r.setPosition(lodPointer);
        uint64_t entriesPointer = r.ReadUInt64();
        uint16_t count = r.ReadUInt16();
        uint16_t capacity = r.ReadUInt16();
        r.ReadUInt32(); // pad
        r.setPosition(backup);

        auto pointers = r.ReadUlongsAt(entriesPointer, capacity);
        models.reserve(count);
        for (uint16_t i = 0; i < count && i < pointers.size(); ++i)
            models.push_back(r.ReadBlockAt<DrawableModel>(pointers[i]));
        return models;
    }

    void DrawableBase::read(ResourceDataReader& r)
    {
        ResourceFileBase::read(r);

        shaderGroupPointer = r.ReadUInt64();
        skeletonPointer = r.ReadUInt64();
        boundingCenter = r.ReadVector3();
        boundingSphereRadius = r.ReadSingle();
        boundingBoxMin = r.ReadVector3();
        r.ReadUInt32();                 // Unknown_3Ch
        boundingBoxMax = r.ReadVector3();
        r.ReadUInt32();                 // Unknown_4Ch
        drawableModelsHighPointer = r.ReadUInt64();
        drawableModelsMediumPointer = r.ReadUInt64();
        drawableModelsLowPointer = r.ReadUInt64();
        drawableModelsVeryLowPointer = r.ReadUInt64();
        lodDistHigh = r.ReadSingle();
        lodDistMed = r.ReadSingle();
        lodDistLow = r.ReadSingle();
        lodDistVlow = r.ReadSingle();
        r.ReadUInt32();                 // RenderMaskFlagsHigh
        r.ReadUInt32();                 // RenderMaskFlagsMed
        r.ReadUInt32();                 // RenderMaskFlagsLow
        r.ReadUInt32();                 // RenderMaskFlagsVlow
        jointsPointer = r.ReadUInt64();
        r.ReadUInt16();                 // Unknown_98h
        r.ReadUInt16();                 // DrawableModelsBlocksSize
        r.ReadUInt32();                 // Unknown_9Ch
        drawableModelsPointer = r.ReadUInt64();

        shaderGroup = r.ReadBlockAt<ShaderGroup>(shaderGroupPointer);

        // Skeleton parsed; Joints not parsed yet (pointer retained).
        skeleton = r.ReadBlockAt<Skeleton>(skeletonPointer);

        modelsHigh = readModelList(r, drawableModelsHighPointer);
        modelsMed = readModelList(r, drawableModelsMediumPointer);
        modelsLow = readModelList(r, drawableModelsLowPointer);
        modelsVLow = readModelList(r, drawableModelsVeryLowPointer);
    }

    std::vector<std::shared_ptr<DrawableModel>> DrawableBase::allModels() const
    {
        std::vector<std::shared_ptr<DrawableModel>> all;
        all.insert(all.end(), modelsHigh.begin(), modelsHigh.end());
        all.insert(all.end(), modelsMed.begin(), modelsMed.end());
        all.insert(all.end(), modelsLow.begin(), modelsLow.end());
        all.insert(all.end(), modelsVLow.begin(), modelsVLow.end());
        return all;
    }

    void Drawable::read(ResourceDataReader& r)
    {
        DrawableBase::read(r);

        namePointer = r.ReadUInt64();
        // LightAttributes (ResourceSimpleList64) — 16-byte header, skipped here.
        r.ReadUInt64(); r.ReadUInt16(); r.ReadUInt16(); r.ReadUInt32();
        r.ReadUInt64();                 // UnkPointer
        boundPointer = r.ReadUInt64();

        name = r.ReadStringAt(namePointer);
    }

    void DrawableDictionary::read(ResourceDataReader& r)
    {
        ResourceFileBase::read(r);

        r.ReadUInt64();                 // Unknown_10h
        r.ReadUInt64();                 // Unknown_18h
        hashesPointer = r.ReadUInt64();
        hashesCount = r.ReadUInt16();
        r.ReadUInt16();                 // HashesCount2
        r.ReadUInt32();                 // Unknown_2Ch
        drawablesPointer = r.ReadUInt64();
        drawablesCount = r.ReadUInt16();
        r.ReadUInt16();                 // DrawablesCount2
        r.ReadUInt32();                 // Unknown_3Ch

        hashes = r.ReadUintsAt(hashesPointer, hashesCount);

        if (drawablesPointer != 0 && drawablesCount != 0)
        {
            uint64_t backup = r.position();
            r.setPosition(drawablesPointer);
            drawables.read(r, drawablesCount);
            r.setPosition(backup);
        }
    }
}
