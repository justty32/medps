# EnTT 教學:序列化 snapshot + cereal（medps 版）

> 本篇是 [entt_tutorial.md](entt_tutorial.md) 的子篇,對應原文第 7–9 節:EnTT snapshot API、cereal archive adapter、與本專案架構對應、常見陷阱。
> 上一篇:[View / System / entity 型別 / 訊號](entt_tutorial_views_systems.md)

---

## 7. 序列化:snapshot + cereal(本專案核心)

EnTT 自己**不決定位元格式**——它用 `snapshot` 遍歷 registry,再把每個值丟給你提供的 **archive 物件**。本專案把這個 archive 接到 **cereal 的 `PortableBinaryArchive`**(見 `cereal_tutorial.md`)。

### 7.1 v3.16.0 的 snapshot API

> ⚠️ 版本差異:**3.16.0 用 `.get<T>(ar)`**(舊版是 `.component<T>()` / `.entities()`)。這是 pin 在 3.16.0 的原因之一。

```cpp
// ---- 存檔 ----
output_archive out{cereal_oarchive};        // 見 7.2 的 adapter
entt::snapshot{registry}
    .get<entt::entity>(out)   // 先存所有 entity(含 version)
    .get<Position>(out)       // 再逐 component 存
    .get<Velocity>(out)
    .get<Owner>(out);

// ---- 讀檔(讀進一個空 registry)----
input_archive in{cereal_iarchive};
entt::snapshot_loader{registry}
    .get<entt::entity>(in)    // 還原 entity,保留原本的 id
    .get<Position>(in)
    .get<Velocity>(in)
    .get<Owner>(in)
    .orphans();               // 清掉沒有任何 component 的孤兒 entity(選用)
```

- `snapshot_loader`:**保留原 id**,適合「整個世界存讀」。因為 id 不變,component 內存的 `entt::entity` 參照(如 `Owner::faction`)自動仍然有效。
- `continuous_loader`(3.16.0 同樣是 `.get<T>(ar)`):**重新映射 id**,適合把存檔合併進一個已有內容的 registry;此時參照欄位要透過 loader 做 id 轉換。**本專案整檔存讀用 `snapshot_loader` 即可。**

### 7.2 cereal⇄EnTT 的 archive adapter

EnTT 對 archive 的呼叫協定固定(size、entity、entity+component 三種),而 cereal **預設不序列化 enum**,所以 `entt::entity` 要顯式轉成底層整數。這個 adapter 是 EnTT 官方範例的標準寫法,放在 `projects/medp/src/gcore/serialize/`:

```cpp
// projects/medp/src/gcore/serialize/entt_cereal_archive.h
#include <entt.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <type_traits>

using entt_id_t = std::underlying_type_t<entt::entity>;

struct output_archive {
    cereal::PortableBinaryOutputArchive &archive;

    // 數量(entity 個數等)
    void operator()(entt_id_t size) { archive(size); }

    // 單一 entity → 存成底層整數
    void operator()(entt::entity entity) {
        archive(static_cast<entt_id_t>(entity));
    }

    // entity + component → 整數 + component(cereal 逐欄位序列化 T)
    template<typename T>
    void operator()(entt::entity entity, const T &instance) {
        archive(static_cast<entt_id_t>(entity), instance);
    }
};

struct input_archive {
    cereal::PortableBinaryInputArchive &archive;

    void operator()(entt_id_t &size) { archive(size); }

    void operator()(entt::entity &entity) {
        entt_id_t value{};
        archive(value);
        entity = entt::entity{value};
    }

    template<typename T>
    void operator()(entt::entity &entity, T &instance) {
        entt_id_t value{};
        archive(value, instance);
        entity = entt::entity{value};
    }
};
```

### 7.3 Component 型別清單只能有「一份來源」

snapshot 的 save 與 load 必須列出**完全相同、相同順序**的 component。為避免漏列 / 順序不一致,用單一來源驅動兩邊:

```cpp
// projects/medp/src/gcore/serialize/all_components.h
using AllComponents = entt::type_list<Position, Velocity, Owner /* , ... */>;
```

之後用一個 helper 對 `AllComponents` 展開,save 與 load 共用同一份清單,新增 component 只改這一行。

---

## 8. 與本專案架構的對應

| 本專案模組 | 用到的 EnTT 重點 |
|---|---|
| 每個 zone 一個 `entt::registry`(`ZoneManager` 管理) | `entt::registry` 建立 / 銷毀、entity 生命週期 |
| `world/components/`、`common/components/`(POD component) | `Position` / `Velocity`(依賴地圖格)、`Name` / `Owner` / `Location` / `Unit`(跨 zone 身分層)等 aggregate;**跨 registry 的參照存穩定 `uint64_t` id,不存 `entt::entity`** |
| `world/systems/`(吃 `Zone&` 的自由函式) | view + `each`、`ZoneManager::tick` 依註冊順序跑 system(root 也參加) |
| `serialize/`(snapshot + cereal) | `snapshot` / `snapshot_loader`、adapter、`AllComponents` 單一來源 |

---

## 9. 常見陷阱

- **single-include vs src**:只用 `#include <entt.hpp>`;`extern/entt/src` 與 single_include 在 master 上可能不一致,我們已 pin v3.16.0 規避。
- **iterator 失效**:遍歷 view 時對「同一組 component」做 `emplace`/`remove` 可能使 view 失效。需要在遍歷中建/刪時,先收集 entity、迴圈外再處理,或用 `registry.insert` 批次。
- **`get` vs `try_get`**:`get` 對不存在的 component 是 UB;不確定時用 `try_get`(回傳指標)。
- **snapshot 順序**:存與讀的 `.get<T>` 順序必須一致,否則資料錯位——這也是要單一來源清單的理由。
- **enum 不會自動序列化**:`entt::entity`(以及你自己的 enum component 欄位)要嘛用 adapter 轉整數,要嘛為該 enum 寫 cereal serialize。

---

## 參考

- EnTT 官方文件(對應 3.x):https://github.com/skypjack/entt/wiki
- 本專案序列化搭配:`docs/references/cereal_tutorial.md`
- 核心世界結構設計:`docs/work/design/zone_layers.md`

---

回[目錄](entt_tutorial.md)
