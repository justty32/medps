# medps 世界結構設計:三層 zone + root

> 初版 2026-05-27;2026-05-30 重構後對齊原始碼。
> 本文是核心**世界結構 / ZoneType / 地格換算**的設計依據。
> 標記說明:**【定案】**=使用者已拍板;**【建議】**=我的推薦待確認;**【開放】**=尚未決定。

---

## 0. 遊戲定位

奇幻版「太閣立志傳 × 騎馬與砍殺 × 上古卷軸 × 三國志」的結合體。
核心是純 C++ 模擬(`medp`),前端用 Godot 4 GDExtension。地圖分三層,**三層都是 tile-based、turn-based**,其上有一個非地圖的全局層(root)。

---

## 1. 三層 + root 總表 【定案】

| 層 | ZoneType | 體驗參照 | 一張圖尺寸 | 一格尺度 | 時間模型 | 全局數量級 |
|---|---|---|---|---|---|---|
| (全局) | `Invalid=0` / ROOT | — | 無地圖 | — | — | 1(永久存活) |
| 世界層 | `World=1` | 文明(Civ) | 預設 200×200 | 一格 ≈ 數公里 | 純回合,1 回合 ≈ 一日 | 1(每 z 層一張) |
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
- **刻意不叫 `Zone`**:本專案裡「zone」已是「任一 registry / 載入單位」的通稱(`ZoneType`、`ZoneKey`、GlobalManager 管的就是 zones),底層若也叫 Zone 會語意打架。t-engine 用 World→Zone→Level,我們改用 World→Region→Area 避開。
- **嚴格階層的紅利**:`zone_key_type(k)` 一看就知道它在第幾層、parent 是哪一型,parent 座標可由整除推回(見 §3),不必額外存。

---

## 3. 地格換算 / 座標巢狀 【定案 2026-05-27】

定案比例(實作在 `zone_key.h` 的 `zone_scale` namespace,`constexpr`、單一來源、可調):

```
WORLD_DIM_DEFAULT   = 200   // 預設世界層邊長(world-格);實際值存 ROOT 上的 WorldConfig
WORLD_LAYERS_DEFAULT = 3    // 預設垂直層數:Underground / Ground / Sky(z = −1/0/1)
REGION_DIM          = 15    // 1 個 World tile  → 1 張 Region 圖(15×15 region-tiles)
AREA_DIM            = 250   // 1 個 Region tile → 1 張 Area   圖(250×250 area-tiles)
MAX_WORLD_DIM       = 32766 / REGION_DIM   // = 2184;world_dim 上限,確保 key x/y 不溢位
// valid_world_dim(wd):wd > 0 && wd * REGION_DIM < 32767
```

> WORLD_DIM 不再是單一常數:它是 PER-SAVE 執行期設定,存在 ROOT 上的
> `WorldConfig{world_dim_x, world_dim_y, world_dim_z}`(見 §4)。`zone_scale` 只給預設值。
> 座標換算一律引用這些常數 / `WorldConfig`、不散落硬編碼。下文 R1 = `REGION_DIM`、R2 = `AREA_DIM`。

### ZoneKey 各層 (x,y) 語意 【定案】

`ZoneKey = ZoneType:16 | x:16 | y:16 | z:16`(位元佈局不動,`make_zone_key` / `zone_key_*` 不變,理由見下)。
每層的 (x,y) = **該 zone 在其上一層全局 tile 網格中的座標**:

| 型別 | (x,y) 意義 | 範圍 |
|---|---|---|
| World | 單一世界(每 z 一張),固定 (0,0) | 0 |
| Region | 它所屬的 **world-tile 全局座標** (wx,wy) | 0..(world_dim−1) |
| Area | 它所屬的 **region-tile 全局座標** (gx,gy) = (wx·R1+rx, wy·R1+ry) | 0..(world_dim·R1−1) |

- `rx,ry` = 該 Area 在其 Region 內的本地座標(0..14)。
- **parent 由整除推回**:
  - Area (gx,gy) → 上層 Region = world-tile `(gx / R1, gy / R1)`;Region 內本地位置 `(gx % R1, gy % R1)`。
  - Region (wx,wy) → 上層 = World(0,0)(同 z)/ 最終 root。

### 為何不用重排 ZoneKey 位元 【關鍵結論】

先前擔心 16-bit x/y 放不下細層全局座標。代入預設數字後**綽綽有餘**:預設 world_dim=200 時,Area 的全局 region-tile 座標最大 = 199·15+14 = **2999**,而 int16 上限 32767。
→ **維持現有 16/16/16/16 佈局,不動 `make_zone_key` / `zone_key_*`**。
不變量(由 `valid_world_dim` 把關):`world_dim · R1 < 32767`(預設 3000,留有約 10× 餘裕;world_dim 上限 `MAX_WORLD_DIM`=2184)。

### z 軸 = 垂直三層 【定案】

三種地圖**共用** z 軸:地下 / 地面 / 天空。建議以地面為原點的有號編碼:

```cpp
namespace zlayer { constexpr int16_t Underground = -1, Ground = 0, Sky = +1; }
```

- z 是 ZoneKey 的一部分 → **每個垂直層是各自獨立的 zone**(地下世界圖 ≠ 地面世界圖)。
- parent 鏈**同 z 不變**:地下 Area 的 parent 是地下 Region(`parent_of` 沿鏈往上時 z 保持不變)。
- 垂直層數由 `WorldConfig::world_dim_z` 決定(預設 3 = Underground/Ground/Sky)。
- 世界觀呼應:泰坦融入大地(地下)、古龍融入天空(`notes/a.txt`)。

---

## 4. 「一切皆 zone」+ 按需載入持久化(t-engine / Rimworld 驗證)

### 4.1 一切皆 zone(t-engine 驗證)

t-engine(ToME4)沒有獨立的「世界地圖系統」——**大地圖只是一個設了特殊屬性的普通 Zone**(`pas/analysis/t-engine/tutorial/12-world-map.md`)。
對應我們:三層共用同一套 `entt::registry` + `GlobalManager` 機制,只靠 `ZoneType` tag 與屬性區分。**一個 zone = 一個 `entt::registry`**,全部由 `GlobalManager` 管;root 永久存活、放全局實體,其餘 zone 按需載入。

### 4.2 規模現實:絕不全載

預設 world_dim=200 時,World 200×200 → 最多 4 萬個 Region;再乘 R1²=225 → **最多約 900 萬個 Area**。顯然永遠不可能全部載入。
現有不變量(`zone_key.h`:全局唯一定址、path 由 key 推、不存全域清單、記憶體只跟載入數成正比)就是為了撐這個規模而設,**方向正確**。

### 4.3 zone 的生命週期與持久化

- **一個 zone = 一個 `entt::registry`**,由 `GlobalManager` 持有(`loaded_` map,key 為 `ZoneKey`);root 永久存活。
- 序列化:EnTT snapshot + cereal(`serialize/zone_io.h` save/load + `entt_cereal_archive.h`)把整個 registry 打包成位元組;`ZoneStore` 負責位元組↔儲存。
- 預設後端 **`FolderZoneStore`**:**一 zone 一檔**——`dir_/<16 碼 hex key>.bin`,ROOT 特例為 `dir_/root.bin`;path 由 key 推導,不另存全域清單。`GlobalManager()` 無參數建構子預設用 `FolderZoneStore("zones")`,要換位置可注入自訂 `ZoneStore`。
- **按需載入 / 卸載**:`load(key)` 從 store 反序列化(已載入則回傳既有);`unload(key)` 序列化回 store 並從記憶體移除;`create(key, parent)` 在 parent 底下建立新 zone 並植入 `ZoneMeta`。
- **整局存檔 / 讀檔**:`save_all()` 把 root + 目前所有已載入的 zone 寫入 store(不卸除);`load_root()` 只載入 root,子 zone 留在 store 中按需 load。

### 4.4 tick

- `GlobalManager::tick()` 對**每個已載入的 zone**執行所有已註冊的 per-zone 系統(`add_zone_system` 依註冊順序);root 被排除(它放全局 entity,不是地圖角色)。
- per-zone 系統型別為 `std::function<void(entt::registry&)>`。
- 【開放】tick 依 `ZoneType` 分派(只有 World 是純回合/1 日;Region WeGo;Area 即時/JRPG)——之後再處理。

---

## 5. Area 層內部結構(Rimworld `map_system.md`)

Rimworld 的一張 Map = **一組平行的稠密 2D 網格**(TerrainGrid / RoofGrid / FogGrid / PathGrid…,皆 250×250),外加「Things」(pawn / 物品 / 建築);用 Lister 做快速查找,**絕不遍歷整張地圖**。

對應到 EnTT 的 Area zone(一個 registry):

- **地格資料 = 稠密陣列,不要一格一 entity**。250×250 = 62500 格,做成 entity 太重。做成**掛在單例 entity 上的 component**:已實作 `AreaTerrain { tdarray<Tile> }`,`Tile{ uint16 terrain; uint8 flags }`(flags 快取可走 / 擋視線)。**不可放 `registry.ctx()`**——`zone_io` 用 snapshot 遍歷 component 存檔,ctx 不會被帶走。第一版只一張 terrain 網格;roof / fog / path 等平行網格之後按系統需要再加。這也正是當初保留 `tdarray` 的理由。
- **離散 / 會動的物件(pawn、掉落物、建築)= registry 裡的 entity**(逐 entity 阻擋已實作 `Blocking{ blocks_move, blocks_sight }`)。
- **Lister 不用自己造**:`registry.view<Building>()`、`view<Pawn>()` 本身就是 lister。Rimworld「別掃全圖、用 lister」在 EnTT 等於「用 view、別自己 iterate 全 entity」。
- 同理 World / Region 層的地形也走稠密網格(World world_dim²、Region 15×15),只是格上承載的語意不同(World 格=影響力場/地形;Region 格=戰術地形)。

---

## 6. 與現有程式的對應 / 待改清單

> 框架已落地;2026-05-30 精簡重構。已完成標 [x];component / system 由使用者後續主導填。

- [x] `zone_key.h`:`ZoneType{Invalid,World,Region,Area}`、`zlayer::{Underground=-1,Ground=0,Sky=+1}`、換算 helper(`world_key`/`region_key`/`area_key`/`parent_of`)、常數集中 `zone_scale::{WORLD_DIM_DEFAULT,WORLD_LAYERS_DEFAULT,REGION_DIM,AREA_DIM,MAX_WORLD_DIM,valid_world_dim}`。
- [x] 路徑推導:zone 檔名由 key 的 16 進位推出(`FolderZoneStore::path`,ROOT→`root.bin`);type+z 已含在 key 內。
- [x] `WorldConfig{world_dim_x,world_dim_y,world_dim_z}`:PER-SAVE 設定,以 ROOT singleton component 存檔;`GlobalManager::init_world(x,y,z)` 植入、`world_config()` 取用;`valid_world_dim` 把關 x/y 不溢位。
- [x] zone 生命週期:`GlobalManager` 的 `create(key,parent)` / `load(key)` / `unload(key)` / `get(key)` + `save_all()` / `load_root()`;per-zone registry,path 由 key 推、不存全域清單。
- [x] 序列化:EnTT snapshot + cereal(`serialize/zone_io.h`、`entt_cereal_archive.h`)+ `ZoneStore`(write/read/has/flush);唯一且預設後端 `FolderZoneStore`(一 zone 一檔)。component 型別清單單一來源 `all_components.h`:`AllComponents = type_list<ZoneMeta, Position, Velocity, AreaTerrain, Blocking, WorldConfig>`。
- [x] **Area 地格 component**:`AreaTerrain`(`tdarray<Tile>` 單例 component,`Tile{terrain,flags}`,走 snapshot 存檔)、`Blocking{blocks_move,blocks_sight}`(逐 entity 阻擋);均登錄 `all_components.h`。
- [ ] (後)`GlobalManager::tick` 依 ZoneType 分派。
- [ ] (後)更多遊戲 component / system:actor 生命(部位傷害)/ 耐力 / 士氣、需求(馬斯洛)、AI、日程…由使用者主導,參 gamecore 004/005。
- [ ] (後)roof / fog / path 等平行地格網格(按系統需要)。

---

## 7. 已決(記錄)與仍開放

**已決:**
- **registry 粒度 → 每 zone 一個 registry**:不合併 registry(合併會撞 entity 命名空間、需重寫序列化路徑)。持久化在 `ZoneStore` 層做,預設 `FolderZoneStore` 一 zone 一檔;按需 `load()` / `unload()`。

**仍開放:**
- **跨 z 連通**:地面↔地下↔天空怎麼接(哪些格可通、單向?)。
- **行動者跨 zone**:跨 zone 的身份 / 移動機制(留待 actor 設計階段)。
- **世界邊緣**:預設**有界**(大陸 + 周邊海洋,邊緣為海 / 不可越);要環形(toroidal)再議(gamecore `002:37`)。
- **時間 / tick 模型**:tick 依 ZoneType 分派——之後再處理(見 §4.4)。
- **存檔規模**:900 萬潛在 Area + 物件持久化的增量存檔 / 壓縮(gamecore `006:33`)。

---

## 參考

- 遊戲設計本體:`C:/code/mine/pas/others/gamecore/plans/`(`002` 世界結構、`003` 機制/時間、`006` 技術)。
- t-engine(ToME4)分析:`C:/code/mine/pas/analysis/t-engine/`(`tutorial/12-world-map.md` 大地圖即 zone、`tutorial/03-zones.md`)。
- Rimworld 分析:`C:/code/mine/pas/analysis/rimworld/`(`architecture/map_system.md` Area 內部結構、`architecture/outpost_archiving_strategy.md` 多地圖持久化路線)。**注意:不參考其 hex world grid。**
