#pragma once
#include <entt.hpp>
#include "../components/zone_meta.h"
#include "../components/child_zone_summary.h"
#include "../components/cross_zone_ref.h"
#include "../components/position.h"
#include "../components/velocity.h"
#include "../components/area_terrain.h"
#include "../components/blocking.h"

// Single source of truth for snapshot save/load order.
// Add new component types here; both save and load expand this list.
using AllComponents = entt::type_list<
    ZoneMeta,
    ChildZoneSummary,
    CrossZoneRef,
    Position,
    Velocity,
    AreaTerrain,
    Blocking
>;
