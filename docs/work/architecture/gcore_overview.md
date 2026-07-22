# gcore 逐檔導覽

> 對象：`projects/medp/src/gcore/`（遊戲核心框架，EnTT + cereal）。最後更新：2026-07-22（Zone/ZoneManager 新核心）。
> 線性導讀（含帶行號的閱讀順序）另見 [CODE_TOUR](../../../workflows/common/code-map/CODE_TOUR.md)。

## 全域心智模型

一個 **zone = 一個 `Zone`**：自己的 `entt::registry`（實體）＋自己的多層 tile 地圖（`layers`）＋身分（`id`/`parent`）。
`ZoneManager` 持有永駐的 root（id=0，放非地圖的全局實體）與所有已載入 zone，負責 id 配發、tick、存讀檔。
zone id 是**零語意**的單調序號；層級只活在 `parent` 鏈上。存檔一 zone 一檔＋一個 `manifest.bin`，無版本欄位。

依賴方向：

```
util/tdarray.hpp ── zone/tile.h ── zone/zone.h ─┬─ zone/zone_manager.h/.cpp
components/* ── serialize/all_components.h      │        │
                        │                       │        │
serialize/entt_cereal_archive.h ── serialize/registry_io.h ── serialize/zone_io.h
systems/movement.h（吃 Zone&）
```

## §1 zone/ — 核心

- **`zone.h`**：`ZONE_ROOT=0`；`Zone{ uint64_t id, parent; entt::registry reg; map<int, tdarray<Tile>> layers; }`。
  layers 鍵即 z（地面=0、往下為負），層是稀疏的。地圖是 zone 固有結構、不走 ECS，
  代價是 registry snapshot 不含它，存檔由 zone_io 分兩塊處理。registry 不可複製 → Zone 只能移動。
- **`tile.h`**：`Tile{ uint32 terrain, flags }`＋`TILE_WALKABLE`/`TILE_BLOCKS_SIGHT`。不是 component。
- **`zone_manager.h/.cpp`**：
  - `create_child(parent)`：id 單點配發（`next_id_++`、永不復用），配發即原子寫 manifest；撞既有檔或 parent 未載入 → throw。
  - 開檔協定（建構子）：有 manifest → 還原 next_id＋必讀回 root.bin（缺失 throw）；無 manifest 但有 .bin → throw；乾淨目錄 → 新世界。
  - `load` 驗檔內 id；`destroy` 同步刪盤上檔案；`unload` 寫檔後移除（root 不可）。
  - `tick()`：對**所有**已載入 zone（含 root）依註冊順序跑 ZoneSystem（`void(Zone&)`）。
  - 三條契約（註解明文）：tick 內禁止 zone 結構性變更；存檔目錄=單槽活儲存；`Zone*` 不跨 tick 持有。

## §2 components

- `Position{ int x, y, z }`：zone 內 grid 座標，z 即 layers 的鍵。
- `Velocity{ int dx, dy }`：每 tick 移動步（示範用）。

## §3 serialize

- `all_components.h`：**唯一清單**，列出所有要序列化的 component。新 component 必加。
- `entt_cereal_archive.h`：EnTT snapshot ⇄ cereal PortableBinary 的 adapter。
- `registry_io.h`：單一 registry 的 snapshot save/load；`loader.orphans()` 清掉無 component 的 entity（陷阱：只帶未登記 component 的 entity 整個消失）。
- `zone_io.h`：完整 Zone 的 save/load——第一塊（id/parent/layers）直接 cereal，第二塊（reg）走 registry_io；兩塊依序接在同一 stream。

## §4 systems

- `movement.h`：`move_by(Zone&, entity, dx, dy)` 是**位置變更唯一入口**（將來 tile flag 檢查、空間索引維護掛此）；
  `movement(Zone&)` 是 system 形狀範本——自由函式、簽章即 ZoneSystem、可直接 `zm.add_zone_system(systems::movement)`。

## §5 util

- `tdarray.hpp`：泛型 2D 稠密陣列（`std::vector<T>` 打平，索引 `x*sy+y`），已 cereal 化；回傳 bool 的操作一律「true=失敗/越界」。
- `mydef.h`：metaprogramming 巨集（仍使用中）。

## 速查表

| 檔案 | 角色 |
|------|------|
| `zone/zone.h` | Zone 核心結構（id/parent/reg/layers） |
| `zone/tile.h` | 地圖格＋通行 flags |
| `zone/zone_manager.h/.cpp` | 生命週期/配號/tick/存讀/開檔協定 |
| `components/position.h` | zone 內 grid 座標（x,y,z） |
| `components/velocity.h` | 移動步（示範） |
| `systems/movement.h` | move_by 收口＋movement 系統範本 |
| `serialize/all_components.h` | component 清單唯一來源 |
| `serialize/entt_cereal_archive.h` | EnTT ⇄ cereal adapter |
| `serialize/registry_io.h` | 單一 registry snapshot 存讀 |
| `serialize/zone_io.h` | 完整 Zone 存讀（兩塊接合） |
| `util/tdarray.hpp` | 泛型 2D 稠密陣列 |
| `util/mydef.h` | metaprogramming 巨集 |
