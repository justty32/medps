# 待過目／待驗證 — 策略遊戲原型（`projects/game/`）

母檔入口：[WAIT_USER.md](../WAIT_USER.md)。完成即從母檔與本檔一併移除。

---

## 待親自驗證：可playable 原型能不能真的玩

建置已成功，但**沒有人實際跑過完整一局**。

Windows：

```bash
cmake -S projects/game -B projects/game/build -G "MinGW Makefiles" && cmake --build projects/game/build
```

執行檔 `projects/game/build/bin/medp_game.windows.debug.64.exe`。

Linux：

```bash
cmake -S projects/game -B projects/game/build && cmake --build projects/game/build
```

執行檔 `projects/game/build/bin/medp_game.linux.debug.64`。**終端需開到至少 100×28**——Linux 沒有 Windows 那段自動放大主控台的程式碼。

**功能**：80×40 ASCII 世界地圖（FBM 生成），玩家 2 城 3 部隊 vs AI 2 城 3 部隊，回合制，佔領所有敵城為勝。
**控制**：Enter 選取，方向鍵／hjkl 移動，S 略過，T 結束回合，Q 離開。

需親自確認四件事：

1. 世界地圖能正確顯示地形色彩；
2. 部隊可選取／移動／戰鬥；
3. AI 回合後能繼續玩家回合；
4. 勝利／失敗畫面出現。

---

## 待過目：Linux 可跑性修復

你在 Windows 寫的原型，在 Manjaro 上編譯全綠但**不可玩**——[display.h](../projects/game/src/display.h) 的 `wait_key()` 非 Windows 分支直接回 `Key::UNKNOWN`，而 [game.cpp:764](../projects/game/src/game.cpp:764) 是 `while (!game_over) { render(); handle_input(); }`，等於不吃輸入的忙轉迴圈，實測 3 秒噴 505 MB 終端輸出。

改動：

1. [display.h:31-77](../projects/game/src/display.h:31) 新增 `disp::detail` POSIX raw mode 層（`enter_raw`／`leave_raw`／`apply_raw`／`read_byte`，用 function-local static 存原始 termios 以保 header-only）。
2. [display.h:169-201](../projects/game/src/display.h:169) `wait_key()` 的 `#else` 分支實作：`read()` 逐 byte，`0x1b` 開頭再以 ~0.1 秒 `VTIME` 逾時區分「單獨按 ESC」與方向鍵 CSI 序列（`\033[A~D` 與應用模式 `\033OA~D`）。Windows 分支一行未動。
3. [main.cpp](../projects/game/src/main.cpp) 刪掉 `wait_any_key()`（Linux 走 `getchar()` 其實要按 Enter），統一改 `disp::wait_key()`，並調整為**先等鍵再 shutdown**，讓 raw mode 下才是真的「按任意鍵」。

**驗證**：medp 建置綠、測試 21/21 綠、game 建置綠；pty 實測輸入 `↓↓→→⏎→↓tq` → 游標 (4,35)→(4,36)→(5,36)→(6,36)→(7,36)→(7,37) 逐格對應、Enter 正確回饋、`t` 使回合 1→2（AI 回合跑過）、`q` 乾淨退出 exit=0；忙轉消失（505 MB → 單屏 29 KB）；`stty` 前後 `icanon`／`echo` 一致，確認終端狀態無洩漏。

**看點**：

1. 刻意**只**關 `ICANON`／`ECHO`——保留 `OPOST` 讓 `printf` 的 `\n` 仍補 CR（不然排版階梯化）、保留 `ISIG` 讓 Ctrl-C 留逃生門。這條取捨是否合你意？
2. `read()` 回 EOF 時映射成 `Key::ESCAPE`（而非 `UNKNOWN`），以免非互動 stdin 又變忙轉。
3. 另補了 `std::atexit(leave_raw)`，例外逸出／abort 也會還原終端。
