#pragma once
#include <cstdint>
#include "../util/tdarray.hpp"
#include "../zone/tile.h"

// World 的 worldgen 配方＋terrain 詞彙＋生成入口。演算法本體（libtcod FBM）在 .cpp，
// libtcod 依賴不外洩。本模組只認「配方＋一張 tile 格網」，不依賴 World 型別。

// biome 佔位 def id（寫進 Tile::terrain）。0 保留為「未生成」。
// 將來 Ruleset 的 terrain def 表落地時整組搬遷；屆時存檔照政策作廢重生。
inline constexpr uint32_t TERRAIN_OCEAN     = 1;
inline constexpr uint32_t TERRAIN_TUNDRA    = 2;
inline constexpr uint32_t TERRAIN_GRASSLAND = 3;
inline constexpr uint32_t TERRAIN_FOREST    = 4;

// World 層生成參數。全部有預設值；隨 save_extra 序列化——存檔即記住生成配方，
// 讀回後可用同一組參數重生成同一張圖。
struct WorldGenParams {
    int32_t  width = 200, height = 200;  // 願景尺度：World 200×200，一格≈數公里
    uint32_t seed = 0;
    float    sea_level   = 0.40f;  // 高度 0~1 的水陸閾值：以下是海
    float    noise_scale = 0.05f;  // 取樣頻率：越小地形塊越大
    int32_t  octaves     = 5;      // FBM 疊代層數

    template <class Archive>
    void serialize(Archive& ar) { ar(width, height, seed, sea_level, noise_scale, octaves); }
};

namespace world_gen {

// 依 gen 重建單一 layer 的 grid：FBM 高度場 → sea_level 切水陸 → 溫/濕度場分 biome，
// 寫 Tile::terrain＋陸地設 TILE_WALKABLE。整層清掉重寫（內部 alloc），不做增量；
// 同 seed 必產同圖（決定性）。尺寸非正 → throw（zone_id 僅供錯誤訊息定位）。
void generate(const WorldGenParams& gen, tdarray<Tile>& grid, uint64_t zone_id = 0);

}  // namespace world_gen
