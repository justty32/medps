# dev-env — 開發環境入口

記錄不同機器/環境能做什麼、不能做什麼，以及 fresh clone 後要做的事。

開始前寫 `Done when: <環境能力/缺口/下一步已記錄，必要時 WAIT_USER 已更新>`。

## 環境矩陣

| 環境 | 有什麼 | 能做 | 不能做 |
|------|--------|------|--------|
| linux 本機（Manjaro） | CMake、C++20 工具鏈 | build / 全部 19 項測試 | Godot 內實測（需 Godot 編輯器） |
| windows 本機 | MinGW-w64 g++ 16.1（`C:/dev/mingw64/bin`）、CMake 4.3、vcpkg | build / 測試（generator `MinGW Makefiles`，輸出 `*.windows.debug.64.*`） | — |
| CI | 未設定 | — | — |

> **跨平台是目標**：同一位開發者在 windows（MinGW）與 Manjaro（gcc）兩台間切換。CMake 全用相對路徑、輸出檔名帶平台/位元後綴，各平台建置互不衝突；`projects/*/build/` 已 gitignore，換機器 pull 後重新 configure+build 即可，不共用建置產物。

## Fresh Clone

```bash
# header-only 第三方庫（entt、cereal）已隨 repo 附在 projects/medp/include/
# 編譯型依賴 libtcod（headless 演算法半邊）由 medp 的 CMake FetchContent 鎖 2.2.2 自動抓建，
# 首次 configure 需網路（之後離線可重建）；靜態庫落在 medp build/bin，tests 一併連

# 兩個獨立專案：先建核心庫 medp，再建測試（link medp_static）
# windows 本機加：-G "MinGW Makefiles" -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++（PATH 含 C:/dev/mingw64/bin）
cmake -S projects/medp -B projects/medp/build && cmake --build projects/medp/build
cmake -S projects/tests -B projects/tests/build && cmake --build projects/tests/build
./projects/tests/build/bin/medp_test.linux.debug.64   # 名稱依平台/組態而異
```

## 產出物

- `projects/medp/build/bin/medp.<平台>.<組態>.<位元數>`（shared）＋ `libmedp_static.a`（static）
- `projects/tests/build/bin/medp_test.*` — 測試執行檔（獨立專案，link medp_static）
- `projects/medp/data/` 會在 build 時複製到 `projects/medp/build/bin/data`

## Godot GDExtension（可選）

- 預設關閉；開啟：`-DMEDP_BUILD_GDEXTENSION=ON`。
- 需要本機 godot-cpp checkout，路徑由 CMake cache 變數 `GODOT_CPP_DIR` 指定（預設值是舊機器路徑，換機器要重設）。
- 首次編譯 godot-cpp 很慢（生成 bindings + 編譯 binding 庫）。
- 前端草稿在 `projects/archived/gd/`（重寫前原型）。

## 何時不用

- 只是一次性跑 build/test，走 [testing.md](testing.md) 或原工作流。
- 是外部工具細節，走 tooling。
