#pragma once
#include <cstdint>
#include <map>
#include <memory>
#include <entt.hpp>
#include <cereal/archives/portable_binary.hpp>
#include "../util/tdarray.hpp"
#include "tile.h"

// root zone 的固定 id。它永久存活、沒有地圖，放的是非地圖的全局實體
//（陣營 / 神祇 / 具名角色）。root 的 parent 就是它自己。
inline constexpr uint64_t ZONE_ROOT = 0;

// zone 的具體型別，序列化時寫進 zone 檔頭（uint8），load 讀 tag 後經 make_zone
// 建構對應子類。續編規則：新子類往後加號、不重排；未知值＝存檔損毀（make_zone throw）。
// kind 不進 zone id——id 維持零語意單調序號。
enum class ZoneKind : uint8_t { Plain = 0, World = 1 };

// 一個 zone = 一個獨立的 entt::registry，加上它自己的身分。
//
// 身分直接掛在 Zone 上，不再由 registry 內部的 placeholder entity 攜帶
//（舊做法是 ZoneMeta component，已移除）。好處是不必為了「讓 zone 有身分」
// 而保證 registry 裡永遠有一個活著的 entity。
//
// Zone 是繼承基底（第一個子類：World，見 world.h）。virtual dtor 使隱式
// move 被抑制——Zone 不可複製也不可移動，一律經 unique_ptr / 參照持有
//（ZoneManager 本就如此），子類物件切片在結構上不可能發生。
struct Zone {
    // 全局唯一識別：零座標語意的單調序號，由 ZoneManager::create_child 配發。
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

    virtual ~Zone() = default;

    // 型別由 virtual 回答、不另存欄位——杜絕「欄位與實際型別不一致」一族錯誤。
    // 子類 override 時回傳自己的 KIND。
    static constexpr ZoneKind KIND = ZoneKind::Plain;
    virtual ZoneKind kind() const { return KIND; }

    // 子類專屬資料的序列化掛鉤，由 zone_io 在第一塊（id/parent/layers 之後）呼叫。
    // save / load 必須成對對稱——欄位漏寫即後續位元流整批錯位。基底無專屬資料，no-op。
    virtual void save_extra(cereal::PortableBinaryOutputArchive&) {}
    virtual void load_extra(cereal::PortableBinaryInputArchive&) {}
};

// 工廠：依 kind 建構對應子類（zone_io::load 讀檔頭 tag 後、ZoneManager 建 zone 時呼叫）。
// 未知 kind＝存檔損毀，throw；絕不靜默退回 Plain。
std::unique_ptr<Zone> make_zone(ZoneKind kind);

// 型別安全的向下轉型：kind 相符回傳子類指標，否則 nullptr。
//   if (auto* w = zone_cast<World>(zm.get(id))) w->generate();
template <class T>
T* zone_cast(Zone* z) {
    return (z && z->kind() == T::KIND) ? static_cast<T*>(z) : nullptr;
}
