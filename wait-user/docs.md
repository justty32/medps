# 待過目 — 文件內容（docs/work 與彙整頁）

母檔入口：[WAIT_USER.md](../WAIT_USER.md)。完成即從母檔與本檔一併移除。

本檔管 `docs/work/` 的設計文檔與 `docs/index.html`。另兩份：目錄結構與 code-map／html 導覽層在 [structure.md](structure.md)、`docs/references/` 的庫教學在 [references.md](references.md)（都是本檔撞到 8 KB 上限時分出的）。

---

## docs/work 四份文檔同步新核心

- [progress_overview](../docs/work/progress_overview.md) — 全面重寫的進度快照。
- [gcore_overview](../docs/work/architecture/gcore_overview.md) — 新核心逐檔導覽。
- [zone_layers](../docs/work/design/zone_layers.md) — **定位變更**：降級為設計願景＋實作現況，三層尺度仍有效。
- [lifecycle](../docs/work/design/lifecycle.md) — 名詞對齊 ZoneManager。

**看點**：zone_layers 的「實作現況」節是否符合你對三層願景的想法？

> 2026-08-12 追記：這四份**又動過一輪**，需要重新過目。progress_overview 補上 World 子類／gcore 目錄重整／actor 基礎設施／`projects/game`（測試基準 15→21），並拆出 [world_actor_game](../docs/work/architecture/world_actor_game.md)（沿革）。gcore_overview 原本整份還停在重構前的扁平目錄，已按現行 `zone/`／`world/`／`common/` 分層重寫，並拆出 [zone_serialize](../docs/work/architecture/gcore_overview_zone_serialize.md) 與 [world_common](../docs/work/architecture/gcore_overview_world_common.md) 兩份子檔。兩份新母檔的分工：gcore_overview＝「現在長怎樣」的逐檔地圖，world_actor_game＝「這一期改了什麼」的沿革。

---

## gcore_overview 重寫＋拆檔（2026-08-12）

[gcore_overview.md](../docs/work/architecture/gcore_overview.md) 原本整份還在描述重構前的扁平目錄（`gcore/components/position.h` 那種路徑），已按現行 `zone/`／`world/`／`common/` 三分重寫。母檔留心智模型、Mermaid 依賴圖、速查表（3961 bytes），逐檔細節拆成 [zone_serialize](../docs/work/architecture/gcore_overview_zone_serialize.md)（6175 bytes）與 [world_common](../docs/work/architecture/gcore_overview_world_common.md)（5486 bytes）。

**看點**：與 [world_actor_game](../docs/work/architecture/world_actor_game.md) 的分工是否清楚——前者是「現在長怎樣」的逐檔地圖，後者是「這一期改了什麼」的沿革。

註：`gbind/` 實際是 `projects/medp/src/` 下與 `gcore/` **平行**的目錄，不在 `gcore/` 裡（我原先交辦時說錯，agent 查證後更正）。

---

## 8 KB 上限溯及拆檔（2026-08-12）

新增鐵律「文件單檔上限 8 KB」後，把 6 份既有超標文件按主題拆開：

| 原檔（保留為母檔）| 原大小 | 拆成 |
|---|---|---|
| `docs/work/design/tome4_recommendations.md` | 24930 | 母檔＋5 子檔（§0-1／§2 A-D／§2 E-F／§2 G-I／§3-6）|
| `workflows/specs/zone-addressing-lifecycle-design.md` | 18689 | 母檔＋4 子檔（拍板結果／機制 §3.1-3.6／跨 zone §3.7-3.13／§4-9）|
| `docs/references/entt_tutorial.md` | 12254 | 母檔＋3 子檔 |
| `docs/references/cereal_tutorial.md` | 11046 | 母檔＋3 子檔 |
| `workflows/specs/world-zone-subclass-design.md` | 10390 | 母檔＋2 子檔（設計本體／測試與執行）|
| `docs/references/godot_gdextension_setup.md` | 8534 | 母檔＋2 子檔 |

**慣例（值得你確認是否認同）**：原檔名一律**保留為母檔**變成索引，內容移到 `<母檔名>_<主題>.md`。這樣 repo 各處既有連結一條都不會斷。拆檔是純搬運，**不濃縮內容**——教學與決策紀錄的價值就在細節。

**驗證**：六組都用「原檔每行是否仍存在於子檔集合」逐行比對，**零內容遺失**。少數不完全相符的行都是刻意加的跨檔導覽連結（例如原本「見下方第 6 節」在拆檔後會誤導，改成指向對應子檔），不是刪除。

副作用已處理：拆檔使指向原檔的**行號引用失效**，已修正 [SESSION-LOG](../SESSION-LOG.md) 與 [save-format-position-z.md:70](../workflows/plans/save-format-position-z.md:70) 兩處。

---

## ToME4 研讀→重寫建議報告

[docs/work/design/tome4_recommendations.md](../docs/work/design/tome4_recommendations.md) — 研讀 `C:\code\mine\modding_tome4` 語料，經三路對抗性覆核修訂。

**看點**：§2 的 P0 三項（Position 補 z、存檔版本欄位、生命週期策略）與 §4 落地順序是否合你意？

註：§4 step 0 寫的「測試套件已隨重構斷裂、16 項基準失效」是當時狀態，**現已解除**（測試 21/21 綠）。P0 三項已落地兩項，存檔版本欄位經你裁定放棄。

---

## docs/index.html 文件彙整頁

[docs/index.html](../docs/index.html) — 依「進度總覽／設計架構(work)／外部教學(references)」三區列出各份 md，各附標題、一句摘要、路徑與連結；單一自帶樣式 HTML，可直接用瀏覽器開。**需親自開瀏覽器看。**（原文說「全部 9 份」，2026-08-12 拆檔後子檔由各母檔索引，卡片仍只列母檔。）

**看點**：分區與摘要是否符合你對 docs 的心智？連結指向 .md 原檔（瀏覽器多半顯示原始文字），若想要渲染後閱讀體驗再告知。

註：卡片已隨 2026-08-12 的文件整備更新——補上原本漏列的 ToME4 報告與 `world_actor_game`，並改掉 `zone_streaming_architecture` 的「已過期」警語（該份已重寫）。
