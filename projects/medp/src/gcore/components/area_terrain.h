#pragma once
#include <cstdint>
#include "../util/tdarray.hpp"

// Area terrain grid 中的一格。`terrain` 是 def id（由內容 / 前端對映到視覺呈現）；
// `flags` 快取與模擬相關的通行性，讓 FOV / 尋路不必每格去查 terrain def。
inline constexpr uint8_t TILE_WALKABLE     = 1 << 0;
inline constexpr uint8_t TILE_BLOCKS_SIGHT = 1 << 1;

struct Tile {
    uint16_t terrain{0};
    uint8_t  flags{0};

    template<class Archive>
    void serialize(Archive& ar) { ar(terrain, flags); }
};

// 一個 Area zone 的密集 terrain grid（Rimworld 風格，約 250x250）。以 component 掛在
// 單一「map」entity 上，因此會跟著走正常的 snapshot/cereal 存檔路徑
// -- registry ctx() 不會被 zone_io 序列化。可移動的「things」（actor、
// item）是各自獨立的 entity；tile 並非一格一個 entity。
struct AreaTerrain {
    tdarray<Tile> tiles;   // 維度大小存在 tiles.sx / tiles.sy

    template<class Archive>
    void serialize(Archive& ar) { ar(tiles); }
};
