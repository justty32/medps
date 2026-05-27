# CLAUDE.md — medps

## 專案概述

**medps** 是一個 **C++20 遊戲後端函式庫**（`medp`），為一款**奇幻 4X 策略遊戲**的核心引擎。
預計以 Godot 4 GDExtension 作為前端。目前正在**重寫**階段。

高階架構：
- 世界觀：奇幻設定（神祇、古龍、泰坦、凡人種族）
- 遊戲類型：類文明（Civilization-like）4X，含深層模擬（游牧、宗教、貴族家族）
- 後端：C++ 函式庫（`src/gcore/`），輸出 DLL + 靜態庫
- 前端：Godot 4 GDExtension（規劃中，`notes/gd/` 有草稿）

## 目錄結構

```
src/gcore/          — 遊戲核心框架（EnTT + cereal）
  zone_key.h        — ZoneKey 打包/解包（uint64：ZoneType:16|x:16|y:16|z:16）
  global_manager.*  — GlobalManager：root + 已載入 zones 的生命週期與 tick
  components/        — POD component（zone_meta, child_zone_summary, cross_zone_ref, position, owner, velocity）
  systems/           — 吃 entt::registry& 的自由函式系統（movement.h）
  serialize/         — entt⇄cereal adapter、AllComponents 清單、zone_io、zone_store
  util/              — 共用工具（mydef.h metaprogramming macros、tdarray.hpp 2D 陣列）
src/gbind/          — Godot 4 GDExtension facade（薄殼，CMake 第二 target）
include/            — 第三方 header-only 庫（entt、cereal）
test/               — 測試執行檔（test/src/main.cpp）
notes/              — 設計草稿（非源碼）
work/               — 分析工作空間（Claude Code 輸出）
```

## 核心模組速查

| 檔案 | 說明 |
|---|---|
| `src/gcore/zone_key.h` | `ZoneKey`（uint64，打包 ZoneType:16\|x:16\|y:16\|z:16）；`ZoneType{World,Region,Area}`、`zlayer`、`zone_scale` 常數、`parent_of` / `region_key` / `area_key` 換算 |
| `src/gcore/chunk_key.h` | `chunk_key_of`：把邏輯 zone 對映到儲存用的 chunk（Region 5×5、Area 1:1） |
| `src/gcore/global_manager.h` / `.cpp` | `GlobalManager`：管理 root + 已載入 zones（create/load/unload/save_all/load_root/resolve/children/tick） |
| `src/gcore/serialize/entt_cereal_archive.h` | EnTT snapshot ⇄ cereal `PortableBinaryArchive` 的 archive adapter |
| `src/gcore/serialize/all_components.h` | `AllComponents` type_list——component 型別清單的**單一來源** |
| `src/gcore/serialize/zone_io.h` | 單一 zone 的 snapshot save/load |
| `src/gcore/serialize/zone_store.h` | `GlobalManager` ↔ 磁碟儲存的抽象（`FolderZoneStore` 一 zone 一檔） |
| `src/gcore/serialize/chunked_zone_store.h` | `ChunkedFolderZoneStore`：多個 zone blob 打包進一個 chunk 檔（預設 store，方案 B） |
| `src/gcore/components/*.h` | POD component（zone_meta, child_zone_summary, cross_zone_ref, position, velocity, area_terrain, blocking） |
| `src/gcore/systems/movement.h` | movement 系統（吃 `entt::registry&` 的自由函式） |
| `src/gcore/util/tdarray.hpp` | `tdarray<T>` 2D 陣列模板（已 cereal 化） |
| `src/gcore/util/mydef.h` | metaprogramming macros（仍使用中） |

## 構建

```bash
cmake -S . -B build && cmake --build build
# 輸出: build/bin/medp.windows.debug.64.dll
```

## 關鍵設計模式

1. **多 registry / zone streaming**：一個 zone = 一個 `entt::registry`，由 `GlobalManager` 管理；
   root 永久存活、放全局實體，其餘 zones 按需載入 / 卸載。`ZoneKey` 是全局唯一定址，
   磁碟 path 由 key 推導、不另存全域清單。
2. **序列化**：EnTT `snapshot` 遍歷 registry，cereal `PortableBinaryArchive` 負責位元格式，
   兩者透過 `serialize/entt_cereal_archive.h` 橋接。component 型別清單的**單一來源**是
   `serialize/all_components.h` 的 `AllComponents`，save / load 兩邊共用。
3. **元件即資料**：component 盡量是 POD aggregate；entity 之間的參照存 `entt::entity`，
   跨 zone 參照存 `CrossZoneRef`。system 寫成吃 `entt::registry&` 的自由函式。

## 分析工作空間

詳細分析留存於 `work/`：
- `work/design/zone_layers.md` — 核心世界結構設計（zone 分層 / streaming）
- `work/session_log.md` — 操作日誌

## 開發慣例

- 所有回覆與留檔使用**繁體中文**
- 程式碼引用必須附**路徑:行號**
- 分析內容自動同步寫入 `work/` 對應子資料夾
