#pragma once
#include <stdexcept>
#include <string>
#include <utility>
#include <entt.hpp>
#include "../zone/zone.h"
#include "components/name.h"
#include "components/owner.h"
#include "components/location.h"
#include "components/unit.h"

// actor 與其種類定義（def）的工廠。
//
// actor＝「有身分、被玩家指涉、擺在世界裡」的實體，是地點（Location）與部隊（Unit）
// 兩大家族的共同基底。在 ECS 裡「基底」不是 C++ 父類，而是一組共用元件：
//   身分  = Name ＋ Owner
//   放置  = Position（由放置流程另掛，刻意與身分分離——見 world/components/position.h）
//   家族  = Location{kind} 或 Unit{kind}（互斥，一個 actor 只屬一族）
//
// 種類（kind）不是寫死的 enum，而是**資料驅動的 def**：def 是住在 root 的實體
//（LocationKind{id}／UnitKind{id} ＋ Name），actor 以穩定 id 參照它（跨 registry
// 安全，比照 zone id / Owner.faction）。要新增一種地點/部隊＝多建一個 def 實體，
// 不改任何 enum；將來 Ruleset 落地時 def 由檔案載入、id 由登錄機制發號。
namespace actor {

// ---- 定義（一律建在 root zone）----
//
// def 類型一定掛在 root（root 放全局實體＋規則定義）。define_* 吃 Zone& 並
// fail-fast 檢查——傳非 root 進來＝呼叫端邏輯錯，直接 throw、不靜默寫到別的 zone。

namespace detail {
inline void require_root(const Zone& z, const char* who) {
    if (z.id != ZONE_ROOT)
        throw std::runtime_error(
            std::string(who) + ": def 只能掛在 root zone，收到 zone id=" + std::to_string(z.id));
}
}  // namespace detail

// 定義一種地點：LocationKind{id}＋Name{name}。回傳 def 實體。
inline entt::entity define_location(Zone& root, uint64_t id, std::string name) {
    detail::require_root(root, "actor::define_location");
    auto e = root.reg.create();
    root.reg.emplace<LocationKind>(e, id);
    root.reg.emplace<Name>(e, std::move(name));
    return e;
}

// 定義一種部隊：UnitKind{id}＋Name{name}。回傳 def 實體。
inline entt::entity define_unit(Zone& root, uint64_t id, std::string name) {
    detail::require_root(root, "actor::define_unit");
    auto e = root.reg.create();
    root.reg.emplace<UnitKind>(e, id);
    root.reg.emplace<Name>(e, std::move(name));
    return e;
}

// 依 id 找地點 def（在 root），找不到回 entt::null。線性掃——def 數量小，
// 需要時再上索引（比照 zone 的 unordered_map）。
inline entt::entity find_location_def(const Zone& root, uint64_t id) {
    for (auto e : root.reg.view<const LocationKind>())
        if (root.reg.get<const LocationKind>(e).id == id) return e;
    return entt::null;
}

inline entt::entity find_unit_def(const Zone& root, uint64_t id) {
    for (auto e : root.reg.view<const UnitKind>())
        if (root.reg.get<const UnitKind>(e).id == id) return e;
    return entt::null;
}

// ---- 實體（建在某個 zone 的 registry）----

// 建一個地點 actor：Name＋Owner＋Location{kind}。kind＝某 LocationKind def 的 id。
inline entt::entity spawn_location(entt::registry& r, uint64_t kind,
                                   std::string name, uint64_t owner = 0) {
    auto e = r.create();
    r.emplace<Name>(e, std::move(name));
    r.emplace<Owner>(e, owner);
    r.emplace<Location>(e, kind);
    return e;
}

// 建一個部隊 actor：Name＋Owner＋Unit{kind}。kind＝某 UnitKind def 的 id。
inline entt::entity spawn_unit(entt::registry& r, uint64_t kind,
                               std::string name, uint64_t owner = 0) {
    auto e = r.create();
    r.emplace<Name>(e, std::move(name));
    r.emplace<Owner>(e, owner);
    r.emplace<Unit>(e, kind);
    return e;
}

} // namespace actor
