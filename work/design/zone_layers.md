# medps 世界結構：三層 zone + root

> 最後更新：2026-07-19。2026-05-30 重構後與原始碼對齊。

## 遊戲定位

奇幻版「太閣立志傳 × 騎馬與砍殺 × 上古卷軸 × 三國志」的混合體。
核心是純 C++ 模擬（`medp`），前端用 Godot 4 GDExtension。

## 三層 + root 總表

| 層 | ZoneType | 一張圖尺寸 | 一格尺度 | 時間模型 | 數量級 |
|---|---|---|---|---|---|
| root 全局 | Invalid=0 | 無地圖 | — | — | 1（永久） |
| World | 1 | 200×200 | ≈數公里 | 純回合（1回合≈1日） | 每 z 層 1 張 |
| Region | 2 | 15×15 | ≈數十公尺 | 半即時/WeGo | ≤ 40,000 |
| Area | 3 | 250×250 | ≈公尺 | 即時/JRPG | ≤ 900 萬 |

巢狀：World 的每個 tile = 一個 Region；Region 的每個 tile = 一個 Area。

## ZoneKey 定址

`ZoneKey(uint64)` = `ZoneType:16 | x:16 | y:16 | z:16`

- **不叫 Zone**：本專案裡「zone」是任一個 registry/載入單位的通稱，底層若也叫 Zone 會語意打架
- **嚴格階層紅利**：`zone_key_type(k)` 一看就知道它在第幾層，parent 座標可由整除推回
- z 軸三層共用（Underground=-1 / Ground=0 / Sky=+1）

### 座標換算

- `REGION_DIM=15`：1 World tile → 15² Region tiles
- `AREA_DIM=250`：1 Region tile → 250² Area tiles
- Area 全域座標 `(gx, gy) = (wx·15 + rx, wy·15 + ry)`
- parent = `(gx / 15, gy / 15)` 整除回推，不需額外索引
- `MAX_WORLD_DIM = 2184`，預設 `WORLD_DIM_DEFAULT=200`——int16 綽綽有餘，**不需重排 ZoneKey 位元**

### 為何不用重排 ZoneKey 位元

world_dim=200 時 Area 全域 region-tile 座標最大 = 199·15+14 = 2999，而 int16 上限 32767。
→ 維持 16/16/16/16 佈局，不動 `make_zone_key` / `zone_key_*`。

## 一切皆 zone

- 沒有獨立的「世界地圖系統」——三層共用同一套 `entt::registry` + `GlobalManager` 機制
- 一個 zone = 一個 `entt::registry`，全部由 GlobalManager 管理
- root 永久存活、放全局實體；其餘 zone 按需載入

## 規模現實：絕不全載

World 200×200 → 4 萬個 Region × 225 = 最多約 **900 萬個 Area**。永遠不可能全部載入。
既有設計（全域唯一定址、path 由 key 推、不存全域清單、記憶體只跟載入數成正比）就是為了撐這個規模。

## zone 生命週期與持久化

- 擴充：`create(key, parent)` → 新建空 zone + 塞 `ZoneMeta` placeholder
- 載入：`load(key)` → 從 store 反序列化
- 卸載：`unload(key)` → 序列化回 store + 從記憶體移除
- 整局存檔：`save_all()` → root + 所有已載入 zone 寫回 store
- 讀檔：`load_root()` → 只載 root，子 zone 留在 store 按需 `load()`

## Area 層內部結構

- **地格資料 = `AreaTerrain` (`tdarray<Tile>`)**：稠密陣列，不要一格一 entity。250² = 62,500 格，做成 component 而非 ctx，以走 snapshot 存檔
- **離散/會動的物件**：registry 裡的 entity（Pawn、掉落物、建築），`Blocking{blocks_move, blocks_sight}` 做逐 entity 阻擋
- **Lister 不用自己造**：`registry.view<Building>()`、`view<Pawn>()` 本身就是 lister

## 開啟事項

- 跨 z 連通：地面↔地下↔天空怎麼接
- 行動者跨 zone：身份/移動機制
- 世界邊緣：預設有界（大陸+海洋），環形再議
- tick 依 ZoneType 分派（目前全部走同一套，日後細分）
