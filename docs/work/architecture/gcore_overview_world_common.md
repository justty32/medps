# gcore 逐檔導覽 —— world/ ＋ common/

> 分層導覽之一，回總覽見 [gcore_overview.md](gcore_overview.md)。最後更新：2026-08-12。
> 對象：`projects/medp/src/gcore/world/`（第一個 Zone 子類 World 的專屬物）、
> `projects/medp/src/gcore/common/`（跨 zone 共用的 actor 身分層）。
> 這兩層落地經過（含 commit）見 [world_actor_game.md](world_actor_game.md)；本檔只講現狀。

## world/ —— World 子類與 worldgen

- **`world.h`**：`World : Zone`（`world.h:8-19`）是第一個 Zone 子類，只放 worldgen 與世界
  尺度資料；全局實體仍歸 root，不把全局邏輯堆進來（`world.h:6` 註解）。
  `gen`（`WorldGenParams`，`world.h:9`）隨 `save_extra`/`load_extra` 序列化
  （`world.h:14-15`）——存檔即記住生成配方，讀回後可用同一組參數重生成同一張圖。
  `static constexpr KIND = ZoneKind::World`＋`kind()` override（`world.h:11-12`）。
- **`world.cpp`**：`World::generate()`（`world.cpp:5`）是薄殼，委派 `world_gen::generate`
  並挑 `layers[0]`（目前世界圖是單層），把自己的 `id` 傳去供錯誤定位。
- **`world_gen.h`**：`WorldGenParams`（`world_gen.h:18-27`，width/height/seed/sea_level/
  noise_scale/octaves，全部有預設值）＋terrain 常數（`TERRAIN_OCEAN`/`TUNDRA`/
  `GRASSLAND`/`FOREST`，`world_gen.h:11-14`，0 保留給「未生成」）。
  `world_gen::generate(const WorldGenParams&, tdarray<Tile>&, uint64_t zone_id=0)`
  （`world_gen.h:34`）是**純函式**，只認「配方＋一張 tile 格網」，**不認識 `World`
  型別**——依賴方向是 `World::generate()` 依賴 `world_gen`，反過來不成立。
- **`world_gen.cpp`**：演算法走 libtcod 2.2.2 headless（Mersenne Twister 鎖 seed 的
  FBM noise，`world_gen.cpp:10-26` 的 `Field` helper）：高度場 → `sea_level`
  （`world_gen.cpp:44`）切水陸 → 溫/濕度場分 biome（`world_gen.cpp:48-51`），陸地設
  `TILE_WALKABLE`。同 seed 同圖（決定性）；尺寸非正 → throw（`world_gen.cpp:31-34`）。
  libtcod 由 CMake FetchContent 鎖版，首次 configure 需網路。
- **`components/position.h`**：`Position{ int x, y, z }`（`position.h:7-14`），zone 內
  grid 座標，z 即 `Zone::layers` 的鍵。刻意與 actor 身分分離——spawn 不掛，由放置流程
  另掛（見下 common/actor.h）。
- **`components/velocity.h`**：`Velocity{ int dx, dy }`（`velocity.h:5-11`），每 tick
  移動步（示範用元件）。
- **`systems/movement.h`**：`move_by(Zone&, entity, dx, dy)`（`movement.h:12-16`）是
  **位置變更唯一入口**（將來 tile flag 檢查、空間索引維護掛此）；`movement(Zone&)`
  （`movement.h:20-24`）是 system 形狀範本——自由函式、簽章即 `ZoneSystem`，可直接
  `zm.add_zone_system(systems::movement)`。

## common/ —— actor 身分層

模型是 **ECS 組合，不是 C++ 繼承**（`actor.h:15-18` 註解）：

- 身分＝`Name`（`components/name.h:7-12`，顯示名）＋`Owner`
  （`components/owner.h:8-13`，`faction` id，0＝無主/中立）。
- 家族＝`Location{kind}`（`components/location.h:23-28`）或`Unit{kind}`
  （`components/unit.h:21-26`），互斥，一個 actor 只屬一族。
- 放置＝`Position`（見上 world/），刻意與身分分離，spawn 不含。

種類（kind）是**資料驅動的 def**，不是寫死的 enum：def 是住在 **root zone** 的實體
（`LocationKind{id}`／`UnitKind{id}`＋`Name`），actor 以穩定 `uint64_t` id 參照它——跨
registry 安全，比照 zone id／`Owner.faction`，不用 entt handle（`actor.h:20-23`）。

- **`define_location`/`define_unit`**（`actor.h:40-55`）建 def 實體（`LocationKind`／
  `UnitKind`＋`Name`）；`detail::require_root`（`actor.h:32-36`）fail-fast 檢查
  `z.id == ZONE_ROOT`，傳非 root 進來直接 throw，不靜默寫到別的 zone。
- **`find_location_def`/`find_unit_def`**（`actor.h:59-69`）依 id 線性掃 root
  （def 數量小，需要時再上索引，比照 zone 的 unordered_map）。
- **`spawn_location`/`spawn_unit`**（`actor.h:74-91`）在任一 zone 的 registry 建實際
  actor 實體：`Name`＋`Owner`＋`Location{kind}`或`Unit{kind}`，`kind` 是某 def 的 id。
- `LocationKind`/`UnitKind` 的 id 目前**手動配置**；將來 Ruleset 落地時由檔案載入＋
  登錄機制發號（比照 `ZoneManager` 配 zone id）。

### 已登記進 AllComponents

`Position`、`Velocity`、`Name`、`Owner`、`Location`、`Unit`、`LocationKind`、`UnitKind`
全數列在 `serialize/all_components.h:12-21`——這是序列化唯一來源，細節見
[zone+serialize 導覽](gcore_overview_zone_serialize.md) 的「serialize/」節。

## 速查表

| 檔案 | 角色 |
|---|---|
| `world/world.h/.cpp` | World : Zone 子類，gen 序列化＋generate() 薄殼 |
| `world/world_gen.h/.cpp` | 純函式 worldgen（libtcod FBM），不依賴 World |
| `world/components/position.h` | zone 內 grid 座標（x,y,z） |
| `world/components/velocity.h` | 移動步（示範） |
| `world/systems/movement.h` | move_by 收口＋movement 系統範本 |
| `common/actor.h` | actor 工廠：define/find/spawn |
| `common/components/name.h` | 顯示名稱 |
| `common/components/owner.h` | 陣營歸屬（faction id） |
| `common/components/location.h` | 地點家族 tag＋LocationKind def |
| `common/components/unit.h` | 部隊家族 tag＋UnitKind def |
