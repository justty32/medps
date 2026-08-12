# World：第一個 Zone 子類 — 設計本體：拍板結果、背景與繼承機制

> 子檔（拆分於 2026-08-12，原內容為 [world-zone-subclass-design.md](world-zone-subclass-design.md) §1-§5）。狀態：**已拍板並落地（2026-07-22）**。
> 導覽：[← 回母檔索引](world-zone-subclass-design.md) ｜ 下一篇：[測試、落地順序與執行結果](world-zone-subclass-design-execution.md)

## 1. 拍板結果（權威）

| 題目 | 裁定 |
|------|------|
| World 第一版範圍 | **worldgen**：seed＋生成參數，`generate()` 產出 200×200 的 `layers[0]`（高度→水陸→biome 寫進 `Tile::terrain`）。時間模型掛鉤、Region/Area 子類不在本輪 |
| 演算法來源 | **libtcod headless**：CMake 關 `LIBTCOD_SDL3` 只取演算法半邊（noise/heightmap/RNG）；官方 `samples/worldgen/` 為 C++ 參考實作 |
| 子類型別存檔還原 | **type tag＋工廠**：zone 檔頭寫 `uint8` kind，load 先讀 tag 再 new 對應子類；子類專屬資料走 virtual `save_extra`/`load_extra`。延續 zone_io 手寫風格，不用 cereal polymorphic |
| 格式變更政策 | 沿用[既有拍板](zone-addressing-lifecycle-design.md)：不帶 version 欄位，格式變更＝手動刪存檔目錄 |

## 2. 設計背景：與既有拍板的關係

[zone_layers.md](../../docs/work/design/zone_layers.md) 實作現況節原寫「World/Region/Area 是第幾層由 parent 深度＋玩法層資料表達」——本輪裁定改走**子類**路線：層的「行為與專屬資料」由 C++ 型別表達（World 有 worldgen；將來 Region/Area 各有自己的生成器與時間模型），層級鏈仍活在 `Zone::parent`。zone id 仍是零語意 uint64，kind 不進 id。落地時同步修訂 zone_layers.md。

tome4 報告兩條背書與警告仍有效：「大地圖也只是一個 zone」（World 走同一套 ZoneManager 機制，不建平行系統）；「World 層刻意做薄」（World 只放 worldgen 與世界尺度資料，抵抗把全局邏輯堆進去——全局實體仍歸 root）。

## 3. 繼承機制

### 3.1 Zone 改造（[zone.h](../../projects/medp/src/gcore/zone/zone.h)）

- 加 **virtual destructor**——現行 `unique_ptr<Zone>` 持有子類即 UB（[zone_manager.h:91](../../projects/medp/src/gcore/zone/zone_manager.h)）。
- 新增 `enum class ZoneKind : uint8_t { Plain = 0, World = 1 }`（將來 Region/Area 續編；root 與現行測試造的 zone 都是 Plain）。
- 新增 `virtual ZoneKind kind() const { return ZoneKind::Plain; }`——型別由 virtual 回答、不存欄位，杜絕「欄位與實際型別不一致」一族錯誤。
- 新增序列化掛鉤（預設 no-op）：
  - `virtual void save_extra(cereal::PortableBinaryOutputArchive&) {}`
  - `virtual void load_extra(cereal::PortableBinaryInputArchive&) {}`
- 工廠：`std::unique_ptr<Zone> make_zone(ZoneKind)`（自由函式；kind 未知 → throw，fail-fast 不靜默造 Plain）。

### 3.2 建構路徑（[zone_manager.cpp](../../projects/medp/src/gcore/zone/zone_manager.cpp)）

兩個寫死 `make_unique<Zone>()` 的點改走工廠：

- `emplace_zone(id, parent)` → `emplace_zone(id, parent, ZoneKind)`（[zone_manager.cpp:40](../../projects/medp/src/gcore/zone/zone_manager.cpp)）；root 固定 Plain。
- `create_child(parent)` → `create_child(parent, ZoneKind kind = ZoneKind::Plain)`——預設值使既有呼叫端與測試零改動。
- `load(id)`（[zone_manager.cpp:103](../../projects/medp/src/gcore/zone/zone_manager.cpp)）：改由 zone_io 讀 tag 後經工廠建構（見 3.3）。
- 取用子類介面：呼叫端以 `kind()` 檢查後 `static_cast`；是否包一層 `zone_cast<T>` 輔助函式由 plan 定（傾向做，兩行換掉散落的手動檢查）。

### 3.3 zone 檔格式與 zone_io（[zone_io.h](../../projects/medp/src/gcore/serialize/zone_io.h)）

檔案佈局（單一 stream 依序）：

```text
[cereal 第一塊] kind(uint8) → id → parent → layers → save_extra(子類自寫)
[EnTT snapshot] registry_io（不變）
```

- `zone_io::save(Zone&, ostream)`：第一塊開頭寫 `z.kind()`，塊尾呼叫 `z.save_extra(ar)`；大括號生存期規矩不變。
- `zone_io::load(istream)` 改型為**回傳 `std::unique_ptr<Zone>`**：同一個 input archive 先讀 kind → `make_zone(kind)` → 讀 id/parent/layers → `z->load_extra(ar)` → archive 關閉後接 registry_io。ZoneManager::load 的「檔內 id ≠ 請求 id → throw」防護不變。
- 格式變更使既有存檔作廢：依拍板政策刪存檔目錄，不寫遷移碼。

## 4. World 類

新檔 `gcore/zone/world.h` / `world.cpp`（generate 實作放 .cpp，libtcod 依賴不外洩到 header）：

```cpp
struct WorldGenParams {   // 全部有預設值；隨 save_extra 序列化
    int      width  = 200, height = 200;   // 願景表的 World 尺度
    uint32_t seed   = 0;
    float    sea_level = 0.40f;            // 高度閾值切水陸
    // noise 參數（octaves/scale 等）plan 時定，皆給預設
};

class World : public Zone {
public:
    WorldGenParams gen;
    ZoneKind kind() const override { return ZoneKind::World; }
    void save_extra(...) override;   // 寫 gen
    void load_extra(...) override;   // 讀 gen
    void generate();                 // 依 gen 產出 layers[0]
};
```

### 4.1 generate() 管線（採 libtcod cookbook 的最小可行版）

1. `TCODRandom`/`TCODNoise`（FBM）以 `gen.seed` 鎖定——**同 seed 必產同圖**（決定性是驗收項）。
2. 高度場正規化 0~1，`> sea_level` 切水陸。
3. 另兩張獨立 noise 場（seed 派生）作溫度/濕度，閾值分類 biome。
4. 寫入 `layers[0]`：`Tile::terrain` = biome 佔位 def id（Ocean/Tundra/Grassland/Forest 等常數，先放 world.h，將來歸 Ruleset）；陸地格設 `TILE_WALKABLE`。
5. 重複呼叫 `generate()` 的語意：整層重建（清掉重寫），不做增量。

**不做**（v1 明確排除）：侵蝕/Voronoi 板塊/河道（cookbook 路線 B，觸發：世界地圖品質不夠用時）、連通性驗證與距離場（觸發：首個消費連通性的系統）、多垂直層生成、World tile ↔ child Region 的結構性連結（ChildLink 既有 defer 不變）。

## 5. libtcod 依賴引入

- 組態：`LIBTCOD_SDL3 OFF`（定義 `NO_SDL`，整包不連 SDL），只用 mersenne/noise/heightmap 模組。
- 引入方式傾向 **FetchContent 鎖 tag**（cookbook 方式 B；`~/repo/pas/projects/python-tcod/libtcod/` 有現成 checkout（commit `27c2dbc`）可作離線備援/vendor 來源）。configure 需不需要網路、要不要 vendor 進 repo，plan 時定案。
- 只連進 `medp` target；gbind/tests 經 medp 間接取得，不直接依賴。
- 註：`projects/medp/include/` 現有慣例是 header-only vendor（entt/cereal）；libtcod 是編譯庫，不塞這裡，走 CMake 依賴——INDEX/dev-env 文檔屆時同步。

## 導覽

[← 回母檔索引](world-zone-subclass-design.md) ｜ 下一篇：[測試、落地順序與執行結果](world-zone-subclass-design-execution.md)
