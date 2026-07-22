# medps 世界結構：三層 zone + root（設計願景）

> 最後更新：2026-07-22。**定位變更**：本文檔保留三層世界的設計願景（尺度、時間模型、規模紅線——仍有效）；
> 舊 ZoneKey 座標定址的實作敘述已隨 2026-07-22 重構作廢，現況見文末「實作現況」。

## 遊戲定位

奇幻版「太閣立志傳 × 騎馬與砍殺 × 上古卷軸 × 三國志」的混合體。
核心是純 C++ 模擬（`medp`），前端用 Godot 4 GDExtension。

## 三層 + root 總表（願景，尺度數字為預設值）

| 層 | 一張圖尺寸 | 一格尺度 | 時間模型 | 數量級 |
|---|---|---|---|---|
| root 全局 | 無地圖 | — | — | 1（永久） |
| World | 200×200 | ≈數公里 | 純回合（1回合≈1日） | 少數幾張 |
| Region | 15×15 | ≈數十公尺 | 半即時/WeGo | ≤ 40,000 |
| Area | 250×250 | ≈公尺 | 即時/JRPG | ≤ 900 萬 |

巢狀：World 的每個 tile = 一個 Region；Region 的每個 tile = 一個 Area。

## 一切皆 zone

- 沒有獨立的「世界地圖系統」——各層共用同一套 `Zone` + `ZoneManager` 機制
- 一個 zone = 一個 `entt::registry`＋自帶多層 tile 地圖（`Zone::layers`）
- root 永久存活、放全局實體；其餘 zone 按需載入
- 垂直分層（地面/地下/天空）**收在同一個 zone 內**：`Zone::layers` 的鍵即 z（地面=0、往下為負），
  不再像舊設計那樣「不同 z 是不同 zone」

## 規模現實：絕不全載（紅線，仍有效）

World 200×200 → 4 萬個 Region × 225 = 最多約 **900 萬個 Area**。永遠不可能全部載入。
驗收不變量：任何持久或常駐結構的成長軸，只允許是「已載入 zone 數」（RAM）或「已造訪 zone 數」（磁碟），
**絕不允許是世界總 zone 數**。

## 實作現況（2026-07-22 重構後）

- zone id 是**零座標語意**的 `uint64_t` 單調序號，由 `ZoneManager::create_child` 配發；
  舊 ZoneKey 的位元打包（type/x/y/z）、`parent_of()` 整除回推、`zone_scale` 常數已全部移除。
- 層級只活在 `Zone::parent` 鏈上；「child 在 parent 地圖上的哪一格」的結構性連結（ChildLink/anchor）
  已有設計草稿但 defer，見 [spec 拍板結果](../../../workflows/specs/zone-addressing-lifecycle-design.md)。
- 「World/Region/Area 各層的行為與專屬資料」改由 **Zone 子類**表達（2026-07-22 拍板變更，取代原「parent 深度＋玩法層資料」路線）：
  第一個子類 `World`（worldgen：seed＋參數 → libtcod FBM 產 200×200 水陸/biome 地圖）已落地，
  見 [world-zone-subclass spec](../../../workflows/specs/world-zone-subclass-design.md)。
  kind 仍不進 id（id 維持零語意序號，型別存 zone 檔頭 tag）；層級鏈仍活在 `Zone::parent`；
  tick 依層細分（不同時間模型）尚未動工。

## 開啟事項

- 跨 zone 行動者：身份/移動機制（跨 registry 搬移函式未動工）
- 世界邊緣：預設有界（大陸+海洋），環形再議
- tick 依層分派（目前全部走同一套 ZoneSystem）
- 三層尺度常數落到哪（Ruleset？WorldConfig 後繼者？）未定
