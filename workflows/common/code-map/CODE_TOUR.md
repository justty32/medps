# CODE_TOUR — 給人的程式碼導讀

給專案作者用的線性閱讀路徑：照順序讀，每站說明「這檔在幹嘛、看什麼、讀完該能回答什麼」。
[CODE_MAP.md](CODE_MAP.md) 是 agent 修改前的查表；本檔是人的導讀，兩者同鏈維護（見文末）。
瀏覽器版導覽在 [html/index.html](html/index.html)（嵌入帶行號的原始碼；由 `html/build.py` 生成）。

本檔已拆為兩檔（單檔 8KB 上限）：本檔＝地基層（通用 Zone 框架＋存讀檔＋測試），
[CODE_TOUR_world_actor.md](CODE_TOUR_world_actor.md)＝World 子類與 actor 身分層。
兩檔合計約 1,400 行核心碼，一次讀完約 1.5 小時。

## 全貌一句話

`ZoneManager` 管一堆 `Zone`（繼承基底，`World` 是第一個子類；一個 zone = 一個
`entt::registry` + 自帶多層 tile 地圖），zone id 是零語意的單調序號、由 `create_child`
配發；存讀檔 = 一 zone 一檔（zone_io 兩塊接合，kind tag 決定子類）＋`manifest.bin`
記 id 計數器；`tick()` 對每個已載入 zone（**含 root**）跑所有註冊的 system。
actor（地點/部隊）是 ECS 組合而非 C++ 繼承，細節見下冊。

## 閱讀路徑總覽（9 站，依依賴順序）

1. `util/` — 容器與巨集工具（本檔）
2. `zone/zone.h` + `tile.h` — Zone 繼承基底、ZoneKind、make_zone（本檔）
3. `common/` — actor 身分層：Name/Owner/Location/Unit（[下冊](CODE_TOUR_world_actor.md)）
4. `world/components/` + `world/systems/` — World 專屬 component 與 system 範本（[下冊](CODE_TOUR_world_actor.md)）
5. `world/world.{h,cpp}` + `world_gen.{h,cpp}` — World 子類與 worldgen 演算法（[下冊](CODE_TOUR_world_actor.md)）
6. `serialize/` — 存讀檔管線，AllComponents 唯一登記點（本檔）
7. `zone/zone_manager.{h,cpp}` — 總管（本檔）
8. `projects/tests/src/main.cpp` — 21 個 case（本檔）
9. `gbind/` — Godot GDExtension 接線（附錄，本檔）

## 第 1 站 `projects/medp/src/gcore/util/`（2 檔）— 容器與巨集工具

- `tdarray.hpp`：2D 陣列模板，row-major（`vec[x*sy+y]`），已 cereal 化。檔頭三條使用慣例必讀：
  回傳 bool 的操作一律「**true = 失敗 / 越界 / 中斷**」；座標可傳任何有 `.x/.y` 的型別（`is_coor`）；
  取值家族 get/getref（未檢查）、getptr（檢查、可 nullptr）、getval（複本、可帶 default）。
- `mydef.h`：metaprogramming 巨集，各附使用範例，掃過即可。

讀完該能回答：`arr.set(x, y, v)` 回傳 `true` 代表什麼？（跟直覺相反。）

## 第 2 站 `projects/medp/src/gcore/zone/zone.h` + `tile.h` + `zone.cpp`（~120 行）— 核心資料結構

- `Tile`（`tile.h:12`）：`{uint32 terrain, flags}`＋`TILE_WALKABLE`/`TILE_BLOCKS_SIGHT`。**不是 component**——地圖是 zone 的固有結構。
- `Zone`（`zone.h:28`）：`{id, parent, reg, layers}`，**是繼承基底**（`zone.h:25` 起註解）：
  virtual dtor 使隱式 move 被抑制——Zone 不可複製也不可移動，一律經 `unique_ptr`/參照持有，
  子類物件切片結構上不可能發生。`layers`（`zone.h:44`）是 `map<int, tdarray<Tile>>`，鍵即 z。
- `ZoneKind`（`zone.h:17`）：存檔時寫進檔頭的 uint8 tag（`Plain=0, World=1`），續編只加號不重排。
  `make_zone`（`zone.cpp:6`）依 tag 建構對應子類，未知 tag throw；`zone_cast<T>`（`zone.h:65`）
  是型別安全向下轉型。`save_extra`/`load_extra`（`zone.h:55-56`）是子類專屬資料的序列化掛鉤，
  基底 no-op。**目前唯一子類 `World` 在第 5 站**（本站只需知道掛鉤存在）。
- registry 不可複製；`ZONE_ROOT=0` 永駐、放非地圖的全局實體（陣營/神祇/具名角色）。

讀完該能回答：為什麼地圖掛在 Zone 上而不是做成 component？子類要序列化自己的資料，該覆寫哪兩個掛鉤？

## 第 6 站 `projects/medp/src/gcore/serialize/`（4 檔，~150 行）— 存讀檔管線

建議順序：

1. `all_components.h` — `AllComponents` type_list，**新增 component 唯一要登記的地方**。
   現存 8 型：`Position, Velocity`（world）、`Name, Owner, Location, Unit, LocationKind, UnitKind`（common，
   見下冊第 3 站）。
2. `entt_cereal_archive.h` — 純膠水：把 entt snapshot 的 callback 簽章轉成 cereal 呼叫，`entt::entity` ↔ 底層整數。
3. `registry_io.h:16,23` — `save_impl/load_impl` 用 fold expression 展開 AllComponents；`registry_io.h:30` 的 `loader.orphans()` 是**陷阱點**：load 後沒有任何（已登記）component 的 entity 會被清掉。
4. `zone_io.h` — 完整 Zone 的存讀：第一塊（kind tag/id/parent/layers/子類 extra）直接 cereal、
   第二塊（reg）走 registry_io，兩塊依序接在同一 stream；大括號限制 archive 生存期是 cereal
   解構時才寫出的緣故。`load`（`zone_io.h:35`）先讀 kind tag 經 `make_zone` 建構才知道要讀哪個子類。
   **無版本欄位**（使用者裁定）：格式一變，舊檔讀出來就是壞資料。

讀完該能回答：為什麼新 component 忘了登記 AllComponents，存檔會「默默」漏掉它、不會報錯？

## 第 7 站 `projects/medp/src/gcore/zone/zone_manager.h` / `.cpp`（~225 行）— 總管

把前面站全部接起來。header 註解寫了三條契約（tick 重入禁令、單槽活儲存、`Zone*` 不跨 tick 持有），必讀：

- 建構子（`zone_manager.cpp:9`）：開檔協定——有 manifest → 還原 next_id＋**必讀回 root.bin**（缺失 throw）；無 manifest 但有 .bin → throw；乾淨目錄 → 新世界。
- `create_child`（`zone_manager.cpp:47`）：多帶一個 `ZoneKind kind = Plain` 參數，決定建構的子類
  （id 單點配發、永不復用，配發即原子寫 manifest；撞既有檔或 parent 未載入 → throw）。
  拿子類介面用 `zone_cast<T>`（見第 2 站）。
- `load`（`zone_manager.cpp:97`）：檔案不存在回 false；檔內 id 與請求不符 → throw。
- `destroy`（`zone_manager.cpp:60`）：連盤上檔案一起刪（死 zone 不復活）。
- `tick`（`zone_manager.cpp:125`）：每個已載入 zone × 每個 system 依註冊順序；**root 也參加**。

讀完該能回答：哪三種磁碟狀態會讓建構子 throw？`create_child` 的 `kind` 參數決定了什麼？

## 第 8 站 `projects/tests/src/main.cpp`（~545 行）— 可執行的規格書

21 個 case 的總表在檔尾 `main()`。每個 test 就是一段「這功能該怎麼用」的示範；改任何行為前先看對應 test 的期望。
覆蓋範圍：序列化 round-trip/orphans、tdarray、zone_io（含未知 kind fail-fast）、ZoneManager（開檔協定/配號/
持久化/損毀防護）、World（kind round-trip/generate 決定性/sanity）、movement、**actor**（def 註冊/spawn/
序列化 round-trip，見下冊）。

跑法：先建 medp 再建 tests，執行
`./projects/tests/build/bin/medp_test.<平台>.<組態>.<位元數>`（詳見 workflows/testing.md）。

讀完該能回答：哪個 test 保護「死 zone 不復活」這條語意？哪個 test 保護「root 也參加 tick」這條語意？

## 附錄 `projects/medp/src/gbind/`

目前只有接線 smoke-test（`MedpCore::version()`），可略過；動它前讀 `projects/archived/gd/`。

## 維護規則

- 原始碼檔案新增/刪除/職責改變時，agent 更新 CODE_MAP 的同時檢查本檔（與
  [CODE_TOUR_world_actor.md](CODE_TOUR_world_actor.md)）受影響的站點。
- 行號錨點漂移不必逐次修，重寫該站時一併校正；符號名為準、行號為輔。
- HTML 導覽層（`html/`）由 `html/build.py` 生成：程式碼變動後重跑 `python build.py`
  即重新嵌入最新原始碼；各站導讀文字先改本檔（或下冊），再同步 `build.py` 的 `STATIONS`。
- 單檔上限 8KB：某冊寫到超標時再拆一本，母檔（本檔）補一行連結即可。
