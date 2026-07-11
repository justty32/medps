# CODE_TOUR — 給人的程式碼導讀

給專案作者用的線性閱讀路徑：照順序讀，每站說明「這檔在幹嘛、看什麼、讀完該能回答什麼」。
[CODE_MAP.md](CODE_MAP.md) 是 agent 修改前的查表；本檔是人的導讀，兩者同鏈維護（見文末）。
瀏覽器版導覽在 [html/index.html](html/index.html)（嵌入帶行號的原始碼；由 `html/build.py` 生成）。

目前全部核心碼約 1,200 行，照本路徑一次讀完約 1–1.5 小時。

## 全貌一句話

`GlobalManager` 管一堆 `entt::registry`（一個 zone 一個），zone 用 64-bit `ZoneKey` 定址，
存讀檔 = snapshot 遍歷 registry → cereal 轉位元組 → `ZoneStore` 落盤；`tick()` 對每個已載入 zone 跑所有註冊的 system。

## 閱讀路徑（依依賴順序）

### 第 1 站 `src/gcore/zone_key.h`（105 行）— 座標語言

整個專案的「門牌系統」，其他所有檔案都建立在它的語意上。

- `ZoneType`（`zone_key.h:9`）：World ⊃ Region ⊃ Area 嚴格三層，type 值即樹深度，父層 type 隱含。
- `zone_scale`（`zone_key.h:31`）：尺度常數單一來源；`world_dim` 是唯一 per-save 執行期設定。
- 打包/解包（`zone_key.h:53`）：`ZoneType:16|x:16|y:16|z:16`。
- 換算（`zone_key.h:72-105`）：注意 **Area 的 x,y 是全局 region-格座標**（`world*REGION_DIM + local`），`parent_of` 用整數除法回推。
- 檔內多處 `TODO: 要掛在 ruleset 底下`——這是已知的未來方向。

讀完該能回答：一個 Area key 的父 Region key 怎麼算？z 在換算鏈中如何傳遞？（答案都在 `parent_of`，`zone_key.h:91`）

### 第 2 站 `src/gcore/util/tdarray.hpp`（173 行）— 唯一的容器工具

2D 陣列模板，row-major（`vec[x*sy+y]`，`tdarray.hpp:63`），已 cereal 化。掃過 `resize/out/get/getref` 即可，不必細讀。

### 第 3 站 `src/gcore/components/`（6 檔，共 ~95 行）— 資料積木

全是 POD aggregate + `serialize()` 成員。快速掃過，特別留意兩個「非典型」的：

- `WorldConfig`（`world_config.h:10`）：**singleton，掛在 ROOT** 上，per-save 不可變（world_dim 已烘進 key 運算）。
- `ZoneMeta`（`zone_meta.h:7`）：每個 zone 的 placeholder entity 持有，保證 zone 至少有一個非孤兒 entity（跟第 4 站的 `orphans()` 互相咬合）。
- `AreaTerrain`（`area_terrain.h:22`）：密集 terrain grid 掛在單一「map」entity 上；tile **不是**一格一 entity。通行性兩層：terrain flags（這裡）+ 逐 entity 的 `Blocking`。

### 第 4 站 `src/gcore/serialize/`（4 檔，共 ~175 行）— 存讀檔管線

建議順序：

1. `all_components.h:12` — `AllComponents` type_list，**新增 component 唯一要登記的地方**。
2. `entt_cereal_archive.h` — 純膠水：把 entt snapshot 的 callback 簽章轉成 cereal 呼叫，`entt::entity` ↔ 底層整數。
3. `zone_io.h:14,21` — `save_impl/load_impl` 用 fold expression 展開 AllComponents；`zone_io.h:28` 的 `loader.orphans()` 是**陷阱點**：load 後沒有任何 component 的 entity 會被清掉（所以才需要 ZoneMeta placeholder）。
4. `zone_store.h:15` — `ZoneStore` 抽象（bytes↔儲存）與 `FolderZoneStore`；`path()`（`zone_store.h:37`）：key → `dir_/<16碼hex>.bin`，root 特例 `root.bin`。

讀完該能回答：為什麼新 component 忘了登記 AllComponents 存檔會「默默」漏掉它、不會報錯？

### 第 5 站 `src/gcore/global_manager.h` / `.cpp`（168 行）— 總管

把前四站全部接起來。header 註解已寫得很完整，`.cpp` 每個函式都在 10 行以內：

- `create`（`global_manager.cpp:24`）：植入 ZoneMeta placeholder。
- `load`（`global_manager.cpp:34`）：store 沒有該 key 時**靜默給空 registry**——目前語意如此，不是 bug。
- `tick`（`global_manager.cpp:90`）：對每個已載入 zone × 每個 system 依註冊順序跑；**root 不參加 tick**。
- `init_world`（`global_manager.cpp:66`）：assert 檢查 `valid_world_dim`，冪等覆寫 singleton。

### 第 6 站 `src/gcore/systems/movement.h`（18 行）— system 的樣板

`movement.h:11`：所有未來 system 的形狀範本——自由函式、吃 `entt::registry&`、用 view 遍歷。

### 第 7 站 `test/src/main.cpp`（363 行）— 可執行的規格書

16 個 case 的總表在 `main.cpp:336`。每個 test 就是一段「這功能該怎麼用」的示範；改任何行為前先看對應 test 的期望。

### 附錄 `src/gbind/`

目前只有接線 smoke-test（`MedpCore::version()`），可略過；動它前讀 `notes/gd/`。

## 維護規則

- 原始碼檔案新增/刪除/職責改變時，agent 更新 CODE_MAP 的同時檢查本檔受影響的站點。
- 行號錨點漂移不必逐次修，重寫該站時一併校正；符號名為準、行號為輔。
- HTML 導覽層（`html/`）由 `html/build.py` 生成：程式碼變動後重跑 `python3 build.py`
  即重新嵌入最新原始碼；各站導讀文字先改本檔，再同步 `build.py` 的 `STATIONS`。
