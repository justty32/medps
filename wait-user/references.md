# 待過目 — references 庫教學（`docs/references/`）

母檔入口：[WAIT_USER.md](../WAIT_USER.md)。完成即從母檔與本檔一併移除。

（從 [wait-user/docs.md](docs.md) 分出——該檔撞到 8 KB 上限。docs.md 管 `docs/work/` 的設計文檔，本檔管 `docs/references/` 的庫教學與架構教學。）

---

## references 教學對齊新核心（2026-08-12）

**問題比「過期」嚴重**：[how_to_add_component_and_system.md](../docs/references/how_to_add_component_and_system.md) 整份建立在 2026-07-22 就刪除的 `GlobalManager`／`ZoneKey`／`zone_meta.h` 上——它在教人用**不存在的 API** 新增 component 與 system，照著做必然編譯失敗。

改動：

1. **how_to 實質重寫**（5936→7867 bytes）：新增「Step 0：判斷放哪一族」（`world/components/`＝依賴地圖格如 Position／Velocity，`common/components/`＝跨 zone 身分層如 Name／Owner／Location／Unit）；system 簽章改 `void(Zone&)` 並說明為何不是 `entt::registry&`；註冊／tick 範例全換成 `ZoneManager`；補上「root 也參加 tick」與「tick 內禁止 zone 結構性變更」兩條現行規則；測試範例改為可對照 `projects/tests/src/main.cpp:105,373`。`AllComponents` 登記鐵律保留並強調。
2. **entt 教學**：`_basics` 路徑修正並把範例 `Position` 改為 `{int x,y,z}`（與現行一致，原本是 `{float x,y}`）；`_views_systems` 的 §4 由「吃 `entt::registry&`」改為「吃 `Zone&`」並直接引用現行 `movement.h:12-24`；`_serialize` 的架構對應表把 `GlobalManager`／`ZoneMeta`／`WorldConfig` 換成現行模組。
3. **entt `_basics` 的 `Owner` 命名衝突**（agent 提出、我裁定）：教學範例有個叫 `Owner` 的示意結構、欄位是 `entt::entity faction`，撞上專案真正的 `Owner`（`owner.h:9`，欄位其實是 `uint64_t`，**刻意不用 `entt::entity` 正是因為跨 registry 無效**）。已改名為 `Target` 並加警語說明兩者差別——原樣留著會讓讀者學到與專案相反的做法。
4. **godot 教學**：`_gbind` 的 `GlobalManager` 改 `ZoneManager`；`_build` 的架構圖原是 ASCII 框線圖（違反 AGENTS 「架構圖優先用 Mermaid」那條），已改寫為 Mermaid 並順帶更新內容。

**看點**：how_to 的「Step 0 判斷放哪一族」是否符合你對 `world/` vs `common/` 分界的想法——這是新增 component 時最容易放錯的一步。

（`zone_streaming_architecture.md` 原本也在這條債裡，2026-08-12 已整份重寫——見下一節。）

---

## zone_streaming_architecture 整份重寫（2026-08-12）

原本 8 個章節裡有 4 個（`ZoneKey` 定址、path 整除推導、`ZoneMeta` placeholder、`WorldConfig` singleton）講的東西已完全不存在，等於整份作廢。

**重點在定位收斂**：repo 已有三份講架構的文件，這份若照舊寫只是第四份重複。新定位是**概念教學——回答「為什麼是這個架構」**，母檔開頭直接放一張分工表劃線：`gcore_overview`＝逐檔地圖、`CODE_TOUR`＝線性導讀、`how_to_add_component_and_system`＝操作步驟、本文＝動機與取捨。全文只用連結指過去，不複製檔案清單或 SOP。

拆為母檔（6148 bytes，概念總覽＋分工表）＋ [addressing](../docs/references/zone_streaming_architecture_addressing.md)（5418，定址翻轉／身分掛 struct／跨 registry 引用）＋ [lifecycle](../docs/references/zone_streaming_architecture_lifecycle.md)（6565，單槽活儲存語意／開檔 fail-fast 協定／兩條硬約定／序列化契約與代價）。檔名保留，5 處入站連結未斷。

**看點**：(1) 這個「為什麼 vs 有哪些 vs 怎麼做」的三方切分是否合你意——它決定了以後架構知識該往哪份寫；(2) addressing 篇用一張 Mermaid 對照「舊 ZoneKey 位元打包 → 現行裸序號＋顯式 parent」，刻意保留舊路數當教學對比（標明已推翻），你若不想在 live 教學裡看到舊架構可以拿掉。

我另外修了 agent 產出的兩處 Mermaid 破版寫法：節點標籤用字面 `\n` 換行（新版 Mermaid 會印出 `\n` 而非換行）、以及 `<id-hex>` 的角括號會被當 HTML 吃掉。
