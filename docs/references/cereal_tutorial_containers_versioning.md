# cereal 教學:容器、版本控制與多型序列化（medps 版）

> 本篇是 [cereal_tutorial.md](cereal_tutorial.md) 的子篇,對應原文第 3–5 節:STL 容器序列化、`CEREAL_CLASS_VERSION` 版本控制、多型序列化機制。
> 上一篇:[基礎用法](cereal_tutorial_basics.md)

---

## 3. 序列化 STL 容器

cereal 提供各容器的 header,引入後即支援:

```cpp
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/memory.hpp>   // unique_ptr, shared_ptr

struct FactionData {
    std::string name;
    std::vector<int> territory_ids;
    std::map<int, float> relations;

    template<class Archive>
    void serialize(Archive &ar) {
        ar(name, territory_ids, relations);
    }
};
```

其他常用的:`array.hpp`, `tuple.hpp`, `optional.hpp`, `variant.hpp`。
全部清單在 `projects/medp/include/cereal/types/`。

---

## 4. 版本控制：`CEREAL_CLASS_VERSION`


```cpp
#include <cereal/types/vector.hpp>

struct UnitData {
    int hp{};
    int atk{};
    std::vector<int> skills;  // v2 新增

    template<class Archive>
    void serialize(Archive &ar, const std::uint32_t version) {
        ar(hp, atk);
        if (version >= 2) ar(skills);
    }
};
CEREAL_CLASS_VERSION(UnitData, 2)
```

- 讀舊存檔(version=1)時,`skills` 欄位不讀,保持預設值——**向前相容**。
- **本專案慣例**:每個會進存檔的 class 都加 `CEREAL_CLASS_VERSION`,初始版本 1;之後擴欄位才升版本。

> `CEREAL_CLASS_VERSION` 要寫在**全域 namespace**,不能在類別內或函式內。

---

## 5. 多型序列化（cereal 的一般機制）

> **本專案不用這個**:entity 全由 `entt::registry` 管理、entity 參照存 `entt::entity`,component 內不存多型基底指標,因此這節純屬 cereal 機制備忘。

若要直接存/讀某個多型基底指標,cereal 提供多型支援。以一個中性的 `Shape` 基底為例:

```cpp
// shape.h（基底）
class Shape {
public:
    virtual ~Shape() = default;
    template<class Archive>
    void serialize(Archive &ar) { ar(id); }
private:
    int id{};
};

// circle.h（子類）
#include <cereal/types/polymorphic.hpp>
class Circle : public Shape {
public:
    template<class Archive>
    void serialize(Archive &ar) {
        ar(cereal::base_class<Shape>(this));  // 先序列化父類
        ar(radius);
    }
private:
    float radius{};
};

// 登錄(通常在 .cpp 裡,確保被連結)
CEREAL_REGISTER_TYPE(Circle)
// 若 Circle 是透過 Shape* 被存取,還需指定關係:
CEREAL_REGISTER_POLYMORPHIC_RELATION(Shape, Circle)

// 使用時:
std::shared_ptr<Shape> shape = std::make_shared<Circle>();
out(shape);   // 自動存型別資訊 + Circle 的資料
in(shape);    // 自動用 Circle 建構
```

> 若哪天確實要用,需引入 `cereal/types/polymorphic.hpp` 及 `cereal/types/memory.hpp`。

---

下一篇:[本專案實務:EnTT 整合、格式比較、tdarray、陷阱、smoke test](cereal_tutorial_practice.md)　|　回[目錄](cereal_tutorial.md)
