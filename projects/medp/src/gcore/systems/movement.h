#pragma once
#include <entt.hpp>
#include "../components/position.h"
#include "../components/velocity.h"

namespace systems {

// 每個 zone 的 system：讓每個帶有 Velocity 的 actor 前進一步。
// 簽章為 void(entt::registry&)，要交給 ZoneManager 排程的話包一層：
//   zm.add_zone_system([](Zone& z){ systems::movement(z.reg); });
inline void movement(entt::registry& reg) {
    reg.view<Position, Velocity>().each([](Position& p, Velocity& v) {
        p.x += v.dx;
        p.y += v.dy;
    });
}

} // namespace systems
