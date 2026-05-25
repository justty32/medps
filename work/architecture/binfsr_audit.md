# BinFSR 序列化審查報告

調查日期:2026-05-25。對象:`src/gcore/util/bin_fwr.hpp` 及其使用者(tdarray / obj / scene / component 的 Save/Load)。
動機:即將以「EnTT snapshot + BinFSR archive adapter」做 ECS 存檔(會重用 BinFSR),且未來經 Godot 4 GDExtension 上多平台(Linux/macOS/Android-ARM/Web),跨平台可攜性是真議題。

## 一、缺陷與風險清單

**[高] 泛型 `read/write(T*)` 對非 trivially-copyable 型別是 UB**
`bin_fwr.hpp:13-20` 直接 `memcpy(sizeof(T))`。`T` 含指標/堆配置(`std::string`、巢狀容器)時寫出的是位址,讀回 dangling、析構 double-free。`std::string` 沒有特化,序列化種族名/神祇名等幾乎必踩。

**[高] 整個 POD struct 直接 memcpy(padding/alignment 不可攜)**
`bin_fwr.hpp:15,19`。padding bytes 未定義、成員 offset 隨 ABI(MSVC vs Clang-Linux/ARM)而異。x64-MSVC 寫的存檔在 Android-ARM 讀會錯位。直接影響「整個 EnTT component struct 直接寫」的規劃。`world.cpp:5-12` 的 Tile 逐欄位寫才是正確範式。

**[高] `size_t` 被當序列化寬度(word size 不可攜)**
容器長度、`tdarray::sx/sy` 都用 `size_t`(`bin_fwr.hpp:24,32,40,47,55,66,76,86`、`tdarray.hpp:41-42`、`scene.cpp:7`、`component.cpp:16`)。x64 寫 8 bytes、32-bit/wasm32 讀 4 bytes,整檔偏移。應固定 `uint64_t`/`uint32_t`。

**[高] 讀取端零界限檢查 — 惡意/截斷存檔**
`bin_fwr.hpp:26` `vec->resize(size)` 全信檔案 size → 巨量配置 OOM/DoS;`fs.read` 失敗後不檢查 `good()`/`gcount()`,靜默損壞。

**[高] `Scene::Load` 對未知 tid 用 `operator[]`**
`scene.cpp:38` `_g_default_constructors[tid]()`:未知 tid(跨版本)會插入空 `std::function` 並呼叫 → crash。應先 `find`。

**[中] 無 version header / magic number**:格式無版本標記,schema 演進後舊存檔靜默誤讀。
**[中] Endianness 假設 little-endian**:整數/float 以原生 byte order 寫;x86/ARM 多為 LE 風險低,嚴謹跨平台需正規化。
**[中] enum/bool 隱性寬度**:`sizeof(T)` 寫出跨編譯器有風險,建議顯式轉固定寬度。
**[低] `array` 特化讀寫不對稱**:`bin_fwr.hpp:42` 讀截斷到 S 但未消耗多餘 bytes → 後續錯位;迴圈 `int i` vs `size_t` signed/unsigned 比較。
**[低] `tdarray::Load` 重複 resize**:`tdarray.hpp:48-49`。

## 二、已經 OK 的部分
- 指標序列化策略正確:Obj 存 id(`obj.cpp:7`)、Scene 存 type-id 多型重建(`scene.cpp:15-17`)、`tid<0` 表 null slot。
- `world.cpp:5-12` 逐欄位寫 Tile 是跨平台正確範式。
- map/unordered_map 特化結構正確(問題只在元素型別與 size 寬度)。
- stringstream 緩衝目前可接受。

## 三、修改方向(含工程量)
1. **size 一律 `uint64_t`、內建型別走固定寬度** — 改 `bin_fwr.hpp` 約 8 處 + 呼叫端。小、收益高,**優先**。
2. **concept 約束泛型路徑** — `write(T*)` 僅接受 `is_trivially_copyable_v<T> && has_unique_object_representations_v<T>`;非 trivial 強制特化;補 `std::string` 特化。小至中。
3. **EnTT component 逐欄位序列化**(不整 struct memcpy)— 中,但是跨平台與 schema 演進的根本解。
4. **讀取端健壯化** — read 後檢查 `good()`、size 設上限、配置前校驗剩餘 bytes。小、防 DoS。
5. **magic + version header;Load 改用 `find`**(`scene.cpp:38`)— 小至中。
6. **endianness 正規化** — `write_le/read_le` byte-swap helper。中,可延後到實際上 BE/wasm 前。

建議優先序:1 → 4 → 2 → 5,再視平台時程處理 3、6。
