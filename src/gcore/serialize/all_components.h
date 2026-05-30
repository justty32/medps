#pragma once
#include <entt.hpp>
#include "../components/zone_meta.h"
#include "../components/child_zone_summary.h"
#include "../components/cross_zone_ref.h"
#include "../components/position.h"
#include "../components/velocity.h"
#include "../components/area_terrain.h"
#include "../components/blocking.h"
#include "../components/world_config.h"

// snapshot save/load 順序的單一來源。
// 新增 component 型別請加在這裡；save 與 load 兩邊都會展開這份清單。
using AllComponents = entt::type_list<
    ZoneMeta,
    ChildZoneSummary,
    CrossZoneRef,
    Position,
    Velocity,
    AreaTerrain,
    Blocking,
    WorldConfig
>;
