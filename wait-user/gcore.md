# 待過目 — gcore 核心（`projects/medp/`）

母檔入口：[WAIT_USER.md](../WAIT_USER.md)。由新到舊排列；完成即從母檔與本檔一併移除。

---

## actor 基礎設施（ECS 組合版）

新增 `gcore/common/components/{name,owner,location,unit}.h` ＋ `gcore/common/actor.h`，登記進 `serialize/all_components.h`（**存檔格式已變，舊存檔請刪**），新測試 `actor_defs_and_spawn`／`actor_roundtrip`。**建置綠、測試 21/21 綠**（含 enum→def 改版；exe 一度被 Windows 應用程式控制擋、重試後可跑）。

模型：actor＝共用身分元件（`Name`＋`Owner`），`Location`／`Unit` 為兩大家族 tag；種類是掛在 **root** 的 def 實體（`LocationKind{id}`／`UnitKind{id}`＋`Name`），actor 以穩定 id 參照——`define_*` 吃 `Zone&` 並 fail-fast 檢查 `id == ZONE_ROOT`。

**看點**：

1. 是否認同「def 一律住 root、actor 以穩定 id 參照」這條線？
2. def 現手動配 id，將來 Ruleset 載入＋登錄機制發號（比照 ZoneManager）。
3. 放置（`Position`）刻意沒進 spawn；`Owner.faction`／`Location.kind` 都走穩定 `uint64` id 不用 entt handle（跨 registry 安全）。

---

## World 專屬物集中進 `gcore/world/`

你把 World 專屬物搬進新目錄 `gcore/world/`（world.{h,cpp} ＋ components/ ＋ systems/ 從 `zone/`、`components/`、`systems/` 集中而來，`zone/` 只留通用框架），我補完搬家後斷掉的 6 處 include 接線：`serialize/all_components.h:3-4`（→`../world/components/`）、`world/world.h:2`（→`../zone/zone.h`）、`world/systems/movement.h:5`（→`../../zone/zone.h`）、`zone/zone.cpp:2`（→`../world/world.h`）、`tests/src/main.cpp:2,5-7`（→`gcore/world/...`）。

**驗證**：medp/medp_static 建置綠＋測試 19/19 綠（當時基準）。CODE_MAP 目錄樹與 Runtime 表、CODE_TOUR 第 3/6 站路徑已同步。

**看點**：分層切分（zone＝通用框架／world＝World 子類專屬）是否合你意？

另：`docs/references/how_to_add_component_and_system.md` 等歷史教學仍寫舊 `gcore/components|systems` 路徑，按 AGENTS 慣例先保留未搬。

---

## worldgen 抽成獨立模組並改純簽章

經你確認「整組搬＋薄殼」＋不依賴 World 型別：

- `gcore/world/world_gen.h`：`WorldGenParams`＋`TERRAIN_*` 常數＋`world_gen::generate(const WorldGenParams&, tdarray<Tile>&, uint64_t zone_id=0)`（include `util/tdarray.hpp`＋`zone/tile.h`，不再認識 World）。
- `gcore/world/world_gen.cpp`：`Field`（libtcod FBM）＋實作吃 `(gen, grid, zone_id)`，**不再 include world.h**，內部 `grid.alloc` 整層重寫。
- `gcore/world/world.cpp:6` `World::generate()` 薄殼挑 `layers[0]`：`world_gen::generate(gen, layers[0], id)`。

**驗證**：建置綠＋測試 19/19 綠（當時基準）。

註：worldgen 現為單層（`layers[0]`）作業，挑哪層由 World 薄殼決定，world_gen 本身層數無關。

---

## World 子類落地

[spec＋落地備註](../workflows/specs/world-zone-subclass-design.md)，三題經你拍板後直接動工；當時測試 15→**19 綠**。

1. `projects/medp/src/gcore/zone/zone.h:17,46-71`：`ZoneKind`＋virtual dtor＋extra 掛鉤＋`make_zone` 工廠＋`zone_cast<T>`（Zone 從此不可移動，只經 unique_ptr 持有）。
2. `projects/medp/src/gcore/zone/world.h`／`.cpp`：`World : Zone`＋`WorldGenParams`＋`generate()`（libtcod FBM 高度→水陸→biome，同 seed 同圖）。※ 檔案後來搬到 `gcore/world/`，見上一則。
3. `projects/medp/src/gcore/serialize/zone_io.h:25-49`：檔頭 kind tag、load 改回傳 `unique_ptr<Zone>`（**存檔格式已變，舊存檔目錄請手動刪除**）。
4. `projects/medp/CMakeLists.txt:50-69`：libtcod 2.2.2 headless FetchContent（首次 configure 需網路）。

**看點**：generate 的 biome 佔位分類（world.cpp:44-53）是否符合你要的第一版粒度？

另：CODE_MAP Runtime 表原停在重構前（還列著 zone_key／global_manager／zone_store），已按現行程式碼修正。

---

## 測試套件整套重寫

`projects/tests/src/main.cpp` — 對應新核心整套重寫，當時 **15/15 綠**，取代舊 16 項基準（現已成長為 21 項）。

**看點**：case 總表在檔尾 `main()`；`test_open_protocol_guards`／`test_load_id_mismatch_throws` 是新增的損毀防護驗證；`test_tick_all_zones` 明文固定「root 也參加 tick」的新語意（舊架構 root 不參加）。

---

## zone 定址＋生命週期落地

[spec 拍板結果表](../workflows/specs/zone-addressing-lifecycle-design.md)，已逐條經你裁定。

- `projects/medp/src/gcore/zone/zone_manager.h:38-42`：`create_child` 單點配號（`create` 收 private）。
- `zone_manager.cpp:9-31`：開檔協定（manifest 還原 next_id、root.bin 必讀回、無 manifest 有 .bin → throw）。
- `zone_manager.cpp:79-89`：manifest 原子寫。
- `zone_manager.cpp:60-64`＋`:106-109`：destroy 刪檔＋load 驗 id。

**驗證**：建置綠＋新測試套件 15/15（當時基準）。

---

## P0 落地：Position 補 z、move_by 收口

[計畫＋執行結果](../workflows/plans/save-format-position-z.md)；Task 1 存檔版本欄位經你裁定放棄。

1. `projects/medp/src/gcore/components/position.h:7-14`：Position 補 `int z`（x/y/z 全 int，照你的裁定）。※ 檔案後來搬到 `gcore/world/components/`。
2. `projects/medp/src/gcore/zone/zone.h:36`：`Zone::layers` 鍵 `int16_t`→`int`。
3. `projects/medp/src/gcore/systems/movement.h:12,20`：新增 `move_by` 收口、movement 改吃 `Zone&`（簽章即 ZoneSystem，可直接註冊）。※ 檔案後來搬到 `gcore/world/systems/`。

**驗證**：medp/medp_static 建置綠＋movement.h 臨時 TU 語法編譯過（後續新測試套件亦覆蓋）。

---

## tdarray 補註解

`projects/medp/src/gcore/util/tdarray.hpp:10-27` — 補上全檔僅缺的註解：檔頭三條使用慣例（true＝失敗、is_coor 座標、get/getptr/getval 差異），加上各函式家族短註。純註解、零邏輯變更，測試全綠。
