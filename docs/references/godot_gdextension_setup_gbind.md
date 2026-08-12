# Godot 4 GDExtension 接入教學:gbind 開發與除錯實務（medps 版）

> 本篇是 [godot_gdextension_setup.md](godot_gdextension_setup.md) 的子篇,說明 gbind 的三個組成部分、在 Godot 裡載入測試、如何新增暴露元件、以及驗證/排錯。
> 上一篇:[架構與建置](godot_gdextension_setup_build.md)

---

## 3. gbind 的三個部分

### 3-1. facade 類別

```cpp
// projects/medp/src/gbind/medp_core.h
#pragma once
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/string.hpp>

namespace medp_gd {

class MedpCore : public godot::RefCounted {
    GDCLASS(MedpCore, godot::RefCounted)   // 向 Godot 型別系統登記
protected:
    static void _bind_methods();           // 把方法綁給 GDScript
public:
    godot::String version() const;
};

} // namespace medp_gd
```

```cpp
// projects/medp/src/gbind/medp_core.cpp
#include "medp_core.h"
using namespace godot;

void medp_gd::MedpCore::_bind_methods() {
    ClassDB::bind_method(D_METHOD("version"), &MedpCore::version);
}

String medp_gd::MedpCore::version() const {
    return String("medp core 0.1 (entt v3.16.0 + cereal v1.3.2)");
}
```

- 基類選擇:`RefCounted`(GDScript `new()` 自動管理生命週期,適合資料物件) / `Node`(可掛場景樹、有 `_process` 回調) / `Object`(最基礎,要手動 free)。
- 之後接 `ZoneManager` 時,facade 持有它並轉呼叫:`String new_game()`、`void tick()` 等。

### 3-2. 進入點 register_types

```cpp
// projects/medp/src/gbind/register_types.cpp
#include "register_types.h"
#include "medp_core.h"
#include <gdextension_interface.h>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/core/class_db.hpp>
using namespace godot;

void initialize_medp_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) return;
    GDREGISTER_CLASS(medp_gd::MedpCore);     // 每個要暴露的 class 都要登記
}
void uninitialize_medp_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) return;
}

extern "C" {
// 符號名必須與 .gdextension 的 entry_symbol 一致
GDExtensionBool GDE_EXPORT medp_library_init(
        GDExtensionInterfaceGetProcAddress p_get_proc_address,
        GDExtensionClassLibraryPtr p_library,
        GDExtensionInitialization *r_initialization) {
    GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);
    init_obj.register_initializer(initialize_medp_module);
    init_obj.register_terminator(uninitialize_medp_module);
    init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);
    return init_obj.init();
}
}
```

### 3-3. .gdextension 描述檔

```ini
; projects/medp/src/gbind/medp.gdextension(範本;複製進 Godot 專案)
[configuration]
entry_symbol = "medp_library_init"     ; 對應上面的 extern "C" 函式名
compatibility_minimum = "4.6"

[libraries]
windows.debug.x86_64   = "res://bin/medp_gdext.dll"
windows.release.x86_64 = "res://bin/medp_gdext.dll"
; linux/macos 同理
```

---

## 4. 在 Godot 裡載入並測試

1. 建一個 **Godot 4.6** 專案。
2. 把 `projects/medp/build/bin/medp_gdext.dll` 複製到專案的 `res://bin/`。
3. 把 `projects/medp/src/gbind/medp.gdextension` 複製到 `res://medp.gdextension`。
4. 重開專案(讓 Godot 載入 GDExtension)。
5. GDScript 測試:
   ```gdscript
   var c = MedpCore.new()
   print(c.version())   # → medp core 0.1 (entt v3.16.0 + cereal v1.3.2)
   ```

---

## 5. 如何新增一個暴露給 GDScript 的東西

**新增一個方法**(在既有 facade class):
1. 在 class 加 method 宣告 + 實作(內部呼叫核心 API)。
2. 在 `_bind_methods()` 加 `ClassDB::bind_method(D_METHOD("名字", "參數名"...), &Class::method);`。

**新增一個 class**:
1. 建 `projects/medp/src/gbind/<name>.h/.cpp`,`GDCLASS(X, 基類)` + `_bind_methods`。
2. 在 `register_types.cpp` 的 `initialize_medp_module` 加 `GDREGISTER_CLASS(X);`。
3. 重新 `cmake --build build --target medp_gdext`(gbind 用 GLOB,新檔自動納入;若 configure 後才加檔,重跑一次 configure)。

---

## 6. 驗證 / 排錯

- **DLL 有沒有匯出進入點**:`dumpbin /exports projects/medp/build/bin/medp_gdext.dll` 找 `medp_library_init`(本專案已驗證有匯出)。找不到 → 檢查 `GDE_EXPORT` 與 `extern "C"`。
- **Godot 載入失敗**:多半是 `.gdextension` 的 `entry_symbol` 拼錯、`compatibility_minimum` 高於你的 Godot 版本、或 dll 路徑不對。
- **核心被 godot 污染**:若核心 build 開始要 godot-cpp,檢查是不是有核心檔案 include 了 godot,或 gbind 沒被 GLOB 排除。核心測試應永遠能在不開 option 下 build + 跑。
- **C++ 標準**:godot-cpp 是 C++17,我們的 C++20 可連結;facade 別用會與 godot-cpp header 衝突的 C++20 特性。

---

## 參考

- zone / 核心架構:`docs/references/zone_streaming_architecture.md`
- 新增 component / system:`docs/references/how_to_add_component_and_system.md`
- 程式碼:`projects/medp/src/gbind/`、`projects/medp/CMakeLists.txt`(MEDP_BUILD_GDEXTENSION 區塊)
- godot-cpp checkout:`C:\code\mine\pas\projects\godot-cpp`(target Godot 4.6)

---

回[目錄](godot_gdextension_setup.md)
