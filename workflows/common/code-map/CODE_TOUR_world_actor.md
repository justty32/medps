# CODE_TOUR（下冊）— World 子類與 actor 身分層

[CODE_TOUR.md](CODE_TOUR.md) 的下冊：接續第 2 站之後、第 6 站之前讀（見母檔的閱讀路徑總覽）。
本冊涵蓋第 3～5 站：`common/`（跨 zone 共用身分層）與 `world/`（World 子類專屬物）。

## 第 3 站 `projects/medp/src/gcore/common/`（5 檔，~180 行）— actor 身分層

`common/` 是「可用於多數 Zone 子類的組件」（`common/README.md:3`）。actor（地點/部隊的共同
基底）在 ECS 裡**不是 C++ 父類，是一組共用元件**（`actor.h:12-23`）：

- 身分 = `Name{value}`（`components/name.h:7`）＋`Owner{faction}`（`components/owner.h:8`，
  0=無主/中立）。
- 家族 = `Location{kind}`（`components/location.h:23`）或 `Unit{kind}`（`components/unit.h:21`），
  互斥，一個 actor 只屬一族。
- 放置 = `Position`（`world/components/position.h`，見第 4 站）——**刻意與身分分離**，
  spawn 不掛，需要時另掛。

種類（kind）不是 enum，是**資料驅動的 def**：`LocationKind{id}`/`UnitKind{id}`
（`components/location.h:12`、`components/unit.h:10`）＋`Name`，掛在 **root** 的定義實體上；
actor 以穩定 `uint64_t` id 參照 def（跨 registry 安全，比照 zone id / `Owner.faction`）。

`actor.h` 的工廠函式：

- `define_location`/`define_unit`（`actor.h:40,49`）：在 root 建 def；`detail::require_root`
  （`actor.h:32`）fail-fast 檢查 `Zone& == ZONE_ROOT`，傳非 root 進來直接 throw。
- `find_location_def`/`find_unit_def`（`actor.h:59,65`）：依 id 在 root 線性掃找 def；
  找不到回 `entt::null`。def 數量小，之後量大再上索引（比照 zone 的 unordered_map）。
- `spawn_location`/`spawn_unit`（`actor.h:74,84`）：在某 zone 的 registry 建 actor
  （`Name`＋`Owner`＋家族 tag{kind=def id}）。

讀完該能回答：actor 的「基底」在這個 codebase 裡是什麼形式？為什麼 kind 用 def 實體而不是 enum？

## 第 4 站 `projects/medp/src/gcore/world/components/` + `world/systems/`（3 檔，~55 行）— World 專屬積木與 system 範本

World 子類專屬的 POD component（跟 common/ 的身分層分開放，因為不是所有 Zone 子類都要）：

- `Position{x,y,z}`（`world/components/position.h:7`）：zone 內 grid 座標，z 即
  `Zone::layers` 的鍵；有 `.x/.y` 故滿足 tdarray 的 `is_coor`（見第 1 站）。
- `Velocity{dx,dy}`（`world/components/velocity.h:5`）：每 tick 移動步（示範用）。
- **鐵律：新增 component 必須同步登記 `serialize/all_components.h` 的 `AllComponents`**
  （見第 6 站）。

`world/systems/movement.h`（26 行）是所有未來 system 的形狀範本：

- `move_by`（`movement.h:12`）：位置變更的**唯一入口**，直改 Position 不走這裡視為違規；
  將來 tile flag 檢查（可走性）、空間索引維護都掛在這口上。
- `movement`（`movement.h:20`）：`view<Position, Velocity>` 遍歷，交給 `move_by`；
  簽章即 `ZoneSystem`（`void(Zone&)`），可直接 `zm.add_zone_system(systems::movement)`。

讀完該能回答：要新增一個 system，需要改 ZoneManager 嗎？（不用——寫自由函式，外部
`add_zone_system` 註冊即可。）

## 第 5 站 `projects/medp/src/gcore/world/world.{h,cpp}` + `world_gen.{h,cpp}`（~95 行）— World 子類與 worldgen

`World : Zone`（`world.h:8`）是**第一個 Zone 子類**（三層願景的最上層，時間模型純回合、
tick 分派未動工）：

- `WorldGenParams gen`（`world.h:9`）隨 `save_extra`/`load_extra`（`world.h:14-15`）序列化——
  存檔即記住生成配方，讀回後可用同一組參數重生成同一張圖。
- `generate()`（`world.h:18`，實作 `world.cpp:5`）是**薄殼**：挑 `layers[0]`
  轉呼 `world_gen::generate(gen, layers[0], id)`。
- 只放 worldgen 與世界尺度資料；全局實體仍歸 root，不把全局邏輯堆進 World。

`world_gen` 是純函式模組（`world_gen.h:29-36`），**不認識 World 型別**，只認「配方＋一張
grid」：

- `WorldGenParams`（`world_gen.h:18`）：`width/height`（願景尺度 200×200）、`seed`、
  `sea_level`（水陸閾值）、`noise_scale`、`octaves`（FBM 疊代層數）。
- `world_gen::generate(const WorldGenParams&, tdarray<Tile>&, uint64_t zone_id=0)`
  （`world_gen.h:34`，實作 `world_gen.cpp:30`）：libtcod 2.2.2 headless 的 FBM 高度圖
  （`world_gen.cpp:37`）→ `sea_level` 切水陸（`world_gen.cpp:44`）→ 溫/濕度場分 biome
  （`world_gen.cpp:48-50`）。同 seed 必產同圖（RNG 用 Mersenne Twister 鎖 seed，決定性）。
  尺寸非正 → throw；`zone_id` 僅供錯誤訊息定位，不影響生成結果。
- biome 佔位 terrain 常數在 `world_gen.h:11-14`（`TERRAIN_OCEAN/TUNDRA/GRASSLAND/FOREST`）；
  將來 Ruleset 的 terrain def 表落地時整組搬遷，屆時存檔照政策作廢重生。

讀完該能回答：`world_gen::generate` 為什麼不依賴 `World` 型別？同一組 `WorldGenParams`
兩次呼叫會得到一樣的地圖嗎？

---

讀完本冊回母檔第 6 站（`serialize/`）繼續。
