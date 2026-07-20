# EnTT 教學（medps 版）

> 對應版本:**EnTT v3.16.0**(已 pin,single header `projects/medp/include/entt.hpp`)。
> 環境:C++20 / MSVC。範例直接以本專案的用法為主,核心世界結構設計見 `docs/work/design/zone_layers.md`。
> 引入方式:`#include <entt.hpp>`(single-include amalgamation;不要混用 `extern/entt/src/`,以免版本不一致)。

---

## 0. 為什麼用 EnTT / 什麼是 ECS

ECS(Entity-Component-System)把「資料」與「行為」徹底分離:

- **Entity(實體)**:只是一個 ID,本身不帶資料。在 EnTT 裡型別是 `entt::entity`(一個 enum,內含 index + version)。
- **Component(元件)**:純資料(理想上是 POD aggregate)。一個 entity 可掛任意多種 component。
- **System(系統)**:自由函式,查詢「擁有某組 component 的所有 entity」並處理之。

本專案的取捨:
- **只有動態行動者**(unit / city / hero / faction)是 entity。
- **地圖維持 grid**(`tdarray<Tile>`),不進 registry。
- entity 全由 `entt::registry` 管理;全局 / 單例狀態放 **root registry(`ZONE_ROOT`)或 `GlobalManager`**——不再有 `Obj` / `Scene`。
- System 一律寫成吃 `entt::registry&` 的自由函式。

---

## 1. registry:一切的容器

`entt::registry` 是 entity 與 component 的儲存中心。

```cpp
#include <entt.hpp>

entt::registry registry;

// 建立 entity(回傳 entt::entity)
auto e = registry.create();

// 掛上 component —— emplace 直接就地建構,引數轉發給 component 建構式
registry.emplace<Position>(e, 1.0f, 2.0f);   // Position{1.0f, 2.0f}
registry.emplace<Velocity>(e, 0.0f, 0.0f);

// 讀取(回傳 reference;若不存在會 assert/UB)
Position &pos = registry.get<Position>(e);
auto [p, v] = registry.get<Position, Velocity>(e);   // 多個一次取,回傳 tuple of refs

// 安全讀取(不存在回傳 nullptr)
if (Position *pp = registry.try_get<Position>(e)) {
    pp->x += 1.0f;
}
```

### 修改 component 的三種方式

```cpp
// 1) replace:整顆換掉(component 必須已存在)
registry.replace<Position>(e, 3.0f, 4.0f);

// 2) patch:就地改,並觸發 on_update 訊號(下方第 6 節)
registry.patch<Position>(e, [](Position &p) { p.x += 1.0f; });

// 3) emplace_or_replace:有就換、沒有就建
registry.emplace_or_replace<Position>(e, 5.0f, 6.0f);
```

### 查詢與移除

```cpp
registry.all_of<Position, Velocity>(e);  // 兩者都有?
registry.any_of<Position, Velocity>(e);  // 任一有?

registry.remove<Velocity>(e);   // 安全:沒有也不會錯
registry.erase<Velocity>(e);    // 較快但要求一定存在(否則 UB/assert)

registry.destroy(e);            // 銷毀整個 entity(連帶所有 component)
registry.valid(e);              // e 還活著嗎?(銷毀後即失效)
```

> **重點**:`destroy` 後該 entity 的 id 會被回收再利用,但 **version 會 +1**,所以舊的 `entt::entity` 值會被 `valid()` 判為無效——這正是 entity 內嵌 version 的用途(避免 dangling id)。

---

## 2. Component 設計原則(本專案)

把 component 寫成**單純 aggregate**,利於 cereal 序列化與跨平台:

```cpp
// projects/medp/src/gcore/components/position.h
struct Position {
    float x{};
    float y{};
};

struct Velocity {
    float dx{};
    float dy{};
};

// entity 之間的參照:存「另一個 entity 的 id」,不要存裸指標
struct Owner {
    entt::entity faction{entt::null};   // entt::null 是「空 entity」常數
};
```

規則:
- 盡量是 POD aggregate;不要塞虛擬函式 / 複雜建構式。
- **entity 參照存 `entt::entity`**(序列化時當整數處理,見第 7 節),不要存裸指標(如 `Position*`)。
- 含 STL 成員(`std::vector` 等)沒關係,交給 cereal **逐欄位**序列化,不要整 struct `memcpy`。

---

## 3. View:遍歷「擁有某組 component」的 entity

View 是 ECS 的查詢主力,輕量、可即時建立、不需快取。

```cpp
// 取得同時擁有 Position 與 Velocity 的所有 entity
auto view = registry.view<Position, Velocity>();

// 寫法 A:for-range + view.get
for (auto entity : view) {
    auto &pos = view.get<Position>(entity);
    auto &vel = view.get<Velocity>(entity);
    pos.x += vel.dx;
    pos.y += vel.dy;
}

// 寫法 B:each(callback,直接把 component refs 傳進來)
view.each([](Position &pos, Velocity &vel) {
    pos.x += vel.dx;
    pos.y += vel.dy;
});

// 寫法 C:連 entity 一起拿
view.each([](auto entity, Position &pos, Velocity &vel) { /* ... */ });
```

### 排除某些 component

```cpp
// 有 Position、但「沒有」Frozen 的 entity
auto moving = registry.view<Position>(entt::exclude<Frozen>);
```

### View vs Group

- **View**:零成本、隨建隨用,適合大多數場景。**先用 view 就好。**
- **Group**:對固定的 component 組合做記憶體最佳化(owning group 會重排儲存),遍歷更快但有設定成本與限制。**不要過早最佳化**——本專案 Phase 4 的 system 一律先用 view。

---

## 4. System:吃 `entt::registry&` 的自由函式

本專案不把 system 做成 class,而是自由函式,直接吃所屬 zone 的 registry:

```cpp
// projects/medp/src/gcore/systems/movement.h
inline void movement_system(entt::registry &registry, float dt) {
    registry.view<Position, Velocity>().each(
        [dt](Position &pos, Velocity &vel) {
            pos.x += vel.dx * dt;
            pos.y += vel.dy * dt;
        });
}
```

執行順序:`GlobalManager::tick` 對每個已載入的 zone 依序跑 system,先用一個有序清單,不過度設計。例如:

```cpp
void tick_zone(entt::registry &registry, float dt) {
    movement_system(registry, dt);
    // combat_system(registry);
    // economy_system(registry);
}
```

---

## 5. `entt::entity` 這個型別要懂

```cpp
entt::entity e = registry.create();

// entt::null:空 entity 常數,可比較
if (e == entt::null) { /* ... */ }

// 轉成底層整數(序列化、debug、傳給前端時用)
auto raw = entt::to_integral(e);                          // 取得整數值
using id_t = std::underlying_type_t<entt::entity>;        // 底層整數型別(通常 uint32_t)

// 從整數還原
entt::entity back = entt::entity{raw};
```

> entity 的整數內部**同時編碼 index 與 version**。直接把整個 raw 值存/讀即可(snapshot 會處理 version),不要自己拆位。

---

## 6. 訊號 / 觀察者(進階,選用)

registry 能在 component 生命週期事件上掛 callback:

```cpp
void on_pos_added(entt::registry &reg, entt::entity e) { /* ... */ }

registry.on_construct<Position>().connect<&on_pos_added>();  // 新增時
registry.on_update<Position>().connect<&...>();              // patch/replace 時
registry.on_destroy<Position>().connect<&...>();             // 移除時
```

用途:維護衍生索引、空間分割、通知前端等。**本專案初期用不到**,知道有這機制即可。

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
| 每個 zone 一個 `entt::registry`(`GlobalManager` 管理) | `entt::registry` 建立 / 銷毀、entity 生命週期 |
| `components/`(POD component) | `Position` / `Velocity` / `ZoneMeta` / `WorldConfig` 等 aggregate;entity 參照存 `entt::entity` |
| `systems/`(吃 `entt::registry&` 的自由函式) | view + `each`、`GlobalManager::tick` 有序清單跑 system |
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
