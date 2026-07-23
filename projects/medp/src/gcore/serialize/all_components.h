#pragma once
#include <entt.hpp>
#include "../world/components/position.h"
#include "../world/components/velocity.h"

// snapshot save/load 順序的單一來源。
// 新增 component 型別請加在這裡；save 與 load 兩邊都會展開這份清單。
using AllComponents = entt::type_list<
    Position,
    Velocity
>;
