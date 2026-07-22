# SESSION-LOG — 進度日誌 hub

只放還沒完成的活狀態。完成的不留在這裡；完成後濃縮到對應工作流的 landed/archive、release note、或 git log。

待使用者親自做/驗證的事放 [WAIT_USER.md](WAIT_USER.md)。

建議單一 `session-log.md` 保持短小，只為「下一個 session 接得上」。若超過 50 行，刪舊留新，或按工作流/主題拆檔。

## 最新進度

- 2026-07-10：導入 workflow 組織形式（minimal adoption）；AGENTS.md 改為路由器、CLAUDE.md 改為薄入口、模組速查移入 CODE_MAP。
- 2026-07-10：新增人用導讀 CODE_TOUR、resync 工作流（使用者親改接手）、DAVID 標記協定、WAIT_USER「待過目」佇列。
- 地基階段已完成（zone 系統精簡重構落地，16/16 測試綠）；遊戲玩法內容尚未動工，候選方向見 [docs/work/progress_overview.md](docs/work/progress_overview.md) 末段。
- 2026-07-20：資料夾整理——文件併入 `docs/`、C++ 專案改為 `projects/` 下平級三專案（medp / tests / archived）；CMake 重接為兩個獨立專案（tests 自行 find/link medp_static），已用本機 MinGW（g++ 16.1）兩步建置實測 **16/16 綠**。
- 2026-07-22（晚）：出兩份藍圖文檔，**均未動工**——(1) [workflows/plans/save-format-position-z.md](workflows/plans/save-format-position-z.md)：P0 動工計畫（存檔 magic+version、Position 補 int16_t z、movement 收口 move_by），經稽核員對照實碼修訂（要點：medp 唯一 TU 是 zone_manager.cpp，movement.h 不被建置覆蓋、須臨時 TU 驗證），**待使用者同意動工**；(2) [workflows/specs/zone-addressing-lifecycle-design.md](workflows/specs/zone-addressing-lifecycle-design.md)：定址+生命週期設計 spec（三取向面板+雙評審，贏家最小增量案+11 條補強），**待拍板**——爭點：分桶提前本輪做、save_all 含 Ephemeral、pinned 執行期旗標、tick 重入禁令。測試重建依使用者指示暫緩（頻繁變動期）。
- 2026-07-22：gcore 大重構（**庫已驗證編譯**：medp/medp_static MinGW 建置綠；tests 仍斷裂）——刪 ZoneKey/GlobalManager/ZoneStore/ZoneMeta/WorldConfig/AreaTerrain/Blocking；新核心 `zone/`（`Zone{id,parent,reg,layers}`、`Tile{uint32×2}`、`ZoneManager` root=0 + 一 zone 一檔存讀）+ `serialize/`（registry_io / zone_io）。**斷裂狀態**：`projects/tests/src/main.cpp` 整套仍對舊架構、不可編譯，「16 項全綠」基準失效；docs/work/ 四份設計文檔（progress_overview、gcore_overview、zone_layers；lifecycle 部分）與 CODE_MAP/CODE_TOUR 均未同步新架構。已拍板未實作：z 不進 zone id。ToME4 研讀建議報告見 [docs/work/design/tome4_recommendations.md](docs/work/design/tome4_recommendations.md)（含落地順序，step 0=重建測試基準）。

## 各工作流 session-log

| 工作流 | session-log | open 摘要 |
|--------|-------------|----------|
| feature-dev | [workflows/feature-dev/session-log.md](workflows/feature-dev/session-log.md) | 無 |
| refactor | [workflows/refactor/session-log.md](workflows/refactor/session-log.md) | 無 |
| investigation | [workflows/investigation/session-log.md](workflows/investigation/session-log.md) | 無 |

## 不屬任何工作流的進度

- 無。
