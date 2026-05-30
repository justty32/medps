# cereal 教學（medps 版）

> 對應版本:**cereal v1.3.2**(已 vendor 至 `include/cereal/`)。
> 環境:C++20 / MSVC。本文記錄本專案以 cereal 取代 BinFSR 的用法,核心世界結構設計見 `work/design/zone_layers.md`。
> 引入方式:`#include <cereal/archives/portable_binary.hpp>` 等;**不需要編譯任何 .cpp**,cereal 全 header-only。

---

## 0. 為什麼用 cereal / 為什麼取代 BinFSR

cereal 相對於本專案自行實作的 `BinFSR`:

| 面向 | BinFSR | cereal |
|---|---|---|
| 跨平台位元序 | 未處理(host byte order) | `PortableBinaryArchive` 固定 little-endian |
| STL 容器支援 | 需手動處理 | 一行 `#include <cereal/types/vector.hpp>` |
| 多型序列化 | 手動 type-id 工廠 | `CEREAL_REGISTER_TYPE` |
| 版本控制 | 無 | `CEREAL_CLASS_VERSION` + `version` 參數 |
| 格式 | 二進位(自訂) | Binary / PortableBinary / JSON / XML 可切 |

本專案決策:全面改用 **`PortableBinaryArchive`** 作為存檔格式;`BinFSR` 移除。

---

## 1. 最小範例:序列化 POD struct

```cpp
#include <sstream>
#include <cereal/archives/portable_binary.hpp>

struct Position {
    float x{};
    float y{};

    // cereal 要求這個 serialize 函式(或分拆 save/load,見第 3 節)
    template<class Archive>
    void serialize(Archive &ar) {
        ar(x, y);   // 也可寫 ar(CEREAL_NVP(x), CEREAL_NVP(y))
    }
};

// --- 存 ---
std::ostringstream oss;
{
    cereal::PortableBinaryOutputArchive out{oss};
    Position p{1.0f, 2.0f};
    out(p);     // 等同 out << p 或 out.operator()(p)
}

// --- 讀 ---
std::istringstream iss{oss.str()};
{
    cereal::PortableBinaryInputArchive in{iss};
    Position loaded{};
    in(loaded);
    // loaded.x == 1.0f, loaded.y == 2.0f
}
```

> `ar(a, b, c)` 等於依序呼叫 `ar(a); ar(b); ar(c)`,順序就是位元流的順序,存讀必須一致。

---

## 2. 序列化函式的四種寫法

### 2-A. 侵入式 `serialize`（存讀共用,最常用）

```cpp
struct Velocity {
    float dx{}, dy{};
    template<class Archive>
    void serialize(Archive &ar) { ar(dx, dy); }
};
```

### 2-B. 侵入式 `save` / `load`（存讀分開,用於唯讀欄位或需要不同邏輯時）

```cpp
struct TileData {
    int type{};
    int elevation{};
    mutable int cached_cost{};  // 不序列化

    template<class Archive>
    void save(Archive &ar) const { ar(type, elevation); }

    template<class Archive>
    void load(Archive &ar) { ar(type, elevation); }
};
```

### 2-C. 非侵入式自由函式（無法修改的 struct）

```cpp
// 在 struct 所在的 namespace 或 cereal namespace 裡定義
template<class Archive>
void serialize(Archive &ar, Position &p) {
    ar(p.x, p.y);
}
```

### 2-D. 非侵入式 `save` / `load`（自由函式分拆版）

```cpp
template<class Archive>
void save(Archive &ar, const Position &p) { ar(p.x, p.y); }

template<class Archive>
void load(Archive &ar, Position &p) { ar(p.x, p.y); }
```

> 本專案 POD aggregate 一律用 **2-A 侵入式 serialize**;只有確實需要分開邏輯才用 2-B。

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
全部清單在 `include/cereal/types/`。

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

## 6. 搭配 EnTT snapshot（本專案核心用途）

cereal 作為 EnTT snapshot 的位元格式層,透過 `others/entt_tutorial.md` §7.2 的 adapter 串接。
完整流程(以 `std::stringstream` 為例):

```cpp
#include <sstream>
#include <entt.hpp>
#include <cereal/archives/portable_binary.hpp>
#include "src/gcore/serialize/entt_cereal_archive.h"
#include "src/gcore/serialize/all_components.h"  // AllComponents type_list

// ---- 存（單一 zone 的 registry）----
void save_zone(const entt::registry &registry, std::ostream &os) {
    cereal::PortableBinaryOutputArchive cereal_out{os};
    output_archive out{cereal_out};

    entt::snapshot{registry}
        .get<entt::entity>(out)
        .get<Position>(out)
        .get<Velocity>(out)
        .get<Owner>(out);
    // 實際上用 AllComponents 清單展開,避免手動列出(見 serialize/zone_io.h)
}

// ---- 讀 ----
void load_zone(entt::registry &registry, std::istream &is) {
    cereal::PortableBinaryInputArchive cereal_in{is};
    input_archive in{cereal_in};

    entt::snapshot_loader{registry}
        .get<entt::entity>(in)
        .get<Position>(in)
        .get<Velocity>(in)
        .get<Owner>(in)
        .orphans();
}
```

注意:EnTT snapshot 與 cereal archive 是**同一個 stream**——一個 zone 的存檔是一個連續的位元流,先是 entity 表、再依序是各 component 的資料塊。讀取順序必須與寫入完全一致。

---

## 7. 四種 archive 格式比較

| Archive | header | 特性 | 本專案用途 |
|---|---|---|---|
| `PortableBinaryOutputArchive` | `archives/portable_binary.hpp` | 固定 little-endian,緊湊,不可讀 | **存檔(主要)** |
| `BinaryOutputArchive` | `archives/binary.hpp` | host byte order,略快 | 不跨平台,不用 |
| `JSONOutputArchive` | `archives/json.hpp` | 人類可讀,方便 debug | debug / 設計工具 |
| `XMLOutputArchive` | `archives/xml.hpp` | XML | 不需要 |

**切格式只需換 archive 型別**,`serialize` 函式完全不改——這是 cereal 最大的優點之一。

---

## 8. `tdarray<T>` 的序列化

地圖等 grid 資料以 `tdarray<T>`(`src/gcore/util/tdarray.hpp`)承載,不進 registry,直接用 cereal 序列化:

```cpp
template<typename T>
struct tdarray {
    int sx{}, sy{};
    std::vector<T> vec;

    template<class Archive>
    void serialize(Archive &ar) {
        ar(sx, sy, vec);   // vec 需 T 有 serialize;若 T 是 POD,再加 cereal/types/vector.hpp
    }
};
CEREAL_CLASS_VERSION(tdarray<Tile>, 1)
```

> entity 由 `entt::registry` 經 snapshot 序列化(見第 6 節);grid 這類非 entity 的世界資料則直接交給 cereal。

---

## 9. 常見陷阱

- **stream 壽命**:archive 物件**析構時**才 flush;存檔務必讓 archive 先析構(用大括號限制 scope),再關/讀 stream。
- **順序即格式**:`ar(a, b)` 的順序就是位元流的順序;存讀不一致會靜默讀到錯誤值(不報錯)。
- **`CEREAL_CLASS_VERSION` 位置**:必須在全域 namespace;寫在 class 內或 anonymous namespace 編譯不過。
- **enum 欄位**:cereal 不序列化 enum。要嘛 `static_cast<underlying_type>` 後存整數、讀回再轉,要嘛用 `cereal/types/common.hpp`(只支援 C++14 enum,不保證所有編譯器)。`entt::entity` 也是 enum,本專案在 adapter 裡顯式轉整數處理。
- **模板連結**:`CEREAL_REGISTER_TYPE` 要確保對應的 .cpp 被連結進來;放 header-only 裡可能因為 ODR 有問題,建議放 .cpp。
- **cereal 不支援裸指標存**(`T*`),只支援 `std::shared_ptr` / `std::unique_ptr` 多型;本專案 component 內部不存裸指標,改存 `entt::entity`,因此不踩這個坑。

---

## 10. smoke test 範本（Phase 0 驗收用）

```cpp
// test/src/smoke_cereal.cpp
#include <sstream>
#include <cassert>
#include <entt.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <cereal/types/string.hpp>

struct Position {
    float x{}, y{};
    template<class Archive> void serialize(Archive &ar) { ar(x, y); }
};

int main() {
    // cereal round-trip
    std::string buf;
    {
        std::ostringstream oss;
        cereal::PortableBinaryOutputArchive out{oss};
        int v = 42;
        std::string s = "hello";
        out(v, s);
        buf = oss.str();
    }
    {
        std::istringstream iss{buf};
        cereal::PortableBinaryInputArchive in{iss};
        int v{}; std::string s{};
        in(v, s);
        assert(v == 42);
        assert(s == "hello");
    }

    // EnTT + cereal round-trip(簡化版,不含 adapter)
    entt::registry reg;
    auto e = reg.create();
    reg.emplace<Position>(e, 3.0f, 4.0f);
    assert(reg.get<Position>(e).x == 3.0f);

    return 0;
}
```

---

## 參考

- cereal 官方文件:https://uscilab.github.io/cereal/
- 本專案 EnTT 搭配:`others/entt_tutorial.md`（§7 snapshot + adapter）
- 核心世界結構設計:`work/design/zone_layers.md`
