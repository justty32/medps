# testing — 測試入口

把本專案所有常用驗證指令放這裡。agent 能跑的測試應自己跑；不能跑的記到 [../WAIT_USER.md](../WAIT_USER.md)。

開始前寫 `Done when: <指定測試/驗證命令跑完，結果已回報>`。

## 常用指令

```bash
# build（含測試執行檔）
cmake -S . -B build && cmake --build build

# unit/integration tests（自製測試框架，單一執行檔；名稱依平台而異）
./build/bin/medp_test.linux.debug.64
# 預期輸出結尾：16 / 16 passed（新增測試後更新此基準）

# lint / format：目前無設定

# Godot GDExtension target（可選，首次編譯 godot-cpp 很慢）
cmake -S . -B build -DMEDP_BUILD_GDEXTENSION=ON && cmake --build build
```

## 測試分類

- `fast`: build + `medp_test`——每次小改都跑，秒級完成。
- `full`: 同上（目前沒有更重的測試層）。
- `external`: GDExtension 編譯與 Godot 內實測——需要 godot-cpp checkout 與 Godot 編輯器，見 [dev-env.md](dev-env.md)。

## 已知環境性失敗

- 無。16 項測試在乾淨 build 下應全綠；任何紅燈都視為 regression。

## 何時不用

- 只是查測試指令，直接讀本檔回答。
- 測試是 feature/refactor 的一部分，不需要另開測試工作流；在原工作流內執行即可。
