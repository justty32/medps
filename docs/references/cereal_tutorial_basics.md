# cereal 教學:基礎用法（medps 版）

> 本篇是 [cereal_tutorial.md](cereal_tutorial.md) 的子篇,對應原文第 0–2 節:為什麼用 cereal、最小序列化範例、`serialize`/`save`/`load` 的四種寫法。版本/環境資訊見母檔。

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

下一篇:[容器、版本控制與多型序列化](cereal_tutorial_containers_versioning.md)　|　回[目錄](cereal_tutorial.md)
