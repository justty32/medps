# cereal 教學:本專案實務——EnTT 整合、格式比較、tdarray、陷阱、smoke test（medps 版）

> 本篇是 [cereal_tutorial.md](cereal_tutorial.md) 的子篇,對應原文第 6–10 節。
> 上一篇:[容器、版本控制與多型序列化](cereal_tutorial_containers_versioning.md)

---

## 6. 搭配 EnTT snapshot（本專案核心用途）

cereal 作為 EnTT snapshot 的位元格式層,透過 [entt_tutorial_serialization.md](entt_tutorial_serialization.md) §7.2 的 adapter 串接。
完整流程(以 `std::stringstream` 為例):

```cpp
#include <sstream>
#include <entt.hpp>
#include <cereal/archives/portable_binary.hpp>
#include "projects/medp/src/gcore/serialize/entt_cereal_archive.h"
#include "projects/medp/src/gcore/serialize/all_components.h"  // AllComponents type_list

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

地圖等 grid 資料以 `tdarray<T>`(`projects/medp/src/gcore/util/tdarray.hpp`)承載,不進 registry,直接用 cereal 序列化:

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
// projects/tests/src/smoke_cereal.cpp
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
- 本專案 EnTT 搭配:[entt_tutorial.md](entt_tutorial.md)（§7 snapshot + adapter,見 [entt_tutorial_serialization.md](entt_tutorial_serialization.md)）
- 核心世界結構設計:`docs/work/design/zone_layers.md`

---

回[目錄](cereal_tutorial.md)
