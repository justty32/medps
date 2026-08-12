# World：第一個 Zone 子類 — 設計方案

- 討論日期：2026-07-22
- 來源：使用者發起「實作第一個繼承於 Zone 的 class: World」；參考源 `~/repo/pas/analysis/python-tcod/`（libtcod worldgen cookbook）與 [tome4 報告](../../docs/work/design/tome4_recommendations.md)。
- 狀態：**已拍板並落地（2026-07-22）**。三道核心題經使用者拍板（範圍＝worldgen／演算法源＝libtcod headless／序列化＝type tag＋工廠）後，使用者裁定跳過 plan 直接動工；§7 四步全部完成，測試 19/19 綠。落地偏差記於文末「落地備註」。

> 本檔案因單檔 8 KB 上限拆分為母檔＋子檔（2026-08-12）。內容零遺失，全文完整分散於下列子檔（「§7」「文末」等原文內部指涉見對應子檔），本檔只留狀態摘要與索引。

`Done when:` 繼承機制（型別表達、建構、序列化還原）、World 第一版內容（worldgen 管線與資料）、libtcod 依賴引入方式的每個設計問題都有答案或綁觸發條件的 defer；落地順序與不做範圍明確；使用者過目後可直接展開 plan。（此判準已達成，見子檔落地紀錄。）

## 子檔索引

1. [設計本體：拍板結果、背景與繼承機制](world-zone-subclass-design-mechanism.md) — §1 拍板結果（權威）＋§2 設計背景（與既有拍板的關係）＋§3 繼承機制（Zone 改造/建構路徑/zone 檔格式）＋§4 World 類與 generate() 管線＋§5 libtcod 依賴引入。
2. [測試、落地順序與執行結果](world-zone-subclass-design-execution.md) — §6 測試＋§7 落地順序＋§8 不做範圍＋落地備註（與 spec 原文的偏差）＋§9 風險。

## 相關

- 格式變更政策沿用 [zone 定址與生命週期 — 拍板結果與決策總表](zone-addressing-lifecycle-design-rulings.md)：不帶 version 欄位，格式變更＝手動刪存檔目錄。
