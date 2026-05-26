# medps ECS 重寫規劃

> 初版 2026-05-25,改寫 2026-05-26。原計畫是「漸進保留 Obj/Scene 再汰除」,執行時改為**打掉重來**:
> 完全移除舊 Obj/Scene/Component/BinFSR,直接以 EnTT + cereal 重建,並採用 **多 registry / zone streaming** 架構。
> 本文已更新為反映實際架構與進度。分支:`rewrite/entt-cereal`。

## 已定案的架構決策

- **ECS**:EnTT,**pin v3.16.0**(single header `include/entt.hpp`)。
  - 3.16.0 的 snapshot API 為 `.get<T>(ar)`(save / snapshot_loader / continuous_loader 皆同)。
  - `registry.alive()` / `registry.each()` 已移除;判存活用 `registry.valid(e)`,計數用 `storage<entt::entity>()`。
- **序列化**:EnTT `snapshot` 遍歷 registry + **cereal `PortableBinaryArchive`** 負責位元格式。cereal 全面取代 BinFSR(已移除)。
  - 透過 `serialize/entt_cereal_archive.h` 的 adapter 橋接(`entt::entity` ↔ 整數顯式處理)。
  - component 型別清單單一來源:`serialize/all_components.h` 的 `AllComponents = entt::type_list<...>`,save/load 共用展開。
- **多 registry / zone streaming**(本次重寫核心):
  - 一個 zone(world / province / area...)= 一個 `entt::registry`,由 `GlobalManager` 管理。
  - **root** registry 永久存活,全局實體(faction/文明/神祇...)住這;其餘 zone **按需載入/卸載**。
  - **ZoneKey**(uint64_t):`ZoneType:16 | x:16 | y:16 | z:16`,全局座標,唯一定址。`ZONE_ROOT == 0`。
  - **path 推導**:zone 磁碟路徑由 ZoneKey 算出(`zone_path`),**不存任何全域清單** → 記憶體只跟當前載入數成正比。
  - **分層 index**:父 zone 的 registry 裡每個直屬子 zone 是一個 `ChildZoneSummary` stub entity,可不載入子 zone 而總覽;index 跟著 streaming。
  - **跨 zone 參照**:`CrossZoneRef { ZoneKey zone; entt::entity local_entity; }`,`resolve` 三態回傳(未載入 / stale / 可用),只查已載入、不碰 IO。
  - **placeholder**:每 zone 一個 `ZoneMeta` entity(self + parent),保證 `orphans()` 不會清空 zone、攜帶身份。
- **解耦**:純 C++ 模擬核心(零 godot 依賴、可獨立編譯/測試)+ 薄 `godot::Object` facade(後期)。核心**永不** `#include` godot-cpp。
- **component 原則**:盡量 POD aggregate;entity 參照存 `entt::entity`;含 STL 成員交給 cereal 逐欄位。

## 目前目錄結構

```
src/gcore/
  zone_key.h              — ZoneKey 打包/解包
  global_manager.{h,cpp}  — GlobalManager:root + loaded zones、create/load/unload/save_all/load_root/resolve/children
  components/
    zone_meta.h           — ZoneMeta(zone 身份 placeholder)
    child_zone_summary.h  — ChildZoneSummary(父 zone 內的子 zone stub)
    cross_zone_ref.h      — CrossZoneRef(跨 zone 參照)
  serialize/
    entt_cereal_archive.h — entt⇄cereal archive adapter
    all_components.h      — AllComponents type_list(單一來源)
    zone_io.h             — zone_io::save / load(stream + path 版本)
  util/
    mydef.h               — 保留的 metaprogramming macros
    tdarray.hpp           — 2D 陣列(Save/Load 改 cereal serialize)
test/src/main.cpp         — 15 個測試(全綠)
others/                   — EnTT/cereal/新增component/zone 架構 教學
```

---

## 進度

### ✅ 已完成
- **打底**:刪除 Obj/Scene/Component/BinFSR;EnTT+cereal smoke 驗證;CMake 接線。
- **ZoneKey + GlobalManager**:zone 的建立/載入/卸載、root、path 推導。
- **序列化**:archive adapter、AllComponents 單一來源、zone_io、tdarray cereal 化。
- **跨 zone resolve**:`CrossZoneRef` + `ZoneResolution` 三態。
- **分層 index**:`ChildZoneSummary` stub、`create(key,parent)` 自動登錄(冪等)、`children()` 列舉。
- **整盤存讀**:`save_all`(存檔點)、`load_root`(開遊戲),含真實磁碟 round-trip 測試。
- **教學**:`others/` 四份文件。

### ▶ 下一步:基礎遊戲 component(原 Phase 「先加不減」)
- [ ] `components/` 定義首批遊戲 component:至少 `Position`、`Owner`(entity 參照存 `entt::entity` 或 `CrossZoneRef`)。
- [ ] 登錄進 `AllComponents`,補 round-trip 測試。

### 待辦
- **systems 層**:`src/gcore/systems/`,自由函式吃 `entt::registry&`(或之後的 World/GlobalManager&);先用有序清單跑 tick,不過度設計。
- **ZoneType 定案**:目前只有 `Invalid = 0` 佔位;待遊戲設計成熟再定(Province/Area/...,以及與 z 軸層的關係)。
- **Godot facade(後期,獨立)**:`src/gbind/` 繼承 `godot::Object`,`_bind_methods` 暴露給 GDScript;CMake 第二 target 連 core static lib + godot-cpp。核心保持零 godot。

---

## 跨階段慣例
- 核心任何檔案**不得** `#include` godot-cpp(之後可加 grep guard 防呆)。
- component 盡量 POD aggregate,利於 cereal/跨平台。
- 新增 component:`components/<name>.h` + `all_components.h` 加一行(永遠加在最後,勿插中間)。
- 每階段結束保持可編譯 + 測試綠燈。

## 參考
- 教學:`others/entt_tutorial.md`、`others/cereal_tutorial.md`、`others/how_to_add_component_and_system.md`、`others/zone_streaming_architecture.md`
- 核心模組職責:`work/architecture/level2.md`
- BinFSR 缺陷與「為何取代」:`work/architecture/binfsr_audit.md`
