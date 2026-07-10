# create — 衍生小專案入口

基於 analysis/research 產物建立獨立小專案。此類專案以獨立存在為目標，不預設套回原專案。

## 前置條件

- 已有來源分析：`analysis/<source>/`、research finding、或等價材料。
- 使用者已同意建立獨立專案，而不是直接改原 repo。

## 產物結構

建議放在 `derived/<project>/`：

| 路徑 | 內容 |
|------|------|
| `PROJECT.md` | 目標、來源、技術棧、完成定義 |
| `session_log.md` | 一句話操作日誌，建議上限 50 行 |
| `src/` | 源碼 |
| `tests/` | 測試 |
| `docs/` | 設計決策、實作說明 |
| `docs/decisions/` | 重要決策與追溯來源 |

## PROJECT.md 必答

- 源材料：分析的是哪個專案/文件/論文？
- 衍生目標：要解決什麼問題，或驗證什麼概念？
- 參照素材：主要參考哪些 analysis/research 文件？
- 技術棧：用什麼語言/框架？
- 完成定義：什麼狀態算做完？

## 流程

開始前寫一句：

```text
Done when: <獨立專案達到可執行/可測/可展示的完成條件>
```

1. 建立 `derived/<project>/` 結構。
2. 初始化 `PROJECT.md` 與 `session_log.md`。
3. 依技術棧初始化骨架。
4. 實作時把重要借鑑寫入 `docs/decisions/`。
5. 每個功能點完成後更新日誌；里程碑更新 `PROJECT.md`。

## 追溯連結格式

```md
## 設計決策：<標題>

**參照來源**：`analysis/<source>/architecture/level2_xxx.md`
**借鑑概念**：<從來源學到什麼>
**實作方式**：<在衍生專案中如何應用>
```

## 何時不用

- 修改應回到原 repo，走 feature-dev 或 patch。
- 只是概念還沒確定，先走 idea/spec。
- 只是研究來源材料，走 analysis 或 research。
