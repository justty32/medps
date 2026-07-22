# medps — Agent 專案備忘

最頂層路由器：只放 always-on 規則與入口連結；細節放到各工作流與 CODE_MAP。

## 專案摘要

- 專案一句話：**medps** 是奇幻 4X 策略遊戲的 C++20 後端函式庫（`medp`），前端預計用 Godot 4 GDExtension；目前處於**重寫**階段。
- 主要語言/框架：C++20、EnTT（ECS）、cereal（序列化）、CMake
- 主要 build 指令（兩個獨立專案，先庫後測試）：`cmake -S projects/medp -B projects/medp/build && cmake --build projects/medp/build` 然後 `cmake -S projects/tests -B projects/tests/build && cmake --build projects/tests/build`
- 主要 test 指令：`./projects/tests/build/bin/medp_test.<平台>.<組態>.<位元數>`（如 `medp_test.linux.debug.64`），詳見 [workflows/testing.md](workflows/testing.md)

## 先讀哪裡

- 想看專案長怎樣、頂層有哪些目錄 → [INDEX.md](INDEX.md)：repo 結構地圖。
- 使用者要你動手做某件事 → [WORKFLOWS.md](WORKFLOWS.md)：依意圖派發到對應工作流。
- 碰原始碼 → 先讀 [workflows/common/conventions.md](workflows/common/conventions.md)，再讀 [CODE_MAP](workflows/common/code-map/CODE_MAP.md)。
- 使用者親改了程式碼或留了 `// DAVID:` 註解 → [workflows/resync.md](workflows/resync.md)。
- 人要重讀程式碼 → [CODE_TOUR](workflows/common/code-map/CODE_TOUR.md)（線性導讀）；想快速回顧進度 → [docs/work/progress_overview.md](docs/work/progress_overview.md)。
- 既有設計/分析文檔 → `docs/`（[docs/README.md](docs/README.md)：work 設計架構、references 庫教學）；重寫前原型在 `projects/archived/`。

## Always-on 鐵律

- 所有回覆與留檔使用**繁體中文**。
- 程式碼引用必須附**路徑:行號**。
- 重構/整理必須 behavior-preserving；改完跑測試（15 項全綠為基準）。
- **新增 component 時必須同步登記 `projects/medp/src/gcore/serialize/all_components.h` 的 `AllComponents`**，否則存檔會漏掉它。
- 未經使用者確認，不 push、不開新大型工作。
- 不 revert 使用者或其他 agent 的未確認變更；遇到衝突先停下說明。
- 程式碼中的 `// DAVID:` 註解是使用者指令，看到就優先處理；`// DAVID_WRITE:` 區塊是使用者親寫碼，不改寫。協定見 [workflows/common/conventions.md](workflows/common/conventions.md)。
- 完成非微小變更後，在 [WAIT_USER.md](WAIT_USER.md) 排「待過目」項（路徑:行號 + 一句看點）。
- 非微小工作先定義 `Done when:`。
- 需要使用者親自驗證、外部環境、權限、實機操作時，記到 [WAIT_USER.md](WAIT_USER.md)。
- 跨 session 的 open 狀態記到 [SESSION-LOG.md](SESSION-LOG.md) 或對應工作流的 `session-log.md`。
- 架構圖/流程圖優先用 Mermaid、表格、列點；不要用需要字元對齊的 ASCII 框線圖。

## 分層思想

```text
AGENTS.md → WORKFLOWS.md → 各工作流入口 → 工作流內容 → 子工作流
```

- durable 知識歸到它所屬的工作流，不堆在頂層。
- 程式碼導航細節（目錄結構、模組速查、設計模式）在 [CODE_MAP](workflows/common/code-map/CODE_MAP.md)，不在本檔重複。

## 本地專案規則

- `CLAUDE.md` 是 Claude Code 的薄入口，內容以本檔為單一來源；兩檔不各自維護內容。
- `projects/medp/include/` 是第三方 header-only 庫（entt、cereal），不要修改。
- `projects/medp/build/` 是產出目錄，不 commit。
- 歷史分析/設計文檔在 `docs/work/`；新的調查、設計、計畫走對應工作流（investigation / specs / plans），既有 `docs/work/` 文檔先保留、用連結引用，不強制搬遷。
- Godot GDExtension target 預設關閉（`-DMEDP_BUILD_GDEXTENSION=ON` 開啟），環境細節見 [workflows/dev-env.md](workflows/dev-env.md)。
