#!/usr/bin/env python3
"""medps 程式碼導覽 HTML 生成器。

用法：在本目錄執行 `python3 build.py`。
真相層是原始碼與 CODE_TOUR.md／CODE_MAP.md；本腳本讀取「當下的」原始碼，
連同各站導讀文字一起生成靜態 HTML。程式碼改了 → 重跑本腳本即同步。
各站導讀文字若要改，先改 CODE_TOUR.md，再同步本檔的 STATIONS。
純 Python 標準庫，無外部依賴；語法上色在生成期完成，頁面不含 JavaScript。
"""
import html
import re
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]          # html → code-map → common → workflows → repo 根目錄
REL_TO_ROOT = "../../../.."     # 頁面連回 repo 根目錄的相對前綴

# ---------------------------------------------------------------------------
# 語法上色（生成期、regex tokenizer；只求可讀，不求完備）
# ---------------------------------------------------------------------------

KEYWORDS = {
    "alignas", "alignof", "auto", "bool", "break", "case", "catch", "char",
    "class", "concept", "const", "consteval", "constexpr", "constinit",
    "const_cast", "continue", "decltype", "default", "delete", "do", "double",
    "dynamic_cast", "else", "enum", "explicit", "export", "extern", "false",
    "final", "float", "for", "friend", "goto", "if", "inline", "int", "long",
    "mutable", "namespace", "new", "noexcept", "nullptr", "operator",
    "override", "private", "protected", "public", "register",
    "reinterpret_cast", "requires", "return", "short", "signed", "sizeof",
    "static", "static_assert", "static_cast", "struct", "switch", "template",
    "this", "throw", "true", "try", "typedef", "typeid", "typename", "union",
    "unsigned", "using", "virtual", "void", "volatile", "wchar_t", "while",
    "int16_t", "uint16_t", "int8_t", "uint8_t", "int32_t", "uint32_t",
    "int64_t", "uint64_t", "size_t",
}

TOKEN_RE = re.compile(
    r"""(?P<comment>//[^\n]*|/\*.*?\*/)
      | (?P<string>"(?:\\.|[^"\\\n])*"|'(?:\\.|[^'\\\n])*')
      | (?P<prep>\#\s*\w+)
      | (?P<number>\b(?:0[xX][0-9a-fA-F']+|\d[\d']*(?:\.\d+)?)(?:[uUlLfF]*)\b)
      | (?P<word>[A-Za-z_]\w*)
    """,
    re.VERBOSE | re.DOTALL,
)


def _wrap(cls: str, text: str) -> str:
    """逐行包 span，讓跨行的 block comment 不會產生跨行標籤。"""
    parts = html.escape(text).split("\n")
    return "\n".join(f'<span class="{cls}">{p}</span>' if p else p for p in parts)


def highlight(src: str) -> str:
    out, pos = [], 0
    for m in TOKEN_RE.finditer(src):
        out.append(html.escape(src[pos:m.start()]))
        kind = m.lastgroup
        text = m.group()
        if kind == "word":
            out.append(_wrap("kw", text) if text in KEYWORDS else html.escape(text))
        else:
            out.append(_wrap({"comment": "cm", "string": "st",
                              "prep": "pp", "number": "nu"}[kind], text))
        pos = m.end()
    out.append(html.escape(src[pos:]))
    return "".join(out)


# ---------------------------------------------------------------------------
# 導覽內容定義（鏡像 CODE_TOUR.md；行號錨點在生成期由符號字串定位，不會漂移）
# ---------------------------------------------------------------------------
# 每站：slug、title、tagline（索引卡片一句話）、intro（段落 list）、
# files（repo 相對路徑）、points（看什麼：(說明, 檔案索引, 定位字串或 None)）、
# quiz（讀完該能回答）。

STATIONS = [
    {
        "slug": "01-util",
        "title": "第 1 站 util/ — 容器與巨集工具",
        "tagline": "tdarray：唯一的容器工具，row-major 2D 陣列；mydef：metaprogramming 巨集。",
        "files": ["projects/medp/src/gcore/util/tdarray.hpp", "projects/medp/src/gcore/util/mydef.h"],
        "intro": [
            "<code>tdarray&lt;T&gt;</code> 用單一 vector 打平存 2D 格網"
            "（索引 = x*sy + y），已 cereal 化，是 <code>Zone::layers</code> 的底層容器。",
            "使用前必知三個慣例（檔頭註解也有）：回傳 bool 的操作一律"
            "「true = 失敗 / 越界 / 中斷」；座標參數可傳 pair / tuple / 任何有 .x/.y 的型別；"
            "取值家族 get/getref（未檢查、參照）、getptr（檢查、可 nullptr）、"
            "getval（複本、可帶 default）。",
        ],
        "points": [
            ("is_coor concept：Position 這種有 .x/.y 的 component 可直接當座標傳入。",
             0, "concept is_coor"),
            ("row-major 索引的實作位置。", 0, "inline T& get(int x, int y)"),
            ("operator[] 傳整數時回傳「列指標」供 arr[x][y] 用，且不做越界檢查。",
             0, "auto operator [](TC c)"),
            ("mydef.h 的巨集都附使用範例註解，掃過即可。", 1, "IS_MACRO_EMPTY"),
        ],
        "quiz": "<code>arr.set(x, y, v)</code> 回傳 <code>true</code> 代表什麼？"
                "（提示：跟直覺相反。）",
    },
    {
        "slug": "02-zone-core",
        "title": "第 2 站 zone.h + tile.h — 核心資料結構",
        "tagline": "Zone{id, parent, reg, layers}：一個 zone = 一個 registry + 自帶多層 tile 地圖。",
        "files": ["projects/medp/src/gcore/zone/tile.h", "projects/medp/src/gcore/zone/zone.h"],
        "intro": [
            "zone id 是<b>零座標語意</b>的裸 <code>uint64_t</code> 單調序號"
            "（舊 ZoneKey 位元打包已移除）；層級只活在 <code>parent</code> 鏈上。"
            "<code>ZONE_ROOT=0</code> 永駐、沒有地圖，放非地圖的全局實體。",
            "<code>layers</code> 是 <code>map&lt;int, tdarray&lt;Tile&gt;&gt;</code>，"
            "鍵即 z（地面=0、往下為負、稀疏）。地圖是 zone 的固有結構、不走 ECS——"
            "代價是 registry snapshot 不含它，存檔由 zone_io 分兩塊處理（見第 4 站）。",
        ],
        "points": [
            ("Tile{terrain, flags}：不是 component；flags 快取通行性。", 0, "struct Tile"),
            ("TILE_WALKABLE / TILE_BLOCKS_SIGHT 通行 flags。", 0, "TILE_WALKABLE"),
            ("Zone 本體：身分直接掛 struct，不再用 placeholder entity 攜帶。", 1, "struct Zone"),
            ("layers：鍵即 z 的稀疏多層地圖。", 1, "std::map<int, tdarray<Tile>> layers"),
            ("registry 不可複製 → Zone 只能移動。", 1, "entt::registry reg"),
        ],
        "quiz": "為什麼地圖掛在 Zone 上而不是做成 component？代價是什麼？"
                "（答案在 layers 的註解。）",
    },
    {
        "slug": "03-components",
        "title": "第 3 站 components/ — 資料積木",
        "tagline": "兩個 POD component：Position（x,y,z）與 Velocity（示範用）。",
        "files": [
            "projects/medp/src/gcore/components/position.h",
            "projects/medp/src/gcore/components/velocity.h",
        ],
        "intro": [
            "全部是 POD aggregate + <code>serialize()</code> 成員；entity 之間的參照存 "
            "<code>entt::entity</code>。",
            "<b>鐵律：新增 component 必須同步登記 serialize/all_components.h 的 "
            "AllComponents</b>，否則存檔會默默漏掉它（見第 4 站）。",
        ],
        "points": [
            ("Position{x,y,z}：z 即 Zone::layers 的鍵；有 .x/.y 故滿足 tdarray 的 "
             "is_coor concept。", 0, "struct Position"),
            ("Velocity{dx,dy}：每 tick 移動步（示範用）。", 1, "struct Velocity"),
        ],
        "quiz": "新增一個 component 除了寫 struct 本身，還必須改哪個檔？",
    },
    {
        "slug": "04-serialize",
        "title": "第 4 站 serialize/ — 存讀檔管線",
        "tagline": "registry_io：registry ↔ 位元組；zone_io：完整 Zone 兩塊接合。AllComponents 是唯一登記點。",
        "files": [
            "projects/medp/src/gcore/serialize/all_components.h",
            "projects/medp/src/gcore/serialize/entt_cereal_archive.h",
            "projects/medp/src/gcore/serialize/registry_io.h",
            "projects/medp/src/gcore/serialize/zone_io.h",
        ],
        "intro": [
            "分工：<code>registry_io</code> 負責單一 registry ↔ 位元組（EnTT snapshot 遍歷、"
            "cereal PortableBinary 決定位元格式、entt_cereal_archive.h 純膠水橋接）；"
            "<code>zone_io</code> 負責完整 Zone——第一塊（id/parent/layers）直接 cereal，"
            "第二塊（reg）走 registry_io，兩塊依序接在同一 stream。",
            "save 與 load 都以 fold expression 展開 <code>AllComponents</code> "
            "type_list——清單漏了誰，那個 component 就「默默」不存不讀，不會報錯。"
            "<b>存檔無版本欄位</b>（使用者裁定）：格式一變，舊檔讀出來就是壞資料；"
            "重寫期直接刪存檔目錄。",
        ],
        "points": [
            ("AllComponents：新增 component 唯一要登記的地方。", 0, "using AllComponents"),
            ("純膠水：把 entt snapshot 的 callback 簽章轉成 cereal 呼叫，"
             "entt::entity ↔ 底層整數。", 1, "struct output_archive"),
            ("save_impl / load_impl 用 fold expression 展開清單。", 2, "void save_impl"),
            ("陷阱點 orphans()：load 後沒有任何（已登記）component 的 entity 會被清掉。",
             2, "loader.orphans()"),
            ("兩塊接合：大括號限制 archive 生存期——cereal 是解構時才把緩衝寫出去的。",
             3, "inline void save(Zone& z"),
        ],
        "quiz": "為什麼新 component 忘了登記 AllComponents，存檔會「默默」漏掉它、"
                "不會報錯？",
    },
    {
        "slug": "05-zone-manager",
        "title": "第 5 站 ZoneManager — 總管",
        "tagline": "把前四站接起來：id 配發、開檔協定、tick、存讀檔與損毀防護。",
        "files": ["projects/medp/src/gcore/zone/zone_manager.h", "projects/medp/src/gcore/zone/zone_manager.cpp"],
        "intro": [
            "root 永久存活、放全局實體，其餘 zones 按需載入卸載。header 註解寫了三條契約，必讀："
            "tick 內禁止 zone 結構性變更（迭代器 UB）；存檔目錄=單槽活儲存；"
            "<code>Zone*</code> 不跨 tick 持有（想長駐就存 id、每次 get）。",
            "存檔佈局：一 zone 一檔（<code>&lt;16hex&gt;.bin</code>，root 特例 "
            "<code>root.bin</code>）＋ <code>manifest.bin</code>（僅 next_zone_id，"
            "未來擴充為存檔 metainfo）。",
        ],
        "points": [
            ("開檔協定：有 manifest → 還原 next_id＋必讀回 root.bin（缺失 throw）；"
             "無 manifest 但有 .bin → throw；乾淨目錄 → 新世界。",
             1, "ZoneManager::ZoneManager"),
            ("create_child：id 單點配發（永不復用），配發即原子寫 manifest；"
             "撞既有檔或 parent 未載入 → throw。", 1, "Zone& ZoneManager::create_child"),
            ("destroy：連盤上檔案一起刪（死 zone 不復活）。", 1, "void ZoneManager::destroy"),
            ("manifest 原子寫：先 .tmp 再 rename。", 1, "void ZoneManager::write_manifest"),
            ("load：檔案不存在回 false；檔內 id 與請求不符 → throw。",
             1, "bool ZoneManager::load"),
            ("tick：每個已載入 zone × 每個 system 依註冊順序；root 也參加。",
             1, "void ZoneManager::tick"),
        ],
        "quiz": "哪三種磁碟狀態會讓建構子 throw？為什麼 create_child 要在建 zone 前"
                "先寫 manifest？",
    },
    {
        "slug": "06-systems",
        "title": "第 6 站 systems/movement.h — system 的樣板",
        "tagline": "所有未來 system 的形狀範本：自由函式、吃 Zone&；位置變更收口於 move_by。",
        "files": ["projects/medp/src/gcore/systems/movement.h"],
        "intro": [
            "26 行示範「system 該長什麼樣」：簽章 <code>void(Zone&)</code> 即 ZoneSystem，"
            "可直接 <code>zm.add_zone_system(systems::movement)</code> 註冊。",
            "位置變更一律經 <code>move_by</code> 收口、不直改 Position——"
            "將來 tile flag 檢查（可走性）、空間索引維護都掛在這個口上。",
        ],
        "points": [
            ("move_by：位置變更的唯一入口。", 0, "inline void move_by"),
            ("movement：view 遍歷帶 entity，交給 move_by。", 0, "inline void movement"),
        ],
        "quiz": "要新增一個 system，需要改 ZoneManager 嗎？（不用——寫自由函式，"
                "外部 add_zone_system 註冊即可。）",
    },
    {
        "slug": "07-tests",
        "title": "第 7 站 projects/tests/src/main.cpp — 可執行的規格書",
        "tagline": "15 個 case，每個 test 就是一段「這功能該怎麼用」的示範。",
        "files": ["projects/tests/src/main.cpp"],
        "intro": [
            "改任何行為前先看對應 test 的期望。15 個 case 的總表在檔尾 main()。",
            "跑法：先建 medp 再建 tests，執行 "
            "<code>./projects/tests/build/bin/medp_test.&lt;平台&gt;.&lt;組態&gt;.&lt;位元數&gt;</code>"
            "（詳見 workflows/testing.md）。",
        ],
        "points": [
            ("case 總表（執行順序）。", 0, "int main()"),
            ("registry_orphans_removed：驗證 orphans() 清孤兒的語意。",
             0, "static bool test_registry_orphans_removed()"),
            ("open_protocol_guards：驗證兩條開檔損毀防護都會 throw。",
             0, "static bool test_open_protocol_guards()"),
            ("tick_all_zones：明文固定「root 也參加 tick」的語意。",
             0, "static bool test_tick_all_zones()"),
        ],
        "quiz": "哪個 test 保護「死 zone 不復活」這條語意？",
    },
    {
        "slug": "08-gbind",
        "title": "附錄 gbind/ — Godot GDExtension 接線",
        "tagline": "目前只有 smoke-test facade，可略過；動它前讀 projects/archived/gd/。",
        "files": [
            "projects/medp/src/gbind/medp_core.h",
            "projects/medp/src/gbind/medp_core.cpp",
            "projects/medp/src/gbind/register_types.h",
            "projects/medp/src/gbind/register_types.cpp",
        ],
        "intro": [
            "驗證整條工具鏈（godot-cpp build → bindings → link medp_static → .so/.dll → "
            "Godot 載入 → GDScript 呼叫）能從頭到尾正常運作。CMake 第二 target，"
            "預設不編（<code>-DMEDP_BUILD_GDEXTENSION=ON</code> 開啟）。",
        ],
        "points": [
            ("MedpCore::version()：GDScript 可呼叫的 smoke-test。", 0, "class MedpCore"),
            ("entry symbol 必須與 medp.gdextension 的 entry_symbol 一致。",
             3, "medp_library_init"),
        ],
        "quiz": None,
    },
]

INVARIANTS = [
    ("多 registry / zone 生命週期",
     "一個 zone = 一個 <code>Zone</code>（registry＋自帶多層地圖），由 ZoneManager 管理；"
     "root 永久存活、放全局實體，其餘 zones 按需載入/卸載。zone id 是零語意單調序號、"
     "由 create_child 單點配發，磁碟 path 由 id 推導、不另存全域 zone 清單。"),
    ("序列化",
     "EnTT snapshot 遍歷 registry，cereal PortableBinaryArchive 負責位元格式，透過 "
     "entt_cereal_archive.h 橋接。component 型別清單的單一來源是 all_components.h 的 "
     "<code>AllComponents</code>，save/load 兩邊共用。存檔無版本欄位——格式變更＝刪存檔目錄。"),
    ("元件即資料",
     "component 盡量是 POD aggregate；entity 之間的參照存 <code>entt::entity</code>。"
     "system 寫成吃 <code>Zone&amp;</code> 的自由函式；位置變更一律收口於 "
     "<code>systems::move_by</code>。"),
]


# ---------------------------------------------------------------------------
# HTML 生成
# ---------------------------------------------------------------------------

def page(title: str, body: str, css_prefix: str = "") -> str:
    return f"""<!DOCTYPE html>
<html lang="zh-Hant">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{html.escape(title)}</title>
<link rel="stylesheet" href="{css_prefix}_shared.css">
</head>
<body>
{body}
<footer>由 <code>workflows/common/code-map/html/build.py</code> 生成——真相層是原始碼與
 <a href="../CODE_TOUR.md">CODE_TOUR.md</a>／<a href="../CODE_MAP.md">CODE_MAP.md</a>；
程式碼更新後重跑 <code>python3 build.py</code> 即同步。</footer>
</body>
</html>
"""


def find_line(src_lines: list[str], needle: str) -> int | None:
    for i, ln in enumerate(src_lines, 1):
        if needle in ln:
            return i
    return None


def render_code(relpath: str, slug: str) -> tuple[str, int, list[str]]:
    """回傳 (HTML, 行數, 原始行列表)。"""
    src = (ROOT / relpath).read_text(encoding="utf-8").rstrip("\n")
    lines = highlight(src).split("\n")
    rows = []
    for i, ln in enumerate(lines, 1):
        rows.append(
            f'<div class="line" id="{slug}-L{i}">'
            f'<a class="no" href="#{slug}-L{i}">{i}</a>'
            f'<span class="src">{ln or " "}</span></div>'
        )
    n = len(lines)
    return "\n".join(rows), n, src.split("\n")


def file_slug(relpath: str) -> str:
    return re.sub(r"[^a-z0-9]+", "-", relpath.lower()).strip("-")


def build_station(idx: int) -> tuple[str, int]:
    st = STATIONS[idx]
    file_data = []   # (relpath, slug, code_html, n_lines, raw_lines)
    total = 0
    for rp in st["files"]:
        slug = file_slug(rp)
        code_html, n, raw = render_code(rp, slug)
        file_data.append((rp, slug, code_html, n, raw))
        total += n

    prev_link = (f'<a href="{STATIONS[idx-1]["slug"]}.html">← {html.escape(STATIONS[idx-1]["title"])}</a>'
                 if idx > 0 else "<span></span>")
    next_link = (f'<a href="{STATIONS[idx+1]["slug"]}.html">{html.escape(STATIONS[idx+1]["title"])} →</a>'
                 if idx + 1 < len(STATIONS) else "<span></span>")

    points_html = []
    for text, fi, needle in st["points"]:
        loc = ""
        if fi is not None and needle:
            rp, slug, _, _, raw = file_data[fi]
            ln = find_line(raw, needle)
            if ln:
                loc = (f' <a class="loc" href="#{slug}-L{ln}">'
                       f'{html.escape(Path(rp).name)}:{ln}</a>')
        points_html.append(f"<li>{text}{loc}</li>")

    files_html = []
    for rp, slug, code_html, n, _ in file_data:
        files_html.append(f"""
<section class="file" id="{slug}">
  <h3><code>{html.escape(rp)}</code> <span class="dim">（{n} 行）</span>
      <a class="loc" href="{REL_TO_ROOT}/{html.escape(rp)}">開啟原始檔</a></h3>
  <div class="code">
{code_html}
  </div>
</section>""")

    quiz_html = (f'<div class="quiz"><b>讀完該能回答：</b>{st["quiz"]}</div>'
                 if st["quiz"] else "")

    body = f"""
<nav class="crumb"><a href="index.html">← 回導覽索引</a></nav>
<h1>{html.escape(st["title"])}</h1>
<p class="meta">{len(st["files"])} 個檔案，共 {total} 行 ·
 對照 <a href="../CODE_TOUR.md">CODE_TOUR.md</a></p>
{''.join(f'<p>{p}</p>' for p in st["intro"])}
<h2>看什麼</h2>
<ul class="points">
{''.join(points_html)}
</ul>
{quiz_html}
{''.join(files_html)}
<nav class="pager">{prev_link}{next_link}</nav>
"""
    (HERE / f"{st['slug']}.html").write_text(page(st["title"], body), encoding="utf-8")
    return st["slug"], total


def build_index(station_lines: dict[str, int]) -> None:
    cards = []
    for st in STATIONS:
        n_files = len(st["files"])
        cards.append(f"""
<a class="card" href="{st['slug']}.html">
  <h3>{html.escape(st['title'])}</h3>
  <p>{st['tagline']}</p>
  <p class="dim">{n_files} 檔 · {station_lines[st['slug']]} 行</p>
</a>""")

    inv = "".join(f"<li><b>{t}</b>——{d}</li>" for t, d in INVARIANTS)

    body = f"""
<h1>medps 程式碼導覽</h1>
<p class="meta">奇幻 4X 策略遊戲 C++20 後端（EnTT + cereal）· 核心約 1,100 行 ·
照順序讀完約 1 小時</p>

<div class="hero">
<b>全貌一句話：</b><code>ZoneManager</code> 管一堆 <code>Zone</code>
（一個 zone = 一個 <code>entt::registry</code> + 自帶多層 tile 地圖），zone id 是零語意的
單調序號、由 <code>create_child</code> 配發；存讀檔 = 一 zone 一檔（zone_io 兩塊接合）＋
<code>manifest.bin</code> 記 id 計數器；<code>tick()</code> 對每個已載入 zone（含 root）
跑所有註冊的 system。
</div>

<h2>資料流</h2>
<div class="flow">
  <div class="box">components/<br><span class="dim">POD 資料</span></div>
  <div class="arrow">掛在</div>
  <div class="box">Zone<br><span class="dim">registry + layers</span></div>
  <div class="arrow">管理</div>
  <div class="box">ZoneManager<br><span class="dim">root + 配號 + tick</span></div>
</div>
<div class="flow">
  <div class="box">zone_io<br><span class="dim">Zone ↔ 位元組（兩塊）</span></div>
  <div class="arrow">→</div>
  <div class="box">dir/&lt;16hex&gt;.bin<br><span class="dim">一 zone 一檔</span></div>
  <div class="arrow">＋</div>
  <div class="box">manifest.bin<br><span class="dim">next_zone_id</span></div>
</div>

<h2>閱讀路徑（依依賴順序）</h2>
<div class="grid">
{''.join(cards)}
</div>

<h2>架構不變量（修改前必知）</h2>
<ul class="points">
{inv}
</ul>

<h2>真相層連結</h2>
<ul class="points">
  <li><a href="../CODE_TOUR.md">CODE_TOUR.md</a>——線性導讀（本導覽的 Markdown 真相層）</li>
  <li><a href="../CODE_MAP.md">CODE_MAP.md</a>——agent 修改前的查表</li>
  <li><a href="{REL_TO_ROOT}/AGENTS.md">AGENTS.md</a>——專案備忘與鐵律</li>
  <li><a href="{REL_TO_ROOT}/docs/work/progress_overview.md">docs/work/progress_overview.md</a>——進度總覽</li>
</ul>
"""
    (HERE / "index.html").write_text(page("medps 程式碼導覽", body), encoding="utf-8")


CSS = """/* medps 程式碼導覽共用樣式（build.py 一併維護） */
:root {
  --bg: #ffffff; --fg: #1a1a1a; --dim: #6b7280;
  --card: #f6f7f9; --border: #e2e5ea; --accent: #2563eb;
  --code-bg: #f6f7f9; --ln: #9ca3af;
  --kw: #7c3aed; --cm: #16793c; --st: #b45309; --nu: #0e7490; --pp: #be185d;
  --hero: #eef4ff; --quiz: #fff7e6; --quiz-border: #f0c36d;
}
@media (prefers-color-scheme: dark) {
  :root {
    --bg: #111418; --fg: #e5e7eb; --dim: #9ca3af;
    --card: #1a1f26; --border: #2a313b; --accent: #7aa2ff;
    --code-bg: #161b22; --ln: #4b5563;
    --kw: #c4a7ff; --cm: #7ecb92; --st: #e5b567; --nu: #67c5dc; --pp: #f38bb0;
    --hero: #16202f; --quiz: #2a2416; --quiz-border: #6b5a2e;
  }
}
* { box-sizing: border-box; }
body {
  margin: 0 auto; max-width: 62rem; padding: 2rem 1.25rem 4rem;
  background: var(--bg); color: var(--fg);
  font: 16px/1.75 -apple-system, "Noto Sans TC", "Microsoft JhengHei", sans-serif;
}
h1 { font-size: 1.7rem; margin: .5rem 0; }
h2 { font-size: 1.2rem; margin: 2rem 0 .6rem; border-bottom: 1px solid var(--border); padding-bottom: .3rem; }
h3 { font-size: 1rem; margin: 1.2rem 0 .5rem; }
a { color: var(--accent); text-decoration: none; }
a:hover { text-decoration: underline; }
code { font-family: "JetBrains Mono", Consolas, monospace; font-size: .92em;
       background: var(--code-bg); padding: .1em .3em; border-radius: 4px; }
.meta, .dim { color: var(--dim); font-size: .9rem; }
.crumb { margin-bottom: 1rem; }
.hero { background: var(--hero); border: 1px solid var(--border);
        border-radius: 10px; padding: 1rem 1.2rem; margin: 1rem 0; }
.flow { display: flex; flex-wrap: wrap; align-items: center; gap: .6rem; margin: .8rem 0; }
.flow .box { background: var(--card); border: 1px solid var(--border); border-radius: 8px;
             padding: .5rem .9rem; text-align: center; line-height: 1.4; }
.flow .arrow { color: var(--dim); font-size: .85rem; }
.grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(16rem, 1fr)); gap: .8rem; }
.card { display: block; background: var(--card); border: 1px solid var(--border);
        border-radius: 10px; padding: .9rem 1rem; color: var(--fg); }
.card:hover { border-color: var(--accent); text-decoration: none; }
.card h3 { margin: 0 0 .4rem; color: var(--accent); }
.card p { margin: .2rem 0; font-size: .92rem; }
.points li { margin: .35rem 0; }
.loc { font-family: Consolas, monospace; font-size: .85em; white-space: nowrap; }
.quiz { background: var(--quiz); border: 1px solid var(--quiz-border);
        border-radius: 8px; padding: .7rem 1rem; margin: 1rem 0; }
.file h3 { display: flex; flex-wrap: wrap; gap: .6rem; align-items: baseline; }
.code { background: var(--code-bg); border: 1px solid var(--border); border-radius: 8px;
        padding: .6rem 0; overflow-x: auto; margin: .4rem 0 1.6rem;
        font: 13px/1.55 "JetBrains Mono", Consolas, monospace; }
.code .line { display: flex; white-space: pre; }
.code .line:target { background: color-mix(in srgb, var(--accent) 16%, transparent); }
.code .no { flex: 0 0 3.2em; text-align: right; padding-right: 1em; color: var(--ln);
            user-select: none; }
.code .src { flex: 1; padding-right: 1em; }
.kw { color: var(--kw); } .cm { color: var(--cm); } .st { color: var(--st); }
.nu { color: var(--nu); } .pp { color: var(--pp); }
.pager { display: flex; justify-content: space-between; gap: 1rem;
         margin-top: 2rem; border-top: 1px solid var(--border); padding-top: 1rem; }
footer { margin-top: 3rem; border-top: 1px solid var(--border); padding-top: 1rem;
         color: var(--dim); font-size: .85rem; }
"""


def main() -> None:
    (HERE / "_shared.css").write_text(CSS, encoding="utf-8")
    station_lines = {}
    for i in range(len(STATIONS)):
        slug, total = build_station(i)
        station_lines[slug] = total
        print(f"  {slug}.html（{total} 行原始碼）")
    build_index(station_lines)
    print("  index.html")
    print("完成。")


if __name__ == "__main__":
    main()
