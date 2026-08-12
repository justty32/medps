# medps 進度總覽

> 最後更新：2026-08-12（World 子類＋gcore 目錄重整＋actor 基礎設施＋`projects/game/` 原型 consumer 落地後）。

## 一句話

**medps** 是奇幻 4X 策略遊戲的 C++20 後端函式庫（`medp`），前端預計用 Godot 4 GDExtension。
2026-07-22 完成核心重寫（Zone/ZoneManager）後，07-22～07-25 密集落地三件事：Zone 子類化
（`World`）、actor 基礎設施（ECS 組合式地點／部隊）、以及第一個真實 consumer `projects/game/`
（可玩的 ASCII 策略遊戲原型）。測試基準 15 → 21，21/21 綠（Windows MinGW 實測）。
遊戲內容（種族、宗教、外交等）與 Ruleset 規則庫仍未動工。

## 已落地（地基層）

World／actor／game 三件事的逐檔細節另見
[World/actor/game 落地筆記](architecture/world_actor_game.md)。

| 項目 | 狀態 |
|------|------|
| **Zone 核心** | `Zone{id, parent, reg, layers}`：一 zone 一 `entt::registry`＋自帶多層 tile 地圖（`map<int, tdarray<Tile>>`，鍵即 z，地面=0 往下為負） |
| **定址** | 裸 `uint64_t` 單調序號（0=root、永不復用），`create_child` 單點配發；zone id 零座標語意 |
| **ZoneManager** | root 永駐；get/create_child/load/unload/destroy/save_all/tick；開檔協定（manifest 還原計數器＋root 必讀回＋損毀 throw） |
| **存檔** | 一 zone 一檔（`<16hex>.bin`，root=`root.bin`）＋`manifest.bin`；cereal PortableBinary＋EnTT snapshot；**無版本欄位**（格式變更＝刪存檔） |
| **Zone 子類化** | `ZoneKind` tag（`gcore/zone/zone.h:17`）＋`make_zone`／`zone_cast<T>`；`World : Zone` 是第一個子類，libtcod FBM worldgen（0b7c652，[細節](architecture/world_actor_game.md#zone-子類化world-落地commit-0b7c652)） |
| **gcore 目錄重整** | 拆成 `zone/`（通用）／`world/`（World 專屬）／`common/`（跨 zone 共用）／`serialize/`／`util/`（75c15a4） |
| **actor 基礎設施** | ECS 組合式地點／部隊：`Name`+`Owner`+`Location`/`Unit`，kind 是住 root 的資料驅動 def，`define_location`/`define_unit`（`gcore/common/actor.h:40-55`）（78bacfd，[細節](architecture/world_actor_game.md#actor-基礎設施commit-78bacfd)） |
| **projects/game/ 原型** | `game.cpp`（787 行）可玩 ASCII 策略遊戲，第一個真實呼叫地基層 API 的 consumer——非平行實作（ab9d054＋07e37f3） |
| **測試** | 21 項全綠（15 新核心＋4 World 子類：kind round-trip／generate 決定性／generate sanity／未知 kind fail-fast＋2 actor：defs_and_spawn／roundtrip），是 behavior-preserving 基準 |
| **Godot 接線** | smoke-test 證明前端能呼叫後端（gbind，重寫前驗證，仍有效） |

## 拍板未實作 / defer（見 [spec 拍板結果](../../workflows/specs/zone-addressing-lifecycle-design.md)）

- 目錄分桶、persistence 兩態（Ephemeral/Persistent）、LRU 卸載：defer，痛了再說。
- parent→children 連結、跨 zone 實體引用、返回座標、Portal：defer 且**不凍結設計**，動工前重審。
- 使用者點名的未來需求：「清理很久沒訪問且不重要的 zone .bin」機制。
- 三層世界（World/Region/Area）的尺度願景仍有效（見 [zone_layers](design/zone_layers.md)），
  `World` 是第一層落地；Region/Area 仍 defer。
- actor def id 目前手動配置，defer 到 Ruleset 落地時由登錄機制發號。

## 下一步候選

- Ruleset（規則庫）層：`projects/game/src/game.h:19-20` 把 `KIND_CITY`／`KIND_WARRIOR` 寫死成
  常數、`game.h:12-13` 的 `Health`／`Moves` 刻意不進 `AllComponents` 不序列化——這兩處是 Ruleset
  該接手的具體位置，原型已經替它寫好需求規格。設計方向另見 [lifecycle](design/lifecycle.md)。
- 第一個真正的玩法系統（會觸發 children/引用等 defer 項落地）。
- ToME4 研讀的 P1/P2 建議：見 [tome4_recommendations](design/tome4_recommendations.md)。

## 關鍵 gotcha

- `all_components.h` 是序列化唯一來源——新增 component 忘記登記，存檔會**默默**漏掉它。
- `loader.orphans()`：讀檔時沒有任何（已登記）component 的 entity 會被清掉——只帶未登記
  component 的 entity 整個消失。
- 存檔無版本欄位：格式一變，舊 `.bin` 讀出來就是壞資料，不會報錯；重寫期直接刪存檔目錄。
  **本期已 break 兩次**（World kind tag、actor component），舊存檔一律作廢。
- 位置變更必經 `move_by`，不直改 Position——將來 tile flag 檢查與空間索引維護都掛這個口。
- Zone 子類 virtual dtor 使其不可複製也不可移動，一律經 `unique_ptr`/參照持有——別想用值語意搬 Zone。
