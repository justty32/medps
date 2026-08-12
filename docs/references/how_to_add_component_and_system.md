# 如何新增 Component 與 System

> 本文走一遍在 medps 新增 component 與 per-zone system 的實際流程。
> 對應程式碼:`projects/medp/src/gcore/world/components/`、`projects/medp/src/gcore/common/components/`、`projects/medp/src/gcore/world/systems/`、`projects/medp/src/gcore/serialize/all_components.h`、`projects/medp/src/gcore/zone/zone_manager.h`。

---

## 新增 Component

### Step 0:判斷放哪一族

Component 依歸屬分兩處(actor 兩大家族的完整說明見 `gcore/common/actor.h:12-23`):

| 目錄 | 放什麼 | 現有例子 |
|---|---|---|
| `gcore/world/components/` | 依賴地圖格的 World 專屬資料 | `Position`(tile 座標)、`Velocity`(每 tick 位移) |
| `gcore/common/components/` | 跨 zone 共用的身分層,不管 actor 住哪個 zone 都可能有 | `Name`、`Owner`、`Location`/`LocationKind`、`Unit`/`UnitKind` |

判斷準則:語意是否綁著「這個 zone 的地圖格」?綁著 → `world/components/`;只是屬性/身分 → `common/components/`。

### Step 1:建 header

新元件多半是身分/屬性類,放 `common/components/`:

```cpp
// projects/medp/src/gcore/common/components/skill.h
#pragma once
#include <cstdint>

struct Skill {
    uint32_t skill_id{};
    int      cooldown{};      // 剩餘冷卻回合數

    template<class Archive>
    void serialize(Archive& ar) { ar(skill_id, cooldown); }
};
```

規則:
- Component 盡量是 POD aggregate,不帶虛擬函式。
- STL 成員(`std::vector`、`std::string` 等)要對應引入 cereal type header(`cereal/types/vector.hpp` 等)。
- 跨 registry 引用一律存穩定 uint64 id,不要存裸 `entt::entity`——`entt::entity` 只在自己的 registry 內有效,跨 registry 沒有意義。範例:`Owner.faction`(`gcore/common/components/owner.h:9`)存 faction id、`Location.kind`(`gcore/common/components/location.h:24`)存 def id;zone 之間互指同理用 `ZoneManager::ZoneId`(`gcore/zone/zone_manager.h:23`)。

### Step 2:在 `all_components.h` 登錄

```cpp
// projects/medp/src/gcore/serialize/all_components.h
#include "../common/components/skill.h"    // <-- 1. include

using AllComponents = entt::type_list<
    Position,
    Velocity,
    Name,
    Owner,
    Location,
    Unit,
    LocationKind,
    UnitKind,
    Skill                                  // <-- 2. 加在最後
>;
```

**這是唯一需要改的登錄點,而且是鐵律:漏登記,存檔會靜默漏掉這個 component。** `zone_io::save` / `load` 自動展開 `AllComponents`,序列化不必動別處。

> 順序即位元流順序,save/load 共用這份清單。新增**永遠加在最後,不要插中間**,否則舊存檔讀取時資料會錯位。

---

## 新增 System

System 是查詢 component → 處理的自由函式(見 `docs/references/entt_tutorial_views_systems.md` §4)。本專案 system 簽名固定:

| 種類 | 簽章 | 跑在哪 |
|---|---|---|
| **per-zone** | `void(Zone&)` | `ZoneManager::tick()` 對每個 zone(**含 root**)各跑一次 |

簽章是 `Zone&` 而不是 `entt::registry&`——因為 system 可能需要地圖(`Zone::layers`)而不只是 entity(見 `gcore/zone/zone_manager.h:69-71` 的註解)。

### Step 1:在 `projects/medp/src/gcore/world/systems/` 建 header

不論該 system 處理的 component 放在 `world/` 還是 `common/`,system 本身一律放 `world/systems/`(它要吃 `Zone&`,天然屬於 World 這層):

```cpp
// projects/medp/src/gcore/world/systems/skill_system.h
#pragma once
#include <entt.hpp>
#include "../../common/components/skill.h"
#include "../../zone/zone.h"

namespace systems {

// 簽章即 ZoneSystem,可直接 zm.add_zone_system(systems::tick_cooldown);
inline void tick_cooldown(Zone& z) {
    z.reg.view<Skill>().each([](Skill& sk) {
        if (sk.cooldown > 0) --sk.cooldown;
    });
}

} // namespace systems
```

規則:
- per-zone system 一律 `void(Zone&)` 的自由函式,不做成 class。
- 用 `z.reg` 存取 registry;需要地圖時用 `z.layers`。
- 若 system 需要跨 tick 的暫存狀態,放在呼叫端(lambda capture,或 `ZoneManager` 外部持有的物件),不要放進 component。
- 用 `view` 查詢 component。
- 遍歷 view 時若要建/刪 entity,先收集、迴圈外再做(見 `entt_tutorial_views_systems.md` §3)。

### Step 2:註冊並 tick

```cpp
#include <gcore/zone/zone_manager.h>
#include <gcore/world/systems/skill_system.h>

ZoneManager zm{"save_dir"};
zm.add_zone_system(systems::tick_cooldown);   // 註冊;tick() 依註冊順序跑

// 遊戲每回合:
zm.tick();   // 對每個 zone(含 root)跑所有已註冊的 per-zone system
```

重點:
- **執行順序 = 註冊順序**。先 `add_zone_system` 的先跑。
- `tick()` 對**每個記憶體中的 zone 都跑,含 root**(root 放非地圖的全局實體:陣營/神祇/具名角色,見 `gcore/zone/zone.h:10-11`;也是 def 的家,見 `gcore/common/actor.h:20-23`)。
- **tick 內禁止 zone 結構性變更**(`create_child`/`load`/`unload`/`destroy`):`tick()` 正在迭代內部的 zone 表,改動它是迭代器 UB(見 `gcore/zone/zone_manager.h:79-81`)。未載入的 zone 不在記憶體,不會被 tick;要它跑就先 `load`。

### 需要額外參數(如 dt)?用 lambda 綁進去

`tick()` 不傳參數給 system。若 system 需要 dt 等,註冊時用 lambda 捕捉:

```cpp
float dt = 0.016f;
zm.add_zone_system([dt](Zone& z){
    systems::move_with_dt(z, dt);
});
```

---

## 完整流程一覽

```
新增 component:
  1. gcore/world/components/<name>.h 或 gcore/common/components/<name>.h — 依 Step 0 判斷該放哪族
  2. gcore/serialize/all_components.h — AllComponents 加一行(永遠加在最後)

新增 per-zone system:
  1. gcore/world/systems/<name>.h    — void(Zone&) 自由函式
  2. zm.add_zone_system(systems::<name>)   — 註冊(順序 = 執行順序)
     zm.tick()                             — 對每個 zone(含 root)跑
```

---

## 新增測試(參考 `projects/tests/src/main.cpp`)

**component round-trip(比照 `projects/tests/src/main.cpp:105` 的 `test_zone_io_roundtrip`):**

```cpp
static bool test_skill_roundtrip() {
    auto dir = fresh_dir("skill_roundtrip");
    ZoneManager zm{dir};
    auto& z = zm.create_child(ZONE_ROOT);
    auto e = z.reg.create();
    z.reg.emplace<Skill>(e, uint32_t{99}, 3);

    std::stringstream ss;
    zone_io::save(z, ss);
    auto loaded = zone_io::load(ss);   // 回傳 unique_ptr<Zone>

    bool ok = false;
    for (auto en : loaded->reg.view<Skill>()) {
        auto& sk = loaded->reg.get<Skill>(en);
        ok = (sk.skill_id == 99u && sk.cooldown == 3);
    }
    CHECK("skill survived", ok);
    fs::remove_all(dir);
    return true;
}
```

**system 行為,透過 tick(比照 `projects/tests/src/main.cpp:373` 的 `test_tick_all_zones`):**

```cpp
static bool test_cooldown_ticks_down() {
    auto dir = fresh_dir("cooldown_tick");
    ZoneManager zm{dir};
    zm.add_zone_system(systems::tick_cooldown);

    auto& z = zm.create_child(ZONE_ROOT);
    auto e = z.reg.create();
    z.reg.emplace<Skill>(e, uint32_t{1}, 2);

    zm.tick();
    CHECK("cooldown 2->1", z.reg.get<Skill>(e).cooldown == 1);
    zm.tick();
    CHECK("cooldown 1->0", z.reg.get<Skill>(e).cooldown == 0);
    fs::remove_all(dir);
    return true;
}
```

現行測試基準是 **21 項全綠**(見 `workflows/testing.md`)。

---

## 參考

- EnTT 基礎(view / system / entity):`docs/references/entt_tutorial_views_systems.md`
- cereal 序列化:`docs/references/cereal_tutorial.md`
- Zone / ZoneManager 架構原始碼(有完整註解,比任何教學都準):`gcore/zone/zone.h`、`gcore/zone/zone_manager.h`
- 實際範例:`projects/medp/src/gcore/world/systems/movement.h`、`projects/medp/src/gcore/world/components/`、`projects/medp/src/gcore/common/components/`
- component 型別清單:`projects/medp/src/gcore/serialize/all_components.h`
- 現有測試:`projects/tests/src/main.cpp`
