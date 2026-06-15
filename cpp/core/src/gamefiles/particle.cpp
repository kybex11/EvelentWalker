#include "evw/gamefiles/particle.h"

namespace evw::gamefiles
{
    void ParticleEffectsList::read(ResourceDataReader& r)
    {
        ResourceFileBase::read(r);

        namePointer = r.ReadUInt64();
        unknown18 = r.ReadUInt64();
        textureDictionaryPointer = r.ReadUInt64();
        unknown28 = r.ReadUInt64();
        drawableDictionaryPointer = r.ReadUInt64();
        particleRuleDictionaryPointer = r.ReadUInt64();
        unknown40 = r.ReadUInt64();
        emitterRuleDictionaryPointer = r.ReadUInt64();
        effectRuleDictionaryPointer = r.ReadUInt64();
        unknown58 = r.ReadUInt64();

        if (namePointer != 0)
            name = r.ReadStringAt(namePointer);
    }
}
