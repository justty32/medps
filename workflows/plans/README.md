# plans — 實作規劃入口

真的要動工前的詳細實作規劃：精確到檔案、步驟、測試、驗證。

規劃階梯：

```text
idea → roadmap → spec → plan → feature-dev
```

## 規則

- 開始前寫 `Done when: <每個 task、檔案、測試與驗證都足以直接動工>`。
- 本夾 `*.md` = 各功能的逐步實作計畫。
- 建議命名：`<feature>.md`。
- 對應設計方案：`specs/<feature>-design.md`。
- 計畫要切成 bite-sized task，每步都有驗證。
- 落地或被取代後移到 `archive/`。

## 現役計畫

| 計畫 | 出計畫日期 | 對應 spec | 狀態 |
|------|------------|-----------|------|
| 無 | - | - | - |

## 何時不用

- 小改動已能直接安全實作，走 feature-dev。
- 設計還沒定，走 specs。
- 只是排優先順序，走 roadmap。
