# gcore 逐檔導覽 —— zone/ ＋ serialize/

> 分層導覽之一，回總覽見 [gcore_overview.md](gcore_overview.md)。最後更新：2026-08-12。
> 對象：`projects/medp/src/gcore/zone/`、`projects/medp/src/gcore/serialize/`（通用框架＋存讀檔，
> 不認識任何具體遊戲概念）。

## zone/ —— 通用框架

- **`zone.h`**：`ZONE_ROOT=0`（`zone.h:12`）。`Zone`（`zone.h:28-57`）是繼承基底：
  `{ uint64_t id, parent; entt::registry reg; map<int, tdarray<Tile>> layers; }`；
  layers 鍵即 z（地面=0、往下為負），層是稀疏的。地圖直接掛在 Zone 上、不走 ECS，
  代價是 registry snapshot 不含它，存檔由 `zone_io` 分兩塊處理（見下）。
- **`ZoneKind`**（`zone.h:17`，`Plain=0`/`World=1`）：zone 的具體型別 tag，寫進檔頭；
  `kind()` virtual（`zone.h:50-51`）由子類 override 回答型別，不另存欄位，杜絕
  「欄位與實際型別不一致」。`static constexpr KIND` 是子類比對用的編譯期常數。
- **virtual dtor**（`zone.h:46`）使隱式 move 被抑制——`Zone` 不可複製也不可移動，一律經
  `unique_ptr`/參照持有（`ZoneManager` 本就如此），子類物件切片結構上不可能發生
  （`zone.h:25-27` 註解）。
- **`make_zone(ZoneKind)`**（`zone.h:61`，實作 `zone.cpp:6-14`）依 tag 建對應子類；
  未知 kind 直接 throw，不靜默退回 Plain。
- **`zone_cast<T>(Zone*)`**（`zone.h:65-68`）型別安全向下轉型：kind 相符回傳子類指標，
  否則 nullptr（例：`zone_cast<World>(zm.get(id))`）。
- **`save_extra`/`load_extra`**（`zone.h:55-56`）是子類專屬資料的序列化掛鉤，由 `zone_io`
  在 id/parent/layers 之後呼叫；基底 no-op，save/load 兩邊必須成對對稱，欄位漏寫即
  後續位元流整批錯位。
- **`tile.h`**：`Tile{ uint32 terrain, flags }`（`tile.h:12-18`）＋`TILE_WALKABLE`/
  `TILE_BLOCKS_SIGHT`（`tile.h:9-10`）。不是 component——地圖是 Zone 的固有結構。
- **`zone_manager.h`／`.cpp`**：
  - root（`id==ZONE_ROOT`）在建構時就存在、永久存活，不能 `destroy` 也不能 `unload`
    （`zone_manager.h:12`）。
  - `create_child(parent, kind=Plain)`（`zone_manager.h:43`，實作 `zone_manager.cpp:47-58`）：
    id 單點配發（`next_id_++`、永不復用），配發即原子寫 manifest；撞既有檔或 parent
    未載入 → throw。
  - 開檔協定（建構子 `zone_manager.cpp:9-30`）：manifest 存在 → 還原 `next_id_`＋必讀回
    root.bin（缺失 throw）；manifest 不存在但目錄有 `.bin` → throw（不當新世界靜默覆寫）；
    乾淨目錄 → 新世界。
  - `load`（`zone_manager.cpp:97-111`）驗檔內 id 與請求 id 是否相符；`destroy`
    （`zone_manager.cpp:60-64`）同步刪盤上檔案；`unload`（`zone_manager.cpp:113-119`）
    寫檔後移除（root 不可）。
  - `tick()`（`zone_manager.cpp:125-129`）：對**所有**已載入 zone（含 root）依註冊順序
    跑 `ZoneSystem`（`std::function<void(Zone&)>`，`zone_manager.h:72`）。
  - 三條契約（`zone_manager.h:14-20,79-81` 註解明文）：tick 內禁止 zone 結構性變更
    （create_child/load/unload/destroy，迭代器 UB）；存檔目錄＝單槽活儲存（目錄即權威
    狀態，unload 隨時寫檔，各 zone 可能凍結於不同遊戲時刻）；`Zone*`/`Zone&` 不得跨
    tick 持有，想長駐存 `ZoneId`、每次重新 `get()`。
  - 檔名規則：`path(id)`（`zone_manager.cpp:66-71`）＝`<16碼hex>.bin`，root 特例為
    `root.bin`；`manifest.bin` 只存 `next_zone_id`，tmp+rename 原子寫
    （`zone_manager.cpp:79-89`）。

## serialize/ —— 存讀檔

- **`all_components.h`**（`serialize/all_components.h:12-21`）：`AllComponents` 是
  **唯一登記清單**（`entt::type_list`），save/load 兩邊都展開它；新增 component **必須**
  加進來，否則存檔會**默默**漏掉它（沒有編譯期或執行期警告）。
- **`entt_cereal_archive.h`**：`output_archive`/`input_archive`（`entt_cereal_archive.h:8-36`）
  是 EnTT snapshot API 與 cereal `PortableBinaryArchive` 之間的 adapter，把 entity id
  正規化成 `entt_id_t`（底層整數型別）後再序列化。
- **`registry_io.h`**：單一 registry 的 snapshot save/load（`save`/`load`，
  `registry_io.h:35-45`），內部展開 `AllComponents`（`registry_io.h:16-31`）。
  `loader.orphans()`（`registry_io.h:30`）會清掉最終沒有任何已登記 component 的
  entity——陷阱：只帶**未登記**在 `AllComponents` 裡的 component 的 entity，資料連同
  entity 本身都會消失。
- **`zone_io.h`**：完整 Zone 的 save/load（`zone_io::save`/`load`，`zone_io.h:25-47`）。
  Zone 有兩塊資料走兩條路：第一塊（`kind`/`id`/`parent`/`layers`/子類 `save_extra`）
  直接 cereal；第二塊（`reg`）走 `registry_io`。檔案格式是兩塊依序接在同一 stream；
  第一塊開頭是 `uint8` 的 `ZoneKind` tag，`load` 必須先讀它才知道要 `make_zone` 哪個
  子類，因此 `load` 是「建構＋讀入」一體，回傳 `unique_ptr<Zone>`（`zone_io.h:35-47`）。
  cereal archive 在解構時才把緩衝寫出去，所以第一塊用大括號限制生存期
  （`zone_io.h:26-31`），確保它先完整落地、`registry_io` 才接著寫同一個 stream。
- **存檔無版本欄位**（使用者裁定）：格式一變（新增 component、新增 ZoneKind 子類…）
  舊檔就是壞資料，且不一定會顯式報錯——讀檔前務必確認格式是否已 break。

## 速查表

| 檔案 | 角色 |
|---|---|
| `zone/zone.h` | Zone 繼承基底、ZoneKind、make_zone、zone_cast |
| `zone/zone.cpp` | make_zone 實作 |
| `zone/tile.h` | 地圖格＋通行 flags |
| `zone/zone_manager.h/.cpp` | 生命週期/配號/tick/存讀/開檔協定 |
| `serialize/all_components.h` | component 清單唯一來源 |
| `serialize/entt_cereal_archive.h` | EnTT ⇄ cereal adapter |
| `serialize/registry_io.h` | 單一 registry snapshot 存讀 |
| `serialize/zone_io.h` | 完整 Zone 存讀（兩塊接合＋kind tag） |
