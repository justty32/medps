# zone 定址與生命週期 — 決策詳述：跨 zone 引用與入口契約

> 子檔（拆分於 2026-08-12，原內容為 [zone-addressing-lifecycle-design.md](zone-addressing-lifecycle-design.md) §3 決策詳述之 §3.7-3.13）。狀態：**已拍板並落地（2026-07-22）**。拍板結果總表見 [拍板結果與決策總表](zone-addressing-lifecycle-design-rulings.md)。
> 導覽：[← 決策詳述：定址與生命週期機制](zone-addressing-lifecycle-design-mechanics.md) ｜ 下一篇：[驗收不變量、序列化版本規劃與落地紀錄](zone-addressing-lifecycle-design-followup.md) ｜ [回母檔索引](zone-addressing-lifecycle-design.md)

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

## 導覽

[← 決策詳述：定址與生命週期機制](zone-addressing-lifecycle-design-mechanics.md) ｜ 下一篇：[驗收不變量、序列化版本規劃與落地紀錄](zone-addressing-lifecycle-design-followup.md) ｜ [回母檔索引](zone-addressing-lifecycle-design.md)
