# WAIT_USER — 等待使用者的事

只列需要使用者親自做/驗證才能繼續的 open 項。完成即移除，不留完成清單。

常見類型：

- 實機或 UI 手動驗證
- 外部帳號、權限、下載、授權
- 本機環境變數或工具安裝
- 不能由 agent 代跑的指令
- 高風險操作的確認
- **待過目**：agent 完成的非微小變更，排隊等使用者親讀（路徑:行號 + 一句看點）

本檔是**索引**：每項一行摘要，詳情（改了什麼、怎麼驗證的、看點）在 `wait-user/` 底下的主題檔。
新增項目時同步寫兩邊；結案時兩邊一起刪。
（拆檔原因：本檔曾長到 12 KB，超過專案的文件 8 KB 上限。）

---

## Open — 需要你親自動手

這幾項 agent 做不了，卡在你身上：

| 事項 | 詳情 |
|------|------|
| **跑一局策略遊戲原型** — 建置已綠，但沒人實際玩過完整一局；要確認地形顯示／選取移動戰鬥／AI 回合／勝敗畫面四件事 | [wait-user/game.md](wait-user/game.md) |
| **用瀏覽器開 html 導覽層** — 2026-08-12 剛修好（原本站台路徑指向已刪除的目錄，等於是壞的）並重排為 9 站，看新切分與嵌入原始碼是否合意 | [wait-user/structure.md](wait-user/structure.md) |
| **用瀏覽器開 `docs/index.html`** — 看文件彙整頁的分區與摘要是否符合你的心智 | [wait-user/docs.md](wait-user/docs.md) |

## Open — 待過目（agent 已完成，等你親讀）

### gcore 核心 — [wait-user/gcore.md](wait-user/gcore.md)

| 事項 | 一句看點 |
|------|----------|
| actor 基礎設施（ECS 組合版） | 是否認同「種類 def 一律住 root、actor 以穩定 id 參照」這條線？ |
| World 專屬物集中進 `gcore/world/` | 分層切分（zone＝通用框架／world＝World 子類專屬）是否合你意？ |
| worldgen 抽成獨立模組＋純簽章 | `world_gen` 不再認識 World 型別，`World::generate()` 只是挑 `layers[0]` 的薄殼 |
| World 子類落地 | `generate` 的 biome 佔位分類是否符合你要的第一版粒度？ |
| 測試套件整套重寫 | `test_tick_all_zones` 明文固定「root 也參加 tick」的新語意（舊架構 root 不參加）|
| zone 定址＋生命週期落地 | 開檔協定：無 manifest 但有 .bin 直接 throw，不靜默覆寫舊存檔 |
| P0 落地：Position 補 z、move_by 收口 | 位置變更從此必經 `move_by` 單一入口 |
| tdarray 補註解 | 純註解、零邏輯變更（true＝失敗這條慣例值得確認）|

### 遊戲原型 — [wait-user/game.md](wait-user/game.md)

| 事項 | 一句看點 |
|------|----------|
| Linux 可跑性修復（POSIX raw mode 輸入層）| 刻意只關 `ICANON`／`ECHO`，保留 `OPOST`（排版）與 `ISIG`（Ctrl-C 逃生門）——這條取捨是否合你意？ |

### 目錄結構與導覽層 — [wait-user/structure.md](wait-user/structure.md)

| 事項 | 一句看點 |
|------|----------|
| **html 導覽層修復＋CODE_TOUR 重整**（2026-08-12）| 站台路徑原本指向已刪除的目錄，導覽層是壞的不只是過期；重排為 9 站並拆出 `CODE_TOUR_world_actor.md` |
| 資料夾整理（docs/ ＋ projects/ 分流）| `projects/` 的專案切分與 `docs/` 佈局是否合你意？ |
| 頂層文件整理（對齊乾淨 kernel）| INDEX.md 佈局是否符合你對頂層目錄的描述？ |

### docs/work 設計文檔與彙整頁 — [wait-user/docs.md](wait-user/docs.md)

| 事項 | 一句看點 |
|------|----------|
| **gcore_overview 重寫＋拆檔**（2026-08-12）| 原本整份還在描述重構前的扁平目錄；與 `world_actor_game.md` 的「地圖 vs 沿革」分工是否清楚？ |
| **8 KB 上限溯及拆檔**（2026-08-12）| 6 份既有超標文件按主題拆開；母檔保留原檔名當索引，所以既有連結沒斷。六組都驗過零內容遺失 |
| docs/work 四份文檔同步新核心 | zone_layers 的「實作現況」節是否符合你對三層願景的想法？ |
| ToME4 研讀→重寫建議報告 | §2 的 P0 三項與 §4 落地順序是否合你意？ |
| docs/index.html 文件彙整頁 | 分區與摘要是否符合你對 docs 的心智？ |

### references 庫教學 — [wait-user/references.md](wait-user/references.md)

| 事項 | 一句看點 |
|------|----------|
| **references 教學對齊新核心**（2026-08-12）| `how_to_add_component_and_system.md` 原本整份在教已刪除的 `GlobalManager`／`ZoneKey`／`zone_meta.h`——等於教人用不存在的 API |
| **zone_streaming_architecture 整份重寫**（2026-08-12）| 定位收斂為「為什麼是這個架構」的概念教學，與 gcore_overview／CODE_TOUR／how_to 三份劃清邊界——這個切分決定以後架構知識往哪寫 |
