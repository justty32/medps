# ToME4 架構研讀 → medps 重寫建議報告 —— §2 建議清單 E-F

> 本檔是 [tome4_recommendations.md](tome4_recommendations.md) 的拆分子檔，收錄 §2 建議清單的 E～F 段（存檔、定址與跨 zone 引用）。上一節：[§2 A-D](tome4_recommendations_p0_core.md)；下一節：[§2 G-I（排程／生成管線／方法論）](tome4_recommendations_scheduling_pipeline.md)。

### E. 存檔（P0 兩條 + P1/P2 若干）

現行鏈路：`zone_io`（[zone_io.h](../../../projects/medp/src/gcore/serialize/zone_io.h)：id/parent/layers 走 cereal，reg 走 snapshot）→ `registry_io` → `entt_cereal_archive`。ToME 對照出的缺口：

1. **（P0）檔頭加版本欄位**。ToME 存檔帶 `description.lua`（模組/版本/addon 清單與可讀取旗標；如何用它判定可讀性，語料未載細節）。medps 正在重寫、格式月月變（這輪就變了三次：tdarray sx/sy、Tile 寬度、layers 容器），而現行檔案格式不自帶 schema、程式也未用 cereal 的版本機制（`CEREAL_CLASS_VERSION` 存在但我們沒用）。在 `zone_io::save` 開頭寫一個 `uint32 format_version`，讀時比對，成本近乎零，現在就該加。
2. **（P0）測試基準已斷 + orphans 語意需重新驗證**。現行測試套件（projects/tests/src/main.cpp）仍 include 這輪已刪的 zone_key.h / zone_meta.h / area_terrain.h / blocking.h / global_manager.h，並呼叫舊簽章 `zone_io::save(registry, stream)`——**整套 16 項測試目前不可編譯，AGENTS.md 的「16 項全綠」基準已失效**。舊套件其實已有 round-trip 與 orphans 機制驗證（serialize_roundtrip、serialize_orphans_removed），重寫測試套件時應保留並強化這兩類。orphans 的新風險點：ZoneMeta placeholder 已刪，純地圖無實體的 zone 是安全的（身分在 struct 上），但 [registry_io.h:30](../../../projects/medp/src/gcore/serialize/registry_io.h) 的 `loader.orphans()` 仍會讓「entity 只帶未登記 component」在讀檔時整個消失——這個坑要用「存後即讀回比對 entity 數」的測試升級成機制驗證。
3. **（P1）兩階段載入**。ToME 先反序列化所有物件、重設 metatable，**最後**才統一跑 `:loaded()`，保證相互引用完整才初始化。medps 對應：component 的反序列化不做依賴其他 entity 的初始化；ZoneManager 讀檔流程預留 post-load pass 的位置。
4. **（P2）背景存檔**。ToME 用 coroutine 分批（注意：語料明確說是協程，**不是**執行緒）達到不卡頓。medps 的「一 zone 一檔」天然支援按 zone 分批；將來需要時，「先 snapshot 到記憶體、再丟工作執行緒寫盤」比邊玩邊序列化安全。
5. **（維持）AllComponents 白名單**優於 ToME 的 `_no_save_fields` 黑名單。現行機制不動，鐵律（新 component 必登記）延續。
6. **（避開）**「行為進存檔」：ToME 的 `change_level_check` 函式會被序列化進存檔、因此不可有 upvalue——這是深坑。medps「存檔只有資料、行為在 system」的路線正確，堅持住，抵抗任何可序列化 callback 的誘惑。正面解法見 [G-I 檔](tome4_recommendations_scheduling_pipeline.md) 的 §2-H-8 定義/實體化分離。

### F. 定址與跨 zone 引用（P0 設計題，實作可後置）

zone id 現在是裸 uint64、z 不進 id（本輪口頭拍板）。舊 ZoneKey 位元打包被刪時，同時失去了三樣東西，重新設計時要**逐一決定補齊還是放棄**：

| 舊方案買到的 | 現況 | ToME 給的參考 |
|---|---|---|
| key → 磁碟路徑可推導 | ZoneManager::path 用裸 id 的 hex，仍可推 | ToME 用字串短名當檔名，`+` 前綴帶命名空間 |
| key 自帶層級/座標語意 | 已失去（parent 改顯式欄位） | ToME 的 zone 定址就是字串短名，無座標語意——**它活得很好** |
| 免全域索引 | ZoneManager 的 map 就是索引 | ToME 的 zone 以目錄顯式存在、按短名推導路徑，無全域註冊清單 |

此外兩個 ToME 用血換來的教訓，直接關聯 medps 懸案：

- **跨 zone 實體引用**：`entt::entity` 是 per-registry 的，root zone 的全局角色（陣營、神祇）要指到某 zone 內的實體，現在無法表達。ToME 的做法是全域 uid + 弱引用（查不到就當死亡，容忍失效而非強行保證有效）。medps 若需要，對應物是：自建穩定 uid（uint64）+ 每 zone 的 uid→entity 映射 + 「解引用可失敗」的 API 形狀。
- **返回座標不能是全域一對**：ToME 玩家的 `wild_x/wild_y` 全域唯一（`Game.lua:1238-1248`），第二張大地圖一出現就壞掉（回錯座標、跳海、越界崩潰），原版沒踩到只因為只有一張大地圖。medps 做 zone 切換時，「玩家在每個 parent zone 的返回位置」必須是 **per-zone 記錄**，不是 player 身上的單一欄位。
- **傳送是 tile/entity 上的資料**：ToME 的 `change_zone`（目的地短名）+ `change_level`（層號）掛在 grid 上，回大地圖也是同一機制（樓梯 grid 指向大地圖）。medps 的 zone 傳送應同樣做成資料（Tile flag 或 component），不硬編在流程裡。

---
上一節：[§2 A-D](tome4_recommendations_p0_core.md)　|　下一節：[§2 G-I（排程／生成管線／方法論）](tome4_recommendations_scheduling_pipeline.md)　|　回索引：[tome4_recommendations.md](tome4_recommendations.md)
