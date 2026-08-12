# docs — 專案文件

← [INDEX](../INDEX.md)

medps 的內容文件都收在這裡（路由檔 AGENTS/WORKFLOWS/INDEX 與工作流 `workflows/` 仍在 repo 根）。

| 子目錄 | 內容 |
|--------|------|
| [work/](work/) | 既有設計/架構分析文檔：`design/`（zone 分層、生命週期）、`architecture/`（gcore 總覽）、`progress_overview.md`（進度白話總覽）|
| [references/](references/) | 外部庫教學與 how-to：entt、cereal、godot GDExtension、component/system 新增、zone streaming 架構 |

## 母檔／子檔慣例

專案有**文件單檔 8 KB 上限**（見 [AGENTS.md](../AGENTS.md) 鐵律）。超標的文件按主題拆開，規則是：

- **原檔名保留為母檔**，內容變成索引（各節摘要＋連往子檔的連結）。既有連結因此一律不會斷。
- 子檔用**衍生檔名**（`<母檔名>_<主題>.md`）放在同一目錄。
- 拆檔是純搬運，**不濃縮內容**——教學與決策紀錄的價值就在細節。

2026-08-12 依此拆過：`work/design/tome4_recommendations.md`（→5 子檔）、`work/architecture/gcore_overview.md`（→2）、`work/progress_overview.md`（→`architecture/world_actor_game.md`）、以及 `references/` 的 entt／cereal／godot 三份教學。

> 重寫前的舊原型碼與草稿不在 docs，而在 [`../projects/archived/`](../projects/archived/)（非源碼，已凍結）。

> 新的調查/設計/計畫走對應工作流（investigation / specs / plans），見 [WORKFLOWS.md](../WORKFLOWS.md)；既有 `work/` 文檔先保留、用連結引用，不強制搬遷。
