#pragma once
#include <unordered_map>
#include <memory>
#include <vector>
#include <functional>
#include <entt.hpp>
#include "zone_key.h"
#include "components/cross_zone_ref.h"
#include "components/world_config.h"
#include "serialize/zone_store.h"

// 對 GlobalManager 解析一個 CrossZoneRef 的結果。
//   reg == nullptr           -> 目標 zone 未載入
//   reg != nullptr && !valid -> zone 已載入，但 entity 已失效/消失
//   valid()                  -> 完全可用（reg + entity）
struct ZoneResolution {
    entt::registry* reg{nullptr};
    entt::entity    entity{entt::null};

    bool valid() const { return reg && reg->valid(entity); }
    explicit operator bool() const { return valid(); }
};

class GlobalManager {
public:
    entt::registry root;   // ZONE_ROOT，永遠存活

    // 預設使用位於 "zones" 的 chunked folder 後端（多個 zone 打包進一個 chunk
    // 檔；Plan B）。注入另一個 ZoneStore 即可改變 zone 持久化的位置/方式。
    GlobalManager();
    explicit GlobalManager(std::unique_ptr<ZoneStore> store);

    // 未載入則回傳 nullptr
    entt::registry* get(ZoneKey key);

    // 把一個 cross-zone 參照解析成 {registry, entity}。只查詢已載入的 zone
    //（不做磁碟 IO）；ZONE_ROOT 對映到 `root`。
    ZoneResolution resolve(const CrossZoneRef& ref);

    // 在 `parent` 底下建立一個全新的 zone，植入它的 ZoneMeta placeholder，並在
    // 父 registry 中註冊一個 ChildZoneSummary stub（若父層已載入）。
    // 冪等：對既有 zone 再次 create 不會重複產生 stub。
    entt::registry& create(ZoneKey key, ZoneKey parent);

    // 從 store 反序列化一個 zone；若已載入則回傳既有者。
    entt::registry& load(ZoneKey key);

    // 把一個 zone 序列化到 store 並從記憶體中卸除。
    void unload(ZoneKey key);

    // 載入 `key` 以及與它共用儲存 chunk 的每一個已持久化 zone，各自成為一個
    // registry（folder 後端 = 就只是 load(key)）。預熱整個 chunk 檔，使得在其中
    // 跨越 zone 邊界時不需要額外的磁碟讀取。
    void prefetch(ZoneKey key);

    // Elder Scrolls 風格的開放世界 streaming：在 `center` 周圍維持一個
    //（2*radius+1)^2 的 zone 視窗保持載入（相同 ZoneType + z）-- 載入視窗內磁碟上
    // 存在的 zone，卸除落到視窗外的同層已載入 zone。隨著焦點（玩家所在 zone）移動
    // 時呼叫。建議 radius：Region 約 2（5x5，類似 ES 的 uGridsToLoad）；Area 0
    //（一張沉重的本地地圖，類似室內場景）。
    void stream_around(ZoneKey center, int radius);

    // 列出一個（已載入）父層的直接子 zone，但不載入它們。
    // 若父 zone 未載入則回傳空。
    std::vector<ZoneKey> children(ZoneKey parent);

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
    // load()/create() 按需 stream 進來。
    void load_root();

    // ---- per-save 世界設定（ROOT singleton）----

    // 新遊戲：植入 root 的 WorldConfig（world_dim 可選；小/中/大）。
    // 冪等 -- 覆寫既有的 singleton。一旦有 zone 存在，world_dim 在這局存檔的生命週期
    // 內就不可變（它已被烘進 key/chunk 運算中）。
    // 前置條件：zone_scale::valid_world_dim(world_dim)。
    WorldConfig& init_world(int16_t world_dim = zone_scale::WORLD_DIM_DEFAULT);

    // 目前生效的 per-save 設定（root 的 singleton）；未設定時為預設建構值。
    // 系統以 `const WorldConfig&` 接收回傳值。
    WorldConfig world_config() const;

    ZoneStore& store() { return *store_; }

private:
    entt::registry* parent_registry(ZoneKey parent);          // ZONE_ROOT -> &root
    void ensure_child_stub(entt::registry& parent, ZoneKey child);
    void write_zone(ZoneKey key, entt::registry& reg);        // reg -> bytes -> store（序列化後寫入）

    std::unique_ptr<ZoneStore>                                   store_;
    std::unordered_map<ZoneKey, std::unique_ptr<entt::registry>> loaded_;
    std::vector<ZoneSystem>                                      zone_systems_;
};
