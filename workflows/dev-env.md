# dev-env — 開發環境入口

記錄不同機器/環境能做什麼、不能做什麼，以及 fresh clone 後要做的事。

開始前寫 `Done when: <環境能力/缺口/下一步已記錄，必要時 WAIT_USER 已更新>`。

## 環境矩陣

| 環境 | 有什麼 | 能做 | 不能做 |
|------|--------|------|--------|
| linux 本機（Manjaro） | CMake、C++20 工具鏈 | build / 全部 16 項測試 | Godot 內實測（需 Godot 編輯器） |
| windows | 同上（輸出 `medp.windows.debug.64.dll`） | build / 測試 | — |
| CI | 未設定 | — | — |

## Fresh Clone

```bash
# 第三方庫（entt、cereal）已隨 repo 附在 include/，無外部依賴要裝
cmake -S . -B build && cmake --build build
./build/bin/medp_test.linux.debug.64   # 名稱依平台/組態而異
```

## 產出物

- `build/bin/medp.<平台>.<組態>.<位元數>`（shared）＋ `libmedp_static.a`（static）
- `build/bin/medp_test.*` — 測試執行檔
- `data/` 會在 build 時複製到 `build/bin/data`

## Godot GDExtension（可選）

- 預設關閉；開啟：`-DMEDP_BUILD_GDEXTENSION=ON`。
- 需要本機 godot-cpp checkout，路徑由 CMake cache 變數 `GODOT_CPP_DIR` 指定（預設值是舊機器路徑，換機器要重設）。
- 首次編譯 godot-cpp 很慢（生成 bindings + 編譯 binding 庫）。
- 前端草稿在 `notes/gd/`。

## 何時不用

- 只是一次性跑 build/test，走 [testing.md](testing.md) 或原工作流。
- 是外部工具細節，走 tooling。
