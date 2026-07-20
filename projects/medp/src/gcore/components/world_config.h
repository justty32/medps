#pragma once
#include <cstdint>
#include "../zone_key.h"

// 每份存檔的全域世界設定。以 SINGLETON component 掛在 ROOT 上，因此會跟著
// root 走正常的 snapshot/cereal 存檔路徑（由 save_all 寫出、load_root 還原）
// -- 不需要另外的 meta 檔。在世界生成時決定，且在該存檔的整個生命週期內 IMMUTABLE：
// world_dim 已內嵌進 ZoneKey 的座標語意，遊戲中途更動會讓所有既有 key 失效。
// 需要它的 system 以 `const WorldConfig&`（自由函式風格）取用。
struct WorldConfig {
    // 世界地圖 = world_dim_x × world_dim_y 個 world-tile（大陸 + 海洋）。新遊戲時可由
    // 玩家選擇（小 / 中 / 大）；x、y 必須滿足 zone_scale::valid_world_dim。
    int16_t world_dim_x{zone_scale::WORLD_DIM_DEFAULT};
    int16_t world_dim_y{zone_scale::WORLD_DIM_DEFAULT};
    // 垂直層數（z 軸）。預設 3 = Underground/Ground/Sky（−1/0/1）；保留此欄位以便
    // 日後擴充更多層（冥界、天界…）。
    int16_t world_dim_z{zone_scale::WORLD_LAYERS_DEFAULT};

    template<class Archive>
    void serialize(Archive& ar) { ar(world_dim_x, world_dim_y, world_dim_z); }
};
