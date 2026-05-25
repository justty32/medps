# Level 2 — 核心模組職責與架構設計

## 模組地圖

```
gcore
├── [基礎層] BinFSR        — 二進位序列化命名空間
├── [基礎層] tdarray<T>    — 2D 陣列模板（含序列化）
├── [基礎層] mydef.h       — 編譯期巨集工具
├── [框架層] Obj           — 所有遊戲物件的基底
├── [框架層] Scene         — Obj 容器（稀疏陣列 + ID 池）
├── [框架層] Component     — ECS 元件基底（繼承 Obj）
├── [框架層] ComponentManager — ECS 元件管理器（繼承 Obj）
└── [領域層] obj_types/world — BigMap 遊戲物件（TileMap、MapEntity…）
```

---

## 各模組詳細職責

### 1. `BinFSR` — 二進位流讀寫
**位置**: `src/gcore/util/bin_fwr.hpp`

特性：
- 使用 `std::ostringstream` / `std::istringstream` 做緩衝
- 模板特化支援：`T`（POD）、`vector<T>`、`array<T,S>`、`map<Tk,Tv>`、`unordered_map<Tk,Tv>`
- 對複雜物件（如 `BigMap::Tile`）透過 template specialization 手動串接欄位

**關鍵設計**：目前使用 `istringstream`/`ostringstream` 而非直接 `fstream`，
代表序列化輸出為記憶體緩衝，由呼叫方決定是否寫入磁碟。

---

### 2. `tdarray<T>` — 二維陣列
**位置**: `src/gcore/util/tdarray.hpp`

職責：
- 以 `std::vector<T>` 線性儲存，採用 `x*sy + y` 索引公式
- 提供邊界檢查（`out()`、`getptr()` 安全版本）
- 提供迭代器方法：`each(f)`、`eachxy(f)`、`eachb(f)` 等
- 支援 `is_coor` concept：接受 `{x,y}`、`pair<int,int>`、`tuple<int,int>` 的座標
- 有 `Save()`/`Load()` 直接對接 BinFSR

**設計細節**：採用 `x` 為主索引（row-major by x），`y` 為次索引，即同 x 的格子連續儲存。

---

### 3. `Obj` — 物件基底
**位置**: `src/gcore/util/obj.hpp`, `obj.cpp`

職責：
- 所有遊戲物件的抽象基底
- 持有 `id`（在場景中的唯一索引）、`scene*`（所屬場景指標）
- 提供 `Save()`/`Load()` 虛函數介面（預設僅序列化 `id`）
- 靜態工廠：`_g_default_constructors`（type_id → `std::function<Obj*()>`）

**型別系統**：
- `OBJ_INIT_DEF(TYPE_ID, CLASS, BASE_CLASS)` 巨集：在衍生類別中宣告 `GetTypeID()` (consteval)
- `OBJ_TYPE_LIST_REGISTER_CLASS(CLASS)` 巨集：向工廠登錄無參建構子
- 型別 ID 分配（目前）：
  - `0` → `Obj`
  - `1` → `Scene`
  - `11` → `Component`
  - `12` → `ComponentManager`
  - `200000+` → `BigMap::*`

---

### 4. `Scene` — 場景容器
**位置**: `src/gcore/util/scene.hpp`, `scene.cpp`

職責：
- 持有 `std::vector<Obj*> objs`（稀疏，刪除後留 `nullptr`）
- 持有 `std::list<int> empty_id_pool`：回收空位 ID 供重用
- 工廠方法：`NewObj(type_id)` 透過 `_g_default_constructors` 建立物件並加入場景
- 物件刪除：`DeleteObj(id)` 釋放記憶體，將 id 推入 pool

**設計細節**：ID 即 `objs[]` 的下標，刪除後索引保留但指標為 `nullptr`。
這允許 `Obj*` 之間透過 id 互相引用而不需要指標。

---

### 5. `Component` / `ComponentManager` — ECS 元件系統
**位置**: `src/gcore/util/component.hpp`, `component.cpp`

職責：
- `Component`：繼承自 `Obj`，作為掛載到 `ComponentManager` 的功能單元
- `ComponentManager`：繼承自 `Obj`，管理 `map<int, Component*> comps`（type_id → comp）
- 提供型別安全的 get/add/new/remove/delete 操作，支援模板 `T` 與 raw type_id 兩種介面

**注意**：目前 `ComponentManager` 的 `OBJ_INIT_DEF` 第二個參數有 bug，
傳入的是 `Component`（應為 `ComponentManager`）：`component.hpp:15`。

**設計意圖**：`ComponentManager` 本身也是 `Obj`，
因此可以被 `Scene` 管理，也可作為遊戲實體（如城市、單位）的元件容器。

---

### 6. `obj_types/world` — 遊戲領域層（BigMap）
**位置**: `src/gcore/obj_types/world.h`, `world.cpp`

目前實作（完成）：
- `BigMap::TileMap`：`Component` + `tdarray<BigMap::Tile>`，代表大地圖的圖塊層
- `BigMap::MapEntity`：有位置的地圖實體（`Component`）

草稿定義（world.h 下半部，有重複名稱衝突，尚待整理）：
- `BigMapTile`：含 terrain、biome、resource、buildable、move_attr 的完整圖塊
- `Entity` / `Positioned` / `Settlement` / `City` / `Village`
- `Military` / `Unit` / `Hero` / `ArmyStack` / `UnitStack`
- `Map` / `Soldier` / `SoldierType`

---

## 模組耦合圖

```
BinFSR (無依賴)
    ↑
tdarray<T> (依賴 BinFSR)
    ↑
Obj (依賴 BinFSR)
    ↑
Scene (依賴 Obj)
Component / ComponentManager (依賴 Obj)
    ↑
BigMap::TileMap (依賴 Component, tdarray)
BigMap::MapEntity (依賴 Component)
```

---

## 已知問題 / 技術債

| 位置 | 問題 |
|---|---|
| `component.hpp:15` | `ComponentManager::OBJ_INIT_DEF` 第二個參數錯填為 `Component` |
| `world.h:51-53` | 多個 `Tile`、`Map`、`Entity` 重複宣告（草稿雜訊，編譯時會衝突） |
| `obj_types_list.cpp:6` | `_init_all_subtypes()` 只登錄了 `Obj` 和 `Scene`，`Component`、`TileMap` 等未登錄 |
| `scene.hpp:21` | `NewObj<T>` requires 條件顛倒（`is_base_of_v<T, Obj>` 應為 `is_base_of_v<Obj, T>`） |
| `test/src/main.cpp` | 測試幾乎空白 |

---

## 設計草稿的遊戲規模（notes/）

`notes/strategy_game_system.js` 揭示了預期的遊戲複雜度：

| 系統 | 主要結構 |
|---|---|
| 地圖 | Tile（地形、資源、改良設施、可見度）、BigMap |
| 軍事 | Unit、UnitStack、Hero |
| 城市 | City（人口、生產、建築、貿易） |
| 派系 | Faction（科技、外交、政治） |
| 文明 | Civilization、Culture |
| 環境 | Climate、Religion |
| 經濟 | TradeRoute、Resource、Technology |
| 事件 | Event（觸發條件、效果、選擇） |
| 深層（deep_systems.js）| NomadClan、組織、貴族家族 |
| 魔法 | rpg_magic_elements.js |

世界觀（`notes/a.txt`）是一個奇幻設定：
太古神祇、古龍（能量主宰）、泰坦（物質主宰）、起源野獸、凡人種族……
這是一個**奇幻 4X 策略遊戲**，而非純歷史策略。
