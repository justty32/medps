# medps 生命週期與分層（lifecycle）

> 狀態：定案部分留存如下（2026-05-30）。本檔記錄「medps 作為 GDExtension 被使用」的整體流程與分層決策。
> 相關：[zone_layers.md](zone_layers.md)（zone 分層）。
> 註：ruleset 那層尚未實作，下列為已拍板的設計方向。

## 1. 兩個 scope（已定案）

medps 的狀態分成兩個生命週期完全不同的層：

| 層 | 生命週期 | 內容 | 是否進存檔 |
|---|---|---|---|
| **Ruleset（規則庫）** | process 級，常駐 | 靜態規則資料：地形種類、種族屬性、科技樹、神祇定義… | 否（存檔只存 id） |
| **GlobalManager（世界狀態）** | per-game，可換 | root + 各 zone 的 `entt::registry`、世界實體 | 是（序列化） |

**原則：規則庫常駐、GlobalManager 可重建。** 回主選單時丟掉 GlobalManager，規則庫留著。

## 2. Lifecycle 狀態流

```
process 啟動（Godot 載入 .so，register_types）
        │
        ▼
   init()  ── library 級準備：載入 Ruleset（一次，常駐）
        │     （其餘職責尚未定：log / 亂數種子 / 執行緒池 …）
        ▼
   ┌─ 主選單 ─┐
 新遊戲      載存檔
   │          │
 new GM     load_root()   ← 只載 root，其餘 zone lazy（方案 b）
 init_world      │
   └────┬─────┘
        ▼
     遊戲進行：tick() / load() 按需載入
        │
      存檔：save_all()
        │
   回主選單 → 丟掉 GlobalManager（Ruleset 留著）→ 可再 新遊戲/載存檔
```

## 3. 載入存檔 = root + lazy（方案 b，已定案）

- `load_root()` 只把 root registry 讀進來：world 全局實體（faction/神祇/家族）+ `WorldConfig`。
- World/Region/Area 維持留在磁碟，靠 `load()` 按需載入。
- **不**一次全載進記憶體（方案 a 已否決：大世界會爆記憶體）。
- zone 的階層關係由 `ZoneKey` 與 `parent_of()` 的整數除法推導（World ⊃ Region ⊃ Area），
  磁碟 path 由 key 推導（見 `FolderZoneStore`）；不另存全域 zone 清單或 child 索引。

## 4. Ruleset → systems 的接線（已定案：C 案 — entt ctx）

用 EnTT 的 `registry.ctx()`：GlobalManager 在 `create()/load()` 一個 registry 時注入
`reg.ctx().emplace<const Ruleset*>(&ruleset)`；system 維持 `void(entt::registry&)` 簽名，
內部用 `reg.ctx().get<const Ruleset*>()` 取得。**不必改簽名**，ruleset 隨 registry 流動。

- `root` 是 GlobalManager 直接持有、不經 `create()` 的 registry，ctx 注入要**另外補一份**。
- ctx 不會被 zone_io 序列化（見 `components/area_terrain.h` 注解），所以 ruleset 指標絕不會誤入存檔。
- 所有權：Ruleset 是 process-resident，GlobalManager 只**參照**它。

## 5. Ruleset 資料來源（已定案：傳檔案路徑，dll 自己讀）

gcore 的 ruleset 載入器**吃檔案路徑**，由 .dll 自己開檔讀 bytes。前端只傳路徑字串。

```cpp
Ruleset ruleset::load(const std::filesystem::path& path);
```

- standalone / test：直接傳 `data/...` 路徑。
- gbind / Godot：用 `ProjectSettings.globalize_path()` 把 res://、user:// 換成絕對 OS 路徑傳進來。
- **約束**：規則庫檔案必須有真實 OS 路徑，不能打包進 `.pck`（pck 內無 OS 路徑）。
  → 規則庫放外部資料夾 / `user://` / 散檔；對「可被 modder 替換」反而是優點。
- `data/` = standalone/test 時的規則庫檔案放置處。

## 6. 待決議

1. **Ruleset 檔案格式**：尚未定（文字手寫 vs 工具產生的二進位）。
2. **誰持有 Ruleset**：process 級外殼（init/new_game/load_game 的家），形式未定。
3. **init() 的其餘職責**：尚未定。
