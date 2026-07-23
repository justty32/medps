#include <gcore/zone/zone.h>
#include <gcore/world/world.h>
#include <gcore/zone/zone_manager.h>
#include <gcore/zone/tile.h>
#include <gcore/world/components/position.h>
#include <gcore/world/components/velocity.h>
#include <gcore/world/systems/movement.h>
#include <gcore/serialize/registry_io.h>
#include <gcore/serialize/zone_io.h>

#include <cassert>
#include <cstdio>
#include <filesystem>
#include <sstream>
#include <stdexcept>

namespace fs = std::filesystem;

// ---- 輔助工具 ----

static void pass(const char* name) { std::printf("  [PASS] %s\n", name); }
static void fail(const char* name) { std::printf("  [FAIL] %s\n", name); }

#define CHECK(name, expr) do { if (expr) pass(name); else { fail(name); return false; } } while(0)

// 每個 ZoneManager 測試用自己的乾淨暫存目錄
static fs::path fresh_dir(const char* leaf) {
    auto dir = fs::temp_directory_path() / "medps_test" / leaf;
    fs::remove_all(dir);
    return dir;
}

// ---- 序列化（registry_io）----

static bool test_registry_roundtrip() {
    entt::registry src;
    auto e1 = src.create();
    src.emplace<Position>(e1, 42, 7, -1);
    src.emplace<Velocity>(e1, 3, 4);
    auto e2 = src.create();
    src.emplace<Position>(e2, 0, 0, 0);

    std::stringstream ss;
    registry_io::save(src, ss);
    entt::registry dst;
    registry_io::load(dst, ss);

    int count = 0;
    bool e1_ok = false, e2_ok = false;
    for (auto e : dst.view<Position>()) {
        auto& p = dst.get<Position>(e);
        if (p.x == 42 && p.y == 7 && p.z == -1)
            e1_ok = dst.all_of<Velocity>(e) && dst.get<Velocity>(e).dx == 3;
        if (p.x == 0 && p.y == 0 && p.z == 0) e2_ok = true;
        ++count;
    }
    CHECK("position count", count == 2);
    CHECK("e1 pos+vel",     e1_ok);
    CHECK("e2 pos",         e2_ok);
    return true;
}

static bool test_registry_orphans_removed() {
    // 不持有任何 AllComponents 內 component 的 entity，會被 loader.orphans() 移除
    entt::registry src;
    auto e_empty = src.create();
    auto e_with  = src.create();
    src.emplace<Position>(e_with, 1, 2, 0);

    std::stringstream ss;
    registry_io::save(src, ss);
    entt::registry dst;
    registry_io::load(dst, ss);

    CHECK("orphan removed",  !dst.valid(e_empty));
    CHECK("non-orphan kept",  dst.valid(e_with));
    return true;
}

static bool test_registry_empty() {
    entt::registry src;
    std::stringstream ss;
    registry_io::save(src, ss);
    entt::registry dst;
    registry_io::load(dst, ss);
    CHECK("empty registry", dst.storage<entt::entity>().size() == 0u);
    return true;
}

// ---- tdarray ----

static bool test_tdarray_basics() {
    tdarray<Tile> g;
    CHECK("alloc ok",        !g.alloc(3, 3));
    CHECK("set in bounds",   !g.set(1, 2, Tile{7, TILE_WALKABLE}));
    CHECK("set out of bounds", g.set(3, 0, Tile{}));   // true = 越界失敗
    Tile t = g.getval(1, 2);
    CHECK("stored value", t.terrain == 7 && (t.flags & TILE_WALKABLE));
    return true;
}

// ---- zone_io（完整 Zone 往返）----

static bool test_zone_io_roundtrip() {
    Zone src;
    src.id     = 9;
    src.parent = 4;
    src.layers[0].alloc(2, 2);
    src.layers[0].set(0, 1, Tile{3, TILE_BLOCKS_SIGHT});
    src.layers[-1].alloc(1, 1);                        // 地下層，鍵為負
    auto e = src.reg.create();
    src.reg.emplace<Position>(e, 5, 6, -1);

    std::stringstream ss;
    zone_io::save(src, ss);
    auto dst = zone_io::load(ss);        // load = 讀檔頭 kind tag＋建構一體

    CHECK("plain kind",  dst->kind() == ZoneKind::Plain);
    CHECK("id/parent",   dst->id == 9 && dst->parent == 4);
    CHECK("layer count", dst->layers.size() == 2);
    Tile t = dst->layers[0].getval(0, 1);
    CHECK("tile data",   t.terrain == 3 && (t.flags & TILE_BLOCKS_SIGHT));
    bool pos_ok = false;
    for (auto en : dst->reg.view<Position>()) {
        auto& p = dst->reg.get<Position>(en);
        pos_ok = (p.x == 5 && p.y == 6 && p.z == -1);
    }
    CHECK("entity data", pos_ok);
    return true;
}

static bool test_zone_io_unknown_kind_throws() {
    // 檔頭 kind tag 為未知值 → make_zone throw（存檔損毀 fail-fast，不靜默退回 Plain）
    std::stringstream ss;
    {
        cereal::PortableBinaryOutputArchive ar{ss};
        uint8_t bogus = 0xFF;
        ar(bogus);
    }
    bool threw = false;
    try { zone_io::load(ss); } catch (const std::runtime_error&) { threw = true; }
    CHECK("unknown kind throws", threw);
    return true;
}

// ---- ZoneManager：建構與配號 ----

static bool test_new_world() {
    auto dir = fresh_dir("new_world");
    ZoneManager zm{dir};
    CHECK("root is 0",        ZONE_ROOT == 0);
    CHECK("only root loaded", zm.size() == 1);
    CHECK("root parent self", zm.root().parent == ZONE_ROOT);
    CHECK("no files yet",     !fs::exists(dir / "manifest.bin"));   // 未配號/存檔前不落盤
    fs::remove_all(dir);
    return true;
}

static bool test_create_child_ids() {
    auto dir = fresh_dir("child_ids");
    ZoneManager zm{dir};
    auto& a = zm.create_child(ZONE_ROOT);
    auto& b = zm.create_child(ZONE_ROOT);
    CHECK("ids sequential",    a.id == 1 && b.id == 2);
    CHECK("parent recorded",   a.parent == ZONE_ROOT);
    CHECK("manifest on alloc", fs::exists(dir / "manifest.bin"));   // 配發即落 manifest

    bool threw = false;
    try { zm.create_child(999); } catch (const std::runtime_error&) { threw = true; }
    CHECK("unloaded parent throws", threw);
    fs::remove_all(dir);
    return true;
}

// ---- ZoneManager：持久化 ----

static bool test_persist_across_sessions() {
    auto dir = fresh_dir("persist");
    ZoneManager::ZoneId child_id = 0;

    {   // session 1：root 放一個全局實體、子 zone 放一個 actor，存檔
        ZoneManager zm{dir};
        auto er = zm.root().reg.create();
        zm.root().reg.emplace<Position>(er, 100, 100, 0);

        auto& child = zm.create_child(ZONE_ROOT);
        child_id = child.id;
        auto ec = child.reg.create();
        child.reg.emplace<Position>(ec, 5, 6, 0);
        zm.save_all();
    }
    {   // session 2：root 自動還原；child 不自動載入但可按需載入
        ZoneManager zm{dir};
        CHECK("root auto-restored", !zm.root().reg.view<Position>().empty());
        CHECK("child not auto-loaded", zm.get(child_id) == nullptr);
        CHECK("child loadable",     zm.load(child_id));
        CHECK("child data intact",  !zm.get(child_id)->reg.view<Position>().empty());
        CHECK("child parent kept",  zm.get(child_id)->parent == ZONE_ROOT);
        auto& c2 = zm.create_child(ZONE_ROOT);
        CHECK("next_id survives restart", c2.id == child_id + 1);   // 序號不復用
    }
    fs::remove_all(dir);
    return true;
}

static bool test_unload_roundtrip() {
    auto dir = fresh_dir("unload");
    ZoneManager zm{dir};
    auto& z = zm.create_child(ZONE_ROOT);
    auto id = z.id;
    auto e = z.reg.create();
    z.reg.emplace<Position>(e, 7, 8, 0);

    zm.unload(id);
    CHECK("gone from memory", zm.get(id) == nullptr);
    CHECK("written to disk",  fs::exists(zm.path(id)));
    CHECK("reloadable",       zm.load(id));
    CHECK("data intact",      !zm.get(id)->reg.view<Position>().empty());

    zm.unload(ZONE_ROOT);
    CHECK("root not unloadable", zm.get(ZONE_ROOT) != nullptr);
    fs::remove_all(dir);
    return true;
}

static bool test_destroy_deletes_file() {
    auto dir = fresh_dir("destroy");
    ZoneManager zm{dir};
    auto& z = zm.create_child(ZONE_ROOT);
    auto id = z.id;
    zm.save_all();
    CHECK("file exists before", fs::exists(zm.path(id)));

    zm.destroy(id);
    CHECK("gone from memory", zm.get(id) == nullptr);
    CHECK("file deleted",     !fs::exists(zm.path(id)));
    CHECK("load finds nothing", !zm.load(id));          // 死 zone 不復活

    zm.destroy(ZONE_ROOT);
    CHECK("root not destroyable", zm.get(ZONE_ROOT) != nullptr);
    fs::remove_all(dir);
    return true;
}

// ---- ZoneManager：開檔協定與損毀防護 ----

static bool test_open_protocol_guards() {
    // 有 zone 檔卻無 manifest → throw（不得靜默當新世界）
    auto dir = fresh_dir("guards");
    {
        ZoneManager zm{dir};
        zm.create_child(ZONE_ROOT);
        zm.save_all();
    }
    fs::remove(dir / "manifest.bin");
    bool threw_no_manifest = false;
    try { ZoneManager zm{dir}; } catch (const std::runtime_error&) { threw_no_manifest = true; }
    CHECK("zone files without manifest throw", threw_no_manifest);

    // 有 manifest 卻缺 root.bin → throw（存檔損毀）
    fs::remove_all(dir);
    {
        ZoneManager zm{dir};
        zm.create_child(ZONE_ROOT);   // 配號落 manifest，但不 save_all
    }
    bool threw_no_root = false;
    try { ZoneManager zm{dir}; } catch (const std::runtime_error&) { threw_no_root = true; }
    CHECK("manifest without root.bin throws", threw_no_root);
    fs::remove_all(dir);
    return true;
}

static bool test_load_id_mismatch_throws() {
    auto dir = fresh_dir("id_mismatch");
    ZoneManager::ZoneId id = 0;
    {
        ZoneManager zm{dir};
        id = zm.create_child(ZONE_ROOT).id;
        zm.save_all();
    }
    ZoneManager zm{dir};
    fs::copy_file(zm.path(id), zm.path(id + 500));      // 檔案錯位：內容 id ≠ 檔名 id
    bool threw = false;
    try { zm.load(id + 500); } catch (const std::runtime_error&) { threw = true; }
    CHECK("mismatched file id throws", threw);
    fs::remove_all(dir);
    return true;
}

// ---- World（第一個 Zone 子類）----

static bool test_world_kind_roundtrip() {
    auto dir = fresh_dir("world_kind");
    ZoneManager::ZoneId wid = 0;
    {   // session 1：造一個 World 子 zone、改生成參數、存檔
        ZoneManager zm{dir};
        auto& z = zm.create_child(ZONE_ROOT, ZoneKind::World);
        auto* w = zone_cast<World>(&z);
        CHECK("created as World", w != nullptr);
        w->gen.seed  = 1234;
        w->gen.width = 8;
        wid = w->id;
        zm.save_all();
    }
    {   // session 2：載回後子類型別與 gen 參數完整還原
        ZoneManager zm{dir};
        CHECK("loadable", zm.load(wid));
        auto* w = zone_cast<World>(zm.get(wid));
        CHECK("kind restored", w != nullptr);
        CHECK("gen restored",  w->gen.seed == 1234 && w->gen.width == 8);
        CHECK("root stays plain", zone_cast<World>(&zm.root()) == nullptr);
    }
    fs::remove_all(dir);
    return true;
}

static bool test_world_generate_deterministic() {
    World a, b, c;
    a.gen.seed = b.gen.seed = 42;
    c.gen.seed = 43;
    for (World* w : {&a, &b, &c}) { w->gen.width = 32; w->gen.height = 32; w->generate(); }

    bool same = true, diff = false;
    a.layers[0].eachxy([&](Tile& t, int x, int y) {
        Tile tb = b.layers[0].getval(x, y);
        if (t.terrain != tb.terrain || t.flags != tb.flags) same = false;
        if (t.terrain != c.layers[0].getval(x, y).terrain) diff = true;
    });
    CHECK("same seed same map",      same);
    CHECK("diff seed diff map",      diff);
    return true;
}

static bool test_world_generate_sanity() {
    World w;
    w.gen.seed = 7;
    w.gen.width = 64;
    w.gen.height = 48;
    w.generate();

    auto& g = w.layers[0];
    CHECK("dims match params", g.sx == 64 && g.sy == 48);
    int ocean = 0, land = 0;
    bool flags_ok = true;
    g.each([&](Tile& t) {
        if (t.terrain == TERRAIN_OCEAN) { ++ocean; if (t.flags & TILE_WALKABLE)    flags_ok = false; }
        else                            { ++land;  if (!(t.flags & TILE_WALKABLE)) flags_ok = false; }
    });
    CHECK("has ocean and land",   ocean > 0 && land > 0);
    CHECK("walkable = land only", flags_ok);

    World bad;
    bad.gen.width = 0;
    bool threw = false;
    try { bad.generate(); } catch (const std::runtime_error&) { threw = true; }
    CHECK("non-positive dims throw", threw);
    return true;
}

// ---- movement ----

static bool test_move_by() {
    Zone z;
    auto e = z.reg.create();
    z.reg.emplace<Position>(e, 10, 20, 0);
    systems::move_by(z, e, -1, 2);
    auto& p = z.reg.get<Position>(e);
    CHECK("moved", p.x == 9 && p.y == 22 && p.z == 0);
    return true;
}

static bool test_tick_all_zones() {
    // tick 對所有 zone（含 root）依序執行 system
    auto dir = fresh_dir("tick_all");
    ZoneManager zm{dir};
    zm.add_zone_system(systems::movement);

    auto& z1 = zm.create_child(ZONE_ROOT);
    auto e1 = z1.reg.create();
    z1.reg.emplace<Position>(e1, 0, 0, 0);
    z1.reg.emplace<Velocity>(e1, 1, 2);

    auto& z2 = zm.create_child(ZONE_ROOT);
    auto e2 = z2.reg.create();
    z2.reg.emplace<Position>(e2, 10, 10, 0);
    z2.reg.emplace<Velocity>(e2, -1, 0);

    auto er = zm.root().reg.create();
    zm.root().reg.emplace<Position>(er, 100, 100, 0);
    zm.root().reg.emplace<Velocity>(er, 5, 5);

    zm.tick();

    auto& p1 = z1.reg.get<Position>(e1);
    auto& p2 = z2.reg.get<Position>(e2);
    auto& pr = zm.root().reg.get<Position>(er);
    CHECK("zone1 moved", p1.x == 1 && p1.y == 2);
    CHECK("zone2 moved", p2.x == 9 && p2.y == 10);
    CHECK("root ticked too", pr.x == 105 && pr.y == 105);
    fs::remove_all(dir);
    return true;
}

static bool test_tick_system_order() {
    auto dir = fresh_dir("tick_order");
    ZoneManager zm{dir};
    auto& z = zm.create_child(ZONE_ROOT);
    auto e = z.reg.create();
    z.reg.emplace<Position>(e, 0, 0, 0);

    zm.add_zone_system([](Zone& zn){
        zn.reg.view<Position>().each([](Position& p){ p.x += 1; });   // 第一步：+1
    });
    zm.add_zone_system([](Zone& zn){
        zn.reg.view<Position>().each([](Position& p){ p.x *= 10; });  // 接著：*10
    });

    zm.tick();
    CHECK("order is +1 then *10", z.reg.get<Position>(e).x == 10);
    fs::remove_all(dir);
    return true;
}

// ---- 執行器 ----

int main() {
    struct { const char* name; bool(*fn)(); } cases[] = {
        { "registry_roundtrip",        test_registry_roundtrip        },
        { "registry_orphans_removed",  test_registry_orphans_removed  },
        { "registry_empty",            test_registry_empty            },
        { "tdarray_basics",            test_tdarray_basics            },
        { "zone_io_roundtrip",         test_zone_io_roundtrip         },
        { "zone_io_unknown_kind_throws", test_zone_io_unknown_kind_throws },
        { "new_world",                 test_new_world                 },
        { "create_child_ids",          test_create_child_ids          },
        { "persist_across_sessions",   test_persist_across_sessions   },
        { "unload_roundtrip",          test_unload_roundtrip          },
        { "destroy_deletes_file",      test_destroy_deletes_file      },
        { "open_protocol_guards",      test_open_protocol_guards      },
        { "load_id_mismatch_throws",   test_load_id_mismatch_throws   },
        { "world_kind_roundtrip",      test_world_kind_roundtrip      },
        { "world_generate_deterministic", test_world_generate_deterministic },
        { "world_generate_sanity",     test_world_generate_sanity     },
        { "move_by",                   test_move_by                   },
        { "tick_all_zones",            test_tick_all_zones            },
        { "tick_system_order",         test_tick_system_order         },
    };

    int passed = 0, total = 0;
    for (auto& c : cases) {
        std::printf("[%s]\n", c.name);
        if (c.fn()) ++passed;
        ++total;
    }
    std::printf("\n%d / %d passed\n", passed, total);
    return (passed == total) ? 0 : 1;
}
