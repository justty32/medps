# 待過目 — 目錄結構與程式碼導覽層

母檔入口：[WAIT_USER.md](../WAIT_USER.md)。完成即從母檔與本檔一併移除。

（本檔從 [wait-user/docs.md](docs.md) 分出——該檔逼近 8 KB 上限。docs.md 管**文件內容**，本檔管**目錄結構與 code-map／html 導覽層**。）

---

## html 導覽層修復＋CODE_TOUR 重整（2026-08-12）

**這是壞掉的修復，不只是過期**：`build.py` 的 `STATIONS` 還指著 2026-07-23 重構時就搬走的 `gcore/components/`、`gcore/systems/`（兩個目錄現已不存在），而且完全沒涵蓋 `world/`、`common/actor.h`。

改動：

1. `build.py` 的 `STATIONS` 整份重寫，重排為 **9 站**（原 8 站）：`01-util`／`02-zone-core`／`03-common-actor`（新）／`04-world-components`／`05-world-gen`（新）／`06-serialize`／`07-zone-manager`／`08-tests`／`09-gbind`。順序改為貼合依賴方向，並對齊 CODE_MAP 的既有分組。
2. 依 `build.py` 檔頭自己訂的規則（先改 CODE_TOUR 再同步 STATIONS），[CODE_TOUR.md](../workflows/common/code-map/CODE_TOUR.md) 重寫（7978 bytes，地基層＋9 站閱讀路徑索引），並拆出 [CODE_TOUR_world_actor.md](../workflows/common/code-map/CODE_TOUR_world_actor.md)（5184 bytes，World 子類＋actor 層）——8 KB 上限所致。
3. `build.py` 新增 per-station `tour` 欄位，讓各站 html 的「對照」連結指向真正記載它的那份 .md。
4. 刪掉 6 個孤兒 html 頁後重新生成，9 站＋index 全數產出。

**驗證**：每站 `files` 路徑都對照磁碟確認存在；每條 bullet 的 `path:line` 錨點逐一驗證有解（`find_line` 找不到時是靜默失敗，不能只信 exit code）；index 的「核心約 X 行」改為從生成結果實算（1693 行），不再寫死。我另外獨立確認 9 個站台頁都存在、都非空、沒有殘留舊 slug。

**待你親自驗證**：用瀏覽器開 [index](../workflows/common/code-map/html/index.html)，看新的 9 站切分與嵌入原始碼是否合意。

---

## 資料夾整理

對齊 `~/repo/workflows` 標準 ＋ 你的 docs/src 分流：內容文件 `work/`→`docs/work/`、`references/`→`docs/references/`；C++ 專案改為 `projects/` 下平級專案 `projects/medp/`（原 `src`+`include`+`data`+`CMakeLists`）、`projects/tests/`（原 `test/`）、`projects/archived/`（原 `notes/` 重寫前原型）。

全程 `git mv` 保留歷史；同步更新 AGENTS／INDEX／CODE_MAP／CODE_TOUR／references／dev-env／testing／.gitignore 的路徑與建置指令，並重生 html 導覽層。除 CMake 接線外零邏輯變更。

**看點**：`projects/` 的專案切分與 `docs/` 佈局是否合你意？（現已增為四個：medp／tests／game／archived；2026-08-12 補進 [INDEX.md](../INDEX.md) 的 repo 佈局表——原本漏列 `projects/game/`）

---

## 頂層文件整理

對齊 `~/repo/workflows` 乾淨 kernel：刪除 4 個模板治理檔（`ADOPTION`／`INIT-QUESTIONS`／`MAINTENANCE`／`SYNC`，屬模板 repo 非本專案）、移除壞掉的 `commands/`（README 列的檔全不存在）、`others/`→`references/`（含 5 檔內部交叉引用）、新增 [INDEX.md](../INDEX.md) repo 地圖並在 `AGENTS.md:14` 加入口。頂層 md 12→9。

純文件、零原始碼變更，git 可全復原。

**看點**：INDEX.md 佈局是否符合你對頂層目錄的描述？
