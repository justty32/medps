#pragma once
#include <unordered_map>
#include <memory>
#include <vector>
#include <functional>
#include <entt.hpp>
#include "zone_key.h"
#include "components/world_config.h"
#include "serialize/zone_store.h"

class GlobalManager {
public:
    entt::registry root;   // ZONE_ROOT，永遠存活

    GlobalManager();
    explicit GlobalManager(std::unique_ptr<ZoneStore> store);

    // 未載入則回傳 nullptr
    entt::registry* get(ZoneKey key);

    // 在 `parent` 底下建立一個全新的 zone，植入它的 ZoneMeta placeholder。
    // parent 記錄在 ZoneMeta 中（key 階層本身就隱含父層關係）。
    entt::registry& create(ZoneKey key, ZoneKey parent);

    // 從 store 反序列化一個 zone；若已載入則回傳既有者。
    entt::registry& load(ZoneKey key);

    // 把一個 zone 序列化到 store 並從記憶體中卸除。
    void unload(ZoneKey key);

    // ---- systems ----

    // per-zone 系統：對單一 zone 的 registry 執行。
    using ZoneSystem = std::function<void(entt::registry&)>;

    // 註冊一個 per-zone 系統；tick() 會依註冊順序執行它們。
    void add_zone_system(ZoneSystem sys);

    // 推進一步：對每個已載入的 zone 執行所有已註冊的 per-zone 系統
    //（root 被排除 -- 它存放全局 entity，而非地圖角色；cross-zone 系統會另外處理）。
    void tick();

    // ---- 整局存檔 / 讀檔 ----

    // 檢查點：把 root + 目前所有已載入的 zone 寫入 store（不做卸除）。
    void save_all();

    // 開啟一局遊戲：載入 root registry。子 zone 留在 store 中，直到透過
    // load()/create() 按需載入。
    void load_root();

    // ---- per-save 世界設定（ROOT singleton）----

    // 新遊戲：植入 root 的 WorldConfig（world_dim_x/y/z 可選；小/中/大）。
    // 冪等 -- 覆寫既有的 singleton。一旦有 zone 存在，world_dim 在這局存檔的生命週期
    // 內就不可變（它已被烘進 key 運算中）。
    // 前置條件：x、y 滿足 zone_scale::valid_world_dim。
    WorldConfig& init_world(int16_t world_dim_x = zone_scale::WORLD_DIM_DEFAULT,
                            int16_t world_dim_y = zone_scale::WORLD_DIM_DEFAULT,
                            int16_t world_dim_z = zone_scale::WORLD_LAYERS_DEFAULT);

    // 目前生效的 per-save 設定（root 的 singleton）；未設定時為預設建構值。
    // 系統以 `const WorldConfig&` 接收回傳值。
    WorldConfig world_config() const;

    ZoneStore& store() { return *store_; }

private:
    void write_zone(ZoneKey key, entt::registry& reg);        // reg -> bytes -> store（序列化後寫入）

    std::unique_ptr<ZoneStore>                                   store_;
    std::unordered_map<ZoneKey, std::unique_ptr<entt::registry>> loaded_;
    std::vector<ZoneSystem>                                      zone_systems_;
};
