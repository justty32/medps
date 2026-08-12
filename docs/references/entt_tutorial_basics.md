# EnTT 教學:registry 與 Component（medps 版）

> 本篇是 [entt_tutorial.md](entt_tutorial.md) 的子篇,對應原文第 0–2 節:為什麼用 ECS、`entt::registry` 基本操作、component 設計原則。版本/環境資訊見母檔。

---

## 0. 為什麼用 EnTT / 什麼是 ECS

ECS(Entity-Component-System)把「資料」與「行為」徹底分離:

- **Entity(實體)**:只是一個 ID,本身不帶資料。在 EnTT 裡型別是 `entt::entity`(一個 enum,內含 index + version)。
- **Component(元件)**:純資料(理想上是 POD aggregate)。一個 entity 可掛任意多種 component。
- **System(系統)**:自由函式,查詢「擁有某組 component 的所有 entity」並處理之。

本專案的取捨:
- **只有動態行動者**(unit / city / hero / faction)是 entity。
- **地圖維持 grid**(`tdarray<Tile>`),不進 registry。
- entity 全由 `entt::registry` 管理;全局 / 單例狀態放 **root zone(`ZONE_ROOT`,由 `ZoneManager` 管理)**——不再有 `Obj` / `Scene`。
- System 一律寫成吃 `Zone&` 的自由函式(細節見 [View / System / entity 型別 / 訊號](entt_tutorial_views_systems.md) §4)。

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

// 2) patch:就地改,並觸發 on_update 訊號(見 entt_tutorial_views_systems.md 第 6 節)
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
// projects/medp/src/gcore/world/components/position.h
struct Position {
    int x{};
    int y{};
    int z{};   // 所在 layer(地面=0,往下為負);見 Zone::layers
};

struct Velocity {
    int dx{};
    int dy{};
};

// entity 之間的參照:存「另一個 entity 的 id」,不要存裸指標
struct Target {
    entt::entity value{entt::null};   // entt::null 是「空 entity」常數
};
```

> ⚠ 這是**通用 EnTT 示意，不是本專案的做法**。`entt::entity` 只在自己那個 registry 內有效,而 medps 是**一個 zone 一個 registry**,所以本專案真正的跨實體參照一律存**穩定 `uint64_t` id**——見 [`common/components/owner.h:9`](../../projects/medp/src/gcore/common/components/owner.h:9) 的 `Owner.faction`、[`location.h`](../../projects/medp/src/gcore/common/components/location.h) 的 `Location.kind`。只有「同一個 registry 內」的參照才適合用 `entt::entity`。

規則:
- 盡量是 POD aggregate;不要塞虛擬函式 / 複雜建構式。
- **entity 參照存 `entt::entity`**(序列化時當整數處理,見 entt_tutorial_serialization.md),不要存裸指標(如 `Position*`)。
- 含 STL 成員(`std::vector` 等)沒關係,交給 cereal **逐欄位**序列化,不要整 struct `memcpy`。

---

下一篇:[查詢與系統:View / System / entity 型別 / 訊號](entt_tutorial_views_systems.md)　|　回[目錄](entt_tutorial.md)
