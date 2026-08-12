# Godot 4 GDExtension 接入教學

> 本文說明 medps 如何把純 C++ 核心接上 Godot 4(GDExtension),以及怎麼新增暴露給 GDScript 的 facade。
> 對應程式碼:`projects/medp/src/gbind/`、`projects/medp/CMakeLists.txt` 的 `MEDP_BUILD_GDEXTENSION` 區塊。
> 已驗證環境:godot-cpp(target **Godot 4.6**)、C++20 / MSVC。

---

## 目錄

本教學拆成兩個子檔:

1. [架構與建置](godot_gdextension_setup_build.md) — 核心零 godot / facade 薄殼的架構原則、godot-cpp 如何透過 `add_subdirectory` 接進 CMake、`MEDP_BUILD_GDEXTENSION` option 與 build 流程。
2. [gbind 開發與除錯實務](godot_gdextension_setup_gbind.md) — facade 類別 / `register_types` 進入點 / `.gdextension` 描述檔三個組成部分、在 Godot 專案裡載入測試、如何新增暴露給 GDScript 的方法或 class、常見排錯項目。

---

## 參考

- zone / 核心架構:`docs/references/zone_streaming_architecture.md`
- 新增 component / system:`docs/references/how_to_add_component_and_system.md`
- 程式碼:`projects/medp/src/gbind/`、`projects/medp/CMakeLists.txt`(MEDP_BUILD_GDEXTENSION 區塊)
- godot-cpp checkout:`C:\code\mine\pas\projects\godot-cpp`(target Godot 4.6)
