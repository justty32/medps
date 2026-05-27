#pragma once
#include <cstdint>

// ---- map layers (ZoneType) -----------------------------------------------
// Strict hierarchy: World ⊃ Region ⊃ Area. The type doubles as tree depth, so
// a key's parent TYPE is implied. 0 is reserved for ZONE_ROOT (the global,
// non-map layer that holds factions / gods / named actors); real types from 1.
enum class ZoneType : uint16_t {
    Invalid = 0,   // == ZONE_ROOT
    World   = 1,   // 世界層  Civ-like, ~km/tile, 1 turn ≈ 1 day
    Region  = 2,   // 戰略層  grand-strategy / Fire-Emblem, tens of m/tile
    Area    = 3,   // 區域層  Rimworld / ToME4 / JRPG, ~m/tile
};

// ---- vertical layer (the z field) ----------------------------------------
// All three map types stack the same three vertical layers; z is centred on
// ground (down negative, up positive). A distinct z is a distinct zone.
namespace zlayer {
    inline constexpr int16_t Underground = -1;
    inline constexpr int16_t Ground      =  0;
    inline constexpr int16_t Sky         = +1;
}

// ---- scale constants (tunable, single source of truth) --------------------
// All estimates ("or larger" / "around"); coordinate math must reference these
// constants, never hard-code the numbers.
namespace zone_scale {
    inline constexpr int16_t WORLD_DIM  = 200;  // world map = WORLD_DIM² world-tiles (continent + ocean)
    inline constexpr int16_t REGION_DIM = 15;   // 1 world-tile  → REGION_DIM²  region-tiles
    inline constexpr int16_t AREA_DIM   = 250;  // 1 region-tile → AREA_DIM²    area-tiles

    // chunk = the storage/file unit: how many logical zones per chunk side.
    inline constexpr int16_t REGION_CHUNK = 5;  // 5×5 regions share one chunk file (Plan B)
    inline constexpr int16_t AREA_CHUNK   = 1;  // areas stay 1:1 (one area is already huge)

    // an Area's global region-tile coord = world*REGION_DIM + local; keep it in int16.
    static_assert(WORLD_DIM * REGION_DIM < 32767,
                  "world*region tile grid overflows the 16-bit ZoneKey x/y field");
}

// ---- ZoneKey packing ------------------------------------------------------
using ZoneKey = uint64_t;
constexpr ZoneKey ZONE_ROOT = 0;   // = make_zone_key(ZoneType::Invalid, 0, 0, 0)

// bits 63-48: ZoneType (16) | 47-32: x (16) | 31-16: y (16) | 15-0: z (16)
inline ZoneKey make_zone_key(ZoneType type, int16_t x, int16_t y, int16_t z) {
    return (static_cast<uint64_t>(type)                        << 48)
         | (static_cast<uint64_t>(static_cast<uint16_t>(x))   << 32)
         | (static_cast<uint64_t>(static_cast<uint16_t>(y))   << 16)
         |  static_cast<uint64_t>(static_cast<uint16_t>(z));
}

inline ZoneType zone_key_type(ZoneKey k) { return static_cast<ZoneType>(k >> 48); }
inline int16_t  zone_key_x   (ZoneKey k) { return static_cast<int16_t>(k >> 32); }
inline int16_t  zone_key_y   (ZoneKey k) { return static_cast<int16_t>(k >> 16); }
inline int16_t  zone_key_z   (ZoneKey k) { return static_cast<int16_t>(k);       }

// ---- coordinate conversions / hierarchy ----------------------------------
// A zone's (x,y) = its coordinate in the PARENT layer's GLOBAL tile grid:
//   World : single map per z, at (0,0).
//   Region: the world-tile it expands,        x,y ∈ [0, WORLD_DIM).
//   Area  : the GLOBAL region-tile it expands, x,y ∈ [0, WORLD_DIM*REGION_DIM).
// Parent is recovered by integer division; z is preserved up the chain.

inline ZoneKey world_key(int16_t z = zlayer::Ground) {
    return make_zone_key(ZoneType::World, 0, 0, z);
}

inline ZoneKey region_key(int16_t world_x, int16_t world_y, int16_t z = zlayer::Ground) {
    return make_zone_key(ZoneType::Region, world_x, world_y, z);
}

// build an Area key from its parent world-tile + region-local offset.
inline ZoneKey area_key(int16_t world_x, int16_t world_y,
                        int16_t region_local_x, int16_t region_local_y,
                        int16_t z = zlayer::Ground) {
    return make_zone_key(ZoneType::Area,
        static_cast<int16_t>(world_x * zone_scale::REGION_DIM + region_local_x),
        static_cast<int16_t>(world_y * zone_scale::REGION_DIM + region_local_y),
        z);
}

// the immediate parent zone of `k` (ZONE_ROOT for World and for root itself).
inline ZoneKey parent_of(ZoneKey k) {
    if (k == ZONE_ROOT) return ZONE_ROOT;
    const int16_t z = zone_key_z(k);
    switch (zone_key_type(k)) {
        case ZoneType::Area:
            return make_zone_key(ZoneType::Region,
                static_cast<int16_t>(zone_key_x(k) / zone_scale::REGION_DIM),
                static_cast<int16_t>(zone_key_y(k) / zone_scale::REGION_DIM), z);
        case ZoneType::Region:
            return make_zone_key(ZoneType::World, 0, 0, z);
        case ZoneType::World:
        default:
            return ZONE_ROOT;
    }
}
