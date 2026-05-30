# Zone / Registry 架構教學

> 本文說明 medps 的多 registry / zone 架構：`GlobalManager`、`ZoneKey`、path 推導、per-zone system、整局存讀。
> 對應程式碼：`src/gcore/global_manager.{h,cpp}`、`src/gcore/zone_key.h`、`src/gcore/components/`、`src/gcore/serialize/`。

---

## 0. 核心概念

整個遊戲世界被切成多個 **zone**，每個 zone 是一個獨立的 `entt::registry`：

```
GlobalManager
├── root  (entt::registry)          ← ZONE_ROOT，永久存活，全局實體住這
└── loaded zones (按需)              ← unordered_map<ZoneKey, unique_ptr<registry>>
    ├── World  (0,0,0)
    ├── Region (1,0,0)
    └── Area   (15,0,0)
```

- **root**：永遠載入。faction、文明、神祇等跨 zone 的全局實體放這裡。
- **zone**：World / Region / Area 等地圖層，**只在需要時 `load()`，用完 `unload()`**，其餘留在磁碟。

---

## 1. ZoneKey：zone 的唯一識別 + 定址

`ZoneKey` 是一個 `uint64_t`，把「層級型別 + 全局座標」打包進去：

```
bits 63-48 : ZoneType (16)  — 層級/種類（World / Region / Area）
bits 47-32 : x (16, signed) — 全局座標
bits 31-16 : y (16, signed)
bits 15-0  : z (16, signed) — 垂直層（Underground −1 / Ground 0 / Sky +1）
```

```cpp
ZoneKey k = make_zone_key(ZoneType::Region, 3, 4, 0);
zone_key_type(k);  // ZoneType::Region
zone_key_x(k);     // 3
```

`ZoneType` 同時代表階層深度（`World ⊃ Region ⊃ Area`），所以父層型別是隱含的：

```cpp
parent_of(area_key);    // → 對應的 Region key
parent_of(region_key);  // → 對應的 World key
parent_of(world_key);   // → ZONE_ROOT
```

- **全局座標**：一個 ZoneKey 就能唯一定址任何 zone，不需要路徑。
- **`ZONE_ROOT == 0`**：root 的保留值。`ZoneType::Invalid = 0` 也被保留以免撞值（真正的 type 從 1 開始）。
- 建構輔助：`world_key(z)`、`region_key(world_x, world_y, z)`、`area_key(world_x, world_y, region_local_x, region_local_y, z)`。

尺度常數集中在 `zone_scale`（`zone_key.h`）：`WORLD_DIM_DEFAULT=200`、`WORLD_LAYERS_DEFAULT=3`、`REGION_DIM=15`、`AREA_DIM=250`，以及 `valid_world_dim()` 檢查。

---

## 2. path 用 ZoneKey 推導，不存清單

zone 的磁碟路徑由 key **算出來**，不維護任何全域清單。預設後端 `FolderZoneStore`（`serialize/zone_store.h`）一個 zone 一個檔案：

```cpp
// dir_/<16 碼 hex key>.bin；ZONE_ROOT 特例為 dir_/root.bin
store.path(key);   // → dir_/0002000300040000.bin 之類
```

**為什麼**：若把 `map<ZoneKey, path>` 存進記憶體，zone 一多清單本身就佔住記憶體。改成推導後：

- 「某 zone 存不存在？」→ `store.has(key)` 檢查檔案是否存在
- `GlobalManager` 不持有任何持久化清單，記憶體**只跟當前載入的 zone 數成正比**

`ZoneStore` 是抽象後端（`write/read/has/flush`），把 GlobalManager 與「存放位置」解耦；`FolderZoneStore` 是目前唯一且預設的實作。`zone_io`（`serialize/zone_io.h`）負責 registry⟷位元組，`ZoneStore` 負責位元組⟷儲存。

---

## 3. 每個 zone 的 placeholder：ZoneMeta

每個 zone 至少有一個帶 `ZoneMeta` 的 entity（由 `create` 自動建立）：

```cpp
struct ZoneMeta {
    ZoneKey self;     // 這個 zone 是誰
    ZoneKey parent;   // 直屬父 zone；ZONE_ROOT = 掛在 root 下
};
```

兩個作用：
1. **存活保證**：序列化用 `snapshot_loader::orphans()` 會刪掉「沒有任何 component」的 entity。placeholder 帶著 `ZoneMeta`，確保即使空 zone 也不會被清空。
2. **身份**：載入後 registry 知道自己是哪個 zone、父 zone 是誰。

> CONVENTION：任何新建的 zone 都必須有這個 placeholder。用 `GlobalManager::create` 就會自動處理。

---

## 4. per-save 世界設定：WorldConfig（ROOT singleton）

世界尺寸是每份存檔的執行期設定，以 singleton component `WorldConfig` 掛在 root 上，跟著 root 走正常 snapshot/cereal 存檔路徑（不需另外的 meta 檔）：

```cpp
struct WorldConfig {
    int16_t world_dim_x;   // 世界地圖 = x × y 個 world-tile
    int16_t world_dim_y;
    int16_t world_dim_z;   // 垂直層數，預設 3（Underground/Ground/Sky）
};

gm.init_world(/*x=*/200, /*y=*/200, /*z=*/3);  // 新遊戲，冪等；覆寫既有 singleton
auto cfg = gm.world_config();                  // 目前生效設定（未設定時為預設建構值）
```

`world_dim` 在世界生成時決定，且在該存檔的整個生命週期內 **IMMUTABLE**——它已被烘進 ZoneKey 的座標語意，中途更動會讓所有既有 key 失效。前置條件：x、y 須滿足 `zone_scale::valid_world_dim`。

---

## 5. per-zone system 與 tick

system 寫成吃 `entt::registry&` 的自由函式，向 `GlobalManager` 註冊；`tick()` 對**每個已載入的 zone** 依註冊順序跑所有 system：

```cpp
using ZoneSystem = std::function<void(entt::registry&)>;

gm.add_zone_system(movement_system);
gm.tick();   // 對每個已載入 zone 跑所有 system；root 被排除
```

> root 不參與 tick——它存放全局 entity，而非地圖角色。

---

## 6. 整局遊戲的存讀

| 操作 | 說明 |
|---|---|
| `gm.create(key, parent)` | 新建 zone（自動放 ZoneMeta placeholder） |
| `gm.get(key)` | 取得已載入 zone 的 registry；未載入回傳 `nullptr` |
| `gm.load(key)` | 從推導路徑載入既有 zone；已載入則回傳既有者 |
| `gm.unload(key)` | 序列化寫回 store 並從記憶體移除 |
| `gm.save_all()` | **存檔點**：寫 root + 所有當前載入的 zone（不卸載） |
| `gm.load_root()` | **開遊戲**：只載入 root，子 zone 之後按需 `load()` |

典型流程：

```cpp
// 開新遊戲
GlobalManager gm;                              // 預設 FolderZoneStore("zones")
// 或注入自訂後端：GlobalManager gm{std::make_unique<FolderZoneStore>("save_001")};
gm.init_world(200, 200, 3);                    // 設定世界尺寸
// ... 建立 root 內容、建立初始 zone ...
gm.save_all();                                 // 存檔

// 重開遊戲
GlobalManager gm2{std::make_unique<FolderZoneStore>("save_001")};
gm2.load_root();                               // 只載 root
auto& region = gm2.load(some_region_key);      // 玩家移動到此處才載入
gm2.unload(some_region_key);                   // 離開時卸載
```

---

## 7. 設計要點回顧

- **一個 zone = 一個 registry，root 永久 + zone 按需** → 全局實體集中於 root，世界其餘部分按需 `load()/unload()`。
- **ZoneKey 全局定址，path 推導不存清單** → 記憶體只跟載入數成正比。
- **每 zone 一個 ZoneMeta placeholder** → 保證存活、攜帶身份。
- **WorldConfig 為 ROOT singleton，跟著 root 一起存** → 不需獨立 meta 檔，世界尺寸 immutable。
- **per-zone system + tick** → system 是吃 `entt::registry&` 的自由函式，root 不參與 tick。

---

## 參考

- 序列化機制（snapshot + cereal）：`others/entt_tutorial.md` §7、`others/cereal_tutorial.md`
- 新增 component / system：`others/how_to_add_component_and_system.md`
- 程式碼：`src/gcore/global_manager.{h,cpp}`、`src/gcore/zone_key.h`、`src/gcore/serialize/`、`src/gcore/components/`
- 測試：`test/src/main.cpp`
</content>
</invoke>
