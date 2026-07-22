#pragma once
#include <cstdint>

// 地圖格網中的一格。`terrain` 是 def id（由內容 / 前端對映到視覺呈現）；
// `flags` 快取與模擬相關的通行性，讓 FOV / 尋路不必每格去查 terrain def。
//
// Tile 不是 component——地圖是 Zone 的固有結構（見 zone.h 的 Zone::layers），
// 不掛在任何 entity 上。
inline constexpr uint32_t TILE_WALKABLE     = 1u << 0;
inline constexpr uint32_t TILE_BLOCKS_SIGHT = 1u << 1;

struct Tile {
    uint32_t terrain{0};
    uint32_t flags{0};

    template<class Archive>
    void serialize(Archive& ar) { ar(terrain, flags); }
};
