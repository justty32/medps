# medps ECS 重寫規劃

> 制定日期:2026-05-25。本文件是可分階段執行的實作規劃,逐步進行、每階段結束都應能編譯 + 測試通過。

## 已定案的架構決策
- **ECS**:採用 EnTT,**pin 至穩定發行版 v3.16.0**(已 clone 至 `extern/entt` 並 checkout tag `v3.16.0`)。
  注意:預設 clone master HEAD 會拿到未發行的 4.0-dev,且其 checked-in 的 `single_include/entt.hpp` 仍停在舊的 3.16.0(amalgamation 沒跟著 regenerate),src 與 single_include 不一致——故務必 pin release tag。
  地圖維持 grid(`tdarray<Tile>`),只有動態行動者(unit/city/hero/faction)是 entity。
  已 smoke 驗證:entt v3.16.0 single_include + cereal PortableBinary 在 C++20/MSVC 下能編譯並 round-trip。
  3.16.0 的 snapshot API 為 `snapshot.get<T>(ar)` / `snapshot_loader.get<T>(ar)` / `continuous_loader.get<T>(ar)`(與 Phase 2 規劃一致)。
- **混合**:`Obj`/`Scene` 保留給單例系統;EnTT registry 管 entity。
- **解耦**:純 C++ 模擬核心(零 godot 依賴、可獨立編譯/測試)+ 薄 `godot::Object` facade(只在當 GDExtension 時編譯)。核心**永遠不 `#include` godot-cpp**。
- **序列化**:EnTT `snapshot` 負責遍歷 registry + **cereal**(`PortableBinaryArchive`)負責位元格式。cereal **全面取代 BinFSR**(含 Obj/Scene/tdarray),`bin_fwr.hpp` 移除。
- **component 原則**:盡量是單純 aggregate POD;entity 參照存 `entt::entity`;含 STL 成員交給 cereal(逐欄位、不整 struct memcpy)。

## 目標目錄結構(完成後)
```
src/gcore/
  util/        — Obj, Scene, tdarray, mydef...(cereal 化,移除 bin_fwr.hpp / component.*)
  world.{h,cpp}— World:持有 entt::registry + Scene + 地圖 grid
  components/  — POD component structs
  systems/     — 自由函式 system(吃 World&)
  serialize/   — cereal⇄EnTT snapshot 的 archive adapter、component 型別清單
  obj_types/   — 單例領域物件(TileMap grid 等)
src/gbind/     — (後期)godot::Object facade
include/cereal/— vendored cereal
include/entt.hpp 或 extern/entt — vendored EnTT
```

---

## Phase 0 — vendoring 與 build 接線
- [ ] 決定 vendoring 方式。**建議**:沿用現有 `include/` 單檔慣例 —— 把 `extern/entt/single_include/entt/entt.hpp` 複製到 `include/entt.hpp`;cereal 是 header 樹,複製 `cereal/include/cereal/` 到 `include/cereal/`。複製後可刪 `extern/`(避免 nested .git)。
- [ ] cereal 已 clone 至 `extern/cereal`(v1.3.2,self-contained,自帶 rapidjson/rapidxml/base64),取其 `include/cereal/`。建議也 pin tag `v1.3.2`(目前 master ≈ 1.3.2,風險低)。
- [ ] `CMakeLists.txt`:確認 `include/` 已在 include path(目前 line 38 有)。若改放 extern,需另加 `include_directories`。
- [ ] smoke test:`test/` 加一個 `.cpp`,`#include <entt.hpp>` + `#include <cereal/archives/portable_binary.hpp>`,建立 registry、用 cereal 存讀一個 int,確認編譯 + 連結 + 執行通過。
- **Done when**:smoke test 編譯並跑過。

## Phase 1 — cereal 取代 BinFSR(尚不動 ECS)
- [ ] 設計核心序列化介面:統一用 cereal archive(例如 `cereal::PortableBinaryOutputArchive&` / `...InputArchive&`)取代 `BinFSR::ostream_t&/istream_t&`。
- [ ] 改寫 `Obj`、`Scene`、`tdarray` 的 Save/Load → cereal(`tdarray` 直接 `ar(sx, sy, vec)`;`Scene` 序列化 objs)。
- [ ] (評估,可選)cereal 的 polymorphic 序列化(`CEREAL_REGISTER_TYPE`)能直接序列化 `Obj*` 多型,**有機會取代**現有 type-id 工廠 + `obj_types_list.cpp` 的雙重登錄痛點。本階段先不強做,記為後續優化選項。
- [ ] 移除 `src/gcore/util/bin_fwr.hpp` 及所有引用。
- [ ] 全程用 `CEREAL_CLASS_VERSION` 標版本,為日後存檔相容鋪路。
- [ ] 測試:Scene 建幾個 Obj → 存 → 讀 → 驗證 round-trip。
- **Done when**:無 BinFSR、Scene round-trip 測試通過、跨平台疑慮由 cereal Portable 消化。

## Phase 2 — 導入 World + EnTT registry(先加不減)
- [ ] 新增 `World`:持有 `entt::registry registry` + `Scene scene` + 地圖 grid。systems 一律吃 `World&`。
- [ ] `src/gcore/components/` 定義首批 POD component(至少 `Position`)。
- [ ] `src/gcore/serialize/`:寫 cereal⇄EnTT snapshot 的 **archive adapter**(EnTT 的 `archive(value)` → 轉呼叫 cereal 的 `ar(value)`;`entt::entity` ↔ 整數要顯式處理,cereal 預設不序列化 enum)。
- [ ] **component 型別清單**:用單一來源(如 `using AllComponents = entt::type_list<Position, ...>` 或 X-macro)同時驅動 snapshot 的 save 與 load,避免重蹈 `obj_types_list` 漏列覆轍。
- [ ] World 的 save/load 串起:Scene(cereal)+ registry(snapshot + 同一個 cereal archive)。
- [ ] 測試:建 entity + component → 存 → 讀進空 registry(`snapshot_loader` 保留 id)→ 驗證資料與參照都對。
- **Done when**:World round-trip 測試通過。

## Phase 3 — 汰除舊 Component 系統
- [ ] `TileMap`(原 `Component + tdarray`)→ 改成 World 持有的 grid / 單例 `Obj`,不再是 Component。
- [ ] `MapEntity`(原 Obj)→ 拆成 `Position`(+其他)component,以 entity 形式存在。
- [ ] 移除 `Component` / `ComponentManager`(`component.hpp/.cpp`)及其在 `obj_types_list.cpp` 的登錄。
- [ ] `obj_types_list.cpp` 現在只留真正的單例型別。
- **Done when**:無 Component/ComponentManager、編譯通過、既有測試通過。

## Phase 4 — systems 層
- [ ] `src/gcore/systems/` 加首個 system(自由函式,吃 `World&`),例如移動或回合推進。
- [ ] 簡單的 system 執行順序(先用一個有序清單即可,不過度設計)。
- **Done when**:一個 system 能改 registry 狀態並被測試驗證。

## Phase 5 —(後期,獨立)Godot facade
- [ ] `src/gbind/`:繼承 `godot::Object` 的 facade 類別,`_bind_methods` 暴露給 GDScript。
- [ ] CMake 第二個 target:GDExtension shared lib,連 core static lib + godot-cpp(godot-cpp 的 `classes/` 需先 `binding_generator.py` 生成)。
- [ ] 核心保持零 godot;facade 負責 bridge。core static lib + `medp_test.exe` 仍可獨立 build/跑。
- **Done when**:Godot 載入 GDExtension、GDScript 能呼叫 facade;且獨立 test.exe 不受影響。

---

## 跨階段慣例
- 核心任何檔案**不得** `#include` godot-cpp(之後可加一個 grep guard 防呆)。
- 不把 entity 做成 `godot::Object`;facade 只是上層薄殼。
- component 盡量單純 aggregate,利於 cereal/反射且跨平台。
- 每階段結束保持可編譯 + 測試綠燈,方便慢慢推進。

## 參考
- BinFSR 缺陷與「為何取代」:`work/architecture/binfsr_audit.md`
- 核心模組職責:`work/architecture/level2.md`
