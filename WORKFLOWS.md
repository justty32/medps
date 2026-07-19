# WORKFLOWS — 工作流派發器

使用者說要做某件事 → 從這張表選對應工作流 → 讀入口檔 → 再動手。

先讀 [PRINCIPLES.md](PRINCIPLES.md) 的跳流程規則：小改可直接做，工作流只在它能降低交接、同步或設計風險時啟用。

## 你想做什麼 → 用哪個工作流

| 觸發 | 工作流 | 入口檔 |
|------|--------|--------|
| 「我親自改了 code / 我留了 DAVID 註解，你接手」 | resync | [workflows/resync.md](workflows/resync.md) |
| 「我想開發/修改某個 feature」 | feature-dev | [workflows/feature-dev/README.md](workflows/feature-dev/README.md) |
| 「重構/拆檔/整理結構」 | refactor | [workflows/refactor/README.md](workflows/refactor/README.md) |
| 「調查現有系統/外部專案/可行性」 | investigation | [workflows/investigation/README.md](workflows/investigation/README.md) |
| 「初次接觸陌生專案，建立可延續分析」 | analysis | [workflows/analysis.md](workflows/analysis.md) |
| 「基於分析產物做獨立衍生小專案」 | create | [workflows/create/README.md](workflows/create/README.md) |
| 「做一包可套用到原專案的 patch」 | patch | [workflows/patch/README.md](workflows/patch/README.md) |
| 「讀 paper / 長文 / 技術資料並建索引」 | research | [workflows/research/README.md](workflows/research/README.md) |
| 「把 idea 討論成設計方案」 | spec | [workflows/specs/README.md](workflows/specs/README.md) |
| 「把設計方案展開成動工計畫」 | plan | [workflows/plans/README.md](workflows/plans/README.md) |
| 「記一個不確定要不要做的想法」 | idea | [workflows/idea/ideas.md](workflows/idea/ideas.md) |
| 「記一件確定會做、不確定何時的事」 | roadmap | [workflows/roadmap/README.md](workflows/roadmap/README.md) |
| 「.md 太多，想做瀏覽導覽層」 | html-guide | [workflows/html-guide/README.md](workflows/html-guide/README.md) |
| 「跑測試/確認驗證方式」 | testing | [workflows/testing.md](workflows/testing.md) |
| 「設定/了解開發環境」 | dev-env | [workflows/dev-env.md](workflows/dev-env.md) |
| 「使用/設定外部工具、env var、依賴」 | tooling | [workflows/tooling/README.md](workflows/tooling/README.md) |

規劃管線：

```text
idea → roadmap → spec → plan → feature-dev
```

外部材料管線：

```text
analysis/research → create 或 patch 或 roadmap/spec
```

## 統一形式

- 資料夾型工作流：入口 `README.md`，視需要加 `gotchas.md`、`session-log.md`、`archive/`。
- 單檔工作流：一個 `.md` 同時是入口與內容；膨脹後升級成資料夾。
- `archive/` 放過時、已落地、被取代但值得保留脈絡的文檔；不在現役維護鏈。
- 非微小工作先寫 `Done when:`，完成定義要可驗證，並說清楚不包含什麼。

## 跨工作流活狀態

- open/in-flight 進度 → [SESSION-LOG.md](SESSION-LOG.md)
- 等使用者親自做/驗證 → [WAIT_USER.md](WAIT_USER.md)
