# testing — 測試入口

把本專案所有常用驗證指令放這裡。agent 能跑的測試應自己跑；不能跑的記到 [../WAIT_USER.md](../WAIT_USER.md)。

開始前寫 `Done when: <指定測試/驗證命令跑完，結果已回報>`。

## 常用指令

> `projects/medp`（核心庫）與 `projects/tests`（測試）是**兩個獨立 CMake 專案**：測試 link medp 的 `medp_static`，所以**必須先建 medp、再建 tests**。

```bash
# 1) build 核心庫 medp（產出 medp_static 到 projects/medp/build/bin）
cmake -S projects/medp -B projects/medp/build && cmake --build projects/medp/build

# 2) build 測試（自動 find/link 上一步的 medp_static；找不到會給明確錯誤）
cmake -S projects/tests -B projects/tests/build && cmake --build projects/tests/build

# unit/integration tests（自製測試框架，單一執行檔；名稱依平台而異）
./projects/tests/build/bin/medp_test.linux.debug.64
# 預期輸出結尾：16 / 16 passed（新增測試後更新此基準）

# lint / format：目前無設定

# Godot GDExtension target（可選，首次編譯 godot-cpp 很慢；屬 medp 專案）
cmake -S projects/medp -B projects/medp/build -DMEDP_BUILD_GDEXTENSION=ON && cmake --build projects/medp/build
```

> 測試 CMake 用快取變數 `MEDP_LIB_DIR` 找已建好的庫（預設 `projects/medp/build/bin`）；庫在別處時以 `-DMEDP_LIB_DIR=<路徑>` 指定。

## 測試分類

- `fast`: build + `medp_test`——每次小改都跑，秒級完成。
- `full`: 同上（目前沒有更重的測試層）。
- `external`: GDExtension 編譯與 Godot 內實測——需要 godot-cpp checkout 與 Godot 編輯器，見 [dev-env.md](dev-env.md)。

## 已知環境性失敗

- 16 項測試在乾淨 build 下應全綠；任何紅燈都視為 regression。（2026-07-20 以本機 MinGW g++ 16.1 兩步建置實測 16/16。）
- **MinGW Makefiles 偶發**：建 `medp_static` 時 `ar` 打包可能報 `Error running link command: unknown error`（`ar` 本身正常，重跑 `cmake --build projects/medp/build` 即過）。非 regression。

## 何時不用

- 只是查測試指令，直接讀本檔回答。
- 測試是 feature/refactor 的一部分，不需要另開測試工作流；在原工作流內執行即可。
