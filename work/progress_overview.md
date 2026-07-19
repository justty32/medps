# medps 進度總覽

> 最後更新：2026-07-19

## 一句話

**medps** 是奇幻 4X 策略遊戲的 C++20 後端函式庫（`medp`），前端預計用 Godot 4 GDExtension。
目前處於核心骨架完成階段——世界結構、ECS 基建、存讀檔、Zone 生命週期管理都已穩固，
但真正的遊戲內容（種族、宗教、戰鬥、外交等）還沒開始寫。

## 已落地（地基層）

| 項目 | 狀態 |
|------|------|
| **三層世界結構**（World/Region/Area + root） | ✅ 定案實作，`zone_key.h` 全域定址 |
| **ZoneKey 座標系統** | 64-bit 打包（ZoneType:16\|x:16\|y:16\|z:16），巢狀換算由整除推導，不需額外索引 |
| **ECS 核心**（EnTT） | 每 zone 一個 `entt::registry`，root 永久存活，其餘按需載入 |
| **已有元件** | ZoneMeta（身份）、Position、Velocity、Blocking、AreaTerrain（稠密地形網格）、WorldConfig |
| **移動系統** | per-zone 系統範例：有 Velocity 的 entity 每 tick 走一步 |
| **序列化** | EnTT snapshot + cereal（PortableBinary），`all_components.h` 是元件清單唯一來源 |
| **ZoneStore 抽象** | 預設 `FolderZoneStore`（一 zone 一檔），可抽換後端 |
| **Zone 生命週期** | create/load/unload/tick/save_all/load_root 完整 |
| **Godot 接線驗證** | smoke-test 證明前端能呼叫後端 |
| **測試** | 16 項自動化測試全綠 |

## 設計基礎

### 世界結構（三層 + root）

```
root（全局實體：神祇、家族、文明，永久存活）
  └ World 層（200×200，純回合，一格≈數公里）
       └ Region 層（15×15，半即時/WeGo，一格≈數十公尺）
            └ Area 層（250×250，即時/JRPG 回合，一格≈公尺）
```

- 嚴格巢狀：1 World tile = 1 Region map；1 Region tile = 1 Area map
- 所有三層共用同一套 `entt::registry` + `GlobalManager` 機制，只靠 `ZoneType` 區分
- z 軸：三層共用（Underground=-1 / Ground=0 / Sky=+1），不同 z 為不同 zone

### ZoneKey 座標系統要點

- `ZoneKey(uint64)` = `ZoneType:16 | x:16 | y:16 | z:16`
- 每層 (x,y) = 該 zone 在父層全域 tile 網格的座標
- parent 由整除推回（`parent_of()`），不需額外索引
- `MAX_WORLD_DIM = 2184`（int16 上限 / REGION_DIM），預設 `WORLD_DIM_DEFAULT = 200`
- world_dim 是 per-save runtime 設定，存在 `WorldConfig`，不是編譯期常數

### 全域設計取捨

1. **一切皆 zone**：三層共用同一套機制，只靠 ZoneType tag 和屬性區分（ToME4 驗證過的模式）
2. **絕不全載**：預設 world_dim=200 時最多 900 萬個 Area，永遠不可能全載入。記憶體只跟已載入數成正比
3. **一個 zone = 一個 `entt::registry`**，root 永久存活，子 zone 按需 load/unload
4. **一 zone 一檔**：`FolderZoneStore` 預設後端，路徑由 key 推導，不另存全域清單
5. **Area 地形是稠密網格不是 entity**：`AreaTerrain` 是掛在單例 entity 的 `tdarray<Tile>`，不走 per-tile entity
6. **存檔：cereal 序列化整個 registry**，新增 component 務必登記 `all_components.h`
7. **Ruleset（規則庫）process-resident**，不進存檔；system 透過 `registry.ctx()` 取得

### 關鍵 gotcha

- `all_components.h` 是序列化唯一來源——新增/刪除 component 忘記改它，存檔會悄悄漏掉
- `AreaTerrain` 不能放 `registry.ctx()`（zone_io 不序列化 ctx），要掛在 entity 上走 snapshot
- 新建的 zone 必須有一個帶 `ZoneMeta` 的 placeholder entity，否則 `orphans()` 會清空
- WorldConfig 是 root 上的 singleton component，隨 root 存檔——world_dim 一經決定整局不可改（被烤進 ZoneKey 座標語意）
