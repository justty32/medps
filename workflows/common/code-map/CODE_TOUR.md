# CODE_TOUR — 給人的程式碼導讀

給專案作者用的線性閱讀路徑：照順序讀，每站說明「這檔在幹嘛、看什麼、讀完該能回答什麼」。
[CODE_MAP.md](CODE_MAP.md) 是 agent 修改前的查表；本檔是人的導讀，兩者同鏈維護（見文末）。
瀏覽器版導覽在 [html/index.html](html/index.html)（嵌入帶行號的原始碼；由 `html/build.py` 生成）。

目前全部核心碼約 1,100 行，照本路徑一次讀完約 1 小時。

## 全貌一句話

`ZoneManager` 管一堆 `Zone`（一個 zone = 一個 `entt::registry` + 自帶多層 tile 地圖），
zone id 是零語意的單調序號、由 `create_child` 配發；存讀檔 = 一 zone 一檔（zone_io 兩塊接合）＋
`manifest.bin` 記 id 計數器；`tick()` 對每個已載入 zone（**含 root**）跑所有註冊的 system。

## 閱讀路徑（依依賴順序）

### 第 1 站 `projects/medp/src/gcore/util/`（2 檔，~270 行）— 容器與巨集工具

- `tdarray.hpp`：2D 陣列模板，row-major（`vec[x*sy+y]`），已 cereal 化。檔頭三條使用慣例必讀：
  回傳 bool 的操作一律「**true = 失敗 / 越界 / 中斷**」；座標可傳任何有 `.x/.y` 的型別（`is_coor`）；
  取值家族 get/getref（未檢查）、getptr（檢查、可 nullptr）、getval（複本、可帶 default）。
- `mydef.h`：metaprogramming 巨集，各附使用範例，掃過即可。

讀完該能回答：`arr.set(x, y, v)` 回傳 `true` 代表什麼？（跟直覺相反。）

### 第 2 站 `projects/medp/src/gcore/zone/zone.h` + `tile.h`（55 行）— 核心資料結構

- `Tile`（`tile.h:12`）：`{uint32 terrain, flags}`＋`TILE_WALKABLE`/`TILE_BLOCKS_SIGHT`。**不是 component**——地圖是 zone 的固有結構。
- `Zone`（`zone.h:19`）：`{id, parent, reg, layers}`。id 是裸 `uint64_t` 零座標語意（舊 ZoneKey 位元打包已移除）；
  `layers`（`zone.h:36`）是 `map<int, tdarray<Tile>>`，鍵即 z（地面=0、往下為負、稀疏）。
- registry 不可複製 → Zone 只能移動；`ZONE_ROOT=0` 永駐、放非地圖的全局實體。

讀完該能回答：為什麼地圖掛在 Zone 上而不是做成 component？（代價是什麼——registry snapshot 不含它。）

### 第 3 站 `projects/medp/src/gcore/components/`（2 檔，~25 行）— 資料積木

全是 POD aggregate + `serialize()` 成員。

- `Position{x,y,z}`：zone 內 grid 座標，z 即 `Zone::layers` 的鍵；有 `.x/.y` 故滿足 tdarray 的 `is_coor`。
- `Velocity{dx,dy}`：每 tick 移動步（示範用）。
- **鐵律：新增 component 必須同步登記 `serialize/all_components.h` 的 `AllComponents`**（見第 4 站）。

### 第 4 站 `projects/medp/src/gcore/serialize/`（4 檔，~130 行）— 存讀檔管線

建議順序：

1. `all_components.h` — `AllComponents` type_list，**新增 component 唯一要登記的地方**。
2. `entt_cereal_archive.h` — 純膠水：把 entt snapshot 的 callback 簽章轉成 cereal 呼叫，`entt::entity` ↔ 底層整數。
3. `registry_io.h:16,23` — `save_impl/load_impl` 用 fold expression 展開 AllComponents；`registry_io.h:30` 的 `loader.orphans()` 是**陷阱點**：load 後沒有任何（已登記）component 的 entity 會被清掉。
4. `zone_io.h` — 完整 Zone 的存讀：第一塊（id/parent/layers）直接 cereal、第二塊（reg）走 registry_io，兩塊依序接在同一 stream；大括號限制 archive 生存期是 cereal 解構時才寫出的緣故。**無版本欄位**（使用者裁定）：格式一變，舊檔讀出來就是壞資料。

讀完該能回答：為什麼新 component 忘了登記 AllComponents，存檔會「默默」漏掉它、不會報錯？

### 第 5 站 `projects/medp/src/gcore/zone/zone_manager.h` / `.cpp`（224 行）— 總管

把前四站全部接起來。header 註解寫了三條契約（tick 重入禁令、單槽活儲存、`Zone*` 不跨 tick 持有），必讀：

- 建構子（`zone_manager.cpp:9`）：開檔協定——有 manifest → 還原 next_id＋**必讀回 root.bin**（缺失 throw）；無 manifest 但有 .bin → throw；乾淨目錄 → 新世界。
- `create_child`（`zone_manager.cpp:47`）：id 單點配發（永不復用），配發即原子寫 manifest；撞既有檔或 parent 未載入 → throw。
- `load`（`zone_manager.cpp:97`）：檔案不存在回 false；檔內 id 與請求不符 → throw。
- `destroy`（`zone_manager.cpp:60`）：連盤上檔案一起刪（死 zone 不復活）。
- `tick`（`zone_manager.cpp:126`）：每個已載入 zone × 每個 system 依註冊順序；**root 也參加**。

讀完該能回答：哪三種磁碟狀態會讓建構子 throw？為什麼 create_child 要在建 zone 前先寫 manifest？

### 第 6 站 `projects/medp/src/gcore/systems/movement.h`（26 行）— system 的樣板

`movement.h:20`：所有未來 system 的形狀範本——自由函式、吃 `Zone&`、用 view 遍歷。位置變更一律經 `move_by`（`movement.h:12`）收口，不直改 Position。

### 第 7 站 `projects/tests/src/main.cpp`（~370 行）— 可執行的規格書

15 個 case 的總表在檔尾 `main()`。每個 test 就是一段「這功能該怎麼用」的示範；改任何行為前先看對應 test 的期望。

### 附錄 `projects/medp/src/gbind/`

目前只有接線 smoke-test（`MedpCore::version()`），可略過；動它前讀 `projects/archived/gd/`。

## 維護規則

- 原始碼檔案新增/刪除/職責改變時，agent 更新 CODE_MAP 的同時檢查本檔受影響的站點。
- 行號錨點漂移不必逐次修，重寫該站時一併校正；符號名為準、行號為輔。
- HTML 導覽層（`html/`）由 `html/build.py` 生成：程式碼變動後重跑 `python3 build.py`
  即重新嵌入最新原始碼；各站導讀文字先改本檔，再同步 `build.py` 的 `STATIONS`。
