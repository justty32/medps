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

- **待拍板** [workflows/specs/zone-addressing-lifecycle-design.md](workflows/specs/zone-addressing-lifecycle-design.md) — zone 定址＋生命週期設計 spec（報告 §2-D/§2-F 兩道 P0 設計題）。看點：§2 決策總表 12 條是否合你意；特別是四個爭點——目錄分桶提前到本輪、save_all 含 Ephemeral（checkpoint 語意）、`pinned` 執行期旗標、tick 重入禁令；§8 懸置清單是否可接受。拍板後才展開 plan。
- **待過目** P0 落地（[計畫＋執行結果](workflows/plans/save-format-position-z.md)；Task 1 存檔版本欄位經你裁定放棄）——(1) `projects/medp/src/gcore/components/position.h:7-14`：Position 補 `int z`（x/y/z 全 int，照你的裁定）；(2) `projects/medp/src/gcore/zone/zone.h:36`：`Zone::layers` 鍵 `int16_t`→`int`；(3) `projects/medp/src/gcore/systems/movement.h:12,20`：新增 `move_by` 收口、movement 改吃 `Zone&`（簽章即 ZoneSystem，可直接註冊）。驗證：medp/medp_static 建置綠＋movement.h 臨時 TU 語法編譯過；測試套件仍斷裂未跑。
- **待過目** [docs/work/design/tome4_recommendations.md](docs/work/design/tome4_recommendations.md) — ToME4 架構研讀→重寫建議報告（研讀 `C:\code\mine\modding_tome4` 語料，經三路對抗性覆核修訂）。看點：§2 的 P0 三項（Position 補 z、存檔版本欄位、生命週期策略）與 §4 落地順序是否合你意；step 0 指出**測試套件已隨重構斷裂、16 項基準失效**，這是下一步動工前的硬前置。
- **待過目** 資料夾整理（對齊 `~/repo/workflows` 標準 + 你的 docs/src 分流）：內容文件 `work/`→`docs/work/`、`references/`→`docs/references/`；C++ 專案改為 `projects/` 下平級三專案 `projects/medp/`（原 `src`+`include`+`data`+`CMakeLists`）、`projects/tests/`（原 `test/`）、`projects/archived/`（原 `notes/` 重寫前原型）。全程 `git mv` 保留歷史；同步更新 AGENTS/INDEX/CODE_MAP/CODE_TOUR/references/dev-env/testing/.gitignore 的路徑與建置指令，並重生 html 導覽層。除 CMake 接線（見上）外零邏輯變更；未 commit。看點：`projects/` 三專案切分與 `docs/` 佈局是否合你意。
- **待過目** 頂層文件整理（對齊 `~/repo/workflows` 乾淨 kernel）：刪除 4 個模板治理檔（`ADOPTION`/`INIT-QUESTIONS`/`MAINTENANCE`/`SYNC`，屬模板 repo 非本專案）、移除壞掉的 `commands/`（README 列的檔全不存在）、`others/`→`references/`（含 5 檔內部交叉引用）、新增 [INDEX.md](INDEX.md) repo 地圖並在 `AGENTS.md:14` 加入口。頂層 md 12→9。純文件、零原始碼變更，git 可全復原；未 commit。看點：INDEX.md 佈局是否符合你對頂層目錄的描述。
- **待過目** `projects/medp/src/gcore/util/tdarray.hpp:10-27` — 補上全檔僅缺的註解：檔頭三條使用慣例（true=失敗、is_coor 座標、get/getptr/getval 差異），加上各函式家族短註；純註解、零邏輯變更，16 項測試全綠。
- **待過目＋親自驗證** `docs/index.html` — 新增 `docs/` 文件彙整頁：依「進度總覽 / 設計架構(work) / 外部教學(references)」三區列出全部 9 份 md，各附標題、一句摘要、路徑與連結；單一自帶樣式 HTML，可直接用瀏覽器開。看點：分區與摘要是否符合你對 docs 的心智；連結指向 .md 原檔（瀏覽器多半顯示原始文字），若想要渲染後閱讀體驗再告知。
- **待過目＋親自驗證** `workflows/common/code-map/html/index.html` — 新的程式碼 HTML 導覽層（8 站、嵌入帶行號上色的原始碼、行號錨點、深淺色主題）。請用瀏覽器開啟看瀏覽體驗是否合意；由 `build.py` 生成，程式碼更新後重跑 `python3 build.py` 即同步。

