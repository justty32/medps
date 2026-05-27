# registry chunking 調查:邏輯 zone 與 entt::registry 解耦

> 初版 2026-05-27。分支:`rewrite/entt-cereal`。
> 本文是 `work/design/zone_layers.md` §7「registry 粒度(調查中)」開放問題的延伸調查。
> **不改任何程式碼,只回答可行性與取捨。** 標記:**【事實】**=從現有碼/EnTT 推得;**【推論】**=設計判斷;**【建議】**=我的推薦。
> 所有程式碼引用附 `路徑:行號`。

---

## 0. 結論先行(TL;DR)

**推薦做法:region 層採方案 B(只合併檔案、記憶體仍每 zone 一個 registry),area 層維持現狀 1:1 不 chunk。** 方案 A(共用 registry)的 area 層代價過重、與多項現有 invariant 衝突,不建議。

- 一句話建議:**region 用「打包檔案 + 預取相鄰」拿到 streaming 平順度,不碰 registry 粒度;area 保持一張一 registry。**
- 對現有檔案的具體改動點:**方案 B 約 3 處**(新增 `chunk_key.h`、改 `FolderZoneStore` 的 path 邏輯改成 chunk 分檔、`GlobalManager::load/unload` 走 chunk blob index);**方案 A 約 7~9 處**(見 §6)。

---

## 1. 問題本質

現況是嚴格的 **1 邏輯 zone = 1 `entt::registry` = 1 載入/卸載/存檔單位**。`GlobalManager::loaded_` 是 `unordered_map<ZoneKey, unique_ptr<registry>>`(`src/gcore/global_manager.h:84`),`load/unload/save` 都以單一 ZoneKey + 單一 registry 為粒度(`src/gcore/global_manager.cpp:52-90`)。

使用者的顧慮是**數量級**:`zone_layers.md:22-23` 定案 World 200×200 → 最多 4 萬 Region、再 ×R1²=225 → 最多約 900 萬 Area。即使永不全載,光是「相鄰 region 之間走動」就會在 4 萬個獨立檔案 / registry 之間頻繁 load/unload,造成 **streaming 抖動**;而 area 一張 250×250≈6.25 萬格(`zone_layers.md:54`),檔案與 registry 物件的固定開銷也不小。

提案:引入 **chunk(區塊)** 作為 registry/storage 單位,把 N×N 個邏輯 zone 合進一個 chunk:
- region 層:5×5 個 world-tile → 1 chunk(含 25 region)。
- area 層:3×3 個 region-tile → 1 chunk(含 9 area)。
- 邏輯座標(ZoneKey)不變;chunk 身份由整除推出 `chunk_coord = zone_coord / N`。

**核心張力**:chunk 化把「定址粒度(邏輯 zone,玩家概念)」與「IO/記憶體粒度(chunk)」拆開。好處全在 IO/locality;代價全在「載入粒度變粗」——進一個 region 要連帶實體化它周邊 24 個 region。問題在於這個交換在 region 層划算、在 area 層可能虧本。

---

## 2. 引擎 / 現況事實(關鍵約束)

這些事實決定了哪些方案可行,務必先吃透。

### 2.1 ZoneKey 與整除推導已經到位
- `ZoneKey = ZoneType:16 | x:16 | y:16 | z:16`,`src/gcore/zone_key.h:9-23`。
- `zone_layers.md:71-73` 已定「parent 由整除推回」(`Area (gx,gy) → Region (gx/R1, gy/R1)`)。chunk 推導是**同一個機制再套一層**:`chunk_x = zone_x / N`。位元沒問題:Area 全局座標最大 2999,int16 上限 32767(`zone_layers.md:77`),再除 N 只會更小。
- path 由 key 推、不存全域清單(`zone_key.h` 的 invariant,`zone_layers.md:105` 重申)。

### 2.2 zone_io 的 snapshot 是「整個 registry 一次性」
- `zone_io::save/load` 對整個 `entt::registry` 做 snapshot(`src/gcore/serialize/zone_io.h:33-43`),逐 component type 展開 `AllComponents`(`all_components.h:12-19`)。
- **【事實】沒有 partial-snapshot / 子集 snapshot 的封裝**。要存 chunk 內單一 region,得自己加「依 tag 過濾的 partial save」——EnTT 的 `snapshot.get<C>()` 是吃整個 storage,不接受 entity 子集 filter。可以手動遍歷再寫,但那是新程式碼。
- `loader.orphans()`(`zone_io.h:28`)會清掉載入後沒有任何 component 的 entity;靠 placeholder entity(帶 `ZoneMeta`)保命(`zone_io.h:25-27`、`global_manager.cpp:44-45`)。

### 2.3 snapshot 保留 entity 識別碼 → CrossZoneRef 是「raw entity + zone」
- `CrossZoneRef` 存 `{ZoneKey zone; entt::entity local_entity;}`,序列化時把 entity 當原始整數存(`src/gcore/components/cross_zone_ref.h:5-21`)。
- **【事實】這只在「同一個 registry round-trip」時成立**:`snapshot_loader` 保留 entity 數值(`test/src/main.cpp:96`「snapshot_loader preserves entity values」),所以離開→還原同一張 zone,`local_entity` 仍對得上。
- `resolve` 只查已載入 zone、不做 IO,`ZONE_ROOT→root`(`global_manager.cpp:18-21`)。`entt::registry*` 直接當 zone 把手。
- **這是方案 A 最危險的地方**(見 §5.3):把 N 個 region 合進一個 registry,等於把 N 套各自獨立的 entity 命名空間塞進一個命名空間,會碰撞。

### 2.4 ZoneMeta:每 zone 一個 placeholder
- `ZoneMeta{self, parent}` 由每張 zone 的 placeholder entity 持有(`src/gcore/components/zone_meta.h:7-13`);`create()` 在新 registry 裡建一個 placeholder 並 emplace ZoneMeta(`global_manager.cpp:44-45`)。
- **【事實】目前隱含「一 registry 一個 ZoneMeta」**:沒地方假設「一 registry 多個 ZoneMeta」,但也沒禁止——`view<ZoneMeta>()` 會回傳所有,目前只是剛好一個。

### 2.5 ChildZoneSummary:父 registry 裡的子 zone 索引
- 父 registry 每個直接子 zone 一個 stub entity(`src/gcore/components/child_zone_summary.h:8-13`);`children()` 靠 `view<ChildZoneSummary>()` 列舉(`global_manager.cpp:70-76`),`ensure_child_stub` 線性掃描去重(`global_manager.cpp:33-38`)。
- 這是 zone_layers.md §4.3「僅摘要」狀態的載體(`zone_layers.md:117`)。

### 2.6 ZoneStore:已是「bytes ↔ storage」抽象,且支援 pack 檔
- `ZoneStore` 介面 `write/read/has/flush`(`src/gcore/serialize/zone_store.h:14-24`);`FolderZoneStore` 一 zone 一檔,path 由 key 的 hex 推(`zone_store.h:31-36`)。
- **【事實】註解明說「可換 single pack file / DB」**(`zone_store.h:11-13`、`global_manager.h:30`)。**方案 B 的「合併檔案」幾乎完全落在這層,GlobalManager 上層不必動。**

### 2.7 EnTT 3.16,無 sub-registry
- `include/entt.hpp:29-31` 是 3.16.0。
- **【事實】EnTT 沒有「一個 registry 內的子命名空間 / sub-registry」概念**。registry 就是一組 storage + 一個 entity 命名空間。要在一 registry 內分隔 N 個邏輯 zone,只能靠**每 entity 帶一個 zone-tag component** 再用 `view<Tag>()` 過濾,或用多份 storage(EnTT 支援具名 storage,但仍共享 entity 命名空間)。
- `view<Tag>()` 過濾**不是免費的**:它要嘛建一個只含該 tag 的 group/view(掃 tag storage),要嘛對候選 entity 逐個查 tag。對「我只想對這一個 region 跑 system」這種高頻操作,等於每次都付一次 tag 過濾;而現況是「拿到 registry 指標直接 `view<Position,Velocity>`」零過濾(`systems/movement.h:11-15`、`global_manager.cpp:96-100` 的 tick)。

---

## 3. 方案 A:共用 registry(使用者原提案)

> chunk = 共用 registry = 共用檔。25 個 region 的所有 entity 住在同一個 `entt::registry`。

### 3.1 怎麼落地
1. **ChunkKey**:新增 `chunk_key.h`,`chunk_key = make_zone_key(type, x/N, y/N, z)` 由 ZoneKey 整除推得。`loaded_` 改成 `unordered_map<ChunkKey, registry>`,storage path 用 chunk key 推。
2. **zone-tag**:每個 entity 需要知道自己屬於哪個邏輯 region。最省事是讓既有 `Position` 不夠(它只有 x/y),得加一個 `ZoneTag{ ZoneKey logical_zone; }` component 掛到每個 actor entity 上,或把地格陣列改成「每邏輯 zone 一張 tdarray」放進 ctx。
3. **多 ZoneMeta**:一個 chunk registry 內會有 25 個 ZoneMeta placeholder(每邏輯 region 一個)。`view<ZoneMeta>()` 自然回傳 25 個,可接受,但所有「假設只有一個 self」的程式碼要改成「依 logical key 查對應的 ZoneMeta」。
4. **ChildZoneSummary**:父層(world chunk)指向 25 個 region 邏輯 key 不變,但要注意「載入一個 region = 載入整個 chunk」,`load(region_key)` 內部要 map 到 chunk、整塊載入、回傳同一個 registry 給 25 個 key。
5. **地格**:25 張 region grid 要嘛 25 張獨立 tdarray 放 ctx(`zone_layers.md:135` 已決地格走 tdarray 而非 entity),要嘛拼成一張大 grid。前者乾淨;後者省記憶體但失去「邏輯 region 邊界」。

### 3.2 好處
- registry 物件數 / 檔案數 region 層 ÷25、area 層 ÷9。
- **同 chunk 內跨 region 走動零載入**——streaming 抖動只發生在 chunk 邊界(每 5 格才一次,而非每 1 格)。
- 同 chunk 內的 `CrossZoneRef` 變便宜:目標就在同一 registry,`resolve` 直接命中(`global_manager.cpp:18-21`),不需先 load 目標 zone。
- locality:25 個 region 的 component 連續存放,跨 region 的聚合 system 對 cache 友善。

### 3.3 代價與風險(這裡是地雷區)

- **【風險 R1 — entity 命名空間碰撞,最致命】** §2.3:`CrossZoneRef.local_entity` 是 raw entity,語意是「在它那張 registry 內的 entity」。今天 region(5,6) 的 entity #7 與 region(5,7) 的 entity #7 是兩個不同東西,因為在兩個 registry。合併成一個 chunk registry 後,EnTT 只會給出一套遞增 entity id,**原本兩個 #7 不可能同時存在**。後果:
  - 任何「先各自 build 好再 merge」的離線生成流程不能直接搬。
  - 既有存檔(每 zone 一檔、entity id 各自從 0)無法直接 concat 成 chunk——必須做 **entity 重映射 merge**(EnTT 的 `continuous_loader` 可做跨 registry 重映射,`entt.hpp:41998` 附近,但 `zone_io` 現在用的是 `snapshot_loader` 不重映射,`zone_io.h:21`)。這是新序列化路徑。
  - 跨 chunk 的 `CrossZoneRef` 仍要 resolve(目標在別的 registry),語意不變;但**同 chunk 內**的舊 ref 若曾以「整個 zone 從 0 編號」的假設存下,合併後會指錯。

- **【風險 R2 — 載入粒度變粗,area 層尤甚】** 進一個 region 要實體化 25 個 region 的所有 entity + 25 張地格。region 一張 15×15=225 格(`zone_layers.md:54`),×25=5625 格,還算輕。**但 area 層:一張 250×250≈6.25 萬格(`zone_layers.md:54`),3×3=9 張一次載 ≈ 56 萬格 + 物件。** 玩家只是踏進一個 area,就要還原 9 張完整 Rimworld 級地圖(每張含 terrain/roof/fog/path 多張平行 tdarray,`zone_layers.md:131-135`)+ 所有 pawn/物件 entity。首次載入延遲與常駐記憶體都偏重,且其中 8 張可能玩家根本不會進。

- **【風險 R3 — 存檔肥大 / partial-save 缺口】** §2.2:`zone_io` 只能整 registry snapshot。改動一個 region → 必須重寫整個 chunk blob(含另外 24 個沒動的 region)。對 area 層就是「動一個 area,重寫 9 張地圖」。要避免就得自寫 partial-save(依 ZoneTag 過濾出單一邏輯 zone 的 entity 子集再寫),那是 EnTT 不直接支援、需手刻的新程式碼(§2.2)。

- **【風險 R4 — system 過濾成本】** §2.7:`tick()` 現在對每個 loaded registry 直接跑 system(`global_manager.cpp:96-100`)。chunk 化後若只想 tick 玩家所在的那一個 region,得 `view<ZoneTag>()` 過濾;若整 chunk 一起 tick,則 8 個玩家不在的 region 也吃 tick(正是 `zone_layers.md:119` 引 Rimworld「每張載入地圖都吃 tick」的教訓,reintroduce 了)。

- **【風險 R5 — invariant 衝突盤點】**
  - 「一 registry 一 ZoneMeta」隱含假設要全面改成「依 logical key 查」(§2.4)。
  - `resolve` 回傳 `entt::registry*` 當 zone 把手(`global_manager.h:33-37`);chunk 化後「registry == zone」不再成立,呼叫端拿到 registry 還要知道「我要的是裡面哪個邏輯 zone」。`ZoneResolution` 可能要再帶 logical key。
  - `children()` 列舉假設父 registry 內 stub 對應「可獨立載入的 zone」(`global_manager.cpp:70-76`);現在 load 一個會連帶 24 個,語意要文件化。

---

## 4. 方案 B:只合併檔案(記憶體仍 per-zone registry)

> chunk = 多個邏輯 zone 打包進**同一個檔**,但記憶體中仍每邏輯 zone 一個 registry。只減檔案數,不碰 registry 粒度。

### 4.1 怎麼落地(改動極小)
- **幾乎全落在 `ZoneStore` 層**(§2.6 已是 bytes↔storage 抽象)。新增一個 `ChunkedFolderZoneStore`(或 `PackZoneStore`):
  - `path/檔案` 以 **chunk key** 分檔(`chunk = zone_x/N`),一個 chunk 檔內是「邏輯 zone key → blob」的小 index + 各 zone 的 `zone_io` bytes(就是現在每 zone 那串 bytes,原封不動)。
  - `write(zone_key, bytes)`:算出 chunk,讀入/建立該 chunk 檔,替換該 zone 的 blob,寫回(或 `flush()` 時批次寫,`zone_store.h:23` 已預留)。
  - `read(zone_key, ...)`:算出 chunk,開該 chunk 檔,取出該 zone 的 blob。
- `GlobalManager`、`zone_io`、所有 component **完全不動**。`loaded_` 仍是 per-zone registry;`CrossZoneRef`/`ZoneMeta`/entity 命名空間語意 100% 不變。

### 4.2 好處
- **檔案數**降到 region 層 ÷25、area 層 ÷9(與方案 A 同等的「檔案數」收益)。對「900 萬潛在 area = 900 萬檔」的檔案系統壓力(inode、目錄掃描)是真實緩解。
- **零 invariant 風險**:entity 命名空間、CrossZoneRef、ZoneMeta、resolve 全部不變;不需要 entity 重映射 merge(避開 R1)。
- partial-save 天然成立:一個 chunk 檔內各 zone blob 獨立,改一個 zone 只換它那段 blob,不必重序列化別人(緩解 R3,只剩「重寫 chunk 檔」的 IO,但 blob 不需重算)。
- 實作複雜度最低;可完全在 `ZoneStore` 子類內完成 + 一個 `chunk_key` helper。

### 4.3 代價 / 它**不能**解決的
- **不解決 streaming 抖動**:記憶體仍每 zone 一 registry,跨相鄰 region 走動仍會 load/unload 各自的 registry——只是「從同一個 chunk 檔讀」。要平滑得另外做「**預取**」(踏進一個 chunk 時,把整個 chunk 檔讀進來、把 25 個 zone 一次 deserialize 成 25 個 registry 並放進 `loaded_`)。這個預取是純加法、不破壞 invariant,且可後做。
- **不省 registry 物件固定開銷**:仍有 25 個 registry 物件。但 EnTT 空 registry 開銷很小,且我們本來就靠「已載入集合保持很小」(`zone_layers.md:119`)控總量,這點影響有限。
- chunk 檔的並發寫:同一 chunk 內多個 zone 同時 dirty,要小心 read-modify-write;`flush()` 批次寫可緩解。

---

## 5. 取捨表

### 5.1 三方案橫向比較

| 面向 | (A) 共用 registry | (B) 只合併檔案 | 現狀 1:1 |
|---|---|---|---|
| registry 物件數 | ÷25 / ÷9 | 不變(每 zone 一個) | 1:1 |
| 檔案數 | ÷25 / ÷9 | ÷25 / ÷9 | 1:1(最多 900 萬) |
| streaming 抖動(相鄰 zone) | **chunk 內零**(最佳) | 不變;需另做預取才平滑 | 每跨一格可能抖 |
| 記憶體(載入粒度) | **粗**:進 1 region 載 25;進 1 area 載 **9×6.25 萬格**(R2) | 細:仍可只載 1 個邏輯 zone | 最細 |
| 存檔肥大 | 改 1 個要重寫整 chunk blob(除非自刻 partial-save) | 各 zone blob 獨立,只換 1 段 | 各自獨立 |
| 同 chunk 內 CrossZoneRef | **便宜**(同 registry,免 resolve IO) | 不變 | 需 load 目標 |
| entity 命名空間 | **碰撞**,需重映射 merge(R1) | 不變 | 不變 |
| 實作複雜度 | 高(7~9 處 + 新序列化路徑) | 低(~3 處,全在 ZoneStore) | — |
| 與現有 invariant 相容 | 多處需改(R5) | **完全相容** | — |

### 5.2 「好處」量化(對應提案問題 2)
- registry/檔案減量:region 5×5 ÷25、area 3×3 ÷9 — A、B 在**檔案數**上等價;只有 A 同時減 registry 物件數。
- 「相鄰 zone 邊界不再觸發載入抖動」:**只有 A 天生具備**;B 要靠額外的 chunk 預取才達到同效果(但預取不破壞 invariant,風險低)。

### 5.3 為何 area 層特別不適合方案 A(對應提案問題 2 的重點)
- area 一張 250×250≈6.25 萬格(`zone_layers.md:54`);3×3=9 張 → 一次載 ≈ **56 萬格** + 9 張地圖的全部 pawn/物件 entity。
- area 是「即時 / JRPG」最細模擬層(`zone_layers.md:23`),玩家通常**只在一個 area** 內活動;周邊 8 個 area 多半不需即時模擬。把它們一起載 = 把 `zone_layers.md:119`「已載入集合保持很小」的原則直接違反。
- 因此 **area 層的 N 應該 = 1(不 chunk)**,或最多用「方案 B 合併檔案」緩解檔案數,而**絕不**用方案 A 合併 registry。

---

## 6. 分層建議(region vs area)+ 改動清單

### 6.1 建議的策略矩陣

| 層 | 推薦 N(chunk 邊長) | 推薦方案 | 理由 |
|---|---|---|---|
| **Region** | 5(或先 4) | **B(合併檔案)+ 後續 chunk 預取** | region 圖小(15×15=225 格),即使日後想升級到 A 代價也可控;先用 B 拿檔案數收益、零風險。預取補足 streaming 平滑。 |
| **Area** | **1(不 chunk)** | 維持 1:1;檔案數壓力大時才上 B | 一張 6.25 萬格,合併 registry 記憶體/首載過重(§5.3);保持最細載入粒度。 |
| World | 1 | 不適用(每 z 僅一張,`zone_layers.md:21`) | 數量是 1,無 chunk 需求。 |

> N 取值理由:5×5 對齊使用者提案且 ÷25 收益明顯;若擔心一次預取 25 張仍偏多,先取 **4(÷16)** 更保守。area 維持 1 是核心建議。

### 6.2 方案 B 的具體改動清單(推薦先行,低風險)
1. **新增** `src/gcore/chunk_key.h`:`chunk_key_of(ZoneKey, int N)`、依層查 N 的常數表(region N=5、area N=1),純整除推導(沿用 `zone_key.h:13-23` 的 make/拆解)。
2. **新增** `ChunkedFolderZoneStore`(放 `zone_store.h` 或新檔):實作 `write/read/has/flush`,內部以 chunk 檔 + 小 index 存多個 zone blob。`path()` 改成「chunk 檔路徑」。
3. **改** `GlobalManager` 預設 store:`global_manager.cpp:8` 把 `FolderZoneStore` 換成 `ChunkedFolderZoneStore`(或保留可注入,測試用舊的)。**GlobalManager 上層邏輯不動。**
4. **測試**:在 `test/src/main.cpp` 加「同 chunk 內兩個 zone 各自 round-trip、互不污染」「改一個 zone 不破壞同 chunk 另一個」。

→ 影響面 3 個檔(+1 測試),`zone_io`/components/`resolve` 全不碰。

### 6.3 後續可延後(region streaming 平滑,中風險)
5. **chunk 預取**:`GlobalManager` 加 `prefetch_chunk(ZoneKey)`,踏進 chunk 時把整檔 25 個 zone blob 一次讀出、deserialize 成 25 個 registry 放進 `loaded_`(仍 per-zone registry,不合併)。卸載時整 chunk evict。這拿到方案 A 的 streaming 平滑度,卻保留 per-zone registry 與全部 invariant。

### 6.4 若日後仍要走方案 A(高風險,建議延後且僅限 region)
- 需要的新東西:`ZoneTag` component + 加進 `all_components.h:12`;`loaded_` 改 chunk-keyed;`load/unload` map zone→chunk;`resolve` / `ZoneResolution` 帶 logical key;多 ZoneMeta 查詢改「依 key」;**entity 重映射 merge 序列化路徑**(改用 `continuous_loader`,`entt.hpp:41998`,取代 `zone_io.h:21` 的 `snapshot_loader`);partial-save(依 tag 過濾)。
- → 影響面 7~9 個檔 + 一條全新序列化路徑。**僅在「region streaming 經量測仍是瓶頸,且 B+預取 不夠」時才做,且絕不套用到 area。**

---

## 7. 開放問題

- **預取邊界政策**:踏進 chunk 載 25 個 region,何時整 chunk evict?LRU per-chunk?與 `zone_layers.md:119`「已載入集合很小」如何協調(25 張 region 是否仍算小)?
- **N 是否該可調 / 鎖死**:`WORLD_DIM/R1/R2` 仍是「可調常數」(`zone_layers.md:158`);chunk N 應同樣集中成常數、別硬編碼散落。
- **area 檔案數**:若 area 真到數百萬檔,方案 B 的 chunk 檔 index 要多大才划算?是否改用 DB 後端(`zone_store.h:11` 已預留)而非自刻 pack 格式?
- **chunk 檔的並發 / 損壞**:read-modify-write 同一 chunk 檔的原子性;一個 chunk 檔損壞會連帶失去 N 個 zone(現狀一檔壞只丟一個 zone)——可靠性與檔案數的取捨。
- **跨 z chunk**:chunk 是否跨 z 合併?建議否(z 已是 zone 身份一部分,`zone_layers.md:89`),chunk 應在同一 z 內。
- **既有存檔遷移**:若先發布 1:1 再改 B,需要一支把舊「每 zone 一檔」打包成 chunk 檔的遷移工具(B 因 blob 不變,遷移單純;A 因要重映射 entity,遷移複雜)。

---

## 參考(本調查引用的現碼位置)
- ZoneKey 編碼/推導:`src/gcore/zone_key.h:9-23`
- GlobalManager 粒度:`src/gcore/global_manager.h:33-65,84`、`src/gcore/global_manager.cpp:13-100`
- snapshot 整-registry IO / orphans:`src/gcore/serialize/zone_io.h:13-43`
- snapshot 保留 entity 值(CrossZoneRef 前提):`test/src/main.cpp:96`
- CrossZoneRef = raw entity + zone:`src/gcore/components/cross_zone_ref.h:5-21`
- ZoneMeta / ChildZoneSummary:`src/gcore/components/zone_meta.h:7-13`、`child_zone_summary.h:8-13`
- ZoneStore = bytes↔storage 抽象、可換 pack/DB:`src/gcore/serialize/zone_store.h:11-36`
- EnTT 3.16、continuous_loader 位置:`include/entt.hpp:29-31,41998`
- 數量級 / 地格尺寸 / 已載入集合要小:`work/design/zone_layers.md:22-23,54,104-119,131-135`
