#pragma once
#include <cstdint>
#include <map>
#include <entt.hpp>
#include "../util/tdarray.hpp"
#include "tile.h"

// root zone 的固定 id。它永久存活、沒有地圖，放的是非地圖的全局實體
//（陣營 / 神祇 / 具名角色）。root 的 parent 就是它自己。
inline constexpr uint64_t ZONE_ROOT = 0;

// 一個 zone = 一個獨立的 entt::registry，加上它自己的身分。
//
// 身分直接掛在 Zone 上，不再由 registry 內部的 placeholder entity 攜帶
//（舊做法是 ZoneMeta component，已移除）。好處是不必為了「讓 zone 有身分」
// 而保證 registry 裡永遠有一個活著的 entity。
//
// 注意：entt::registry 不可複製，因此 Zone 也不可複製，只能移動。
struct Zone {
    // 全局唯一識別。原本 ZoneKey 的位元打包（type/x/y/z）已移除，
    // 這裡先用裸整數佔位；分層與座標語意待重新設計。
    uint64_t id{};

    // 直接父 zone 的 id；與自己相同時代表沒有父層。
    uint64_t parent{};

    entt::registry reg;

    // 地圖：一個垂直層一張 tile 格網，鍵就是 z（以地面為中心，往下為負）。
    // 用 map 而非 vector：層是稀疏的（多數 zone 只有地面層），z 可以是負數，
    // 而且日後往上下擴充不必重新編號。
    //
    // 直接掛在 Zone 上，不走 ECS——地圖是 zone 的固有結構，不是某個 entity 的屬性。
    // 代價：它不在 registry 裡，所以 registry_io 的 snapshot 不會碰它，
    // 存檔由 serialize/zone_io.h 分兩塊處理。
    std::map<int, tdarray<Tile>> layers;
};
