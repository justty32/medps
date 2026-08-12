# CODE_MAP — medps 程式碼導航 index

目標：修改前快速定位相關檔案，只讀必要範圍。repo 尚小，先用單檔；領域膨脹後再拆子 index。

## 目錄結構

```text
projects/medp/src/gcore/          — 遊戲核心框架（EnTT + cereal）
  zone/             — 通用 Zone 框架：Zone 基底、ZoneManager、Tile
  common/           — 多數 Zone 子類共用的組件/系統；actor（地點/部隊）家族
    components/     — 共用 POD component（Name/Owner/Location/Unit）
  world/            — World : Zone 子類與其專屬物；worldgen、World 尺度資料
    components/     — World 專屬 POD component（Position/Velocity）
    systems/        — 吃 Zone& 的自由函式系統；位置變更收口於 move_by
  serialize/        — entt⇄cereal adapter、AllComponents 清單、zone_io
  util/             — 共用工具
projects/medp/src/gbind/          — Godot 4 GDExtension facade（薄殼，CMake 第二 target，預設不編）
projects/medp/include/            — 第三方 header-only 庫（entt、cereal）——不要修改
projects/medp/data/               — 資料目錄（build 時複製到 projects/medp/build/bin/data）
projects/tests/                   — 測試執行檔（projects/tests/src/main.cpp，21 項測試）
projects/archived/                — 重寫前原型（非源碼）
docs/work/                        — 歷史分析/設計文檔
```

## Runtime / Production

| 檔案 | 職責 |
|------|------|
| `projects/medp/src/gcore/zone/zone.h` / `.cpp` | `struct Zone{id, parent, reg, layers}`——繼承基底（virtual dtor，一 zone 一 registry＋多垂直層 tile 地圖，`layers` 鍵即 z）；`ZoneKind` enum（存檔 kind tag）、`make_zone` 工廠（未知 kind throw）、`zone_cast<T>`、`ZONE_ROOT`（id=0）常數 |
| `projects/medp/src/gcore/zone/zone_manager.h` / `.cpp` | `ZoneManager`：持有所有 zone＋配號（create_child）＋存讀（load/unload/save_all/destroy/path）＋manifest 開檔協定＋system 註冊與 tick；三條契約（tick 重入禁令、單槽活儲存、Zone* 不跨 tick）見檔頭註解 |
| `projects/medp/src/gcore/zone/tile.h` | `Tile{terrain, flags}` 與 `TILE_WALKABLE`/`TILE_BLOCKS_SIGHT` flag 常數 |
| `projects/medp/src/gcore/common/actor.h` | actor 與種類 def 的工廠：`define_location`/`define_unit`（在 root 建種類 def，吃 `Zone&`＋fail-fast 檢查 `id==ZONE_ROOT`）、`find_location_def`/`find_unit_def`（依 id 於 root 解析 def，線性掃）、`spawn_location`/`spawn_unit`（在某 zone 建 actor：Name＋Owner＋家族 tag{kind=def id}，放置=Position 刻意分離）。actor＝地點/部隊兩大家族的共同基底，在 ECS 裡由共用元件表達而非 C++ 父類 |
| `projects/medp/src/gcore/common/components/*.h` | 共用 POD component。身分：`Name{value}`（含 cereal string）、`Owner{faction}`（陣營穩定 id，0=中立）。種類為**資料驅動 def**（非 enum）：def 元件 `LocationKind{id}`/`UnitKind{id}` 掛在 root 的定義實體上（開放集合，將來由 Ruleset 載入）；actor 家族 tag `Location{kind}`/`Unit{kind}` 住在 zone、以穩定 id 參照 def |
| `projects/medp/src/gcore/world/world.h` / `.cpp` | `World : Zone` 第一個子類：持 `WorldGenParams gen`（隨 extra 塊序列化）＋`save_extra`/`load_extra`；`generate()` 是薄殼，挑 `layers[0]` 呼 `world_gen::generate(gen, layers[0], id)` |
| `projects/medp/src/gcore/world/world_gen.h` / `.cpp` | worldgen 模組（不依賴 World 型別，只認配方＋一張 grid）：`WorldGenParams`（生成配方）＋biome 佔位 terrain 常數＋`world_gen::generate(const WorldGenParams&, tdarray<Tile>&, uint64_t zone_id=0)`（libtcod FBM 高度場→sea_level 水陸→溫/濕度分 biome，內部 alloc 整層重寫，同 seed 同圖；libtcod 依賴收在 .cpp 不外洩） |
| `projects/medp/src/gcore/world/components/*.h` | World 專屬 POD component（position【x/y/z】, velocity） |
| `projects/medp/src/gcore/world/systems/movement.h` | movement 系統與 `move_by` 收口（吃 `Zone&` 的自由函式） |
| `projects/medp/src/gcore/serialize/entt_cereal_archive.h` | EnTT snapshot ⇄ cereal `PortableBinaryArchive` 的 archive adapter |
| `projects/medp/src/gcore/serialize/registry_io.h` | 單一 registry 的 snapshot save/load |
| `projects/medp/src/gcore/serialize/all_components.h` | `AllComponents` type_list——component 型別清單的**單一來源**；新增 component 必登記 |
| `projects/medp/src/gcore/serialize/zone_io.h` | 完整 Zone 的 save/load：第一塊 kind tag＋id/parent/layers＋子類 extra 走 cereal，reg 接 registry_io；load 讀 tag 經工廠建構、回傳 `unique_ptr<Zone>` |
| `projects/medp/src/gcore/util/tdarray.hpp` | `tdarray<T>` 2D 陣列模板（已 cereal 化） |
| `projects/medp/src/gcore/util/mydef.h` | metaprogramming macros（仍使用中） |
| `projects/medp/src/gbind/` | Godot GDExtension facade（medp_core / register_types；目前只有接線 smoke-test） |
| `projects/medp/CMakeLists.txt` | 兩個 target：`medp`（SHARED+STATIC）＋可選 `medp_gdext`（`-DMEDP_BUILD_GDEXTENSION=ON`）；libtcod 2.2.2 headless 以 FetchContent 引入（SDL/zlib/PNG/unicode 全關） |

## Tests

| 檔案 | 覆蓋 |
|------|------|
| `projects/tests/src/main.cpp` | 全部 21 項測試：序列化 round-trip/orphans、tdarray、zone_io（含未知 kind fail-fast）、ZoneManager（開檔協定/配號/持久化/損毀防護）、World（kind round-trip/generate 決定性/sanity）、movement、actor（def 註冊/spawn/序列化 round-trip） |

## Docs / 分析

| 檔案 | 用途 |
|------|------|
| `docs/work/progress_overview.md` | 專案進度白話總覽 |
| `docs/work/design/zone_layers.md` | 核心世界結構設計（zone 分層） |
| `docs/work/design/lifecycle.md` | zone 生命週期設計 |
| `docs/work/architecture/gcore_overview.md` | gcore 架構總覽 |
| `projects/archived/gd/` | Godot 前端草稿（重寫前原型，已歸檔）|
| `workflows/common/code-map/html/` | 程式碼 HTML 導覽層（`build.py` 生成，嵌入帶行號原始碼；真相層是原始碼與 CODE_TOUR） |

## 架構不變量（修改前必知）

1. **多 registry / zone 生命週期**：一個 zone = 一個 `entt::registry`，由 `ZoneManager` 管理；root（id=0）永久存活、放全局實體，其餘 zones 按需載入/卸載。zone id 是零語意 uint64 單調序號（create_child 單點配發、永不復用），磁碟 path 由 id 推導（hex 檔名），manifest 只存 next_zone_id、不存 zone 清單。
2. **序列化**：EnTT `snapshot` 遍歷 registry，cereal `PortableBinaryArchive` 負責位元格式，透過 `serialize/entt_cereal_archive.h` 橋接。component 型別清單的單一來源是 `serialize/all_components.h` 的 `AllComponents`，save/load 兩邊共用。
3. **元件即資料**：component 盡量是 POD aggregate；entity 之間的參照存 `entt::entity`。system 寫成吃 `Zone&` 的自由函式；位置變更一律收口於 `systems::move_by`。

## 修改前規則

1. 先判斷要改的功能屬於哪個領域，只讀該領域列出的檔案。
2. 若 CODE_MAP 缺資料或與程式碼衝突，以程式碼為準，立即修正 CODE_MAP。
3. 新增/刪除原始碼檔案，或檔案職責顯著改變時，同步更新本檔。
