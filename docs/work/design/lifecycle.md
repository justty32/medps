# medps 生命週期與分層

> 最後更新：2026-07-22（名詞對齊 Zone/ZoneManager 新核心）。Ruleset 層尚未實作，以下為已拍板的設計方向。

## 兩個 scope

| 層 | 生命週期 | 內容 | 是否進存檔 |
|---|---|---|---|
| **Ruleset（規則庫）** | process 級，常駐 | 靜態規則資料：地形種類、種族屬性、科技樹、神祇定義... | 否（存檔只存 id） |
| **ZoneManager（世界狀態）** | per-game，可換 | root + 各 zone 的 `Zone`（registry＋地圖）、世界實體 | 是（序列化） |

**原則：規則庫常駐、ZoneManager 可重建。** 回主選單時丟掉 ZoneManager，規則庫留著。

## Lifecycle 狀態流

```
process 啟動 → init() 載入 Ruleset（一次常駐）
    │
  主選單：新遊戲 / 載存檔
    │
  new ZoneManager{dir}（開檔協定自動分流：乾淨目錄=新世界；
  有 manifest=既有存檔，root 自動讀回，其餘 lazy）
    │
  遊戲進行：tick() / load() 按需載入
    │
  存檔：save_all()（另 unload 也隨時寫檔——單槽活儲存語意）
    │
  回主選單 → 丟掉 ZoneManager（Ruleset 留著）
```

## 載入存檔 = root + lazy

- 建構 `ZoneManager{dir}` 即完成「只載 root」：root 的全局實體（faction/神祇/家族）自動還原
- 其餘 zone 維持在磁碟，靠 `load(id)` 按需載入，**不一次全載**（大世界會爆記憶體）
- 磁碟 path 由 id 推導（`<16hex>.bin`），不另存全域 zone 清單；`manifest.bin` 只存 id 計數器

## Ruleset → systems 的接線

用 EnTT `registry.ctx()` 注入 `const Ruleset*`：
- ZoneManager 在建 zone / load zone 時注入 ctx（root 也要）
- system 維持 `void(Zone&)` 簽名，內部 `z.reg.ctx().get<const Ruleset*>()`
- ctx 不會被 registry_io 序列化（存不到檔案裡），所以每次載入都要重新注入
- Ruleset 所有權屬 process-resident，ZoneManager 只參照

## Ruleset 資料來源

gcore 載入器**吃檔案路徑**，自行開檔讀 bytes：
```cpp
Ruleset ruleset::load(const std::filesystem::path& path);
```
- standalone/test：直接傳 `data/...` 路徑
- gbind/Godot：用 `ProjectSettings.globalize_path()` 把 res://、user:// 換成絕對 OS 路徑
- **約束**：規則庫檔案必須有真實 OS 路徑，不能打包進 `.pck`
- **待決議**：檔案格式（文字手寫 vs 工具產生的二進位）、誰持有 Ruleset（process 級外殼形式未定）、`init()` 其餘職責
