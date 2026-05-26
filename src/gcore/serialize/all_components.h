#pragma once
#include <entt.hpp>
#include "../components/zone_meta.h"
#include "../components/cross_zone_ref.h"

// Single source of truth for snapshot save/load order.
// Add new component types here; both save and load expand this list.
using AllComponents = entt::type_list<
    ZoneMeta,
    CrossZoneRef
>;
