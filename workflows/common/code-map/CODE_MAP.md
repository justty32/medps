# CODE_MAP — medps 程式碼導航 index

目標：修改前快速定位相關檔案，只讀必要範圍。repo 尚小，先用單檔；領域膨脹後再拆子 index。

## 目錄結構

```text
src/gcore/          — 遊戲核心框架（EnTT + cereal）
  components/       — POD component
  systems/          — 吃 entt::registry& 的自由函式系統
  serialize/        — entt⇄cereal adapter、AllComponents 清單、zone_io、zone_store
  util/             — 共用工具
src/gbind/          — Godot 4 GDExtension facade（薄殼，CMake 第二 target，預設不編）
include/            — 第三方 header-only 庫（entt、cereal）——不要修改
test/               — 測試執行檔（test/src/main.cpp，16 項測試）
notes/              — 設計草稿（非源碼）
work/               — 歷史分析/設計文檔
data/               — 資料目錄（build 時複製到 build/bin/data）
```

## Runtime / Production

| 檔案 | 職責 |
|------|------|
| `src/gcore/zone_key.h` | `ZoneKey`（uint64，打包 ZoneType:16\|x:16\|y:16\|z:16）；`ZoneType{ZONE_ROOT/Invalid, World, Region, Area}`、`zlayer`（Underground/-1、Ground/0、Sky/+1）、`zone_scale` 常數（WORLD_DIM_DEFAULT、WORLD_LAYERS_DEFAULT、REGION_DIM、AREA_DIM）、`make_zone_key` / `world_key` / `region_key` / `area_key` / `parent_of` 換算 |
| `src/gcore/global_manager.h` / `.cpp` | `GlobalManager`：管理 root + 已載入 zones（get/create/load/unload/add_zone_system/tick/save_all/load_root/init_world/world_config/store） |
| `src/gcore/components/*.h` | POD component（zone_meta, position, velocity, area_terrain, blocking, world_config） |
| `src/gcore/systems/movement.h` | movement 系統（吃 `entt::registry&` 的自由函式） |
| `src/gcore/serialize/entt_cereal_archive.h` | EnTT snapshot ⇄ cereal `PortableBinaryArchive` 的 archive adapter |
| `src/gcore/serialize/all_components.h` | `AllComponents` type_list——component 型別清單的**單一來源**；新增 component 必登記 |
| `src/gcore/serialize/zone_io.h` | 單一 zone 的 snapshot save/load（`zone_io::save/load`） |
| `src/gcore/serialize/zone_store.h` | `ZoneStore` 抽象（write/read/has/flush）；`FolderZoneStore`（一 zone 一檔，`dir_/<16碼hex>.bin`、root 為 `dir_/root.bin`），唯一且預設的 store |
| `src/gcore/util/tdarray.hpp` | `tdarray<T>` 2D 陣列模板（已 cereal 化） |
| `src/gcore/util/mydef.h` | metaprogramming macros（仍使用中） |
| `src/gbind/` | Godot GDExtension facade（medp_core / register_types；目前只有接線 smoke-test） |
| `CMakeLists.txt` | 兩個 target：`medp`（SHARED+STATIC）＋可選 `medp_gdext`（`-DMEDP_BUILD_GDEXTENSION=ON`） |

## Tests

| 檔案 | 覆蓋 |
|------|------|
| `test/src/main.cpp` | 全部 16 項測試：zone_key 打包/換算、zone 生命週期、序列化 round-trip、movement 等 |

## Docs / 分析

| 檔案 | 用途 |
|------|------|
| `work/progress_overview.md` | 專案進度白話總覽 |
| `work/design/zone_layers.md` | 核心世界結構設計（zone 分層） |
| `work/design/lifecycle.md` | zone 生命週期設計 |
| `work/architecture/gcore_overview.md` | gcore 架構總覽 |
| `notes/gd/` | Godot 前端草稿 |
| `workflows/common/code-map/html/` | 程式碼 HTML 導覽層（`build.py` 生成，嵌入帶行號原始碼；真相層是原始碼與 CODE_TOUR） |

## 架構不變量（修改前必知）

1. **多 registry / zone 生命週期**：一個 zone = 一個 `entt::registry`，由 `GlobalManager` 管理；root 永久存活、放全局實體，其餘 zones 按需載入/卸載。`ZoneKey` 是全局唯一定址，磁碟 path 由 key 推導、不另存全域清單。
2. **序列化**：EnTT `snapshot` 遍歷 registry，cereal `PortableBinaryArchive` 負責位元格式，透過 `serialize/entt_cereal_archive.h` 橋接。component 型別清單的單一來源是 `serialize/all_components.h` 的 `AllComponents`，save/load 兩邊共用。
3. **元件即資料**：component 盡量是 POD aggregate；entity 之間的參照存 `entt::entity`。system 寫成吃 `entt::registry&` 的自由函式。

## 修改前規則

1. 先判斷要改的功能屬於哪個領域，只讀該領域列出的檔案。
2. 若 CODE_MAP 缺資料或與程式碼衝突，以程式碼為準，立即修正 CODE_MAP。
3. 新增/刪除原始碼檔案，或檔案職責顯著改變時，同步更新本檔。
