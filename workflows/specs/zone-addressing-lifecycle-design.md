# zone 定址與生命週期 — 設計方案

- 討論日期：2026-07-22
- 來源：[tome4 報告](../../docs/work/design/tome4_recommendations.md) §2-D（生命週期）＋ §2-F（定址與跨 zone 引用），即 §4 step 2 的兩道 P0 設計題。
- 狀態：**已拍板並落地（2026-07-22）**。拍板結果見下節；與原案不同處以使用者裁定為準，原 §2-§9 內文降級為參考草稿。

> 本檔案因單檔 8 KB 上限拆分為母檔＋子檔（2026-08-12）。內容零遺失，全文完整分散於下列子檔（「下節」＝[拍板結果與決策總表](zone-addressing-lifecycle-design-rulings.md)），本檔只留狀態摘要與索引。

`Done when:` 定址（id 形態/配發/children/跨 zone 引用/返回座標/傳送）與生命週期（persistence/卸載）的每個語意問題都有拍板或綁觸發條件的 defer；本輪落地範圍與不做範圍明確；使用者過目後可直接展開 plan。（此判準已達成，見下列子檔的落地紀錄。）

## 子檔索引

1. [拍板結果與決策總表](zone-addressing-lifecycle-design-rulings.md) — 權威拍板結果表（逐條裁定，含定案/已落地/defer 狀態）＋§1 前提（不重議）＋§2 決策總表（12 題全覽）。
2. [決策詳述：定址與生命週期機制](zone-addressing-lifecycle-design-mechanics.md) — §3.1-3.6：id 配發、manifest 與開檔協定、檔案佈局分桶、children 與結構性返回、persistence 兩態、LRU 卸載。
3. [決策詳述：跨 zone 引用與入口契約](zone-addressing-lifecycle-design-cross-refs.md) — §3.7-3.13：跨 zone 實體引用、返回座標與入口契約、Portal、fail-fast 套件與檔案原子性、tick 重入禁令、存檔槽語意、Zone* 生存期契約。
4. [驗收不變量、序列化版本規劃與落地紀錄](zone-addressing-lifecycle-design-followup.md) — §4-9：驗收不變量、序列化影響與版本規劃、落地順序、不做範圍、懸置事項、風險。

## 前提（不重議，摘要）

一切皆 zone；root（id=0）永駐；z 不進 zone id；絕不全載；行為不進存檔；fail-fast 附 zone id；返回座標不可是全域單一欄位。完整文字見[拍板結果與決策總表](zone-addressing-lifecycle-design-rulings.md)。

## 驗收不變量（摘要）

> 任何持久或常駐結構的成長軸，只允許是「已載入 zone 數」（RAM）或「已造訪 zone 數」（磁碟）；絕不允許是世界總 zone 數。

完整推導與後續章節（序列化版本規劃、落地順序、不做範圍、懸置、風險）見[驗收不變量、序列化版本規劃與落地紀錄](zone-addressing-lifecycle-design-followup.md)。

## 相關

- [World：第一個 Zone 子類 — 設計方案](world-zone-subclass-design.md) 的格式變更政策沿用本檔拍板（見[拍板結果與決策總表](zone-addressing-lifecycle-design-rulings.md)）。
