# zone 定址與生命週期 — 設計方案

- 討論日期：2026-07-22
- 來源：[tome4 報告](../../docs/work/design/tome4_recommendations.md) §2-D（生命週期）＋ §2-F（定址與跨 zone 引用），即 §4 step 2 的兩道 P0 設計題。
- 狀態：**待拍板**。拍板後另出 plan 才動工；本文檔只凍結語意。
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

## 3. 決策詳述

### 3.1 id 與配發

- 維持裸 `uint64_t`（[zone_manager.h:15](../../projects/medp/src/gcore/zone/zone_manager.h)），純身分零語意；層級只活在 `Zone::parent`。舊 ZoneKey 三紅利的清算：路徑可推導→hex 檔名保住；免全域索引→manifest 只存計數器保住；自帶座標語意→明確放棄（ToME 短名定址無座標語意，活得很好）。
- 配發單點化：`Zone& ZoneManager::create_child(ZoneId parent, ZonePersistence p = ZonePersistence::Persistent)`，內部 `next_id_++`（0 保留給 root）→ 寫 manifest → `exists(path(id))` 防呆（存在即 throw，杜絕 manifest 回退時靜默覆檔）→ 建 zone → touch。parent 未載入 → throw 附兩個 id。
- 現行 `create(ZoneId, ZoneId)`（[zone_manager.h:27](../../projects/medp/src/gcore/zone/zone_manager.h)）收為 private（改名 `emplace_zone`），只供建構 root 與 load 路徑；外部自選 id 的口子關閉，維持配發不變式。
- id 永不復用（含 Ephemeral 死號）；crash 留下的 id 空洞無害。

### 3.2 manifest 與開檔協定

- `dir_/manifest.bin`（cereal PortableBinary）：`uint32 format_version` + `uint64 next_zone_id`。將來 `next_entity_uid` 落地時直接加欄位＋version bump，不預埋。
- 寫入：create_child 配發後、`save_all()` 第一步；一律先寫 `.tmp` 再 rename 原子替換（撕裂 = 整包存檔打不開，一行成本消掉單點）。
- 讀取（ZoneManager 建構子）：
  - manifest 存在 → 既有存檔：版本不符 throw；還原 `next_id_`；**必須成功載入 root.bin 取代新造的空 root**（缺失 throw）——同時堵住現行 `load(ZONE_ROOT)` 永遠不讀磁碟的真空洞（[zone_manager.cpp:50](../../projects/medp/src/gcore/zone/zone_manager.cpp)，舊 `load_root()` 至今無對應物）。
  - manifest 不存在但目錄有 zone 檔（含 root.bin）→ throw（不得靜默當新世界然後覆寫舊檔）。
  - 目錄乾淨 → 全新世界，行為與現狀全等。

### 3.3 檔案佈局：分桶本輪就做

`dir/<hex 前 2 碼>/<16hex>.bin`，root 特例 `dir/root.bin` 不動。理由（採 4X 尺度案，兩位評審背書）：現在做 = 改 `path()` 三行（[zone_manager.cpp:31-36](../../projects/medp/src/gcore/zone/zone_manager.cpp)）；defer 觸發後 = 搬數萬檔案＋遷移工具。pre-v1 沒有任何存檔資產，這筆帳現在付最便宜。

### 3.4 children 與結構性返回（設計凍結，實作 defer）

- **不變式（現在定死）**：parent→children 資料掛 parent Zone、隨 parent 載卸與序列化；全域 children 索引封死（900 萬 zone × ~16B ≈ 144MB 起跳，違反紅線）。
- 形狀：`Zone::children` 為 `std::vector<ChildLink>`，`ChildLink{ int32_t x, y; int16_t z; uint64_t child; }`——(x,y,z) 是 child 在 parent 地圖上的格位；child 側鏡像 `anchor`（同形座標＋有效旗標）。載入時驗雙向一致，違反 throw 附雙 id。
- **結構性返回**：child 的 anchor 即「回 parent 的落點」——零資料、永不過期，這是對 wild_x/wild_y 教訓最便宜的解；`ReturnTrail`（§3.8）只服務傳送門這類非結構性進入。
- 查詢用線性掃描（Region ≤225 筆；World 4 萬筆但只在低頻進出事件查）；變熱再建 transient map。
- **Ephemeral 禁入 children**：ChildLink 只指 Persistent zone；Ephemeral 由 Portal／玩法層弱引用，查無即重生成。
- 觸發條件:首個 zone 進入/懶生成系統或 cascade destroy 需求；落地時 version bump，並同回合決定 require/load 分流（§3.10）。

### 3.5 persistence：兩態，medps 自定

```cpp
enum class ZonePersistence : uint8_t { Ephemeral = 0, Persistent = 1 };  // zone.h
```

- `Persistent`（預設）＝落盤：unload/逐出寫檔後移除；`Ephemeral`＝可丟棄：**逐出**不寫檔直接移除、並刪除盤上殘檔（防舊 checkpoint 快照復活過期內容）。
- **save_all 是無損 checkpoint：寫出所有已載入 zone，含 Ephemeral**——「不寫盤」只存在於逐出路徑。否則玩家身處臨時 zone 時存檔即丟資料（含玩家實體）。此為對最小增量案的修正，採 ToME 對齊案論證。
- 欄位 `Zone::persistence` 隨 zone 檔序列化（策略聲明在資料、機制在 manager）；執行期不支援翻轉（會留孤兒檔）。
- 不抄 ToME 四態的理由：`true`（逐層存檔）在 medps 無結構位置（垂直層已收進同一 zone 檔）；`"memory"` 無消費者且與 LRU 逐出互相打架（不可寫盤又不可丟＝變相 pin）。`MemoryOnly = 2` defer：生成管線落地後出現具體場景再議。

### 3.6 LRU 卸載

- API:`set_loaded_budget(size_t)`（0=無上限，**預設 0**＝現狀不變）、`enforce_budget()`、`touch(ZoneId)`。狀態：manager 側 `touch_seq_`，Zone 側 `last_touch`（執行期，不序列化）。
- touch 只有三個來源：load 成功、create_child、上層顯式 touch。**tick 不 touch**（否則人人恆熱）；**get() 不 touch**（機制性讀取不擾動逐出序）。
- 逐出只在 tick 尾端（＋手動呼叫）：絕不在 load/create_child 途中逐出，避免同幀懸置指標。逐出對象 =（`last_touch`, id）最小的非 root、非 pinned zone——帶 id tie-break 使逐出順序可在測試中精確斷言。
- `Zone::pinned`（`bool`，執行期欄位，**不序列化**——session 關注態不進存檔）：root 建構時 `pinned=true`；玩法層對玩家所在 zone 設 pin，比「每 tick 記得 touch」的紀律契約多一道機制防線。規則現在拍死：**跨 registry 搬移函式落地時，搬移必須自動 touch 目的 zone**。
- 模擬 LOD 是另一條軸（報告 §2-G 能量制），本策略只管記憶體；被逐出的 Persistent zone 就是凍結，離線補算 defer（觸發：玩法需要離線推進，屆時 zone 檔加 `last_saved_tick`）。
- decay defer（LRU 已單獨守住紅線）；O(n) 逐出掃描換 heap defer（budget >1000 且實測可感）。

### 3.7 跨 zone 實體引用（設計凍結，實作 defer）

- 弱引用語意：解引用失敗是**預期結果**（當死亡/不在場），不是錯誤；fail-fast 保留給結構損毀。
- `StableId{ uint64_t uid; }` opt-in component（只掛具名角色/勢力領袖級實體），登記 AllComponents；`EntityRef{ uint64_t zone; uint64_t uid; }`（0,0 = null）——必帶 zone id，否則解引用變全域搜尋。
- uid 由 `ZoneManager::alloc_uid()` 配發，來源 manifest 新欄位 `next_entity_uid`（落地時 version bump）。
- `Zone::uid_index`（執行期，load 後由 `view<StableId>` 重建；重建撞號 = 存檔損毀，throw 附 zone id + uid）。維護收口在 spawn/destroy/搬移少數入口，不用 on_destroy hook。
- 解引用兩段式：`zm.get(ref.zone)` → nullptr＝zone 未載入（呼叫端決定 load 或跳過）；`zone->find_uid(uid)` → `entt::null`＝已死亡。
- 觸發條件：首個「root 全局實體指向 zone 內實體」的玩法系統。

### 3.8 返回座標與入口契約（設計凍結，實作 defer）

- `ZoneReturn{ uint64_t zone; int32_t x, y; int16_t z; }`＋`ReturnTrail{ vector<ZoneReturn> }` 掛旅行實體（per-entity per-zone，結構上杜絕全域欄位）；以 zone id 為鍵後寫覆蓋（upsert），回到某 zone 時取出並移除。
- 落點決策順序：ReturnTrail 記錄 → 結構性 anchor 推導（§3.4）→ zone 顯式預設入口 → **都沒有就 throw 附 zone id**。絕不隱式「地圖中心」（報告 §2-H-4 慘案）。預設入口的宣告機制綁 §2-H spots 設計，spots 落地前查無入口就是 throw。
- 與跨 registry 搬移函式同回合落地（搬移是消費 ReturnTrail 的時點；搬移收口為單一函式，未來空間索引/uid_index 維護同掛此入口）。

### 3.9 Portal（設計凍結，實作 defer）

- Portal 是 zone registry 裡的 entity：`Position` + `Portal{ uint64_t to_zone; Mode mode; int32_t x, y; int16_t z; }`，`enum class Mode : uint8_t { ExplicitPos, ReturnPos, DefaultEntry }`——顯式枚舉取代座標哨兵（哨兵值正是報告 §2-I 點名的坑源）。樓梯、地圖邊緣出口、魔法門是同一機制的三種資料。
- 不用 Tile flag 承載目的地（稠密陣列加欄位＝每 zone 數 MB 死重；portal 每 zone 個位數）。`TILE_PORTAL` 提示 flag 連同空間索引再議。
- 與 §3.8 同回合落地。

### 3.10 fail-fast 套件與檔案原子性

本輪落地：load() 成功後驗 `z.id == 請求 id`（不符 throw 附兩 id）；建構子的 manifest/目錄一致性檢查（§3.2）；create_child 的 exists() 防呆（§3.1）；manifest 原子寫（§3.2）。
`destroy()` 補完整語意：**同步刪除盤上檔案**（現行 [zone_manager.cpp:26-29](../../projects/medp/src/gcore/zone/zone_manager.cpp) 不刪檔，Persistent zone 銷毀後 load 會靜默復活）；children 落地後加「children 非空 → throw」。
defer：`require(id)`（結構引用指向的檔案缺失＝損毀，throw）與 `load(id)`（探測性，回 false）分流——children 落地回合一起做；跨檔一致性掃描工具——觸發：實際出現孤兒檔/懸空連結事故。

### 3.11 tick 重入禁令（即刻文件約定）

`tick()` 遍歷 `zones_` 期間（[zone_manager.cpp:74-78](../../projects/medp/src/gcore/zone/zone_manager.cpp)），system 呼叫 create_child/load/unload/destroy 會 rehash/刪元素 → 迭代器 UB。拍板：**system 內禁止 zone 結構性變更**，寫進 zone_manager.h 註解；enforce_budget 只在迭代結束後。升級觸發：首個需要在 system 內造/載 zone 的玩法系統 → 改命令緩衝（結構性操作排隊、tick 尾端統一執行）。

### 3.12 存檔槽語意（即刻文件約定）

現階段明文「**單槽活儲存**」：存檔目錄即世界的權威狀態，unload/逐出隨時寫檔，save_all 是檢查點不是槽位快照；各 zone 凍結於不同遊戲時刻是接受的語意（4X 離線 zone 本就凍結），時間錨點將來由 `last_saved_tick` 與離線補算一併解。多槽/另存 = 目錄複製，defer：需要玩家可見的存檔槽功能時。

### 3.13 Zone* 生存期契約（即刻文件約定）

「不跨 tick 持有 `Zone*`/`Zone&`」寫進 zone_manager.h 註解。升級判準拍死：**第一個想長駐快取 Zone* 的系統出現時，強制改 handle 式存取（存 id、每次 get）**，不等「反覆咬人」。

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
