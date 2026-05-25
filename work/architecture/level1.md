# Level 1 — 初始探索與基礎架構

## 專案定位

**medps** 是一個以 **C++20** 撰寫的 **4X 策略遊戲後端函式庫**（library），專案名稱縮寫為 `medp`。
它不是獨立可執行的遊戲，而是一個提供核心資料模型、場景系統、ECS 元件框架與二進位序列化的靜態/動態函式庫，
預計搭配 **Godot 4 GDExtension** 前端使用（輸出命名格式與 Godot 一致，如 `medp.windows.debug.64.dll`）。

---

## 目錄結構

```
medps/
├── CMakeLists.txt          # 主構建檔，輸出 medp (SHARED) 與 medp_static (STATIC)
├── src/
│   └── gcore/              # 遊戲核心（Game Core）
│       ├── gcore.h         # 總 include 入口
│       ├── util.h          # 工具類總 include
│       ├── obj_types.h     # 遊戲物件型別總 include
│       ├── objs.h          # 物件實例集合（目前空）
│       ├── obj_types_list.cpp  # 型別登錄表（_init_all_subtypes）
│       └── util/
│           ├── obj.hpp/cpp         # 基底物件 Obj
│           ├── scene.hpp/cpp       # 場景（Obj 容器）
│           ├── component.hpp/cpp   # Component / ComponentManager
│           ├── tdarray.hpp         # 2D 陣列模板
│           ├── bin_fwr.hpp         # 二進位 read/write
│           ├── mydef.h             # 巨集工具
│           ├── utildef.h           # 編譯器偵測
│           └── util.h              # 工具類總 include
│       └── obj_types/
│           └── world.hpp/cpp       # BigMap 相關型別（TileMap、MapEntity…）
├── include/                # 第三方 header-only 函式庫
│   ├── fplus.hpp           # FunctionalPlus（函數式程式設計）
│   ├── json.hpp            # nlohmann/json
│   ├── magic_enum.hpp      # 枚舉反射
│   ├── metal.hpp           # 泛型演算法
│   └── nameof.hpp          # 名稱反射
├── test/
│   ├── CMakeLists.txt      # 測試執行檔（連結 medp_static）
│   └── src/main.cpp        # 目前僅有 tuple 巨集測試
└── notes/                  # 設計草稿（非源碼）
    ├── a.txt               # 世界觀/神話設定（英文）
    ├── struc.txt           # 超高階結構草稿
    ├── strategy_game_system.js   # 詳盡的遊戲資料結構設計文件
    ├── deep_systems.js           # 深層模擬系統（游牧、組織、貴族家族）
    ├── advanced_systems.js       # 進階系統
    ├── rpg_magic_elements.js     # 魔法元素系統
    ├── sdl_map_generator.cpp     # SDL 地圖生成實驗
    ├── sdl_tilemap.cpp           # SDL 圖塊地圖實驗
    ├── sql1/               # SQLite/資料庫整合實驗
    └── gd/                 # Godot 4 GDScript 前端草稿
```

---

## 技術棧

| 項目 | 說明 |
|---|---|
| 語言 | C++20（使用 Concepts、consteval、constexpr if） |
| 構建 | CMake 3.14+，支援 MSVC / GCC / Clang |
| 輸出 | SHARED DLL + STATIC LIB |
| 平台 | Windows（主要）、可能跨平台（Android 路徑存在） |
| 前端（規劃中）| Godot 4 GDExtension |
| 第三方 | FunctionalPlus, nlohmann/json, magic_enum, nameof |

---

## 入口點

- **函式庫入口**: `src/gcore/gcore.h` — 組合 `util.h`, `obj_types.h`, `objs.h`
- **測試入口**: `test/src/main.cpp:12` — `int main()` 目前僅測試巨集
- **型別登錄**: `src/gcore/obj_types_list.cpp:5` — `Obj::_init_all_subtypes()`

---

## 構建指令

```bash
# 標準 CMake 流程
cmake -S . -B build
cmake --build build

# 輸出：build/bin/medp.windows.debug.64.dll
#        build/bin/medp_static.windows.debug.64.lib
#        build/bin/medp_test.windows.debug.64.exe
```

---

## 專案成熟度

**非常早期**（Early-stage skeleton）：
- 核心框架（Obj/Scene/Component/BinFSR/tdarray）已完成設計與部分實作
- 遊戲領域物件（world.h）僅有骨架，大量設計仍在 `notes/` 草稿中
- 使用者表示準備**重寫**此專案
