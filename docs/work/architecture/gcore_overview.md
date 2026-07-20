# gcore 逐檔導覽

> 對象：`projects/medp/src/gcore/`（遊戲核心框架，EnTT + cereal）。最後更新：2026-07-19。

## 全域心智模型

一個 **zone = 一個 `entt::registry`**。`GlobalManager` 持有一個永久存活的 `root` registry（全局實體），其餘 zone 按需從磁碟載入/卸載。每個 zone 由 `ZoneKey(uint64)` 定址，磁碟路徑直接由 key 推導。存檔走 EnTT snapshot + cereal。

依賴方向：

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

## §1 定址層：zone_key.h

整個世界結構的定址基礎。全部是型別別名 + 常數 + 自由函式。

- **`ZoneType(uint16_t)`**：`Invalid=0(=ZONE_ROOT)`、`World=1`、`Region=2`、`Area=3`。型別值兼作樹深度——parent 型別是隱含的。
- **`zlayer`**：`Underground=-1`、`Ground=0`、`Sky=+1`。三種地圖型別共用。
- **`zone_scale`**：尺度常數，座標運算一律引用此處：
  - `WORLD_DIM_DEFAULT=200`（僅預設值，實際是 per-save runtime 設定）
  - `REGION_DIM=15`、`AREA_DIM=250`
  - `MAX_WORLD_DIM=2184`、`valid_world_dim()`
- **`ZoneKey=uint64_t`**：`ZoneType:16 | x:16 | y:16 | z:16`。`ZONE_ROOT=0`。
- **座標換算**：parent 由整除回推、z 沿鏈保留。

## §2 生命週期：GlobalManager

協調 root + 已載入 zones 的生死、tick、存讀。

- `root`：ZONE_ROOT，永久存活
- 建構：預設 `FolderZoneStore("zones")`，可注入自訂 `ZoneStore`
- `get(key)`：取已載入 registry
- `create(key, parent)`：新建空 zone + 塞 `ZoneMeta` placeholder（冪等）
- `load(key)`：從 store 反序列化
- `unload(key)`：序列化回 store 後清除
- `add_zone_system(sys)`：註冊 `void(entt::registry&)` 的 per-zone 系統
- `tick()`：對每個已載入 zone 依序跑所有系統（root 排除）
- `save_all()`：root + 所有已載入 zone 寫回 store
- `load_root()`：只載 root
- `init_world(wx, wy, wz)`：把 `WorldConfig` 種到 root

## §3 components

- `ZoneMeta`：zone 身份（self/parent ZoneKey），保證 zone 有非 orphan entity
- `Position{x,y}`：tile 座標
- `Velocity{dx,dy}`：每 tick 移動步（示範用）
- `Blocking{blocks_move, blocks_sight}`：逐 entity 阻擋，疊在地形之上
- `AreaTerrain`：稠密地形網格（`tdarray<Tile>`），掛在單例 entity 上走 snapshot
  - `Tile{uint16 terrain, uint8 flags}`：terrain = def id，flags 快取可走性
- `WorldConfig{world_dim_x/y/z}`：per-save 設定，root singleton component

## §4 serialize

- `all_components.h`：**唯一清單**，列出所有要序列化的 component。新 component 必加。
- `entt_cereal_archive.h`：EnTT snapshot ⇄ cereal PortableBinary 的 adapter。`entt::entity` ⇄ `entt_id_t` 互轉以求可攜。
- `zone_io.h`：單一 zone 的 snapshot save/load。`load_impl` 結尾 `loader.orphans()` 清掉無 component 的 entity。
- `zone_store.h`：`ZoneStore`（抽象） + `FolderZoneStore`（預設後端，一 zone 一檔）。`path(key)` 由 16 進位推檔名。

## §5 systems

- `movement.h`：per-zone 系統範例。對所有 `Position + Velocity` entity 走一步。

## §6 util

- `mydef.h`：metaprogramming 巨集（`LET_CONCEPT_BE_CHECKABLE` 等）
- `tdarray.hpp`：泛型 2D 稠密陣列，backed by `std::vector<T>`
  - `is_coor` concept：接受 `pair<int,int>` / `tuple<int,int>` / 有 `.x .y` 的型別
  - 索引公式 `x*sy + y`，已 cereal 化

## 速查表

| 檔案 | 角色 |
|------|------|
| `zone_key.h` | 全域定址 + 尺度語意 |
| `global_manager.h/.cpp` | root+zones 生命週期/tick/存讀 |
| `components/zone_meta.h` | zone 身份 placeholder |
| `components/position.h` | tile 座標 |
| `components/velocity.h` | 移動步（示範） |
| `components/blocking.h` | 逐 entity 阻擋 |
| `components/area_terrain.h` | Area 稠密地形網格 |
| `components/world_config.h` | per-save 世界設定 |
| `systems/movement.h` | per-zone 移動系統範例 |
| `serialize/all_components.h` | component 清單唯一來源 |
| `serialize/entt_cereal_archive.h` | EnTT ⇄ cereal adapter |
| `serialize/zone_io.h` | 單一 zone snapshot 存讀 |
| `serialize/zone_store.h` | bytes⇄storage 抽象 |
| `util/mydef.h` | metaprogramming 巨集 |
| `util/tdarray.hpp` | 泛型 2D 稠密陣列 |
