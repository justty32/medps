# zone 定址與生命週期 — 決策詳述：定址與生命週期機制

> 子檔（拆分於 2026-08-12，原內容為 [zone-addressing-lifecycle-design.md](zone-addressing-lifecycle-design.md) §3 決策詳述之 §3.1-3.6）。狀態：**已拍板並落地（2026-07-22）**。拍板結果總表見 [拍板結果與決策總表](zone-addressing-lifecycle-design-rulings.md)。
> 導覽：[← 拍板結果與決策總表](zone-addressing-lifecycle-design-rulings.md) ｜ 下一篇：[決策詳述：跨 zone 引用與入口契約](zone-addressing-lifecycle-design-cross-refs.md) ｜ [回母檔索引](zone-addressing-lifecycle-design.md)

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

## 導覽

[← 拍板結果與決策總表](zone-addressing-lifecycle-design-rulings.md) ｜ 下一篇：[決策詳述：跨 zone 引用與入口契約](zone-addressing-lifecycle-design-cross-refs.md) ｜ [回母檔索引](zone-addressing-lifecycle-design.md)
