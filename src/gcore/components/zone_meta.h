#pragma once
#include "../zone_key.h"

// Identity component held by every zone's placeholder entity.
// Guarantees a zone always has at least one non-orphan entity (see zone_io.h),
// and lets a loaded registry know which ZoneKey it represents and its parent.
struct ZoneMeta {
    ZoneKey self{ZONE_ROOT};
    ZoneKey parent{ZONE_ROOT};   // direct parent zone; ZONE_ROOT = hangs off root

    template<class Archive>
    void serialize(Archive& ar) { ar(self, parent); }
};
