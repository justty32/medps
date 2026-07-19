# WAIT_USER — 等待使用者的事

只列需要使用者親自做/驗證才能繼續的 open 項。完成即移除，不留完成清單。

常見類型：

- 實機或 UI 手動驗證
- 外部帳號、權限、下載、授權
- 本機環境變數或工具安裝
- 不能由 agent 代跑的指令
- 高風險操作的確認
- **待過目**：agent 完成的非微小變更，排隊等使用者親讀（路徑:行號 + 一句看點）

## Open

- **待過目** 頂層文件整理（對齊 `~/repo/workflows` 乾淨 kernel）：刪除 4 個模板治理檔（`ADOPTION`/`INIT-QUESTIONS`/`MAINTENANCE`/`SYNC`，屬模板 repo 非本專案）、移除壞掉的 `commands/`（README 列的檔全不存在）、`others/`→`references/`（含 5 檔內部交叉引用）、新增 [INDEX.md](INDEX.md) repo 地圖並在 `AGENTS.md:14` 加入口。頂層 md 12→9。純文件、零原始碼變更，git 可全復原；未 commit。看點：INDEX.md 佈局是否符合你對頂層目錄的描述。
- **待過目** `src/gcore/util/tdarray.hpp:10-27` — 補上全檔僅缺的註解：檔頭三條使用慣例（true=失敗、is_coor 座標、get/getptr/getval 差異），加上各函式家族短註；純註解、零邏輯變更，16 項測試全綠。
- **待過目＋親自驗證** `workflows/common/code-map/html/index.html` — 新的程式碼 HTML 導覽層（8 站、嵌入帶行號上色的原始碼、行號錨點、深淺色主題）。請用瀏覽器開啟看瀏覽體驗是否合意；由 `build.py` 生成，程式碼更新後重跑 `python3 build.py` 即同步。

