# P0：存檔版本欄位 + Position 補 z + 移動收口 — 實作計畫

- 出計畫日期：2026-07-22
- 依據：[tome4 報告](../../docs/work/design/tome4_recommendations.md) §2-A、§2-E-1、§2-B 收口警訊（[報告:67](../../docs/work/design/tome4_recommendations.md)）；§4 step 1。
- 對應 spec：無（設計已定，直接出 plan）。定址/生命週期另見 [zone-addressing-lifecycle-design](../specs/zone-addressing-lifecycle-design.md)（待拍板，與本計畫獨立）。
- 計畫草案經獨立稽核員對照實碼覆核，本文已含其修正（驗證盲區、缺 include、cereal 檔頭細節、幻覺引用點、慣例收尾）。

`Done when:` Task 1-3 全部落地；medp/medp_static 建置全綠**且** movement.h 通過臨時 TU 語法編譯；CODE_MAP/CODE_TOUR 過期敘述同步；WAIT_USER 排待過目、SESSION-LOG 記 open 狀態。**不包含**：測試套件重建（使用者指示暫緩）、spec 落地、html 導覽層重生（記待辦）。

## 執行順序

Task 1 → Task 2（同一 commit 或緊接——「v1 定版含 z」的宣稱才成立，避免中間態寫出無版本的三欄 Position 檔）；Task 3 獨立，可並行。

## Task 1 — 存檔版本欄位（[zone_io.h](../../projects/medp/src/gcore/serialize/zone_io.h)）

1. 補 include：`<cstdint>`、`<stdexcept>`、`<string>`（現行僅 `<istream>/<ostream>`，uint32_t 靠 zone.h 間接取得）。
2. 新增常數：

   ```cpp
   inline constexpr uint32_t SAVE_FORMAT_MAGIC   = 0x5044454D; // 小端輸出下磁碟讀作 "MEDP"
   inline constexpr uint32_t SAVE_FORMAT_VERSION = 1;
   ```

3. `save`：第一個 cereal 區塊改為 `ar(SAVE_FORMAT_MAGIC, SAVE_FORMAT_VERSION, z.id, z.parent, z.layers);`
4. `load`：先讀 magic 與 version，任一不符 → `throw std::runtime_error`（訊息含實得/期望值），相符才續讀。
5. 檔頭註解寫明檔案佈局：offset 0 是 cereal PortableBinary 的 endian byte（archive 建構時自寫），**magic 實際落在 offset 1-4**；第二塊（registry_io）另帶自己的 endian byte。
6. 例外契約（註解明示）：格式錯誤 → `std::runtime_error` **或其衍生**——空檔/截斷檔會在 magic 檢查前就由 cereal 丟 `cereal::Exception`（其繼承 std::runtime_error），上游 catch `std::runtime_error&` 即涵蓋兩者。
7. `ZoneManager::load` 不改：檔案不存在回 false；格式例外往上傳（fail-fast）。
8. 已知取捨：磁碟上舊格式檔一律作廢，重寫期可接受。

驗證：建置全綠（zone_manager.cpp 的 include 鏈覆蓋本檔）。

## Task 2 — Position 補 z（[position.h](../../projects/medp/src/gcore/components/position.h)）

1. 補 `#include <cstdint>`（現行零 include）。
2. `struct Position { int x{}; int y{}; int16_t z{}; }`；`serialize` 改 `ar(x, y, z);`。z 型別對齊 `Zone::layers` 鍵（int16_t）；註解說明 z 即 layers 的鍵、地面=0 往下為負。
3. 順手修 stale 名稱「Zone::map」→「Zone::layers」：[position.h:3](../../projects/medp/src/gcore/components/position.h) 與 [tdarray.hpp:13](../../projects/medp/src/gcore/util/tdarray.hpp)（一字之改，behavior-preserving）。
4. 不動的（已確認）：AllComponents 清單（Position 已登記，變的是佈局）；tdarray 的 is_coor concept（只要求 .x/.y）。
5. 不另 bump 版本：與 Task 1 同屬 v1 定版。

驗證：建置全綠（include 鏈同樣覆蓋本檔）。

## Task 3 — 移動收口（[movement.h](../../projects/medp/src/gcore/systems/movement.h)）

1. 補 include `../zone/zone.h`（需要 Zone 型別；zone.h→entt/tile/tdarray，無循環相依，稽核已確認）。
2. 新增收口函式：

   ```cpp
   // 位置變更的唯一入口：將來 tile flag 檢查、空間索引維護都掛在這裡（報告 §2-B）。
   inline void move_by(Zone& z, entt::entity e, int dx, int dy);  // 現階段僅座標加法
   ```

3. `systems::movement` 簽章 `(entt::registry&)` → `(Zone&)`；view 迭代帶 entity：`z.reg.view<Position, Velocity>().each([&](entt::entity e, Position&, Velocity& v){ move_by(z, e, v.dx, v.dy); });`。z 不動（Velocity 無 dz；跨層移動屬將來的傳送/樓梯機制）。
4. 檔頭註解加「直改 Position 僅限 zone 內移動且必經 move_by」警語（回應報告 §2-B）。
5. 更新過期引用點（稽核清單，**zone.h 無引用點**、原草案誤列）：
   - [zone_manager.h:53-55](../../projects/medp/src/gcore/zone/zone_manager.h)：範例語境整段改寫——movement 改簽章後本身即 ZoneSystem，可直接 `zm.add_zone_system(systems::movement);`，「包一層」的示範需換情境重寫，不是逐字替換。
   - [movement.h:8-10](../../projects/medp/src/gcore/systems/movement.h) 檔頭。
   - [CODE_MAP.md](../common/code-map/CODE_MAP.md)：10（systems/ 目錄描述）、28（movement 條目）、59（架構不變量 3）——改述為「system 是吃 Zone& 的自由函式；位置變更收口於 move_by」。
   - [CODE_TOUR.md:62](../common/code-map/CODE_TOUR.md)（system 形狀範本）；:46 的 zone_io 行號漂移順手校正。
   - html 生成層（06-systems.html 等）不重生，記待辦（`python3 build.py`）。
6. 完成後在報告 [tome4_recommendations.md:67](../../docs/work/design/tome4_recommendations.md) 的收口警訊處回註「已收口（move_by）」。

驗證：**建置對本檔零覆蓋**（medp 唯一 TU 是 zone_manager.cpp，其 include 鏈不含 movement.h——稽核發現的盲區）。故在 scratchpad 建臨時 TU：

```cpp
#include "gcore/systems/movement.h"
int main() { return 0; }
```

以 `g++ -std=c++20 -fsyntax-only -I projects/medp/src -I projects/medp/include <臨時TU>` 編一次；臨時檔不進 repo。

## 收尾（慣例要求）

- WAIT_USER 排待過目：zone_io.h 的 magic/version 檢查、position.h 的 z、movement.h 的 move_by 收口（各附路徑:行號＋看點）。
- SESSION-LOG 記：本輪未跑測試（套件斷裂中，基準待重建）；存檔格式 v1 break（舊 .bin 一律讀不回）。
- 總驗證聲明：「建置全綠」只覆蓋 Task 1/2，Task 3 靠臨時 TU——兩者都過才算 Done。

## 建置指令

```bash
cmake -S projects/medp -B projects/medp/build && cmake --build projects/medp/build
```
