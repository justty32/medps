# gcore 逐檔導覽

> 對象:`src/gcore/`(遊戲核心框架,EnTT + cereal)。
> 產出日期:2026-05-30(重構後)。行號對應當時 `main` 的原始碼狀態,改檔後可能微偏。
> 閱讀順序建議:先看 §1 定址層 → §2 生命週期(GlobalManager) → §4 序列化 → 其餘按需。

## 全域心智模型

一個 **zone = 一個 `entt::registry`**。`GlobalManager` 持有一個永久存活的 `root` registry(放全局實體:神祇、家族),其餘 zone 按需從磁碟載入 / 卸載。每個 zone 由一個全域唯一的 **`ZoneKey`(uint64)** 定址,磁碟路徑直接由 key 推導,不另存全域清單。存檔走 **EnTT snapshot 遍歷 component + cereal 二進位格式**,中間靠一個 archive adapter 橋接。

依賴方向(誰 include 誰):

```
zone_key.h ─┬─ components/*
            │     │
            │     └─ serialize/all_components.h ─ zone_io.h
            │              (entt_cereal_archive.h)
            │
            ├─ serialize/zone_store.h
            └─ global_manager.h/.cpp
util/{mydef.h, tdarray.hpp} ── components/area_terrain.h
```

---

## §1 定址層

### `zone_key.h` 〔核心,多介紹〕

整個世界結構的定址基礎。沒有 class,全是型別別名 + 常數 + 自由函式;它是「座標語意的單一來源」。

- **`enum class ZoneType : uint16_t`**(`zone_key.h:8`)
  嚴格三層階層:`Invalid=0`(即 `ZONE_ROOT`,非地圖的全局層)、`World=1`、`Region=2`、`Area=3`。型別值同時 **兼作樹深度** —— 一個 key 的父層型別是隱含的(Area 的父一定是 Region)。
- **`namespace zlayer`**(`zone_key.h:18`)
  垂直層,即 key 的 z 欄位:`Underground=-1`、`Ground=0`、`Sky=+1`。三種地圖型別共用同一組垂直層,**不同 z 視為不同 zone**。
- **`namespace zone_scale`**(`zone_key.h:31`)— 尺度常數,座標運算一律引用這裡、禁止寫死數字。
  - `WORLD_DIM_DEFAULT=200`(`:32`):世界地圖預設邊長(world-tile)。注意這只是**預設值** —— 實際 `world_dim_x/y` 是 per-save runtime 設定,住在 `WorldConfig`(見 §3)。
  - `WORLD_LAYERS_DEFAULT=3`(`:35`):預設垂直層數(z 軸)= Underground / Ground / Sky;實際 `world_dim_z` 同樣住在 `WorldConfig`。
  - `REGION_DIM=15`(`:36`):1 個 world-tile 展開成 15² 個 region-tile。
  - `AREA_DIM=250`(`:37`):1 個 region-tile 展開成 250² 個 area-tile(≈ 一張 Rimworld 地圖)。
  - `MAX_WORLD_DIM`(`:42`)、`valid_world_dim(int)`(`:43`):因 `world_dim` 改成 runtime,溢位保護是 **runtime 前置條件**(`world_dim*REGION_DIM` 須塞進 16-bit);`MAX_WORLD_DIM=2184`(在 `REGION_DIM=15` 下)。
- **`ZoneKey`(=`uint64_t`)與打包**(`zone_key.h:50`)
  位元佈局:`ZoneType:16 | x:16 | y:16 | z:16`。`ZONE_ROOT=0`(`:51`)。
  - `make_zone_key(type,x,y,z)`(`:54`):打包。
  - `zone_key_type / _x / _y / _z`(`:61-64`):解包。
- **座標換算 / 階層**(`zone_key.h:73` 起)。一個 zone 的 (x,y) = 它在**父層全域 tile 網格**裡的座標,父層靠整除回推、z 沿鏈保留。
  - `world_key(z)`(`:73`):World 每個 z 只一張,固定在 (0,0)。
  - `region_key(world_x, world_y, z)`(`:77`):Region 的 (x,y) = 它展開的那個 world-tile。
  - `area_key(world_x, world_y, region_local_x, region_local_y, z)`(`:82`):由父 world-tile + region 內偏移,合成 Area 的全域 region-tile 座標(`world*REGION_DIM + local`)。
  - `parent_of(k)`(`:91`):回推直接父 zone(Area→Region 用整除;Region→World;World→ROOT)。

---

## §2 生命週期管理

### `global_manager.h` / `global_manager.cpp` 〔核心,多介紹〕

協調 root + 已載入 zones 的生死、tick、存讀。

- **`class GlobalManager`**(`global_manager.h:11`)
  - `entt::registry root`(`.h:13`):`ZONE_ROOT`,永久存活、放全局實體。
  - 建構:預設用 `FolderZoneStore("zones")`;也可注入自訂 `ZoneStore`(`.cpp:7-11`)。
  - **查詢**:`get(key)`(`.cpp:13`)取已載入 registry(未載入回 nullptr)。
  - **建立 / 載入 / 卸載**:
    - `create(key, parent)`(`.cpp:24`):新建空 zone,塞一個帶 `ZoneMeta` 的 placeholder entity(避免被 `orphans()` 清掉),`parent` 記在 `ZoneMeta` 裡;冪等。
    - `load(key)`(`.cpp:34`):從 store 反序列化;已載入則回現有。
    - `unload(key)`(`.cpp:45`):序列化寫回 store 後從記憶體丟掉。
  - **系統 / tick**:`add_zone_system(sys)`(`.cpp:86`)註冊一個 `void(entt::registry&)` 的 per-zone 系統(型別 `ZoneSystem`,`.h:34`);`tick()`(`.cpp:90`)對**每個已載入 zone**依註冊順序跑所有系統(root 不在內 —— 它放全局實體,跨 zone 系統另議)。
  - **整局存讀**:`save_all()`(`.cpp:52`)把 root + 所有已載入 zone 寫回 store(不卸載),最後 `flush()`;`load_root()`(`.cpp:59`)開局只載 root,子 zone 留在 store 待 `load()`/`create()` 按需載入。
  - **per-save 世界設定**:`init_world(world_dim_x, world_dim_y, world_dim_z)`(`.cpp:66`)開新遊戲時把 `WorldConfig` 種到 root 單例(前置條件 `valid_world_dim`);`world_config()`(`.cpp:80`)讀 root 上的 `WorldConfig`(未設回預設)。詳見 §3 的 `world_config.h`。`store()`(`.h:66`)取出底層 `ZoneStore&`。
  - 私有:`write_zone`(registry→bytes→store,`.cpp:18`);狀態 `store_`、`loaded_`(key→registry map)、`zone_systems_`。

---

## §3 components/(POD 元件)

慣例:component 盡量是 POD aggregate;entity 間參照存 `entt::entity`;每個型別有 `serialize`(或 `save`/`load`)讓 cereal 帶走。新型別都要登錄到 `all_components.h`。

- **`zone_meta.h` — `struct ZoneMeta`**(`:7`):每個 zone 的 placeholder entity 上的身份 component。存 `self` / `parent` 兩個 ZoneKey。它的存在保證 zone 至少有一個非 orphan entity(否則會被 `zone_io` 的 `orphans()` 清空)。
- **`position.h` — `struct Position`**(`:6`):actor 在 zone 網格內的整數 tile 座標 `{x, y}`。有 `.x/.y` 故同時滿足 `tdarray` 的 `is_coor` concept。
- **`velocity.h` — `struct Velocity`**(`:5`):每 tick 的移動步 `{dx, dy}`。movement 系統的示範 component,真正的回合模型之後再精修。
- **`blocking.h` — `struct Blocking`**(`:6`):站在 tile 上的離散物件(關著的門、巨石、大型生物)的逐 entity 阻擋 `{blocks_move, blocks_sight}`。地形層的可走性在 `AreaTerrain` 的 tile flags;這是疊在地形**之上**的阻擋。
- **`area_terrain.h`**〔較重〕
  - `struct Tile`(`:11`):一格地形 = `{uint16 terrain, uint8 flags}`。`terrain` 是 def id(內容 / 前端負責對映視覺);`flags` 快取模擬相關的可走性(`TILE_WALKABLE`、`TILE_BLOCKS_SIGHT`,`:8-9`),讓 FOV / 尋路不必每格查 terrain def。
  - `struct AreaTerrain`(`:23`):一個 Area zone 的**稠密地形網格**(Rimworld 風 ~250×250),做成 `tdarray<Tile>`。關鍵設計:**掛在單例 "map" entity 上的 component**,藉此走正常的 snapshot/cereal 存檔路徑 —— 不可放 `registry.ctx()`(zone_io 不序列化 ctx)。會動的「things」(actor、物品)是另外的 entity;tile **不是**一格一 entity。
- **`world_config.h` — `struct WorldConfig`**(`:10`):per-save 的全域世界設定,目前有 `world_dim_x` / `world_dim_y`(世界邊長,預設 `WORLD_DIM_DEFAULT=200`)與 `world_dim_z`(垂直層數,預設 `WORLD_LAYERS_DEFAULT=3`)。做成 **root 上的單例 component**(沿用 `AreaTerrain` 同模式),靠 root 的 snapshot 自動進存檔。語意:**world-gen 時決定、寫進存檔、整局不可改**(因為 `world_dim` 被烤進 ZoneKey 座標語意,中途改會讓既有 key 全錯)。需要它的系統吃 `const WorldConfig&`。

---

## §4 serialize/(EnTT ⇄ cereal)

### `all_components.h`

存讀的**單一來源**。一個 `entt::type_list<...>`(`:13`)列出所有要序列化的 component 型別;save / load 兩邊都展開這份清單。**新 component 必須加到這裡。**

### `entt_cereal_archive.h`

EnTT snapshot 的 archive 介面 ⇄ cereal `PortableBinaryArchive` 的 adapter。

- `output_archive`(`:8`)/ `input_archive`(`:19`):兩個薄 functor。EnTT snapshot 用 `operator()` 呼叫 archive;這裡把呼叫轉發給 cereal,並把 `entt::entity` ⇄ 其底層整數型別(`entt_id_t`,`:6`)互轉以求可攜。

### `zone_io.h`

單一 zone 的 snapshot save / load。

- `detail::save_impl` / `load_impl`(`:13` / `:20`):用 variadic + fold expression 展開 `AllComponents`,依序 `snapshot.get<C>` / `snapshot_loader.get<C>`。
- `load_impl` 結尾呼叫 `loader.orphans()`(`:28`):清掉載入後沒有任何 component 的 entity —— **故有「每個新建 zone 必須有一個帶至少一個 component 的 placeholder」的慣例**(見 `ZoneMeta`)。
- 對外 `save/load`:有吃 `std::ostream/istream`(`:33` / `:39`)與吃 `std::filesystem::path`(`:45` / `:52`)兩組多載。

### `zone_store.h`

把 `GlobalManager` 與「zone bytes 存哪、怎麼存」解耦。`zone_io` 管 registry⇄bytes,`ZoneStore` 管 bytes⇄storage。

- **`struct ZoneStore`**(`:15`):抽象基底。純虛 `write/read/has`(`:18-20`);`flush()`(`:24`)預設 no-op(給單檔 / DB 後端的提交點)。
- **`class FolderZoneStore`**(`:28`):唯一且預設的後端 —— 一個目錄下一 zone 一檔。`path(key)`(`:37`)由 key 的 16 進位推檔名(`ZONE_ROOT` → `root.bin`,其餘 → `<16 碼 hex>.bin`)。`write`(`:44`)建目錄後寫檔;`read`(`:50`)不存在回 `nullopt`;`has`(`:58`)查存在。

---

## §5 systems/

### `movement.h` — `systems::movement`(`:11`)

per-zone 系統的範例。簽名 `void(entt::registry&)`,故可註冊進 `GlobalManager::add_zone_system` 並對每個已載入 zone 跑。內容:對每個同時有 `Position` + `Velocity` 的 entity,`p += v` 走一步。系統一律寫成吃 `entt::registry&` 的自由函式。

---

## §6 util/(共用工具)

### `mydef.h`

metaprogramming 巨集(仍使用中)。重點是 `LET_CONCEPT_BE_CHECKABLE(STRUCT_NAME, CONCEPT)`(`:45`)/ `LET_CONCEPT_BE_CHECKABLE_V(CONCEPT)`(`:56`):把一個 concept 包成可在執行邏輯裡查詢的 `CONCEPT_v<T>` 布林值(用兩個重載 + SFINAE 判斷 T 是否滿足 concept)。`tdarray` 用它把 `is_coor` 變成 `is_coor_v`。

### `tdarray.hpp`〔泛型 2D 陣列,多介紹〕

- **`concept is_coor`**(`:11`):接受 `pair<int,int>` / `tuple<int,int>` / 任何有 `.x .y` 的型別,讓座標 API 可同時吃這三種形式;`LET_CONCEPT_BE_CHECKABLE_V`(`:14`)再衍生出 `is_coor_v`。
- **`template<typename T> class tdarray`**(`:16`):以一維 `std::vector<T>`(`:31`)backing 的稠密 2D 陣列,索引 `x*sy + y`。已 cereal 化(`serialize`,`:39`,存 `sx, sy, vec`)。是 `AreaTerrain` 的底層容器。
  - 私有座標拆解:`bx/by`(`:19` / `:25`)從任意 `is_coor` 取 x/y。
  - 尺寸 / 配置:`sx, sy`(`:33`)、`alloc(x,y)`(`:42`)、`clear()`(`:51`)、`usable()/unusable()`(`:40-41`)。
  - 邊界:`out(x,y)` / `out(coord)`(`:52` / `:57`)。
  - 取值族(各有 int 版與 `is_coor` 版):`get`(回參照,不檢查)、`getptr`(越界回 nullptr)、`getval`(回值,可帶 default)、`getref`(`:63-88`)。
  - 寫入:`set(x,y,v)` / `set(coord,v)`(`:129` / `:134`,越界回 true 表失敗)、`set_all(v)`(`:139`)。
  - 走訪(以 concept 約束 callable):`each`(逐格)、`eachb`(callable 回 bool 可提早中止)、`eachxy`(帶 i,j 座標)、`eachxyb`(`:89-128`)。
  - 拷貝:`copy_to()` / `copy_from()`(`:144` / `:155`,trivially-copyable 走 `std::copy` 快路徑)。
  - `operator[]`(`:167`):吃 `is_coor` 走 `getref`,吃整數則回該列起點指標。

---

## 速查表

| 檔案 | 角色 | 主要型別 / 函式 |
|---|---|---|
| `zone_key.h` | 全域定址 + 尺度語意 | `ZoneType`、`zlayer`、`zone_scale`、`ZoneKey`、`make/zone_key_*`、`world/region/area_key`、`parent_of` |
| `global_manager.h/.cpp` | root + zones 生命週期 / tick / 存讀 | `GlobalManager` |
| `components/zone_meta.h` | zone 身份 placeholder | `ZoneMeta` |
| `components/position.h` | 網格座標 | `Position` |
| `components/velocity.h` | 移動步(示範) | `Velocity` |
| `components/blocking.h` | 逐 entity 阻擋 | `Blocking` |
| `components/area_terrain.h` | Area 稠密地形網格(單例 component) | `Tile`、`AreaTerrain` |
| `components/world_config.h` | per-save 世界設定(root 單例) | `WorldConfig` |
| `systems/movement.h` | per-zone 移動系統範例 | `systems::movement` |
| `serialize/all_components.h` | component 清單單一來源 | `AllComponents` |
| `serialize/entt_cereal_archive.h` | EnTT snapshot ⇄ cereal adapter | `output_archive`、`input_archive` |
| `serialize/zone_io.h` | 單一 zone snapshot 存讀 | `zone_io::save/load` |
| `serialize/zone_store.h` | bytes ⇄ storage 抽象 | `ZoneStore`、`FolderZoneStore` |
| `util/mydef.h` | metaprogramming 巨集 | `LET_CONCEPT_BE_CHECKABLE(_V)` |
| `util/tdarray.hpp` | 泛型 2D 稠密陣列 | `is_coor`、`tdarray<T>` |
