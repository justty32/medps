# ToME4 架構研讀 → medps 重寫建議報告（索引）

最後更新：2026-07-22（gcore 大重構之後：ZoneKey/GlobalManager/ZoneStore/ZoneMeta/WorldConfig/AreaTerrain/Blocking 已刪，現行核心為 `Zone{id, parent, reg, layers}` + `ZoneManager`）。

**一句話定位**：研讀 `C:\code\mine\modding_tome4` 語料（ToME4 引擎架構）、產出對 medps 重寫的建議，經三路對抗性覆核（ToME4 事實 / medps 程式碼一致性 / 摘要完整性）修訂。全文因單檔 8 KB 上限已拆為 5 個子檔，本檔只留索引與摘要，內容零遺失——所有段落、表格、程式碼引用、覆核意見都完整搬到對應子檔，逐檔連結見下表。

> 本次拆檔（8 KB 鐵律溯及既往）未改動任何內文，僅新增本索引與各子檔的導覽檔頭；子檔內部的「§2-A」「§4」等既有章節編號全數保留，跨檔引用時請一併標註子檔名。

## 落地現況（本索引新增小節，非原文）

以下三點是 §2 P0 建議在**目前**（非文件當時 2026-07-22）的落地狀態，供讀子檔內文時對照，避免被舊敘述誤導；子檔內文本身**保持原文不動**：

- **Position 補 z**（原 §2-A，見 [p0_core 檔](tome4_recommendations_p0_core.md)）：已落地。
- **Zone 生命週期策略**（原 §2-D，見 [p0_core 檔](tome4_recommendations_p0_core.md)）：已落地。
- **存檔版本欄位**（原 §2-E-1，見 [save_addressing 檔](tome4_recommendations_save_addressing.md)）：使用者已裁定放棄。
- 另，§2-E-2 提到的「16 項測試基準已斷」與 §4 step 0 的「測試套件斷裂」敘述已解除；現行測試基準為 **21 項**（子檔內文仍寫舊數字，為歷史快照，不追改）。

## 各章索引

| 章節 | 內容 | 子檔 |
|---|---|---|
| §0 | 方法與可信度聲明——語料分級、可信度標註 | [tome4_recommendations_background.md](tome4_recommendations_background.md) |
| §1 | 兩邊的世界結構對照——ToME World/Zone/Level/Map vs medps ZoneManager/Zone | [tome4_recommendations_background.md](tome4_recommendations_background.md) |
| §2-A | 位置模型（P0）——Position 補 z | [tome4_recommendations_p0_core.md](tome4_recommendations_p0_core.md) |
| §2-B | 空間索引（P1）——格→實體反查表 | [tome4_recommendations_p0_core.md](tome4_recommendations_p0_core.md) |
| §2-C | 阻擋模型劃界（P1）——Tile flags vs 動態 Blocking | [tome4_recommendations_p0_core.md](tome4_recommendations_p0_core.md) |
| §2-D | Zone 生命週期策略（P0）——persistent/decay/LRU | [tome4_recommendations_p0_core.md](tome4_recommendations_p0_core.md) |
| §2-E | 存檔（P0 兩條＋P1/P2 若干）——版本欄位、測試基準、兩階段載入等 | [tome4_recommendations_save_addressing.md](tome4_recommendations_save_addressing.md) |
| §2-F | 定址與跨 zone 引用（P0 設計題） | [tome4_recommendations_save_addressing.md](tome4_recommendations_save_addressing.md) |
| §2-G | 排程（P1）——能量制 | [tome4_recommendations_scheduling_pipeline.md](tome4_recommendations_scheduling_pipeline.md) |
| §2-H | 生成管線（P2 實作／P1 定介面） | [tome4_recommendations_scheduling_pipeline.md](tome4_recommendations_scheduling_pipeline.md) |
| §2-I | 方法論（貫穿性）——靜默失敗、三層驗證 | [tome4_recommendations_scheduling_pipeline.md](tome4_recommendations_scheduling_pipeline.md) |
| §3 | 舊設計意圖：推翻 vs 存續（落地狀態檢查表） | [tome4_recommendations_status_summary.md](tome4_recommendations_status_summary.md) |
| §4 | 建議落地順序（step 0-4） | [tome4_recommendations_status_summary.md](tome4_recommendations_status_summary.md) |
| §5 | 語料存疑處（引用前需複驗） | [tome4_recommendations_status_summary.md](tome4_recommendations_status_summary.md) |
| §6 | 一句話總結 | [tome4_recommendations_status_summary.md](tome4_recommendations_status_summary.md) |

## 子檔清單（依閱讀順序）

1. [tome4_recommendations_background.md](tome4_recommendations_background.md) —— §0～§1
2. [tome4_recommendations_p0_core.md](tome4_recommendations_p0_core.md) —— §2-A～D
3. [tome4_recommendations_save_addressing.md](tome4_recommendations_save_addressing.md) —— §2-E～F
4. [tome4_recommendations_scheduling_pipeline.md](tome4_recommendations_scheduling_pipeline.md) —— §2-G～I
5. [tome4_recommendations_status_summary.md](tome4_recommendations_status_summary.md) —— §3～§6
