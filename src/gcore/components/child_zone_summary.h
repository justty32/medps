#pragma once
#include "../zone_key.h"

// Lightweight stub stored in a PARENT zone's registry, one per direct child zone.
// Lets a loaded parent enumerate / overview its children without loading them.
// Attach extra components (map position, icon, explored flag...) to the same
// stub entity as needed.
struct ChildZoneSummary {
    ZoneKey key{ZONE_ROOT};

    template<class Archive>
    void serialize(Archive& ar) { ar(key); }
};
