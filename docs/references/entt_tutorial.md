# EnTT 教學（medps 版）

> 對應版本:**EnTT v3.16.0**(已 pin,single header `projects/medp/include/entt.hpp`)。
> 環境:C++20 / MSVC。範例直接以本專案的用法為主,核心世界結構設計見 `docs/work/design/zone_layers.md`。
> 引入方式:`#include <entt.hpp>`(single-include amalgamation;不要混用 `extern/entt/src/`,以免版本不一致)。

---

## 目錄

本教學依主題拆成三個子檔:

1. [registry 與 Component](entt_tutorial_basics.md) — 為什麼用 ECS、`entt::registry` 的基本操作(create / emplace / get / try_get / replace / patch / remove / destroy)、component 該怎麼設計(POD aggregate、entity 參照用 `entt::entity`)。
2. [View / System / entity 型別 / 訊號](entt_tutorial_views_systems.md) — view 的三種遍歷寫法與排除語法、view vs group 的取捨、本專案 system 的自由函式慣例、`entt::entity` 底層整數細節、訊號/觀察者機制。
3. [序列化:snapshot + cereal](entt_tutorial_serialization.md) — EnTT snapshot / snapshot_loader API(v3.16.0 的 `.get<T>(ar)`)、cereal archive adapter 寫法、`AllComponents` 單一來源清單、與本專案模組的對應表、常見陷阱。

---

## 參考

- EnTT 官方文件(對應 3.x):https://github.com/skypjack/entt/wiki
- 本專案序列化搭配:[cereal_tutorial.md](cereal_tutorial.md)
- 核心世界結構設計:`docs/work/design/zone_layers.md`
