# medps 世界結構設計:三層 zone + root

> 初版 2026-05-27。分支:`rewrite/entt-cereal`。
> 本文取代已刪除的 `work/plan_ecs_rewrite.md`,是核心**世界結構 / ZoneType / 地格換算**的設計依據。
> 標記說明:**【定案】**=使用者已拍板;**【建議】**=我的推薦待確認;**【開放】**=尚未決定。

---

## 0. 遊戲定位

奇幻版「太閣立志傳 × 騎馬與砍殺 × 上古卷軸 × 三國志」的結合體。
核心是純 C++ 模擬(`medp`),前端用 Godot 4 GDExtension。地圖分三層,**三層都是 tile-based、turn-based**,其上有一個非地圖的全局層(root)。

---

## 1. 三層 + root 總表 【定案】

| 層 | ZoneType | 體驗參照 | 一張圖尺寸 | 一格尺度 | 時間模型 | 全局數量級 |
|---|---|---|---|---|---|---|
| (全局) | `Invalid=0` / ROOT | — | 無地圖 | — | 跨層 delta 結算 | 1(永久存活) |
| 世界層 | `World=1` | 文明(Civ) | ≤ 200×200 | 一格 ≈ 數公里 | 純回合,1 回合 ≈ 一日 | 1(每 z 層一張) |
| 戰略層 | `Region=2` | 大戰略 / 聖火降魔錄 | 15×15(或更大) | 一格 ≈ 數十公尺 | 半即時 / WeGo | ≤ 200×200 = 4 萬 |
| 區域層 | `Area=3` | Rimworld / ToME4 / JRPG | ~250×250 | 一格 ≈ 公尺級 | 即時 / JRPG 回合 | ≤ 3000×3000 ≈ 900 萬 |

- **巢狀**:World 的每個 tile = 一個 Region;Region 的每個 tile = 一個 Area。嚴格階層。
- root 放**全局實體**:faction / 文明 / 神祇 / 具名 NPC 等不屬於任何單一地圖的東西。

---

## 2. ZoneType 命名 【定案 2026-05-27】

```cpp
enum class ZoneType : uint16_t {
    Invalid = 0,   // == ZONE_ROOT,全局層(保留;真實地圖型別從 1 起)
    World   = 1,   // 世界層
    Region  = 2,   // 戰略層
    Area    = 3,   // 區域層
};
```

- 採 `World / Region / Area`(你傾向 world→region;Area 收尾)。
- **刻意不叫 `Zone`**:本專案裡「zone」已是「任一 registry / streaming 單位」的通稱(`ZoneType`、`ZoneKey`、GlobalManager 管的就是 zones),底層若也叫 Zone 會語意打架。t-engine 用 World→Zone→Level,我們改用 World→Region→Area 避開。
- **嚴格階層的紅利**:`zone_key_type(k)` 一看就知道它在第幾層、parent 是哪一型,parent 座標可由整除推回(見 §3),不必額外存。

---

## 3. 地格換算 / 座標巢狀 【定案 2026-05-27】

定案比例(實作在 `zone_key.h` 的 `zone_scale` namespace,`constexpr`、單一來源、可調):

```
WORLD_DIM    = 200   // 世界層 200×200 tiles(一塊大陸 + 周邊海洋)
REGION_DIM   = 15    // 1 個 World tile  → 1 張 Region 圖(15×15 region-tiles)
AREA_DIM     = 250   // 1 個 Region tile → 1 張 Area   圖(250×250 area-tiles)
REGION_CHUNK = 5     // chunk 邊長:5×5 個 region 共一個 chunk 檔(方案 B)
AREA_CHUNK   = 1     // area 不 chunk(1:1)
```

> 座標換算一律引用這些常數、不散落硬編碼;都是估計值(「或更大」「左右」「最大可能」),保留可調。下文 R1 = `REGION_DIM`、R2 = `AREA_DIM`。

### ZoneKey 各層 (x,y) 語意 【定案】

`ZoneKey = ZoneType:16 | x:16 | y:16 | z:16`(位元佈局不動,`make_zone_key` / `zone_key_*` 不變,理由見下)。
每層的 (x,y) = **該 zone 在其上一層全局 tile 網格中的座標**:

| 型別 | (x,y) 意義 | 範圍 |
|---|---|---|
| World | 單一世界(每 z 一張),固定 (0,0) | 0 |
| Region | 它所屬的 **world-tile 全局座標** (wx,wy) | 0..199 |
| Area | 它所屬的 **region-tile 全局座標** (gx,gy) = (wx·R1+rx, wy·R1+ry) | 0..2999 |

- `rx,ry` = 該 Area 在其 Region 內的本地座標(0..14)。
- **parent 由整除推回**:
  - Area (gx,gy) → 上層 Region = world-tile `(gx / R1, gy / R1)`;Region 內本地位置 `(gx % R1, gy % R1)`。
  - Region (wx,wy) → 上層 = World(0,0)(同 z)/ 最終 root。

### 為何不用重排 ZoneKey 位元 【關鍵結論】

先前擔心 16-bit x/y 放不下細層全局座標。代入定案數字後**綽綽有餘**:Area 的全局 region-tile 座標最大 = 199·15+14 = **2999**,而 int16 上限 32767。
→ **維持現有 16/16/16/16 佈局,不動 `make_zone_key` / `zone_key_*`**。
不變量(要寫進測試):`WORLD_DIM · R1 < 32767`(目前 3000,留有約 10× 餘裕,Region 放大到 ~150 仍安全)。

### z 軸 = 垂直三層 【定案】

三種地圖**共用** z 軸:地下 / 地面 / 天空。建議以地面為原點的有號編碼:

```cpp
namespace zlayer { constexpr int16_t Underground = -1, Ground = 0, Sky = +1; }
```

- z 是 ZoneKey 的一部分 → **每個垂直層是各自獨立的 zone**(地下世界圖 ≠ 地面世界圖)。
- parent 鏈**同 z 不變**:地下 Area 的 parent 是地下 Region。跨 z 的移動(階梯 / 飛行)走 `CrossZoneRef` 跨 zone resolve,不靠 parent 鏈。
- 世界觀呼應:泰坦融入大地(地下)、古龍融入天空(`notes/a.txt`)。

---

## 4. 「一切皆 zone」+ 兩層持久化(t-engine / Rimworld 驗證)

### 4.1 一切皆 zone(t-engine 驗證)

t-engine(ToME4)沒有獨立的「世界地圖系統」——**大地圖只是一個設了特殊屬性的普通 Zone**,層間切換靠地形格上的 `change_zone` 欄位(`pas/analysis/t-engine/tutorial/12-world-map.md`)。
對應我們:三層共用同一套 `entt::registry` + GlobalManager 機制,只靠 `ZoneType` tag 與屬性區分;層間切換 = 站到帶 **`CrossZoneRef`** 的格/實體上,resolve 出目標 ZoneKey 與入口位置。`CrossZoneRef` 已存在,正是 t-engine `change_zone` 的對應物。

### 4.2 規模現實:絕不全載

World 200×200 → 最多 4 萬個 Region;再乘 R1²=225 → **最多約 900 萬個 Area**。顯然永遠不可能全部載入。
現有不變量(`zone_key.h`:全局唯一定址、path 由 key 推、不存全域清單、記憶體只跟載入數成正比)就是為了撐這個規模而設,**方向正確**。

### 4.3 兩層模型(Rimworld 教訓的對照)

Rimworld 的 `outpost_archiving_strategy.md` 把「閒置地圖怎麼處理」攤開,結論是引擎傾向「**離開就拆、要用再重生成,人保留、場景丟棄**」,因為它**沒有官方的地圖快照/還原 API**,自序列化(路線 B)既肥又脆。

**我們的處境正好相反——我們自己造了那個 API**:EnTT snapshot + cereal(`serialize/zone_io.h` + `entt_cereal_archive.h` + `zone_store.h`)就是一套乾淨的整-registry 快照/還原容器。所以對 medps,Rimworld 不敢用的「路線 B(精準還原)」對我們是便宜安全的。據此分兩層:

| 狀態 | 載體 | 用途 | 對應 Rimworld 路線 |
|---|---|---|---|
| **已載入** | 完整 `entt::registry` | 玩家當前所在 / 需精細模擬的少數 zone | — (在 `Game.Maps`) |
| **已卸載但存檔** | 磁碟上的 zone blob(`zone_io`) | 離開過、之後可原貌還原 | 路線 B(我們做得起) |
| **僅摘要** | 父 zone 內的 `ChildZoneSummary` stub | 上百萬個沒去過的 zone:跑廉價的聚合 off-screen 模擬,不載全圖 | 路線 C(數值抽象化) |

→ `ChildZoneSummary` = Rimworld「把哨站濃縮成 `WorldObjectComp` 數值」的對應物。**已載入集合保持很小**(Rimworld 教訓:每張在清單裡的地圖都吃 tick;它上限 128 張),其餘靠摘要做聚合演進。

### 4.4 tick 與 off-screen 模擬

- Rimworld:每張載入地圖每 tick 都跑 → 成本。我們 `GlobalManager::tick` 目前對每個載入 zone 跑同一套 system。
- 【建議】tick 依 `ZoneType` 分派(只有 World 是純回合/1 日;Region WeGo;Area 即時/JRPG),且**只 tick 已載入的少數**。
- 未載入 zone 的演進 = 在父層用 `ChildZoneSummary` 做聚合 delta(對應 gamecore「微觀→宏觀 delta 寫入佇列」`pas/others/gamecore/plans/002:27`)。

---

## 5. Area 層內部結構(Rimworld `map_system.md`)

Rimworld 的一張 Map = **一組平行的稠密 2D 網格**(TerrainGrid / RoofGrid / FogGrid / PathGrid…,皆 250×250),外加「Things」(pawn / 物品 / 建築);用 Lister 做快速查找,**絕不遍歷整張地圖**。

對應到 EnTT 的 Area zone(一個 registry):

- **地格資料 = 稠密陣列,不要一格一 entity**。250×250 = 62500 格,做成 entity 太重。做成**掛在單例 entity 上的 component**:已實作 `AreaTerrain { tdarray<Tile> }`,`Tile{ uint16 terrain; uint8 flags }`(flags 快取可走 / 擋視線)。**不可放 `registry.ctx()`**——`zone_io` 用 snapshot 遍歷 component 存檔,ctx 不會被帶走。第一版只一張 terrain 網格;roof / fog / path 等平行網格之後按系統需要再加。這也正是當初保留 `tdarray` 的理由。
- **離散 / 會動的物件(pawn、掉落物、建築)= registry 裡的 entity**(逐 entity 阻擋已實作 `Blocking{ blocks_move, blocks_sight }`)。
- **Lister 不用自己造**:`registry.view<Building>()`、`view<Pawn>()` 本身就是 lister。Rimworld「別掃全圖、用 lister」在 EnTT 等於「用 view、別自己 iterate 全 entity」。
- 同理 World / Region 層的地形也走稠密網格(World 200×200、Region 15×15),只是格上承載的語意不同(World 格=影響力場/地形;Region 格=戰術地形)。

---

## 6. 與現有程式的落差 / 待改清單

> 框架已落地(2026-05-27,`medp_test` 25/25 綠燈)。已完成標 [x];component / system 由使用者後續主導填。

- [x] `zone_key.h`:`ZoneType{World,Region,Area}`、`zlayer::{Underground=-1,Ground=0,Sky=+1}`、換算 helper(`world_key`/`region_key`/`area_key`/`parent_of`)、常數集中 `zone_scale::{WORLD_DIM,REGION_DIM,AREA_DIM,REGION_CHUNK,AREA_CHUNK}` + 溢位 `static_assert`。
- [x] 路徑推導:zone/chunk 檔名由 key 的 16 進位推出(`ChunkedFolderZoneStore::chunk_path` / `FolderZoneStore::path`);type+z 已含在 key 內。
- [x] 不變量測試:`zone_layers_and_parent`(Area↔Region 整除往返、同 z parent 鏈)、`chunk_key_grouping`;`WORLD_DIM·REGION_DIM < INT16_MAX` 由 `static_assert` 保證。
- [x] **registry chunk(方案 B)**:`chunk_key.h`(`chunk_key_of`,Region N=5、Area N=1)+ `serialize/chunked_zone_store.h`(chunk 檔 = cereal `map<ZoneKey,blob>`,write-through);`global_manager` 預設 store 已換;測試 `chunked_store_packs_zones`(同 chunk 兩 zone 互不污染 + partial update + 打包成單檔)。
- [x] **chunk 預取 + ES 式滾動視窗**:`ZoneStore::group_of`(回傳同 chunk 已存檔 keys)+ `GlobalManager::prefetch(key)`(暖整個 storage chunk)+ `stream_around(center, radius)`(以玩家所在 zone 為中心維持 (2r+1)² 同層視窗:載入磁碟上存在的、卸載滾出的;對應上古卷軸 `uGridsToLoad`,建議 Region radius≈2、Area=0)。測試 `prefetch_loads_chunk`、`stream_around_window`。仍是 per-zone registry、不合併,invariant 不變。
- [ ] (後)`GlobalManager::tick` 依 ZoneType 分派;off-screen 聚合走 `ChildZoneSummary`。
- [x] **Area 第一批 component**:`AreaTerrain`(`tdarray<Tile>` 單例 component,走 snapshot 存檔)、`Blocking`(逐 entity 阻擋);登錄 `all_components.h`、各有 round-trip 測試。(移除了示範用的 `Owner`;底層 `CrossZoneRef` 機制保留。)
- [ ] (後)更多遊戲 component / system:actor 生命(部位傷害)/ 耐力 / 士氣、需求(馬斯洛)、AI、日程…由使用者主導,參 gamecore 004/005。
- [ ] (後)roof / fog / path 等平行地格網格(按系統需要)。

---

## 7. 已決(記錄)與仍開放

**已決,待實作 / 後議:**
- **registry 粒度 → 方案 B**(2026-05-27,詳見 `work/design/registry_chunking_investigation.md`):不合併 registry(方案 A 會撞 entity 命名空間、需重寫序列化路徑);改在 `ZoneStore` 層**合併檔案**(`ChunkedFolderZoneStore`)。Region chunk N=5、Area 1:1。已落地;streaming 平順靠 `prefetch` + ES 式 `stream_around`(已實作)。
- **行動者跨 zone → C 案**:權威身份 + 世界座標住 root;載入的 zone 放本地分身、以 `CrossZoneRef` 連回 root;未載入軍隊靠 root / `ChildZoneSummary` 聚合模擬(≈ Rimworld WorldPawn / gamecore 影響力場)。細節留待 actor 設計階段。

**仍開放:**
- **跨 z 連通**:地面↔地下↔天空怎麼接(用 `CrossZoneRef`;哪些格可通、單向?)。
- **世界邊緣**:預設**有界**(大陸 + 周邊海洋,邊緣為海 / 不可越);要環形(toroidal)再議(gamecore `002:37`)。
- **時間 / tick 模型**:tick 依 ZoneType 分派、off-screen 聚合演進——之後再處理(見 §4.4)。
- **存檔規模**:900 萬潛在 Area + 物件持久化的增量存檔 / 壓縮(gamecore `006:33`)。

---

## 參考

- 遊戲設計本體:`C:/code/mine/pas/others/gamecore/plans/`(`002` 世界結構、`003` 機制/時間、`006` 技術)。
- t-engine(ToME4)分析:`C:/code/mine/pas/analysis/t-engine/`(`tutorial/12-world-map.md` 大地圖即 zone、`tutorial/03-zones.md`)。
- Rimworld 分析:`C:/code/mine/pas/analysis/rimworld/`(`architecture/map_system.md` Area 內部結構、`architecture/outpost_archiving_strategy.md` 多地圖持久化路線)。**注意:不參考其 hex world grid。**
