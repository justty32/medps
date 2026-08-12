# cereal 教學（medps 版）

> 對應版本:**cereal v1.3.2**(已 vendor 至 `projects/medp/include/cereal/`)。
> 環境:C++20 / MSVC。本文記錄本專案以 cereal 取代 BinFSR 的用法,核心世界結構設計見 `docs/work/design/zone_layers.md`。
> 引入方式:`#include <cereal/archives/portable_binary.hpp>` 等;**不需要編譯任何 .cpp**,cereal 全 header-only。

---

## 目錄

本教學依主題拆成三個子檔:

1. [基礎用法](cereal_tutorial_basics.md) — 為什麼用 cereal 取代 BinFSR、最小序列化範例(存/讀 POD struct)、`serialize`/`save`/`load` 的四種寫法(侵入式/非侵入式)。
2. [容器、版本控制與多型序列化](cereal_tutorial_containers_versioning.md) — STL 容器(vector/string/map/…)序列化、`CEREAL_CLASS_VERSION` 版本控制與向前相容、多型序列化機制(本專案不用,純備忘)。
3. [本專案實務:EnTT 整合、格式比較、tdarray、陷阱、smoke test](cereal_tutorial_practice.md) — 搭配 EnTT snapshot 的完整存讀流程、四種 archive 格式比較、`tdarray<T>` 序列化、常見陷阱、Phase 0 smoke test 範本。

---

## 參考

- cereal 官方文件:https://uscilab.github.io/cereal/
- 本專案 EnTT 搭配:[entt_tutorial.md](entt_tutorial.md)
- 核心世界結構設計:`docs/work/design/zone_layers.md`
