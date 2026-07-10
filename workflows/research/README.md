# research — 論文/長文閱讀入口

用於讀 paper、規格、長文技術材料，建立可追蹤索引、摘要、翻譯、深掘與 backlog。

## 產物結構

建議放在 `research/` 或主題資料夾下：

| 路徑 | 內容 |
|------|------|
| `index.md` | 研究條目總表，含狀態 |
| `raw/` | 原始 PDF/HTML/text，通常 gitignore |
| `summarize/` | 每篇重點摘要 |
| `translate/` | 全文或節選翻譯，按需 |
| `deep/` | 跨文獻綜述、主題深掘 |
| `backlog.md` | 候選池與優先順序 |
| `session_log.md` | 一句話操作日誌，建議上限 50 行 |
| `html/` | 選用導覽層 |

## 流程

開始前寫一句：

```text
Done when: <指定條目處理完、index 狀態更新、摘要/翻譯/深掘產物落檔>
```

1. 在 `index.md` 建條目，狀態用 `⬜/✅/⏸️/🧊` 或專案慣例。
2. 取得原文，保存來源、URL、paper id、下載日期。
3. 自己讀原文或可驗證抽取文字；不要只信搜尋摘要或二手轉述。
4. 產出 `summarize/`，需要時產出 `translate/`。
5. 更新 `index.md` 狀態與分類。
6. 多篇形成主題時，整理到 `deep/`。
7. 候選但未處理的放 `backlog.md`，不要混進已完成結論。

## 摘要模板

```md
# <paper id> <title> 摘要

## 一句話

## 問題

## 方法

## 關鍵結果

## 限制

## 對本專案的價值

## 來源
- paper: `<id or URL>`
- local raw: `raw/...`
```

## 規則

- 最新資訊、paper metadata、URL、版本號要可驗證。
- 候選池與已讀結論分開。
- 大量條目時用分類子索引，頂層 `index.md` 只做 hub。
- HTML 導覽層只呈現，不取代 `.md` 真相層。

## 何時不用

- 只是查一個即時事實且不需留 durable 知識，直接查證回答。
- 是外部 repo 結構分析，走 analysis。
- 結論已成熟要實作，升 specs/plans/feature-dev。
