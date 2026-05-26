#pragma once
#include "cross_zone_ref.h"

// Which faction owns this actor. Factions are global entities living in root,
// so the reference is cross-zone (resolve via GlobalManager::resolve).
struct Owner {
    CrossZoneRef faction;   // typically { ZONE_ROOT, <faction entity> }

    template<class Archive>
    void serialize(Archive& ar) { ar(faction); }
};
