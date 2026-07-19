# INDEX — medps 專案地圖

整個專案的頂層導航。medps = **奇幻 4X 策略遊戲的 C++20 後端函式庫（`medp`）**，前端預計用 Godot 4 GDExtension，目前處於重寫階段。AGENTS.md 只放鐵律 + 指向本檔；細節從這裡分流。

---

## Repo 佈局

| 路徑 | 內容 |
|------|------|
| `src/` | 後端函式庫原始碼（`medp`）；程式碼導航見 [CODE_MAP](workflows/common/code-map/CODE_MAP.md)、線性導讀見 [CODE_TOUR](workflows/common/code-map/CODE_TOUR.md) |
| `test/` | 測試；跑法見 [workflows/testing.md](workflows/testing.md) |
| `data/` | 執行期資料（build 時複製到 `build/bin/data/`）|
| `include/` | 第三方 header-only 庫（entt、cereal）——**不修改** |
| `workflows/` | 開發工作流（入口見 [WORKFLOWS.md](WORKFLOWS.md)）|
| `work/` | 既有設計/架構分析文檔（design、architecture、progress_overview）|
| `notes/` | 設計草稿（`.js`/`.gd`/`.cpp` prototype，非源碼；前端草稿在 `notes/gd/`）|
| `references/` | 外部庫教學與 how-to（entt、cereal、godot、component/system、zone streaming）|
| `build/` | 產出目錄（不 commit）|

## 工作流

工作流的**選擇與入口**見 **[WORKFLOWS.md](WORKFLOWS.md)** 的派發表。每個工作流的 durable 知識歸在 `workflows/<該工作流>/` 或單檔 `workflows/<該工作流>.md`。

[DEV-GUIDE](DEV-GUIDE.md) 是**被動的結構整理參考**（結構整理原則 + 四級成長軌跡）——只在要重構/整理結構時取用。always-on 的**鐵律**在 [AGENTS.md](AGENTS.md)；碰原始碼的**程式碼慣例**在 [workflows/common/conventions.md](workflows/common/conventions.md)；跳流程規則在 [PRINCIPLES.md](PRINCIPLES.md)。

## 活狀態（只列還沒完成的）

| 檔案 | 用途 |
|------|------|
| [SESSION-LOG](SESSION-LOG.md) | 進度 hub → 各工作流 session-log（open-only）|
| [WAIT_USER](WAIT_USER.md) | 待**使用者**親自做/驗證的入口 |
