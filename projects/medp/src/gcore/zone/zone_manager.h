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
//
// 存檔目錄是「單槽活儲存」：目錄即世界的權威狀態，unload 隨時寫檔、
// save_all 是檢查點而非槽位快照；各 zone 凍結於不同遊戲時刻是接受的語意。
// 多槽/另存 = 複製整個目錄。目錄裡的 manifest.bin 是這份存檔的 metainfo
//（目前僅 next_zone_id，之後存檔層級的中繼資料都放這）。
//
// 指標生存期：get()/create_child()/root() 給出的 Zone*/Zone& 不得跨 tick
// 持有——想長駐就存 ZoneId、每次重新 get()。
class ZoneManager {
public:
    using ZoneId = uint64_t;

    // dir 是這局存檔的資料夾（相對 / 絕對皆可）。開檔協定：
    //   - dir/manifest.bin 存在 → 既有存檔：還原 next_id_，並必須成功
    //     載入 root.bin（缺失 = 存檔損毀，throw）。
    //   - manifest 不存在但目錄裡有 .bin 檔 → throw（不得靜默當新世界
    //     然後覆寫舊檔）。
    //   - 目錄乾淨 → 全新世界。
    explicit ZoneManager(std::filesystem::path dir);

    // 永久存活的 root zone，放非地圖的全局實體。
    Zone& root() { return *zones_.at(ZONE_ROOT); }

    // 不在記憶體中則回傳 nullptr（不會去讀磁碟，要讀請用 load）。
    Zone* get(ZoneId id);

    // 建立一個新 zone，id 由內部單調序號配發（永不復用），parent 必須已
    // 在記憶體中（否則 throw）。配發後即更新 manifest；若配到的 id 在磁碟
    // 上已有檔案（manifest 損毀/回退的徵兆）→ throw，絕不靜默覆寫。
    Zone& create_child(ZoneId parent);

    // 銷毀一個 zone：移出記憶體，並同步刪除盤上檔案（否則之後 load 會把
    // 死 zone 靜默復活）。id 為 ZONE_ROOT 時什麼都不做。
    void destroy(ZoneId id);

    std::size_t size() const { return zones_.size(); }

    // ---- 存檔 / 讀檔 ----

    // id → 檔案路徑：dir_/<16 碼 hex>.bin；ZONE_ROOT 特例為 dir_/root.bin。
    std::filesystem::path path(ZoneId id) const;

    // 檢查點：把目前記憶體中所有 zone（含 root）寫出＋更新 manifest，不做移除。
    void save_all();

    // 從磁碟讀入一個 zone。已在記憶體中則直接成功、不碰磁碟；
    // 檔案不存在則回傳 false 且不建立任何東西。
    // 檔案內容的 id 與請求的 id 不符 = 存檔損毀，throw 附兩個 id。
    bool load(ZoneId id);

    // 寫出後從記憶體移除。root 不可 unload；id 不存在時什麼都不做。
    void unload(ZoneId id);

    // ---- systems ----

    // per-zone 系統。吃 Zone& 而非 entt::registry&，因為系統可能需要地圖
    //（Zone::layers）而不只是 entity。同簽章的自由函式可直接註冊，例如：
    //   zm.add_zone_system(systems::movement);
    using ZoneSystem = std::function<void(Zone&)>;

    // 註冊一個 per-zone 系統；tick() 會依註冊順序執行它們。
    void add_zone_system(ZoneSystem sys);

    // 推進一步：對每個 zone 執行所有已註冊的系統（root 也算在內）。
    //
    // 重入禁令：system 內禁止 zone 結構性變更（create_child/load/unload/
    // destroy）——tick 正在迭代 zones_，改動它是迭代器 UB。將來第一個需要
    // 在 system 內造/載 zone 的玩法出現時，改成命令緩衝（排隊、tick 尾端執行）。
    void tick();

private:
    Zone& emplace_zone(ZoneId id, ZoneId parent);  // 純記憶體建構，不配號、不碰磁碟
    void  write(Zone& z);                          // zone -> 檔案
    void  write_manifest();                        // next_id_ -> manifest.bin（tmp+rename 原子寫）

    // 用 unique_ptr 而非直接存 Zone：unordered_map 重新雜湊時會搬動元素，
    // 而 get()/create_child()/root() 對外發出 Zone*、Zone&，位址必須穩定。
    std::filesystem::path                             dir_;
    std::unordered_map<ZoneId, std::unique_ptr<Zone>> zones_;
    std::vector<ZoneSystem>                           systems_;
    ZoneId                                            next_id_{1};  // 0 保留給 root
};
