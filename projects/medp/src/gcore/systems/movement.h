#pragma once
#include <entt.hpp>
#include "../components/position.h"
#include "../components/velocity.h"

namespace systems {

// 每個 zone 的 system：讓每個帶有 Velocity 的 actor 前進一步。
// 簽章為 void(entt::registry&)，因此可以註冊為 GlobalManager 上的
// zone system，並對每個已載入的 zone 執行。
inline void movement(entt::registry& reg) {
    reg.view<Position, Velocity>().each([](Position& p, Velocity& v) {
        p.x += v.dx;
        p.y += v.dy;
    });
}

} // namespace systems
