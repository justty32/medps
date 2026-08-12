# World / Actor / Game 原型落地筆記

> 對象：2026-07-22～07-25 落地的三件事——Zone 子類化、gcore 目錄重整、actor 基礎設施、
> 以及第一個真實 consumer `projects/game/`。回總覽見 [progress_overview.md](../progress_overview.md)。

## Zone 子類化：World 落地（commit 0b7c652）

- `Zone`（`projects/medp/src/gcore/zone/zone.h:28-57`）改成繼承基底：`ZoneKind` enum tag
  （`zone.h:17`，Plain=0/World=1）寫進 zone 檔頭；`kind()` virtual 由子類 override 回答型別，
  不另存欄位，杜絕「欄位與實際型別不一致」。
- `make_zone(ZoneKind)`（`zone.h:61`）依 tag 建構子類，未知 kind 直接 throw，不靜默退回 Plain。
- `zone_cast<T>(Zone*)`（`zone.h:65-68`）型別安全向下轉型：kind 相符回傳子類指標，否則 nullptr。
- virtual dtor 使隱式 move 被抑制——`Zone` 不可複製也不可移動，一律經 `unique_ptr`/參照持有，
  子類物件切片結構上不可能發生（`zone.h:25-27` 註解）。
- `save_extra`/`load_extra`（`zone.h:55-56`）是子類專屬資料的序列化掛鉤，由 zone_io 在
  id/parent/layers 之後呼叫；基底 no-op，save/load 兩邊必須成對對稱。
- `World : Zone`（`projects/medp/src/gcore/world/world.h:8-19`）是第一個子類：`gen`
  （`WorldGenParams`）隨 `save_extra`/`load_extra` 序列化（`world.h:14-15`），`generate()`
  （`world.h:18`，實作在 `world.cpp:5`）是薄殼，委派 `world_gen::generate` 並挑 `layers[0]`。
- worldgen 演算法在 `world_gen.{h,cpp}`：FBM 高度圖 → `sea_level` 切水陸 → 溫濕度分 biome
  （`world_gen.h:29-36`），純函式簽章 `generate(const WorldGenParams&, tdarray<Tile>&, uint64_t zone_id=0)`，
  **不認識 `World` 型別**——依賴方向反過來，`World::generate()` 才認識它。同 seed 同圖（決定性）。
  走 libtcod 2.2.2 headless（CMake FetchContent 鎖版，首次 configure 需網路）。

## gcore 目錄重整（commit 75c15a4）

`gcore/` 從扁平單層拆成：

- `zone/`：通用框架——`zone.h`/`.cpp`、`zone_manager.h`/`.cpp`、`tile.h`。
- `world/`：World 專屬——`world.{h,cpp}`、`world_gen.{h,cpp}`、`components/{position,velocity}.h`、
  `systems/movement.h`。
- `common/`：跨 zone 共用——`actor.h`、`components/{name,owner,location,unit}.h`。
- `serialize/`、`util/`：不變。

worldgen 同批抽成獨立模組（見上）。**注意**：`docs/work/architecture/gcore_overview.md` 仍是
重整前的扁平版導覽，尚未跟上這次拆分與本文件的內容，讀時留意，待補。

## actor 基礎設施（commit 78bacfd）

模型是 ECS 組合，不是 C++ 繼承——「actor 基底」＝一組共用元件：

- 身分：`Name` ＋ `Owner`（`projects/medp/src/gcore/common/components/{name,owner}.h`）
- 家族：`Location{kind}` 或 `Unit{kind}`，互斥，一個 actor 只屬一族
  （`common/components/{location,unit}.h`）
- 放置：`Position` 刻意與身分分離，由放置流程另掛，spawn 不含（`actor.h:17` 註解）

種類（kind）是資料驅動的 def，不是寫死的 enum：def 是住在 **root zone** 的實體
（`LocationKind{id}`／`UnitKind{id}` ＋ `Name`），actor 以穩定 `uint64_t` id 參照它
（跨 registry 安全，比照 zone id／`Owner.faction`，不用 entt handle）。

- `define_location`/`define_unit`（`actor.h:40-55`）建 def 實體，`detail::require_root`
  （`actor.h:32-36`）fail-fast 檢查 `id == ZONE_ROOT`，傳非 root 直接 throw。
- `find_location_def`/`find_unit_def`（`actor.h:59-69`）依 id 線性掃 root（def 數量小，
  需要時再上索引）。
- `spawn_location`/`spawn_unit`（`actor.h:74-91`）建實際 actor 實體。
- 全部已登記進 `projects/medp/src/gcore/serialize/all_components.h:12-21`（`AllComponents` 清單）。
- def id 目前手動配置；將來 Ruleset 落地時由檔案載入＋登錄機制發號（比照 ZoneManager 配號）。

## 第一個真實 consumer：projects/game/（commit ab9d054 + 07e37f3）

- `game.cpp`（787 行）＋ `game.h`/`display.h`/`main.cpp`，連結 `medp_static` ＋ `libtcod_static`。
- 80×40 FBM 世界地圖、玩家 2 城 3 部隊 vs AI 2 城 3 部隊、回合制、貪婪 AI、戰鬥／佔城／勝敗判定。
- 真的在用地基層 API，不是平行實作：`actor::define_location`/`define_unit`
  （`projects/game/src/game.cpp:60-61`）、`actor::spawn_location`（`game.cpp:103`）、
  `actor::spawn_unit`（`game.cpp:111`）。
- 遊戲專用元件 `Health`/`Moves`（`game.h:12-13`）刻意不進 `AllComponents`，不序列化——純運行期狀態。
- 種類目前寫死常數 `KIND_CITY`/`KIND_WARRIOR`（`game.h:19-20`），是 Ruleset 落地前的暫代。
- 07e37f3（07-25）補 POSIX raw-mode 輸入層（`projects/game/src/display.h`）：非 Windows 分支原本
  `wait_key()` 直接回 `Key::UNKNOWN`，配上主迴圈就成了不吃輸入的忙轉迴圈（實測 3 秒噴 505 MB
  終端輸出）；修法是 `disp::detail` 下的 `enter_raw`/`leave_raw`/`apply_raw`/`read_byte`，`read()`
  逐 byte 讀，`0x1b` 開頭以 ~0.1 秒 `VTIME` 逾時區分單獨 ESC 與方向鍵 CSI 序列
  （`\033[A~D`／`\033OA~D`）。Linux 從「編譯得過」變成「真能玩」。

## 存檔格式 break 紀錄

這期間存檔格式 break 兩次：World 子類加了 zone 檔頭 kind tag（0b7c652）、actor 加了新
component（78bacfd）。舊 `.bin` 一律作廢，呼應 [progress_overview.md](../progress_overview.md)
的「無版本欄位」gotcha。
