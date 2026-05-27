#pragma once
#include "zone_key.h"

// A chunk is the storage/file unit (Plan B). Several logical zones of the same
// ZoneType and z share one chunk, addressed by integer-dividing the zone's
// (x,y) by a per-type chunk side. Logical addressing (ZoneKey) is unchanged --
// this only groups blobs on disk to cut file count and smooth streaming.
// See ChunkedFolderZoneStore and work/design/registry_chunking_investigation.md.

inline int16_t chunk_side(ZoneType type) {
    switch (type) {
        case ZoneType::Region: return zone_scale::REGION_CHUNK;  // 5×5
        case ZoneType::Area:   return zone_scale::AREA_CHUNK;     // 1:1
        default:               return 1;                         // World / root
    }
}

// the chunk that owns `key`. ZONE_ROOT and any 1:1 type are their own chunk.
inline ZoneKey chunk_key_of(ZoneKey key) {
    if (key == ZONE_ROOT) return ZONE_ROOT;
    const ZoneType t = zone_key_type(key);
    const int16_t  n = chunk_side(t);
    if (n <= 1) return key;
    auto fdiv = [](int16_t a, int16_t b) -> int16_t {           // floor division
        int q = a / b;
        if ((a % b != 0) && ((a < 0) != (b < 0))) --q;
        return static_cast<int16_t>(q);
    };
    return make_zone_key(t, fdiv(zone_key_x(key), n), fdiv(zone_key_y(key), n), zone_key_z(key));
}
