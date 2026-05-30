#pragma once
#include <cstdint>

// ---- 地圖分層 (ZoneType) -------------------------------------------------
// 嚴格階層：World ⊃ Region ⊃ Area。type 同時也代表樹的深度，所以一個 key 的
// 父層 TYPE 是隱含的。0 保留給 ZONE_ROOT（非地圖的全局層，存放陣營 / 神祇 /
// 具名角色）；真正的 type 從 1 開始。
// TODO: 要掛在ruleset底下
enum class ZoneType : uint16_t {
    Invalid = 0,   // == ZONE_ROOT
    World   = 1,   // 世界層  類 Civ，約 km/格，1 回合 ≈ 1 天
    Region  = 2,   // 戰略層  大戰略 / Fire-Emblem，數十 m/格
    Area    = 3,   // 區域層  Rimworld / ToME4 / JRPG，約 m/格
};

// ---- 垂直分層 (z 欄位) ---------------------------------------------------
// 三種地圖 type 都疊上同樣的三個垂直層；z 以地面為中心（往下為負，往上為正）。
// 不同的 z 就是不同的 zone。
// TODO: 要掛在ruleset底下
namespace zlayer {
    inline constexpr int16_t Underground = -1;
    inline constexpr int16_t Ground      =  0;
    inline constexpr int16_t Sky         = +1;
}

// ---- 尺度常數（單一來源） -------------------------------------------------
// 全部都是估計值（「或更大」/「大約」）；座標運算必須引用這些常數，絕不要把
// 數字寫死。world_dim 是唯一的 PER-SAVE 執行期設定（存在 ROOT 上的
// components/world_config.h）；region/area 則是常數。
// TODO: 要掛在ruleset底下，各種dim應該要分xyz，而不是dim^2, dim^3
namespace zone_scale {
    inline constexpr int16_t WORLD_DIM_DEFAULT = 200; // 預設世界地圖邊長（world-格）；見 WorldConfig
    // 預設垂直層數：Underground / Ground / Sky（z = −1 / 0 / 1）。日後可擴充（冥界往下、
    // 天界往上）；z 以地面為中心，層數即 zlayer 的涵蓋範圍，見 WorldConfig::world_dim_z。
    inline constexpr int16_t WORLD_LAYERS_DEFAULT = 3;
    inline constexpr int16_t REGION_DIM = 15;   // 1 個 world-格  → REGION_DIM²  個 region-格
    inline constexpr int16_t AREA_DIM   = 250;  // 1 個 region-格 → AREA_DIM²    個 area-格

    // 一個 Area 的全局 region-格座標 = world*REGION_DIM + local；最大值
    //（= world_dim*REGION_DIM - 1）必須塞得進 16 位元的 ZoneKey x/y 欄位。由於
    // TODO:這個要寫死，之後在ruleset載入時檢查
    inline constexpr int16_t MAX_WORLD_DIM = 32766 / REGION_DIM;   // REGION_DIM=15 時為 2184
    inline constexpr bool valid_world_dim(int wd) {
        return wd > 0 && wd * REGION_DIM < 32767;
    }
}

// ---- ZoneKey 打包 ---------------------------------------------------------
using ZoneKey = uint64_t;
constexpr ZoneKey ZONE_ROOT = 0;   // = make_zone_key(ZoneType::Invalid, 0, 0, 0)

// 位元 63-48: ZoneType (16) | 47-32: x (16) | 31-16: y (16) | 15-0: z (16)
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

// ---- 座標換算 / 階層 ------------------------------------------------------
// 一個 zone 的 (x,y) = 它在父層 GLOBAL 格網中的座標：
//   World : 每個 z 一張地圖，位於 (0,0)。
//   Region: 它所展開的那個 world-格，        x,y ∈ [0, world_dim)。
//   Area  : 它所展開的那個 GLOBAL region-格，x,y ∈ [0, world_dim*REGION_DIM)。
// 父層用整數除法回推；z 沿著鏈往上保持不變。

inline ZoneKey world_key(int16_t z = zlayer::Ground) {
    return make_zone_key(ZoneType::World, 0, 0, z);
}

inline ZoneKey region_key(int16_t world_x, int16_t world_y, int16_t z = zlayer::Ground) {
    return make_zone_key(ZoneType::Region, world_x, world_y, z);
}

// 由父層 world-格 + region-local 偏移量建構出一個 Area key。
inline ZoneKey area_key(int16_t world_x, int16_t world_y,
                        int16_t region_local_x, int16_t region_local_y,
                        int16_t z = zlayer::Ground) {
    return make_zone_key(ZoneType::Area,
        static_cast<int16_t>(world_x * zone_scale::REGION_DIM + region_local_x),
        static_cast<int16_t>(world_y * zone_scale::REGION_DIM + region_local_y),
        z);
}

// `k` 的直接父 zone（World 與 root 本身都回傳 ZONE_ROOT）。
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
