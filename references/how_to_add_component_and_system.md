# 如何新增 Component 與 System

> 本文走一遍在 medps 新增 component 與 per-zone system 的實際流程。
> 對應程式碼:`src/gcore/components/`、`src/gcore/systems/`、`src/gcore/serialize/all_components.h`、`src/gcore/global_manager.{h,cpp}`。

---

## 新增 Component

### Step 1：在 `src/gcore/components/` 建 header

```cpp
// src/gcore/components/skill.h
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
- 想存某個 zone 的識別,存它的 `ZoneKey`(uint64,可直接序列化),不要存裸 `entt::entity`——`entt::entity` 只在自己的 registry 內有效,跨 registry 沒有意義(範例:`components/zone_meta.h` 就是存 `ZoneKey self/parent`)。

---

### Step 2：在 `all_components.h` 登錄

```cpp
// src/gcore/serialize/all_components.h
#include "../components/skill.h"          // <-- 1. include

using AllComponents = entt::type_list<
    ZoneMeta,
    Position,
    Velocity,
    AreaTerrain,
    Blocking,
    WorldConfig,
    Skill          // <-- 2. 加在最後
>;
```

**這是唯一需要改的登錄點。** `zone_io::save` / `load` 自動展開 `AllComponents`,序列化不必動別處。

> 順序即位元流順序,save/load 共用這份清單。新增**永遠加在最後,不要插中間**,否則舊存檔讀取時資料會錯位。

---

## 新增 System

回顧:**system 就是一個查詢 component → 處理的普通函式**(見 `references/entt_tutorial.md` §4)。本專案的 system 簽名固定:

| 種類 | 簽名 | 跑在哪 |
|---|---|---|
| **per-zone** | `void(entt::registry&)` | `GlobalManager::tick()` 對每個 loaded zone 各跑一次 |

### Step 1：在 `src/gcore/systems/` 建 header

per-zone system 簽名固定是 `void(entt::registry&)`,才能註冊給 `GlobalManager`:

```cpp
// src/gcore/systems/skill_system.h
#pragma once
#include <entt.hpp>
#include "../components/skill.h"

namespace systems {

inline void tick_cooldown(entt::registry& reg) {
    reg.view<Skill>().each([](Skill& sk) {
        if (sk.cooldown > 0) --sk.cooldown;
    });
}

} // namespace systems
```

規則:
- per-zone system 一律 `void(entt::registry&)` 的自由函式,不做成 class。
- 若 system 需要跨 tick 的暫存狀態,放在呼叫端(`GlobalManager` 或其持有的物件),不要放進 component。
- 用 `view` 查詢 component。
- 遍歷 view 時若要建/刪 entity,先收集、迴圈外再做(見 `entt_tutorial.md` §9)。

---

### Step 2：註冊並 tick

```cpp
#include <gcore/global_manager.h>
#include <gcore/systems/skill_system.h>

GlobalManager gm;
gm.add_zone_system(systems::tick_cooldown);   // 註冊;tick() 依註冊順序跑

// 遊戲每回合:
gm.tick();   // 對每個「已載入」的 zone 跑所有註冊的 per-zone system
```

重點:
- **執行順序 = 註冊順序**。先 `add` 的先跑。
- `tick()` **只跑 loaded zones**;**root 不跑**(它放全局實體,不是地圖 actor)。
- 未載入的 zone 不在記憶體,不會被 tick;要它跑就先 `load`(離線追算之後再規劃)。

### 需要額外參數(如 dt)?用 lambda 綁進去

`tick()` 不傳參數給 system。若 system 需要 dt 等,註冊時用 lambda 捕捉:

```cpp
float dt = 0.016f;
gm.add_zone_system([dt](entt::registry& reg){
    systems::move_with_dt(reg, dt);
});
```

---

## 完整流程一覽

```
新增 component:
  1. src/gcore/components/<name>.h         — struct + cereal serialize
  2. src/gcore/serialize/all_components.h  — AllComponents 加一行(永遠加在最後)

新增 per-zone system:
  1. src/gcore/systems/<name>.h            — void(entt::registry&) 自由函式
  2. gm.add_zone_system(systems::<name>)   — 註冊(順序 = 執行順序)
     gm.tick()                             — 對每個 loaded zone 跑
```

---

## 新增測試(參考 `test/src/main.cpp`)

**component round-trip:**

```cpp
static bool test_skill_roundtrip() {
    entt::registry src;
    auto e = src.create();
    src.emplace<Skill>(e, uint32_t{99}, 3);

    std::stringstream ss;
    zone_io::save(src, ss);

    entt::registry dst;
    zone_io::load(dst, ss);

    bool ok = false;
    for (auto en : dst.view<Skill>()) {
        auto& sk = dst.get<Skill>(en);
        ok = (sk.skill_id == 99u && sk.cooldown == 3);
    }
    CHECK("skill survived", ok);
    return true;
}
```

**system 行為(透過 tick):**

```cpp
static bool test_cooldown_ticks_down() {
    GlobalManager gm;
    gm.add_zone_system(systems::tick_cooldown);

    auto& z = gm.create(make_zone_key(ZoneType{1}, 0, 0, 0), ZONE_ROOT);
    auto e = z.create();
    z.emplace<Skill>(e, uint32_t{1}, 2);

    gm.tick();
    CHECK("cooldown 2->1", z.get<Skill>(e).cooldown == 1);
    gm.tick();
    CHECK("cooldown 1->0", z.get<Skill>(e).cooldown == 0);
    return true;
}
```

---

## 參考

- EnTT 基礎(view / system / entity):`references/entt_tutorial.md`
- cereal 序列化:`references/cereal_tutorial.md`
- zone / registry / tick 架構全貌:`references/zone_streaming_architecture.md`
- 實際範例:`src/gcore/systems/movement.h`、`src/gcore/components/`
- component 型別清單:`src/gcore/serialize/all_components.h`
- 現有測試:`test/src/main.cpp`
