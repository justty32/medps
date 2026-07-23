#pragma once
#include <entt.hpp>
#include "../components/position.h"
#include "../components/velocity.h"
#include "../../zone/zone.h"

namespace systems {

// 位置變更的唯一入口：直改 Position 僅限 zone 內移動且必經 move_by。
// 將來 tile flag 檢查（可走性）、空間索引維護都掛在這裡。
// 現階段僅座標加法；z 不動（Velocity 無 dz，跨層移動屬將來的傳送/樓梯機制）。
inline void move_by(Zone& z, entt::entity e, int dx, int dy) {
    auto& p = z.reg.get<Position>(e);
    p.x += dx;
    p.y += dy;
}

// 每個 zone 的 system：讓每個帶有 Velocity 的 actor 前進一步。
// 簽章即 ZoneSystem，可直接 zm.add_zone_system(systems::movement);
inline void movement(Zone& z) {
    z.reg.view<Position, Velocity>().each([&](entt::entity e, Position&, Velocity& v) {
        move_by(z, e, v.dx, v.dy);
    });
}

} // namespace systems
