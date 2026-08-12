# EnTT 教學:View / System / entity 型別 / 訊號（medps 版）

> 本篇是 [entt_tutorial.md](entt_tutorial.md) 的子篇,對應原文第 3–6 節:View 查詢、System 寫法、`entt::entity` 型別細節、訊號/觀察者機制。
> 上一篇:[registry 與 Component](entt_tutorial_basics.md)

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

## 4. System:吃 `Zone&` 的自由函式

本專案不把 system 做成 class,而是自由函式。**簽章固定是 `void(Zone&)`,不是 `void(entt::registry&)`**——因為 system 可能需要地圖(`Zone::layers`),不能只吃 registry(見 `gcore/zone/zone_manager.h:69-71` 的 `ZoneSystem` 型別注解)。

```cpp
// projects/medp/src/gcore/world/systems/movement.h
inline void move_by(Zone& z, entt::entity e, int dx, int dy) {
    auto& p = z.reg.get<Position>(e);
    p.x += dx;
    p.y += dy;
}

inline void movement(Zone& z) {
    z.reg.view<Position, Velocity>().each([&](entt::entity e, Position&, Velocity& v) {
        move_by(z, e, v.dx, v.dy);
    });
}
```

執行順序:`ZoneManager::tick()` 對每個 zone(**含 root**)依註冊順序跑所有已註冊的 system。同簽章的自由函式可直接註冊:

```cpp
ZoneManager zm{dir};
zm.add_zone_system(systems::movement);   // 註冊;順序 = 執行順序
zm.tick();                               // 對每個 zone 都跑一次
```

若 system 需要額外參數(如 dt),`tick()` 本身不傳參數,註冊時用 lambda 捕捉即可(完整流程見 `docs/references/how_to_add_component_and_system.md`)。

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

下一篇:[序列化:snapshot + cereal](entt_tutorial_serialization.md)　|　回[目錄](entt_tutorial.md)
