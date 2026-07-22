# medps 進度總覽

> 最後更新：2026-07-22（Zone/ZoneManager 新核心＋定址/生命週期拍板落地後）。

## 一句話

**medps** 是奇幻 4X 策略遊戲的 C++20 後端函式庫（`medp`），前端預計用 Godot 4 GDExtension。
2026-07-22 完成核心重寫：拆掉 ZoneKey/GlobalManager 舊架構，換上更薄的 Zone/ZoneManager 新核心；
存讀檔、id 配發、移動收口、測試基準都已重建。遊戲內容（種族、宗教、戰鬥、外交等）仍未動工。

## 已落地（地基層）

| 項目 | 狀態 |
|------|------|
| **Zone 核心** | `Zone{id, parent, reg, layers}`：一 zone 一 `entt::registry`＋自帶多層 tile 地圖（`map<int, tdarray<Tile>>`，鍵即 z，地面=0 往下為負） |
| **定址** | 裸 `uint64_t` 單調序號（0=root、永不復用），`create_child` 單點配發；**zone id 零座標語意**（舊 ZoneKey 位元打包已移除） |
| **ZoneManager** | root 永駐；get/create_child/load/unload/destroy/save_all/tick；開檔協定（manifest 還原計數器＋root 必讀回＋損毀 throw） |
| **存檔** | 一 zone 一檔（`<16hex>.bin`，root=`root.bin`）＋`manifest.bin`（僅 `next_zone_id`，未來擴充為存檔 metainfo）；cereal PortableBinary＋EnTT snapshot；**無版本欄位**（使用者裁定，格式變更＝刪存檔） |
| **元件** | Position{x,y,z}、Velocity（示範用）；清單唯一來源 `all_components.h` |
| **移動** | `systems::move_by` 是位置變更唯一入口；system 是吃 `Zone&` 的自由函式，簽章即 ZoneSystem |
| **測試** | 15 項全綠（序列化/tdarray/zone_io/ZoneManager 損毀防護/movement），是 behavior-preserving 基準 |
| **Godot 接線** | smoke-test 證明前端能呼叫後端（gbind，重寫前驗證，仍有效） |

## 拍板未實作 / defer（見 [spec 拍板結果](../../workflows/specs/zone-addressing-lifecycle-design.md)）

- 目錄分桶、persistence 兩態（Ephemeral/Persistent）、LRU 卸載：defer，痛了再說。
- parent→children 連結、跨 zone 實體引用、返回座標、Portal：defer 且**不凍結設計**，動工前重審。
- 使用者點名的未來需求：「清理很久沒訪問且不重要的 zone .bin」機制。
- 三層世界（World/Region/Area）的尺度願景仍有效（見 [zone_layers](design/zone_layers.md)），但以 parent 樹＋玩法層表達，不再烤進 id。

## 下一步候選

- Ruleset（規則庫）層：設計方向見 [lifecycle](design/lifecycle.md)，尚未實作。
- 第一個真正的玩法系統（會觸發 children/引用等 defer 項落地）。
- ToME4 研讀的 P1/P2 建議：見 [tome4_recommendations](design/tome4_recommendations.md)。

## 關鍵 gotcha

- `all_components.h` 是序列化唯一來源——新增 component 忘記登記，存檔會**默默**漏掉它。
- `loader.orphans()`：讀檔時沒有任何（已登記）component 的 entity 會被清掉——只帶未登記 component 的 entity 整個消失。
- 存檔無版本欄位：格式一變，舊 `.bin` 讀出來就是壞資料，不會報錯；重寫期直接刪存檔目錄。
- 位置變更必經 `move_by`，不直改 Position——將來 tile flag 檢查與空間索引維護都掛這個口。
