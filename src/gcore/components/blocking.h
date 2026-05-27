#pragma once

// Per-entity passability for a discrete thing standing on a tile: a closed door,
// a boulder, a large creature. Terrain-level passability lives in AreaTerrain's
// tile flags; this blocks on TOP of the terrain.
struct Blocking {
    bool blocks_move{true};
    bool blocks_sight{false};

    template<class Archive>
    void serialize(Archive& ar) { ar(blocks_move, blocks_sight); }
};
