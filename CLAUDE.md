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
src/gcore/          — 遊戲核心框架
  util/             — 基礎工具（Obj, Scene, Component, BinFSR, tdarray）
  obj_types/        — 遊戲領域物件（BigMap 相關）
include/            — 第三方 header-only 庫
test/               — 測試執行檔
notes/              — 設計草稿（非源碼）
work/               — 分析工作空間（Claude Code 輸出）
```

## 核心模組速查

| 檔案 | 說明 |
|---|---|
| `src/gcore/util/obj.hpp` | `Obj` 基底類別 + 型別工廠系統 |
| `src/gcore/util/scene.hpp` | `Scene` 物件容器（稀疏陣列 + ID 池） |
| `src/gcore/util/component.hpp` | ECS `Component` / `ComponentManager` |
| `src/gcore/util/bin_fwr.hpp` | `BinFSR` 二進位序列化 |
| `src/gcore/util/tdarray.hpp` | `tdarray<T>` 2D 陣列模板 |
| `src/gcore/obj_types/world.h` | `BigMap::TileMap` 等遊戲物件 |
| `src/gcore/obj_types_list.cpp` | 型別登錄表（`_init_all_subtypes`） |

## 構建

```bash
cmake -S . -B build && cmake --build build
# 輸出: build/bin/medp.windows.debug.64.dll
```

## 關鍵設計模式

1. **型別ID系統**：每個 `Obj` 子類別有唯一 `consteval int GetTypeID()`，
   透過 `OBJ_INIT_DEF` 巨集宣告，透過 `OBJ_TYPE_LIST_REGISTER_CLASS` 登錄工廠。
2. **序列化**：所有 `Obj` 實作 `Save(BinFSR::ostream_t&)` / `Load(BinFSR::istream_t&)`，
   子類呼叫父類版本後再序列化自己的欄位。
3. **ECS**：`ComponentManager`（本身也是 Obj）持有 `map<type_id, Component*>`；
   遊戲實體透過繼承 `ComponentManager` 或持有其指標來組合功能。

## 分析工作空間

詳細分析留存於 `work/`：
- `work/architecture/level1.md` — 初始探索（目錄結構、技術棧、入口點）
- `work/architecture/level2.md` — 核心模組職責與耦合關係
- `work/session_log.md` — 操作日誌

## 開發慣例

- 所有回覆與留檔使用**繁體中文**
- 程式碼引用必須附**路徑:行號**
- 分析內容自動同步寫入 `work/` 對應子資料夾
