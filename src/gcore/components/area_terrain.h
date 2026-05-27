#pragma once
#include <cstdint>
#include "../util/tdarray.hpp"

// One cell of an Area's terrain grid. `terrain` is a def id (content / frontend
// maps it to visuals); `flags` caches sim-relevant passability so FOV / pathing
// need not look up the terrain def per cell.
inline constexpr uint8_t TILE_WALKABLE     = 1 << 0;
inline constexpr uint8_t TILE_BLOCKS_SIGHT = 1 << 1;

struct Tile {
    uint16_t terrain{0};
    uint8_t  flags{0};

    template<class Archive>
    void serialize(Archive& ar) { ar(terrain, flags); }
};

// The dense terrain grid of one Area zone (Rimworld-style, ~250x250). Held as a
// component on a single "map" entity so it rides the normal snapshot/cereal save
// path -- registry ctx() is NOT serialized by zone_io. Mobile "things" (actors,
// items) are separate entities; tiles are NOT one-entity-per-cell.
struct AreaTerrain {
    tdarray<Tile> tiles;   // dimensions live in tiles.sx / tiles.sy

    template<class Archive>
    void serialize(Archive& ar) { ar(tiles); }
};
