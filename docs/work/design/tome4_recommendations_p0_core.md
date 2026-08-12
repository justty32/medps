# ToME4 架構研讀 → medps 重寫建議報告 —— §2 建議清單 A-D

> 本檔是 [tome4_recommendations.md](tome4_recommendations.md) 的拆分子檔，收錄 §2 建議清單的 A～D 段（位置模型、空間索引、阻擋模型劃界、Zone 生命週期策略）。上一節：[§0-§1 方法與背景](tome4_recommendations_background.md)；下一節：[§2 E-F（存檔／定址）](tome4_recommendations_save_addressing.md)。

## 2. 建議清單

每條標註：優先級（P0=下一步就該處理的設計債 / P1=近期 / P2=用到再說）、對應的 medps 現況。

### A. 位置模型（P0）——「actor 在哪個垂直層」目前無法表達

ToME 的 actor 位置語意實際上是 **(Level, x, y)**——Level 物件本身就是第三維；由 Level 的容器結構（`entities`/`e_array`）可推斷換層即在兩個 Level 容器間搬移實體（推論，語料未直述搬移機制）。medps 把垂直層收進同一個 Zone（`layers` 的 key 是 z）之後，`Position{x,y}`（[position.h](../../../projects/medp/src/gcore/components/position.h)）就缺了一維，跨層的阻擋/視線/移動判定都無從做起。

三個選項（本報告推薦 ①）：

| 選項 | 做法 | 代價 |
|---|---|---|
| ① Position 加 z 欄位 | `Position{x, y, z}`，z 對應 `layers` 的 key | 最小改動；空間查詢多比一欄 |
| ② Layer component | 另掛 `Layer{int16_t z}`，可用 EnTT group 加速同層查詢 | 兩個 component 要保持一致 |
| ③ 一層一 registry | 回到舊設計「不同 z 不同 zone」 | 推翻本輪的 layers 決定，跨層互動更難 |

推薦 ① 的理由：本輪已決策 z 不進 zone id（口頭拍板，記錄於此；[zone.h](../../../projects/medp/src/gcore/zone/zone.h) 註解亦標記 id 座標語意待重新設計），那 z 就是純座標，跟 x/y 同性質；等實測發現「按層遍歷」是熱路徑再升級到 ②。

### B. 空間索引（P1）——按格查實體目前只能全 registry 掃

ToME 的 Level/Map 職責分離：Level 管「誰在這層」（`entities` uid 查找 + `e_array` 行動順序），Map 管「誰在哪格」——稀疏 `[x+y*w]`，**每格的值不是單一 entity，而是「分類槽→entity」的小表**（TERRAIN=1 / TRAP=50 / ACTOR=100 / PROJECTILE=500 / OBJECT=1000 / TRIGGER=10000），同格可同時容納地形、陷阱、角色、物品。medps 有 Level 那半（registry）沒 Map 那半：回答「(x,y) 有沒有 actor」要 `view<Position>` 全掃。

建議：等第一個需要它的系統（碰撞、FOV）出現時，給 Zone 加 per-layer 的格→實體反查表，且結構第一天就做成「分類→entity」的多值容器——4X 必然出現 actor+物品+陷阱同格，單值表會返工。分類用 `enum class`、與序列化數值解耦（ToME 的裸魔術數字滲入所有呼叫點，是反面教材）。維護學 ToME 懶更新（實體移動時寫入，不每 tick 重建）。

不必急著做，但設計移動系統時要預留「位置變更必經一個函式」的口子——ToME 的 Actor 禁止直接設 x/y、必須走 `move()`（合理推測正是為了讓索引維護有單一入口，語料未明述動機）。這對 medps 是具體警訊：~~現行 [movement.h](../../../projects/medp/src/gcore/systems/movement.h) 直接 `p.x += v.dx` 改 Position，將來有空間索引後這條路必須收口~~ **已收口（2026-07-22）：位置變更統一走 `systems::move_by`（movement.h:12），tile flag 檢查與空間索引維護將來掛這裡**。

### C. 阻擋模型劃界（P1）——重構刪掉了 Blocking，模型只剩一半

ToME 走「一切皆實體」：地形格（Grid）也是實體，阻擋判定 = `checkAllEntities(x,y,"block_move")` 問遍格內所有實體。極靈活（門、可破壞牆、大型生物都自然表達），但每步移動都是多實體查詢。medps 的 `Tile{terrain, flags}` + `TILE_WALKABLE`（[tile.h](../../../projects/medp/src/gcore/zone/tile.h)）是反向選擇：快而死板。

建議維持混合模型並**明確劃界**：

- 靜態地形通行性 → Tile flags（現狀，保持）
- 動態阻擋源（門、召喚牆、站在格上的大型單位）→ 重建 ECS component（舊 `Blocking{blocks_move, blocks_sight}` 這輪被刪了，這個概念遲早要回來）
- 移動判定 = tile flag **AND** 空間索引查詢（B 做好之後）

反面教訓：不要把動態阻擋壓進 Tile flags——會產生「門開了誰負責還原 flag」的狀態同步難題。

### D. Zone 生命週期策略（P0）——ToME 的 persistent + decay + LRU 是現成答案骨架

medps 的 `ZoneManager` 有 load/unload/save_all 原語（[zone_manager.h](../../../projects/medp/src/gcore/zone/zone_manager.h)），但「**何時**載、**何時**卸」完全空白。ToME 的做法是把策略聲明在 zone 資料上、機制放引擎：

- `persistent`——每個 zone 自己聲明生命週期。⚠ 語料兩處說法**不一致**：architecture 層說三態 `"zone" | "level" | false`（engine_detail/3-世界結構.md），tutorial 層說**四態**且更細（tutorial/10-base-camp-basic/core-concepts.md）：`false`=離開即重生成、`"memory"`=存 `game.memory_levels`（存檔後消失）、`"zone"`=整 zone 以 `.teaz` 檔存磁碟、`true`=每層獨立存檔；機制為 leaveLevel 寫 memory_levels、`Zone:save()` 寫磁碟、getLevel 優先從 memory_levels 取。設計 medps 的 persistence 枚舉時以較細的四態版為藍本，或回原始碼裁決（前置見 §0，[background 檔](tome4_recommendations_background.md)）。
- `decay = {min, max}`——離開多久（遊戲 tick）後可回收。
- `enableLastPersistZones(max)`——LRU：最近造訪的 N 個 zone 留在記憶體。

建議：Zone struct 加 persistence 枚舉 + ZoneManager 加 LRU 卸載，策略是 zone 的資料、不寫死在 manager。對 4X 尺度（舊設計估算：Region ≤4 萬、Area ≤900 萬）這不是優化而是生存必需——「絕不全載、記憶體正比於已載入數」的舊紅線仍然成立。

殘餘未知：decay 與 persistent 的互動、LRU 逐出時機與逐出時是否寫盤，語料未載（見 §5，[status/summary 檔](tome4_recommendations_status_summary.md)）。

---
上一節：[§0-§1 方法與背景](tome4_recommendations_background.md)　|　下一節：[§2 E-F（存檔／定址）](tome4_recommendations_save_addressing.md)　|　回索引：[tome4_recommendations.md](tome4_recommendations.md)
