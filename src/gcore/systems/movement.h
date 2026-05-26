#pragma once
#include <entt.hpp>
#include "../components/position.h"
#include "../components/velocity.h"

namespace systems {

// Per-zone system: advance every actor with a Velocity by one step.
// Signature is void(entt::registry&) so it can be registered as a zone system
// on the GlobalManager and run against each loaded zone.
inline void movement(entt::registry& reg) {
    reg.view<Position, Velocity>().each([](Position& p, Velocity& v) {
        p.x += v.dx;
        p.y += v.dy;
    });
}

} // namespace systems
