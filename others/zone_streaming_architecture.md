# Zone Streaming 架構教學

> 本文說明 medps 的多 registry / zone streaming 架構:`GlobalManager`、`ZoneKey`、分層 index、整盤存讀、跨 zone 參照。
> 對應程式碼:`src/gcore/global_manager.{h,cpp}`、`src/gcore/zone_key.h`、`src/gcore/components/`。

---

## 0. 核心概念

整個遊戲世界被切成多個 **zone**,每個 zone 是一個獨立的 `entt::registry`:

```
GlobalManager
├── root  (entt::registry)          ← ZONE_ROOT,永久存活,全局實體住這
└── loaded zones (按需)              ← unordered_map<ZoneKey, registry>
    ├── province (1,0,0)
    │   └── area (3,4,0)
    └── province (2,0,0)
```

- **root**:永遠載入。faction、文明、神祇等跨 zone 的全局實體放這裡。
- **zone**:world / province / area 等,**只在需要時載入**(玩家看得到的),其餘留在磁碟。這就是開放世界式的 streaming。

---

## 1. ZoneKey:zone 的唯一識別 + 定址

`ZoneKey` 是一個 `uint64_t`,把「層級型別 + 全局座標」打包進去:

```
bits 63-48 : ZoneType (16)  — 層級/種類(Province/Area/...)
bits 47-32 : x (16, signed) — 全局座標
bits 31-16 : y (16, signed)
bits 15-0  : z (16, signed) — 層(地下/地表/天空...)
```

```cpp
ZoneKey k = make_zone_key(ZoneType::Province, 3, 4, 0);
zone_key_type(k);  // ZoneType::Province
zone_key_x(k);     // 3
```

- **全局座標**:一個 ZoneKey 就能唯一定址任何 zone,不需要路徑。
- **`ZONE_ROOT == 0`**:root 的保留值。`ZoneType::Invalid = 0` 也被保留以免撞值(真正的 type 從 1 開始)。

---

## 2. path 用 ZoneKey 推導,不存清單

zone 的磁碟路徑由 key **算出來**,不維護任何全域清單:

```cpp
gm.zone_path(key);   // → zones_dir / "00010003_0004_0000.bin" 之類
```

**為什麼**:若把 `map<ZoneKey, path>` 存進記憶體,zone 一多(上萬、上百萬)清單本身就佔住記憶體,違背 streaming 的初衷。改成推導後:

- 「某 zone 存不存在?」→ 檢查檔案是否存在
- `GlobalManager` 不持有任何持久化清單,記憶體**只跟當前載入的 zone 數成正比**

---

## 3. 每個 zone 的 placeholder:ZoneMeta

每個 zone 至少有一個帶 `ZoneMeta` 的 entity(由 `create` 自動建立):

```cpp
struct ZoneMeta {
    ZoneKey self;     // 這個 zone 是誰
    ZoneKey parent;   // 直屬父 zone;ZONE_ROOT = 掛在 root 下
};
```

兩個作用:
1. **存活保證**:序列化用 `snapshot_loader::orphans()` 會刪掉「沒有任何 component」的 entity。placeholder 帶著 `ZoneMeta`,確保即使空 zone 也不會被清空。
2. **身份**:載入後 registry 知道自己是哪個 zone、父 zone 是誰。

> CONVENTION:任何新建的 zone 都必須有這個 placeholder。用 `GlobalManager::create` 就會自動處理。

---

## 4. 分層 index:不載入也能總覽子 zone

需求:畫世界地圖時,要列出所有 province / area,但**不能**把它們全載入。

做法:**父 zone 的 registry 裡,每個直屬子 zone 是一個 stub entity**,帶 `ChildZoneSummary`:

```cpp
struct ChildZoneSummary {
    ZoneKey key;
    // 可在同一 stub entity 加 Position / MapIcon / 探索狀態...
};
```

於是:

```cpp
gm.create(province, ZONE_ROOT);   // 在 root 建一個指向 province 的 stub
gm.create(area, province);        // 在 province 建一個指向 area 的 stub

gm.children(ZONE_ROOT);    // → [province, ...]   只讀 root 的 stub,不載入 province
gm.children(province);     // → [area, ...]        需 province 已載入,但不載入 area
```

**index 跟著 streaming**:子 zone 摘要存在父 zone 裡,父 zone 卸載時摘要也卸載——沒有全域常駐清單。要總覽哪一層,就載入那一層的父 zone。

要畫地圖時,直接對父 registry 跑 view(stub 上可附加更多 component):

```cpp
parent_reg.view<ChildZoneSummary, MapIcon>().each(
    [](auto& summary, auto& icon){ /* 畫出子 zone,不載入它 */ });
```

---

## 5. 跨 zone 參照:CrossZoneRef

zone 內部的 entity 互相參照用普通 `entt::entity`。但**跨 zone**(通常指向 root 的全局實體)要用 `CrossZoneRef`:

```cpp
struct CrossZoneRef {
    ZoneKey      zone;          // 目標 zone;ZONE_ROOT = root
    entt::entity local_entity;  // 該 zone registry 內的 entity
};
```

解析:

```cpp
CrossZoneRef ref{ZONE_ROOT, faction_entity};
auto res = gm.resolve(ref);

if (res.valid()) {
    auto& faction = res.reg->get<FactionData>(res.entity);
}
```

`resolve` 回傳的 `ZoneResolution` 區分三種狀態:

| 狀態 | 意義 |
|---|---|
| `res.reg == nullptr` | 目標 zone **未載入** |
| `res.reg != nullptr && !res.valid()` | zone 載入了,但 entity 已失效(stale) |
| `res.valid()` | 完全可用 |

`resolve` **只查已載入的 zone,不碰磁碟**。跨 zone 存取較少見、可接受慢一點;若目標未載入,由呼叫端決定要不要 `load`。

---

## 6. 整盤遊戲的存讀

| 操作 | 說明 |
|---|---|
| `gm.create(key, parent)` | 新建 zone(自動放 placeholder + 在父 registry 登錄 stub),冪等 |
| `gm.load(key)` | 從推導路徑載入既有 zone |
| `gm.unload(key)` | 存回磁碟並從記憶體移除(streaming out) |
| `gm.save_all()` | **存檔點**:寫 root + 所有當前載入的 zone(不卸載) |
| `gm.load_root()` | **開遊戲**:載入 root,子 zone 之後按需 streaming |

典型流程:

```cpp
// 開新遊戲
GlobalManager gm;
gm.zones_dir = "save_001";
// ... 建立 root 內容、建立初始 zone ...
gm.save_all();                 // 存檔

// 重開遊戲
GlobalManager gm2;
gm2.zones_dir = "save_001";
gm2.load_root();               // 只載 root
auto& province = gm2.load(some_province_key);  // 玩家移動到此處才載入
```

---

## 7. 設計要點回顧

- **path 推導,不存清單** → 記憶體只跟載入數成正比。
- **分層 index(父存子摘要)** → 總覽不需載入子 zone,且 index 也 streaming。
- **root 永久 + zone 按需** → 全局實體集中,世界其餘部分 stream。
- **跨 zone 用 CrossZoneRef + resolve** → 三態回傳,只查已載入,不偷做 IO。
- **每 zone 一個 ZoneMeta placeholder** → 保證存活、攜帶身份。

---

## 參考

- 序列化機制(snapshot + cereal):`others/entt_tutorial.md` §7、`others/cereal_tutorial.md`
- 新增 component / system:`others/how_to_add_component_and_system.md`
- 程式碼:`src/gcore/global_manager.{h,cpp}`、`src/gcore/zone_key.h`、`src/gcore/components/`
- 測試:`test/src/main.cpp`
