# zone 定址與生命週期 — 驗收不變量、序列化版本規劃與落地紀錄

> 子檔（拆分於 2026-08-12，原內容為 [zone-addressing-lifecycle-design.md](zone-addressing-lifecycle-design.md) §4-§9）。狀態：**已拍板並落地（2026-07-22）**。拍板結果總表見 [拍板結果與決策總表](zone-addressing-lifecycle-design-rulings.md)。
> 導覽：[← 決策詳述：跨 zone 引用與入口契約](zone-addressing-lifecycle-design-cross-refs.md) ｜ [回母檔索引](zone-addressing-lifecycle-design.md)

## 4. 驗收不變量（後續所有 F/D 相關變更的審查準則）

> 任何持久或常駐結構的成長軸，只允許是「已載入 zone 數」（RAM）或「已造訪 zone 數」（磁碟）；**絕不允許是世界總 zone 數**。

manifest 不放 zone 清單、children 掛 parent、uid_index 只在已載入 zone 上、ReturnTrail 掛實體，全部由此推出。

## 5. 序列化影響與版本規劃

- **v1**（[P0 計畫](../plans/save-format-position-z.md)，先行）：magic + format_version 進 zone 檔頭；Position 補 z。
- **v2**(本 spec 落地回合):`Zone::persistence` 進第一塊;`manifest.bin` 新檔;目錄分桶。
- 之後每個 defer 項落地各自 +1（children/anchor、next_entity_uid、StableId/ReturnTrail/Portal 進 AllComponents、last_saved_tick）。
- 政策：重寫期**版本不符一律 fail-fast，不寫遷移碼**；遷移碼觸發：首個對外可玩版本、存檔成為使用者資產時。AllComponents 清單變動本就默默改變 snapshot 位元組流——版本欄位把「靜默讀壞」變成「大聲拒讀」，這正是它先行的理由。

## 6. 落地順序（拍板後展開為 plan）

1. zone_io v2：persistence 欄位＋序列化。
2. manifest 讀寫＋`next_id_`＋create_child＋create 收 private＋建構子開檔協定＋分桶 path()＋destroy 刪檔＋load 驗 id。
3. unload/save_all 依 persistence 分流（save_all 含 Ephemeral；Ephemeral 逐出刪殘檔）。
4. touch/pinned/set_loaded_budget/enforce_budget＋tick 尾端接線＋三條文件約定註解（§3.11-3.13）。

每步的驗證點併入報告 §4 step 0 的測試套件重建（roundtrip/orphans 精神延續）；測試重建時程依使用者指示另議，plan 屆時以「可編譯＋臨時驗證程式」為過渡驗證。

## 7. 不做範圍

decay、HiLo id 塊配發、`MemoryOnly` 三態、模擬 LOD/能量制（§2-G 另案）、背景存檔、離線補算、存檔遷移碼、root 側 uid forwarding 表、pin 之外的逐出保護機制。各項觸發條件見對應小節。

## 8. 懸置（本 spec 不解，記錄在案）

- root zone 自身的成長軸：全局實體全進 root、恆駐恆 tick、save_all 全量重寫，終局規模與分塊策略未觸及。
- 世界邊緣（玩家走到 world 地圖盡頭）：舊懸案四條之一，仍無人認領。
- 跨檔落盤一致性（parent.children 與 child 檔各自落盤的 crash 窗口）：children 落地回合連同雙向驗證再議。

## 9. 風險

1. **Zone\* 懸置**：逐出析構 Zone；靠 §3.13 契約＋逐出時點收斂（tick 尾端）緩解，升級判準已拍死。
2. **touch/pin 紀律依賴玩法層**：pinned 旗標已是第二道防線；測試套件應含「玩家 zone 未 touch/pin 時不被誤逐」情境。
3. **budget 預設 0 = 出廠不設防**：刻意的 behavior-preserving 選擇；接玩法層時必須設 budget，記入 WAIT_USER 提醒。
4. **Ephemeral 在生成器存在前=逐出即永久消失**：現階段只用於真正可丟的 zone，header 註解明示。
5. **loader.orphans() 吞實體**（報告 §2-E-2，[registry_io.h:30](../../projects/medp/src/gcore/serialize/registry_io.h)）：本 spec 未觸碰未惡化，靠測試套件 entity 數比對覆蓋。

## 導覽

[← 決策詳述：跨 zone 引用與入口契約](zone-addressing-lifecycle-design-cross-refs.md) ｜ [回母檔索引](zone-addressing-lifecycle-design.md)
