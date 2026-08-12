# zone 定址與生命週期 — 拍板結果與決策總表

> 子檔（拆分於 2026-08-12，原內容為 [zone-addressing-lifecycle-design.md](zone-addressing-lifecycle-design.md) 的「拍板結果」表＋§1 前提＋§2 決策總表）。狀態：**已拍板並落地（2026-07-22）**。
> 導覽：[← 回母檔索引](zone-addressing-lifecycle-design.md) ｜ 下一篇：[決策詳述：定址與生命週期機制](zone-addressing-lifecycle-design-mechanics.md)

## 拍板結果（權威，逐條經使用者裁定）

| 題目 | 裁定 | 狀態 |
|------|------|------|
| 版本欄位 | **全滅**：zone 檔、manifest 都不帶 magic/version（原 P0 Task 1 已放棄）。格式變更＝手動刪存檔目錄，重寫期成本使用者自負 | 定案 |
| id 配發 | `create_child(parent)` 內部 `next_id_++` 單點配發（0 保留 root、永不復用）；`create` 收 private（`emplace_zone`）；配發撞既有檔 → throw | **已落地** |
| manifest.bin | 只存 `next_zone_id`；tmp+rename 原子寫；**定位：未來擴充成這份存檔的 metainfo 檔** | **已落地** |
| 開檔協定 | manifest 存在 → 還原 next_id＋必須讀回 root.bin（缺失 throw）；無 manifest 但有 .bin → throw；乾淨 → 新世界 | **已落地** |
| fail-fast | load 後驗檔內 id＝請求 id；destroy 同步刪盤上檔案 | **已落地** |
| 文件約定三條 | tick 重入禁令、單槽活儲存、Zone* 不跨 tick 持有——已寫進 zone_manager.h 註解 | **已落地** |
| 目錄分桶 | **不做**，痛了再說（重寫期存檔隨時作廢，屆時遷移＝刪檔重玩） | defer |
| persistence 兩態 | **不做**，目前全部 Persistent、連 enum 都不建。觸發：第一個產生一次性地圖的玩法。另記使用者需求：**未來要做「清理很久沒訪問且不重要的 zone .bin」機制**，屆時與此題一起設計 | defer |
| LRU 卸載 | **不做**（touch/pinned/budget 全套不建）。觸發：第一個大量載入 zone 的玩法／記憶體實際成為問題；可與上條「久未訪問就處理」一族合併設計 | defer |
| children／跨 zone 引用／返回座標／Portal | **不在此輪凍結**：原 §3.4/3.7/3.8/3.9 降為參考草稿，屆時動工前逐條重審 | defer |
- 產出方法：三取向設計面板（最小增量／ToME 對齊／4X 尺度）＋雙評審交叉評分。兩位評審一致選最小增量案為骨架（91/89 分），本文檔 = 骨架＋自另兩案併入的 11 條補強＋評審點名漏洞的補拍板。設計責任在本文檔，不在面板。

`Done when:` 定址（id 形態/配發/children/跨 zone 引用/返回座標/傳送）與生命週期（persistence/卸載）的每個語意問題都有拍板或綁觸發條件的 defer；本輪落地範圍與不做範圍明確；使用者過目後可直接展開 plan。

## 1. 前提（已拍板，不重議）

一切皆 zone；root（id=0）永駐；z 不進 zone id（z = `Zone::layers` 鍵 + `Position::z`，後者由 [P0 計畫](../plans/save-format-position-z.md) 先行落地）；絕不全載；行為不進存檔；fail-fast 附 zone id；返回座標不可是全域單一欄位。

## 2. 決策總表

| # | 題目 | 拍板 | 落地時機 |
|---|------|------|----------|
| 1 | zone id 形態 | 裸 `uint64_t` 單調序號，零語意；`create_child` 單點配發 | spec 落地回合 |
| 2 | 世界 manifest | 新檔 `manifest.bin`：`format_version` + `next_zone_id`（僅此二欄，絕不放 zone 清單）；tmp+rename 原子寫 | spec 落地回合 |
| 3 | 檔案佈局 | 目錄分桶 `dir/<hex 前 2 碼>/<16hex>.bin` **本輪就做**（pre-v1 零資產是免費窗口） | spec 落地回合 |
| 4 | parent→children | 掛 parent zone 的 `ChildLink` 表＋child 側 `anchor` 鏡像；**絕不做全域索引** | defer：首個 parent→child 消費者 |
| 5 | persistence | 兩態 `ZonePersistence{Ephemeral, Persistent}`，預設 Persistent；medps 自定語意，不抄 ToME 四態 | spec 落地回合 |
| 6 | 卸載 | LRU 軟上限（預設 0=不設限），tick 尾端逐出；`pinned` 執行期旗標防呆 | spec 落地回合 |
| 7 | 跨 zone 實體引用 | `StableId` uid + `EntityRef{zone, uid}` 弱引用，解引用兩段可失敗 | defer：首個 root→zone 內引用需求 |
| 8 | 返回座標 | 結構性返回（anchor 推導）優先；`ReturnTrail` component 只管非結構性進入 | defer：首個 zone 進出系統 |
| 9 | 傳送 | Portal entity＋`Mode` 枚舉（不用座標哨兵） | defer：同上，與 8 同回合 |
| 10 | 存檔槽語意 | 單槽活儲存，明文化 | 即刻（文件約定） |
| 11 | tick 重入 | system 內禁止 zone 結構性變更 | 即刻（文件約定） |
| 12 | Zone* 生存期 | 不跨 tick 持有；升級判準明定 | 即刻（文件約定） |

## 導覽

[← 回母檔索引](zone-addressing-lifecycle-design.md) ｜ 下一篇：[決策詳述：定址與生命週期機制](zone-addressing-lifecycle-design-mechanics.md)
