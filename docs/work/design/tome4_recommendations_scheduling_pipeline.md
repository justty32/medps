# ToME4 架構研讀 → medps 重寫建議報告 —— §2 建議清單 G-I

> 本檔是 [tome4_recommendations.md](tome4_recommendations.md) 的拆分子檔，收錄 §2 建議清單的 G～I 段（排程、生成管線、方法論），為 §2 的最後一段。上一節：[§2 E-F（存檔／定址）](tome4_recommendations_save_addressing.md)；下一節：[§3-§6（舊設計意圖清點／落地順序／存疑處／總結）](tome4_recommendations_status_summary.md)。

### G. 排程（P1）——能量制值得低成本引入

ToME 的核心排程只有一條公式：`energy.value += energy_per_tick * energy.mod * global_speed`，滿 `energy_to_act`（1000）就 `act()`。回合制（GameTurnBased）只是這套加一個 `paused` 旗標，**全部 54 行**。天然支援：速度差（mod）、全局時間縮放（global_speed）、以及 medps 特別需要的——**不同 zone 用不同 `energy_per_tick` 實現「非活躍 zone 低頻模擬」**。

對 medps：一個 `Energy{value, mod}` component + 一個 per-zone system 就能起步，與現行 `ZoneManager::tick()`（[zone_manager.cpp:74](../../../projects/medp/src/gcore/zone/zone_manager.cpp)）完全相容。舊設計「World 純回合 / Region WeGo / Area 即時」的三種時間模型，用 per-zone 的能量參數差異就能統一表達，不需要三套迴圈。

行動順序警訊：ToME 用 `e_array` 維護回合順序、`last_iteration` 處理迭代中移除。EnTT view 迭代中 destroy 當前實體是安全的，但「跨實體的行動順序」EnTT 不管——需要確定性順序時（回合制戰鬥）得自己維護序列。

### H. 生成管線（P2 實作，P1 先定介面形狀）

medps 地圖生成完全空白。ToME 的管線形狀值得整套參考：

1. **四類 generator 拆開**（map/actor/object/trap 各自獨立）——對應 medps 天然分工：map generator 寫 `Zone::layers`，actor/object generator 寫 `Zone::reg`。
2. **宣告式綁定**：zone 定義用 `{class="...", 參數...}` 指定生成器，換演算法只改資料。medps 目前沒有「zone 藍圖/定義」概念——這是接 Ruleset（見 §4，[status/summary 檔](tome4_recommendations_status_summary.md)）時該一起設計的。
3. **中間表示 + 草稿 commit**：ToME 部分生成器先在抽象 tilemap（字元格網）上跑演算法，最後才映射成實體。medps 版本：生成在 `tdarray` 草稿上做，連通性驗證通過才 commit 進 Zone——順便解決「生成失敗重試不污染正式資料」。
4. **連通性驗證是管線內建步驟**：A* 驗證入口可達出口，失敗丟棄整張重來（上限 50 次）。真相層有個具體慘案：Static 生成的出口未定義時**預設落在地圖正中心**（`Static.lua:520-522`），中心被牆圍住 → 重試 50 次 → changeLevel **靜默失敗**、玩家留在原地。教訓兩條：生成失敗必須是一級回傳值（不是 log 一行了事）；隱式預設（出口=中心）是坑源，要求顯式宣告。連通性的建構面：**MST 保底連通 + `fattenRandom`/`fattenShorter` 加環路**的兩段式介面值得照抄——4X 的道路、河谷連通同樣適用。
5. **spots**：生成器除了地形還輸出語意化生成點（出口、boss 房、寶藏位），後續放置階段消費。介面上第一天就留這個欄位。
6. **overlay 語意**：ToME 的子生成器貼圖時「nil 格跳過、只貼有東西的格子」，多來源共編一張地圖互不衝突。medps 若做模組疊加，Tile 層面預留「未定義」哨兵值。
7. **非同步介面 + 尺度分家**：ToME 的 WFC 生成器可獨立非同步跑、上層 `waitAll` 平行等待多實例——重演算法放 C++ 核心、參數與樣本放資料層。medps 將來按需背景生成多個 zone，介面第一天就以「可非同步 + 可平行等待」設計比同步阻塞好。另外不同尺度配不同演算法家族：world 層（200²）用 heightmap/noise，area 層（250²）用 Cavern/BSP/Forest——正是 generator 可插拔架構的用武之地。
8. **定義/實體化分離（resolver 的教訓）**：ToME 的實體定義可含延遲亂數規則（`resolvers.rngrange(1,5)` 只是標記 table），生成期才 `resolve()` 展開，並以 `current_level` 等情境變數計算（越深越強）。對 medps 的關鍵啟示：**生成規則屬「定義層」資料（Ruleset 側），存檔只存展開後的實體化結果**——這一刀切下去，「cereal 怎麼存 lambda/規則」的難題整個消失，也正是 [E-F 檔](tome4_recommendations_save_addressing.md) §2-E-6「行為不進存檔」的正面解法。另注意 ToME 用 `__resolve_instant`/`__resolve_last` 標記展開順位——medps 的生成規則管線需要顯式的階段順序，不能仰賴定義順序。
9. **ToME 幫不上的部分**：它全部是單層 2D 生成，**多垂直層生成（跨層樓梯/坑洞連通）是 medps 自己的功課**，generator 介面第一天就要以「layer 為生成單位 + 跨層連通驗證」設計。

### I. 方法論（貫穿性）

真相層 README 的核心教訓：**這個引擎很愛靜默失敗**——寫錯了不拋錯，只是安靜地什麼都不發生（`grids.lua` 缺席→整張地圖靜默空白；`wda.script` 指錯檔→玩家走第一步才炸）。該 repo 因此建立三層驗證（靜態檢查→無頭載入→實機操作）。

medps 的對應：

- **缺席行為要分級且文件化**：必填缺席 → fail-fast 並報 zone id；可選缺席 → 文件化的預設值。絕不允許「缺了就靜默空白」。medps 現行最大的同類風險就是 AllComponents 漏登記（悄悄漏存），E-2（見 [E-F 檔](tome4_recommendations_save_addressing.md)）的 round-trip 驗證是對症藥。
- **三層驗證的 medps 版**：compile-time（type_list / static_assert）→ 單元測試 → 序列化 round-trip + 「入口走得到出口」這類語意性整合測試。「Level unconnected 只有實機才現形」說明：zone 能建出來 ≠ zone 是對的。
- **避開包裹式巨型旗標**：ToME 的 `wilderness=true` 一個布林同時改時間流速、FOV 管線、技能鎖、掉落語意，不能只挑一部分用。medps 的 per-zone system 註冊機制天生能做對：把這類差異做成可獨立掛/不掛的 system 組合。
- **渲染不進資料層**：ToME 的 Map.lua 一半欄位是渲染態，是資料/渲染耦合的反面教材。medps 後端庫保持純資料、渲染歸 Godot 前端——現行設計已正確，維持。

---
上一節：[§2 E-F（存檔／定址）](tome4_recommendations_save_addressing.md)　|　下一節：[§3-§6（舊設計意圖清點／落地順序／存疑處／總結）](tome4_recommendations_status_summary.md)　|　回索引：[tome4_recommendations.md](tome4_recommendations.md)
