# World：第一個 Zone 子類 — 測試、落地順序與執行結果

> 子檔（拆分於 2026-08-12，原內容為 [world-zone-subclass-design.md](world-zone-subclass-design.md) §6-§9＋落地備註）。狀態：**已拍板並落地（2026-07-22）**，四步全部完成，測試 19/19 綠。
> 導覽：[← 設計本體：拍板結果、背景與繼承機制](world-zone-subclass-design-mechanism.md) ｜ [回母檔索引](world-zone-subclass-design.md)

## 6. 測試（納入既有套件，基準 15 → 15+n）

1. **kind round-trip**：`create_child(parent, ZoneKind::World)` → save → unload → load → `kind()==World` 且 `gen`（含 seed）完整還原。
2. **Plain 相容**：既有 15 項零改動全綠（`create_child` 預設 Plain、root 為 Plain）。
3. **generate 決定性**：同 seed 兩次 generate，`layers[0]` 逐格相等。
4. **generate sanity**：尺寸符合 params；有陸有海；`TILE_WALKABLE` 與水陸一致。
5. **工廠 fail-fast**：檔頭 kind 為未知值 → load throw。

## 7. 落地順序（plan 展開用）

1. libtcod 接進 CMake（headless），smoke：`TCODNoise` 可呼叫。
2. 繼承機制：Zone virtual dtor＋kind＋extra 掛鉤＋工廠；zone_io 檔頭 tag＋load 改型；ZoneManager 兩建構點改工廠。此步結束既有 15 項須全綠（格式已變，測試臨時目錄不受舊檔影響）。
3. World＋generate＋biome 佔位常數。
4. 新測試 5 項＋文檔同步（CODE_MAP、zone_layers.md 實作現況、INDEX/dev-env 的依賴說明）。

## 8. 不做範圍

Region/Area 子類、tick 依 kind 分派（時間模型）、ChildLink/anchor（既有 defer）、侵蝕/河道/連通性驗證、FOV/尋路接線、Ruleset 化的 terrain def（佔位常數先行）、cereal polymorphic。

## 落地備註（2026-07-22，與 spec 原文的偏差）

- libtcod 引入定案：FetchContent 鎖 **tag 2.2.2**（＝cookbook 分析的 commit 27c2dbc），headless 全關（SDL/zlib/PNG/unicode），stb 用其 repo 內 vendored 源——首次 configure 需網路，之後離線可重建。本機無現成 checkout（§5 提到的備援路徑實際不存在），未 vendor 進 repo。
- `zone_cast<T>` 有做（zone.h），配 `Zone::KIND`／子類 `KIND` 靜態常數。
- virtual dtor 使 Zone 的隱式 move 被抑制——Zone 現在不可複製也不可移動（§9-2 的切片風險就此結構性消除）；經查無任何呼叫端依賴移動。
- tests 專案是裸 .a 連結（無 CMake target 傳遞），故 libtcod 靜態庫隨 medp 落到同一 `build/bin`，tests 的 CMake 自行 find＋連上。
- 測試計數：新增 4 個測試函式（spec §6 的五項中「Plain 相容」由既有 15 項覆蓋），基準 15 → **19**。

## 9. 風險

1. **libtcod 是第一個編譯型第三方依賴**——build 變慢、跨平台面（將來 Godot 端）擴大；headless 子集無 SDL 面，風險有限，但 plan 須驗 gbind 組態不受影響。
2. **子類物件切片**：`Zone` 可移動的既有性質＋繼承 → 誤以值傳遞會切片；Zone 全程走 `unique_ptr`/引用（現行慣例已如此），plan 時考慮刪除 Zone 的 move 建構子暴露面或加註警語。
3. **save_extra 忘記對稱**：save/load extra 手寫成對，欄位漏寫即靜默錯位——kind round-trip 測試（§6-1）是對症藥。

## 導覽

[← 設計本體：拍板結果、背景與繼承機制](world-zone-subclass-design-mechanism.md) ｜ [回母檔索引](world-zone-subclass-design.md)
