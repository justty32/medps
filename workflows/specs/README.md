# specs — 設計方案入口

一個 idea/roadmap 項認真討論後產出的設計方案：目標、架構、資料流、權衡、取捨。

規劃階梯：

```text
idea → roadmap → spec → plan → feature-dev
```

## 規則

- 開始前寫 `Done when: <設計問題被回答，取捨/不做範圍清楚，可進 plan>`。
- 本夾 `*.md` = 各功能的設計方案。
- 建議命名：`<feature>-design.md`。
- 設計涉及 code 結構時參考 [common/conventions](../common/conventions.md)。
- 落地或被取代後移到 `archive/`。

## 現役設計方案

| 設計方案 | 討論日期 | 對應 idea/roadmap | 狀態 |
|----------|----------|-------------------|------|
| 無 | - | - | - |

## 何時不用

- 小功能已清楚可直接做，走 feature-dev。
- 已決定設計，只缺實作步驟，走 plans。
- 還只是靈感，走 idea 或 roadmap。
