# SESSION-LOG — 進度日誌 hub

只放還沒完成的活狀態。完成的不留在這裡；完成後濃縮到對應工作流的 landed/archive、release note、或 git log。

待使用者親自做/驗證的事放 [WAIT_USER.md](WAIT_USER.md)（索引；詳情在 `wait-user/`）。

本檔保持短小，只為「下一個 session 接得上」。已落地事項的**決策理由**不在這裡重複，去看對應的 spec／plan 與 git log。

## 現況一句話

地基層可用且穩定：建置綠、測試 **21/21**（2026-08-12 Windows MinGW 實測）。`projects/` 下四個平級子專案（medp／tests／game／archived）。遊戲玩法內容仍未動工。

## 近期落地（一行一則，由新到舊）

- 2026-08-12：**文件大整備**。(1) 測試基準對齊——15/16/19 混雜 → 統一 **21**（AGENTS、CODE_MAP、testing、dev-env）。(2) 新增**文件單檔 8 KB 上限**鐵律，並溯及既往拆檔：`WAIT_USER`（12 KB→索引＋`wait-user/{gcore,game,docs}.md`）、`SESSION-LOG`、`progress_overview`、`gcore_overview`、`tome4_recommendations`、`CODE_TOUR`、`workflows/specs/` 兩份 spec、`docs/references/` 三份教學。**拆檔慣例：原檔名保留為母檔（索引），內容移到衍生檔名子檔——既有連結一律不斷**；六組拆檔都逐行驗過零內容遺失。(3) `gcore_overview` 原本整份仍描述重構前的扁平目錄，已按 `zone/`／`world/`／`common/` 三分重寫。(4) **html 導覽層原本是壞的**（`build.py` 站台指向已刪除的 `gcore/components/`、`gcore/systems/`），已重接並重排為 9 站、重新生成。
- 2026-07-25：`projects/game/` 補齊 POSIX raw mode 輸入層——Linux 從「編譯得過但不可玩」（忙轉迴圈，3 秒噴 505 MB）變成真能玩。
- 2026-07-24：**策略遊戲可玩原型落地**，新建 `projects/game/`（787 行）。接 medp_static＋libtcod_static，80×40 ASCII 世界地圖、玩家 vs 貪婪 AI、回合制、戰鬥／佔城／勝敗。意義：地基層第一個真實 consumer。
- 2026-07-23：actor 基礎設施——地點／部隊兩大家族＋種類為 root 底下的資料驅動 def（非 enum）。測試 19→21。
- 2026-07-23：World 專屬物集中進 `gcore/world/`（`zone/` 只留通用框架）＋worldgen 抽成不認識 World 型別的獨立模組。
- 2026-07-22：World 第一個 Zone 子類落地（kind tag＋`make_zone` 工廠＋`zone_cast`＋libtcod headless worldgen）。測試 15→19。
- 2026-07-22：zone 定址＋生命週期拍板落地（`create_child` 單點配號＋manifest＋開檔協定＋destroy 刪檔）——見 [spec 拍板結果表](workflows/specs/zone-addressing-lifecycle-design.md)。
- 2026-07-22：P0 落地（Position 補 z、`move_by` 移動收口）——見 [計畫＋執行結果](workflows/plans/save-format-position-z.md)。存檔版本欄位經使用者裁定**放棄**。
- 2026-07-22：換裝 Zone/ZoneManager 新核心，移除 ZoneKey/GlobalManager 舊架構；測試套件整套重寫，基準恢復。

## Open — 還沒完成的

**下一步主線**

- **Ruleset（規則庫）層尚未實作**，是「下一步候選」第一項。需求來源現在很具體：`projects/game/src/game.h:19-20` 把 `KIND_CITY`／`KIND_WARRIOR` 寫死成常數、`game.h:12-13` 的 `Health`／`Moves` 刻意不進 `AllComponents` 不序列化——這兩處正是 Ruleset 該接手的位置。設計方向見 [lifecycle](docs/work/design/lifecycle.md)。
- 遊戲玩法內容（種族、宗教、戰鬥、外交）仍未動工。第一個真正的玩法系統會觸發下列 defer 項落地。

**defer 且不凍結設計**（動工前重審，見 [spec 拍板結果](workflows/specs/zone-addressing-lifecycle-design.md)）

- 目錄分桶、persistence 兩態（Ephemeral/Persistent）、LRU 卸載：痛了再說。
- parent→children 連結、跨 zone 實體引用、返回座標、Portal。
- 使用者點名的未來需求：「清理很久沒訪問且不重要的 zone .bin」機制。

**文件債**

- ~~`docs/references/zone_streaming_architecture.md` 整份對應舊架構~~ **已重寫（2026-08-12）**：定位收斂為「為什麼是這個架構」的概念教學（與 gcore_overview 逐檔地圖／CODE_TOUR 線性導讀／how_to 操作步驟劃清邊界），拆為母檔＋定址／生命週期兩子檔。
- ~~`docs/references/` 教學仍寫舊 `gcore/components|systems` 路徑與 `GlobalManager`~~ **已清（2026-08-12）**：`how_to_add_component_and_system.md` 實質重寫、entt 教學三處、godot 教學兩處全部對齊新核心。除下一條外，live 教學已無殘留舊 API。
- [tome4_recommendations_status_summary.md:19](docs/work/design/tome4_recommendations_status_summary.md:19) 的檢查清單仍寫 15/16 項測試（歷史紀錄性質，刻意未追改；母檔索引區有「落地現況」小節說明現狀）。

**待使用者**

- [WAIT_USER.md](WAIT_USER.md)：3 項需你親自動手（跑遊戲原型、開瀏覽器看 html 導覽層與 docs/index.html）＋19 項待過目，最舊可回溯到 2026-07-19。積壓偏長。

## 各工作流 session-log

| 工作流 | session-log | open 摘要 |
|--------|-------------|----------|
| feature-dev | [workflows/feature-dev/session-log.md](workflows/feature-dev/session-log.md) | 無 |
| refactor | [workflows/refactor/session-log.md](workflows/refactor/session-log.md) | 無 |
| investigation | [workflows/investigation/session-log.md](workflows/investigation/session-log.md) | 無 |

## 不屬任何工作流的進度

- 無。
