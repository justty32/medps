#pragma once
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>
#include "zone.h"

// 持有所有 zone，並負責對它們執行 system 與存取磁碟。
//
// root（id == ZONE_ROOT）在建構時就存在、永久存活，不能 destroy 也不能 unload。
class ZoneManager {
public:
    using ZoneId = uint64_t;

    // dir 是這局存檔的資料夾（相對 / 絕對皆可）
    explicit ZoneManager(std::filesystem::path dir);

    // 永久存活的 root zone，放非地圖的全局實體。
    Zone& root() { return *zones_.at(ZONE_ROOT); }

    // 不在記憶體中則回傳 nullptr（不會去讀磁碟，要讀請用 load）。
    Zone* get(ZoneId id);

    // 建立一個新 zone。id 已存在時回傳既有者，不覆寫、不改它的 parent。
    Zone& create(ZoneId id, ZoneId parent);

    // 銷毀一個 zone，連同它的 registry 與地圖，且不寫回磁碟。
    // id 不存在、或 id 為 ZONE_ROOT 時什麼都不做。
    void destroy(ZoneId id);

    std::size_t size() const { return zones_.size(); }

    // ---- 存檔 / 讀檔 ----

    // id → 檔案路徑：dir_/<16 碼 hex>.bin；ZONE_ROOT 特例為 dir_/root.bin。
    std::filesystem::path path(ZoneId id) const;

    // 檢查點：把目前記憶體中所有 zone（含 root）寫出，不做移除。
    void save_all();

    // 從磁碟讀入一個 zone。已在記憶體中則直接成功、不碰磁碟；
    // 檔案不存在則回傳 false 且不建立任何東西。
    bool load(ZoneId id);

    // 寫出後從記憶體移除。root 不可 unload；id 不存在時什麼都不做。
    void unload(ZoneId id);

    // ---- systems ----

    // per-zone 系統。吃 Zone& 而非 entt::registry&，因為系統可能需要地圖
    //（Zone::layers）而不只是 entity。只用到 entity 的系統包一層即可，例如：
    //   zm.add_zone_system([](Zone& z){ systems::movement(z.reg); });
    using ZoneSystem = std::function<void(Zone&)>;

    // 註冊一個 per-zone 系統；tick() 會依註冊順序執行它們。
    void add_zone_system(ZoneSystem sys);

    // 推進一步：對每個 zone 執行所有已註冊的系統（root 也算在內）。
    void tick();

private:
    void write(Zone& z);   // zone -> 檔案

    // 用 unique_ptr 而非直接存 Zone：unordered_map 重新雜湊時會搬動元素，
    // 而 get()/create()/root() 對外發出 Zone*、Zone&，位址必須穩定。
    std::filesystem::path                             dir_;
    std::unordered_map<ZoneId, std::unique_ptr<Zone>> zones_;
    std::vector<ZoneSystem>                           systems_;
};
