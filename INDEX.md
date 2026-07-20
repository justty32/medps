# INDEX — medps 專案地圖

整個專案的頂層導航。medps = **奇幻 4X 策略遊戲的 C++20 後端函式庫（`medp`）**，前端預計用 Godot 4 GDExtension，目前處於重寫階段。AGENTS.md 只放鐵律 + 指向本檔；細節從這裡分流。

---

## Repo 佈局

| 路徑 | 內容 |
|------|------|
| `projects/` | 各獨立、平級的子專案（見下）|
| `projects/medp/` | 後端函式庫（`medp`）：`src/` 原始碼、`include/` 第三方 header-only 庫（entt、cereal，**不修改**）、`data/` 執行期資料、`CMakeLists.txt`、`build/`（產出，不 commit）。程式碼導航見 [CODE_MAP](workflows/common/code-map/CODE_MAP.md)、線性導讀見 [CODE_TOUR](workflows/common/code-map/CODE_TOUR.md) |
| `projects/tests/` | 測試（獨立專案）；跑法見 [workflows/testing.md](workflows/testing.md) |
| `projects/archived/` | 重寫前的舊原型碼與草稿（非源碼，已凍結）|
| `workflows/` | 開發工作流（入口見 [WORKFLOWS.md](WORKFLOWS.md)）|
| `docs/` | 專案文件（見 [docs/README.md](docs/README.md)）：`work/` 設計架構、`references/` 庫教學 |

## 工作流

工作流的**選擇與入口**見 **[WORKFLOWS.md](WORKFLOWS.md)** 的派發表。每個工作流的 durable 知識歸在 `workflows/<該工作流>/` 或單檔 `workflows/<該工作流>.md`。

[DEV-GUIDE](DEV-GUIDE.md) 是**被動的結構整理參考**（結構整理原則 + 四級成長軌跡）——只在要重構/整理結構時取用。always-on 的**鐵律**在 [AGENTS.md](AGENTS.md)；碰原始碼的**程式碼慣例**在 [workflows/common/conventions.md](workflows/common/conventions.md)。

## 活狀態（只列還沒完成的）

| 檔案 | 用途 |
|------|------|
| [SESSION-LOG](SESSION-LOG.md) | 進度 hub → 各工作流 session-log（open-only）|
| [WAIT_USER](WAIT_USER.md) | 待**使用者**親自做/驗證的入口 |
