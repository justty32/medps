# 如何新增 Component 與 System

> 本文以「技能（Skill）系統」為例，完整走一遍新增一個 component 與對應 system 的流程。

---

## 新增 Component

### Step 1：在 `src/gcore/components/` 建 header

```cpp
// src/gcore/components/skill.h
#pragma once
#include <cstdint>
#include <vector>
#include <cereal/types/vector.hpp>

struct Skill {
    uint32_t skill_id{};
    float    cooldown{};      // 目前冷卻剩餘秒數
    float    max_cooldown{};

    template<class Archive>
    void serialize(Archive& ar) {
        ar(skill_id, cooldown, max_cooldown);
    }
};
```

規則：
- Component 盡量是 POD aggregate，不帶虛擬函式。
- STL 成員（`std::vector`、`std::string` 等）需對應引入 cereal type header（`cereal/types/vector.hpp` 等）。
- `entt::entity` 欄位要用 `save`/`load` 分拆處理（參考 `cross_zone_ref.h`），cereal 預設不序列化 enum。

---

### Step 2：在 `all_components.h` 登錄

```cpp
// src/gcore/serialize/all_components.h
#pragma once
#include <entt.hpp>
#include "../components/cross_zone_ref.h"
#include "../components/skill.h"          // <-- 加這行

using AllComponents = entt::type_list<
    CrossZoneRef,
    Skill          // <-- 加這行
>;
```

**這是唯一需要改的登錄點。** `zone_io::save` 與 `zone_io::load` 會自動展開 `AllComponents`，不需要其他地方改動。

> 注意：save 與 load 展開的順序必須一致（type_list 的宣告順序即是位元流順序）。新增永遠加在最後，**不要插在中間**，否則舊存檔讀取時資料會錯位。

---

## 新增 System

System 是吃 `entt::registry&`（或未來的 `World&`）的自由函式，不是 class。

### Step 1：在 `src/gcore/systems/` 建 header

```cpp
// src/gcore/systems/skill_system.h
#pragma once
#include <entt.hpp>
#include "../components/skill.h"

namespace systems {

inline void tick_cooldown(entt::registry& reg, float dt) {
    reg.view<Skill>().each([dt](Skill& sk) {
        if (sk.cooldown > 0.f)
            sk.cooldown -= dt;
        if (sk.cooldown < 0.f)
            sk.cooldown = 0.f;
    });
}

} // namespace systems
```

規則：
- System 一律是**自由函式**（或 namespace 內的函式），不做成 class。
- 若 system 需要跨 tick 的暫存狀態，改成有成員的 struct 放在呼叫端（`GlobalManager` 或未來的 `World`），不要放在 component 裡。
- 先用 `view`，不要過早用 `group`。

---

### Step 2：在 tick 裡呼叫

目前 `GlobalManager` 還沒有 tick 入口；未來 `World` 定義後會統一管理。
暫時在測試或上層邏輯中直接呼叫：

```cpp
#include <gcore/systems/skill_system.h>

// 每幀或每回合
systems::tick_cooldown(registry, delta_time);
```

---

## 完整流程一覽

```
新增 component:
  1. src/gcore/components/<name>.h      — 定義 struct + cereal serialize
  2. src/gcore/serialize/all_components.h — AllComponents 加一行（永遠加在最後）

新增 system:
  1. src/gcore/systems/<name>_system.h  — 自由函式，吃 registry& 或 World&
  2. 在 tick 裡按順序呼叫
```

---

## 新增測試

每個新 component 建議補一條 round-trip 驗證（參考 `test/src/main.cpp` 的結構）：

```cpp
static bool test_skill_roundtrip() {
    entt::registry src;
    auto e = src.create();
    src.emplace<Skill>(e, uint32_t{99}, 1.5f, 3.0f);

    std::stringstream ss;
    zone_io::save(src, ss);

    entt::registry dst;
    zone_io::load(dst, ss);

    auto view = dst.view<Skill>();
    CHECK("count", std::distance(view.begin(), view.end()) == 1);
    for (auto en : view) {
        auto& sk = view.get<Skill>(en);
        CHECK("skill_id",      sk.skill_id      == 99u);
        CHECK("cooldown",      sk.cooldown      == 1.5f);
        CHECK("max_cooldown",  sk.max_cooldown  == 3.0f);
    }
    return true;
}
```

---

## 參考

- EnTT 基礎用法：`others/entt_tutorial.md`
- cereal 序列化：`others/cereal_tutorial.md`
- component 型別清單：`src/gcore/serialize/all_components.h`
- 現有測試：`test/src/main.cpp`
