# ToME4 架構研讀 → medps 重寫建議報告 —— §3-§6 落地狀態／順序／存疑處／總結

> 本檔是 [tome4_recommendations.md](tome4_recommendations.md) 的拆分子檔，收錄 §3 舊設計意圖清點（落地狀態檢查表）、§4 建議落地順序、§5 語料存疑處、§6 一句話總結，為本報告最後一段。上一節：[§2 G-I（排程／生成管線／方法論）](tome4_recommendations_scheduling_pipeline.md)；回索引：[tome4_recommendations.md](tome4_recommendations.md)。
>
> ⚠ 本檔內容為 2026-07-22 當時的狀態快照，依專案慣例歷史紀錄不追改；§3 表格中「16 項測試全綠基準」一行等處已知與現況（測試基準已改為 21 項）不符，讀者請以母檔索引區的「落地現況」小節為準。

## 3. 舊設計意圖：推翻 vs 存續

這輪重構後，`docs/work/` 的四份文檔**全部**不同程度過時（含 lifecycle.md——其兩-scope 原則存續，但文中 GlobalManager/load_root/WorldConfig/ZoneKey 推導等段落引用的都是已刪之物）。逐項清點：

| 舊意圖 | 狀態 | 備註 |
|---|---|---|
| ZoneKey 位元打包、parent 整除推導 | **已推翻** | parent 改顯式欄位；失去的三樣東西見 §2-F（[E-F 檔](tome4_recommendations_save_addressing.md)） |
| 不同 z = 不同 zone | **已推翻** | z 收進 `Zone::layers`；連帶產生 §2-A 的 Position 缺口（[A-D 檔](tome4_recommendations_p0_core.md)） |
| 地圖=component 走 snapshot | **已推翻** | layers 掛 struct，`zone_io` 另行序列化（已實作） |
| ZoneMeta placeholder 防 orphans 清空 | **已推翻** | 身分改由 struct 承載；殘餘風險見 §2-E-2（[E-F 檔](tome4_recommendations_save_addressing.md)） |
| 「底層型別不叫 Zone」命名戒律 | **已推翻** | 現行就叫 `struct Zone`，讀舊文檔時注意措辭對映 |
| WorldConfig root singleton / world_dim 凍結 | **已推翻** | 隨 ZoneKey 刪除；新定址方案可自由重新決定 |
| 16 項測試全綠基準 | **已重建（2026-07-22）** | 整套重寫為對應新核心的 15 項（含 orphans 機制驗證與 ZoneManager 損毀防護），15/15 綠 |
| 絕不全載、記憶體正比於已載入數 | **存續** | §2-D 的硬約束（[A-D 檔](tome4_recommendations_p0_core.md)） |
| create/load/unload/save_all API 形狀 | **存續** | ZoneManager 與舊 GlobalManager 同形（唯 load_root 已無對應物） |
| Ruleset 兩-scope 生命週期原則 | **存續（文本過時）** | process-resident / ctx 注入 / 規則檔吃 OS 路徑等方向有效；lifecycle.md 內文的 GlobalManager、load_root()、WorldConfig、ZoneKey 段落需對映到新 API 讀。`Tile::terrain` 作為 def id 索引 Ruleset 的語意仍適用 |
| AllComponents 單一來源 + entt_id_t 可攜互轉 | **存續** | 鐵律延續 |
| 三層尺度數字（200²/15²/250²）與三種時間模型 | **存續** | 唯一寫下遊戲設計意圖的資料；§2-G 的能量制可統一表達三種時間模型（[G-I 檔](tome4_recommendations_scheduling_pipeline.md)） |
| 舊開啟事項四條（跨層連通/跨 zone 身份/世界邊緣/分派 tick） | **全部仍懸置** | 分別對應本報告 §2-H-9 / §2-F / （未涉及） / §2-G |

## 4. 建議落地順序

0. **前置（P0）**：重寫測試套件對齊新架構、重定綠色基準——不做這步，後面所有「behavior-preserving」都無從驗證（AGENTS.md 鐵律的前提）。保留舊套件的 serialize_roundtrip / orphans 兩類測試精神。
1. **馬上（P0，小成本）**：存檔版本欄位（§2-E-1）；Position 補 z（§2-A）；round-trip + entity 數比對測試（§2-E-2，併入 step 0 的新套件）。（§2-A/E 見 [A-D 檔](tome4_recommendations_p0_core.md) / [E-F 檔](tome4_recommendations_save_addressing.md)）
2. **下一個設計回合（P0，設計題）**：zone 定址語意（§2-F）——決定 id 形態、跨 zone 引用要不要 uid、返回座標的 per-zone 記錄；生命週期策略（§2-D）——persistence 枚舉（以四態版為藍本）+ LRU。這兩題互相牽動，建議一起談。
3. **有第一個玩法系統時（P1）**：能量制排程（§2-G）；空間索引 + move() 收口（§2-B）；Blocking 重建（§2-C）；ZoneManager 讀檔流程預留 post-load pass（§2-E-3）；**先定生成器介面形狀**（§2-H：spots 欄位、以 layer 為生成單位、跨層連通驗證、可非同步）。（§2-B/C 見 [A-D 檔](tome4_recommendations_p0_core.md)；§2-G/H 見 [G-I 檔](tome4_recommendations_scheduling_pipeline.md)）
4. **開始做內容時（P2）**：生成管線實作（§2-H）；定義/實體化分離與 Ruleset 接線（§2-H-8 + lifecycle.md 原則）；背景存檔（§2-E-4，按需）。

## 5. 語料存疑處（引用前需複驗）

- `persistent` 的枚舉值兩處文件**衝突**：architecture 層三態（`"zone"|"level"|false`）vs tutorial 層四態（`false|"memory"|"zone"|true`，且「逐層存檔」對應 `true` 而非 `"level"`）。本報告 §2-D 採較細的四態版；裁決需回原始碼（前置見 §0，[background 檔](tome4_recommendations_background.md)）。
- `decay` 與 persistent 的互動、LRU `enableLastPersistZones` 的逐出時機/是否寫盤：語料未載。
- 「resolver 展開結果進存檔、規則不進存檔」是**推論**（由「resolver 依附定義、生成期 resolve」推得），語料無直接證據；「已生成但含未 resolve 欄位的實體被存檔」的邊角情況語料全無描述。
- Cavern 演算法兩份文件說法不一（Perlin+flood-fill vs 細胞自動機）；GOL 代數不一（3 代 vs 多代）；生成器清單可能不完整（範例用的 `Roomer` 不在清單上）。
- Level 的欄位（`e_array`/`entities`/`spots`/`sublevels`）僅單一文件來源，無交叉驗證。
- SavefilePipe 是 coroutine 分批，語料**無**「背景執行緒」的證據——別把 ToME 想得比實際多執行緒。
- `docs/references/` 底下有一篇 zone streaming 架構教學**本次未讀**，與 §2-D 直接相關，值得補讀。
- analysis 層的 zone 統計（89 個、Infinite Dungeon `max_level=10^9` 等）無行號複驗，採信降一級。

## 6. 一句話總結

ToME4 用一個不起眼的 Zone 抽象撐起了約 89 個 zone（世界大地圖也是其中之一），證明 medps「一切皆 zone」的骨架是對的；它真正值得抄的不是資料結構（Lua 的實體湯 medps 不該學），而是**策略聲明在資料上（persistent/generator）、失敗是一級公民（連通性驗證、fail-fast）、行為不進存檔（定義層規則生成期展開、存檔只存結果）**這三條紀律——以及它用一個全域 `wild_x/wild_y` 欄位和一堆靜默失敗換來的反面教材。

---
上一節：[§2 G-I（排程／生成管線／方法論）](tome4_recommendations_scheduling_pipeline.md)　|　回索引：[tome4_recommendations.md](tome4_recommendations.md)
