#include <gcore/zone_key.h>
#include <gcore/components/zone_meta.h>
#include <gcore/components/position.h>
#include <gcore/components/velocity.h>
#include <gcore/components/area_terrain.h>
#include <gcore/components/blocking.h>
#include <gcore/systems/movement.h>
#include <gcore/serialize/zone_io.h>
#include <gcore/serialize/zone_store.h>
#include <gcore/global_manager.h>

#include <cassert>
#include <sstream>
#include <cstdio>
#include <vector>
#include <filesystem>
#include <memory>

// ---- 輔助工具 ----

static void pass(const char* name) { std::printf("  [PASS] %s\n", name); }
static void fail(const char* name) { std::printf("  [FAIL] %s\n", name); }

#define CHECK(name, expr) do { if (expr) pass(name); else { fail(name); return false; } } while(0)

// ---- 測試 ----

static bool test_zone_key_roundtrip() {
    auto key = make_zone_key(ZoneType{1}, -100, 200, -1);
    CHECK("type",  zone_key_type(key) == ZoneType{1});
    CHECK("x",     zone_key_x(key)    == -100);
    CHECK("y",     zone_key_y(key)    == 200);
    CHECK("z",     zone_key_z(key)    == -1);
    return true;
}

static bool test_zone_key_root() {
    CHECK("root is 0", ZONE_ROOT == 0);
    return true;
}

static bool test_serialize_roundtrip() {
    entt::registry src;

    auto e1 = src.create();
    src.emplace<Position>(e1, 42, 7);

    auto e2 = src.create();
    // e2 沒有 Position

    auto e3 = src.create();
    src.emplace<Position>(e3, 0, 0);

    std::stringstream ss;
    zone_io::save(src, ss);

    entt::registry dst;
    zone_io::load(dst, ss);

    // e1 應持有 Position{42, 7}
    auto view = dst.view<Position>();
    int count = 0;
    bool e1_ok = false, e3_ok = false;
    for (auto e : view) {
        auto& p = view.get<Position>(e);
        if (p.x == 42 && p.y == 7) e1_ok = true;
        if (p.x == 0  && p.y == 0) e3_ok = true;
        ++count;
    }

    CHECK("component count", count == 2);
    CHECK("e1 data",         e1_ok);
    CHECK("e3 data",         e3_ok);
    return true;
}

static bool test_serialize_orphans_removed() {
    // 不持有任何 AllComponents 內 component 的 entity，會被 loader.orphans() 移除
    entt::registry src;
    auto e_empty = src.create();        // 無 component，load 後會成為孤兒
    auto e_with  = src.create();
    src.emplace<Position>(e_with, 1, 2);

    std::stringstream ss;
    zone_io::save(src, ss);

    entt::registry dst;
    zone_io::load(dst, ss);

    // snapshot_loader 會保留 entity 值；orphans() 則會銷毀 e_empty
    CHECK("orphan removed",  !dst.valid(e_empty));
    CHECK("non-orphan kept",  dst.valid(e_with));
    return true;
}

static bool test_zone_meta_placeholder_survives() {
    // 一個唯一 entity 只是 ZoneMeta placeholder 的 zone，必須能在 round-trip 後存活
    entt::registry src;
    auto ph = src.create();
    src.emplace<ZoneMeta>(ph, ZoneKey{12345});

    std::stringstream ss;
    zone_io::save(src, ss);

    entt::registry dst;
    zone_io::load(dst, ss);

    auto view = dst.view<ZoneMeta>();
    int count = 0;
    bool key_ok = false;
    for (auto e : view) { key_ok = (view.get<ZoneMeta>(e).self == ZoneKey{12345}); ++count; }

    CHECK("placeholder count", count == 1);
    CHECK("placeholder key",   key_ok);
    return true;
}

static bool test_zone_path_deterministic() {
    FolderZoneStore s{"zones"};
    auto k = make_zone_key(ZoneType{1}, 5, 6, 7);
    CHECK("same key same path", s.path(k) == s.path(k));
    CHECK("diff key diff path", s.path(k) != s.path(ZoneKey{k + 1}));
    CHECK("root special name",  s.path(ZONE_ROOT) != s.path(k));
    return true;
}

static bool test_save_load_root() {
    auto dir = std::filesystem::temp_directory_path() / "medps_test_root";
    std::filesystem::remove_all(dir);

    auto child = make_zone_key(ZoneType{1}, 1, 0, 0);

    {   // session 1：建立一場遊戲並做存檔
        GlobalManager gm{std::make_unique<FolderZoneStore>(dir)};
        auto e = gm.root.create();
        gm.root.emplace<ZoneMeta>(e, ZONE_ROOT, ZONE_ROOT);
        gm.create(child, ZONE_ROOT);   // 建立並持久化一個子 zone
        gm.save_all();
    }

    bool ok = true;
    {   // session 2：重新開啟，驗證 root 已還原、子 zone 不自動載入但可按需載入
        GlobalManager gm2{std::make_unique<FolderZoneStore>(dir)};
        gm2.load_root();

        if (gm2.root.view<ZoneMeta>().empty()) ok = false;   // root 的 snapshot 已還原
        if (gm2.get(child) != nullptr) ok = false;           // 子 zone 尚未載入（按需）
        if (gm2.load(child).view<ZoneMeta>().empty()) ok = false;  // 但能從 store 載入
    }

    std::filesystem::remove_all(dir);
    CHECK("root persisted, child not auto-loaded but loadable", ok);
    return true;
}

static bool test_world_config_persists() {
    auto dir = std::filesystem::temp_directory_path() / "medps_test_worldcfg";
    std::filesystem::remove_all(dir);

    bool ok = true;
    {   // 新遊戲：未設定的 config 會讀到預設值，接著選定 world size 並存檔
        GlobalManager gm{std::make_unique<FolderZoneStore>(dir)};
        if (gm.world_config().world_dim_x != zone_scale::WORLD_DIM_DEFAULT) ok = false;
        gm.init_world(64, 48);
        if (gm.world_config().world_dim_x != 64) ok = false;   // 設定後立即可讀
        if (gm.world_config().world_dim_y != 48) ok = false;
        gm.save_all();
    }
    {   // 重新開啟：world_dim 由 root 的 snapshot 還原（每份存檔各自固定、不可變）
        GlobalManager gm2{std::make_unique<FolderZoneStore>(dir)};
        gm2.load_root();
        if (gm2.world_config().world_dim_x != 64) ok = false;
        if (gm2.world_config().world_dim_y != 48) ok = false;
    }

    std::filesystem::remove_all(dir);
    CHECK("world_dim set at new-game persists in root snapshot", ok);
    return true;
}

static bool test_world_dim_bounds() {
    using namespace zone_scale;
    CHECK("default valid",     valid_world_dim(WORLD_DIM_DEFAULT));
    CHECK("zero invalid",     !valid_world_dim(0));
    CHECK("max valid",         valid_world_dim(MAX_WORLD_DIM));
    CHECK("over-max invalid", !valid_world_dim(MAX_WORLD_DIM + 1));
    return true;
}

static bool test_position_roundtrip() {
    entt::registry src;
    auto e = src.create();
    src.emplace<Position>(e, 3, 4);

    std::stringstream ss;
    zone_io::save(src, ss);

    entt::registry dst;
    zone_io::load(dst, ss);

    auto view = dst.view<Position>();
    int count = 0; bool ok = false;
    for (auto en : view) { auto& p = view.get<Position>(en); ok = (p.x == 3 && p.y == 4); ++count; }
    CHECK("position count", count == 1);
    CHECK("position data",  ok);
    return true;
}

static bool test_tick_runs_per_loaded_zone() {
    GlobalManager gm;
    gm.add_zone_system(systems::movement);

    // 兩個已載入的 zone，各有一個會移動的 actor
    auto& z1 = gm.create(make_zone_key(ZoneType{1}, 1, 0, 0), ZONE_ROOT);
    auto e1 = z1.create();
    z1.emplace<Position>(e1, 0, 0);
    z1.emplace<Velocity>(e1, 1, 2);

    auto& z2 = gm.create(make_zone_key(ZoneType{1}, 2, 0, 0), ZONE_ROOT);
    auto e2 = z2.create();
    z2.emplace<Position>(e2, 10, 10);
    z2.emplace<Velocity>(e2, -1, 0);

    // root 中的 actor 不應被 per-zone system tick 到
    auto er = gm.root.create();
    gm.root.emplace<Position>(er, 100, 100);
    gm.root.emplace<Velocity>(er, 5, 5);

    gm.tick();

    auto& p1 = z1.get<Position>(e1);
    auto& p2 = z2.get<Position>(e2);
    auto& pr = gm.root.get<Position>(er);

    CHECK("zone1 moved", p1.x == 1 && p1.y == 2);
    CHECK("zone2 moved", p2.x == 9 && p2.y == 10);
    CHECK("root untouched", pr.x == 100 && pr.y == 100);
    return true;
}

static bool test_tick_system_order() {
    // 對同一個 zone，system 依註冊順序執行
    GlobalManager gm;
    auto& z = gm.create(make_zone_key(ZoneType{1}, 3, 0, 0), ZONE_ROOT);
    auto e = z.create();
    z.emplace<Position>(e, 0, 0);

    gm.add_zone_system([](entt::registry& r){
        r.view<Position>().each([](Position& p){ p.x += 1; });   // 第一步：+1
    });
    gm.add_zone_system([](entt::registry& r){
        r.view<Position>().each([](Position& p){ p.x *= 10; });  // 接著：*10
    });

    gm.tick();
    CHECK("order is +1 then *10", z.get<Position>(e).x == 10);
    return true;
}

static bool test_zone_layers_and_parent() {
    using namespace zone_scale;
    // 由 world-tile (7,8) + region-local (3,4) 構成的 Area，位於地下層
    auto a = area_key(7, 8, 3, 4, zlayer::Underground);
    CHECK("area type",  zone_key_type(a) == ZoneType::Area);
    CHECK("area gx",    zone_key_x(a) == 7 * REGION_DIM + 3);
    CHECK("area gy",    zone_key_y(a) == 8 * REGION_DIM + 4);

    auto r = parent_of(a);                       // Area -> Region，以整數除法換算
    CHECK("parent is region", zone_key_type(r) == ZoneType::Region);
    CHECK("region wx",        zone_key_x(r) == 7);
    CHECK("region wy",        zone_key_y(r) == 8);
    CHECK("z preserved",      zone_key_z(r) == zlayer::Underground);

    auto w = parent_of(r);                       // Region -> World
    CHECK("parent is world",  zone_key_type(w) == ZoneType::World);
    CHECK("world parent root", parent_of(w) == ZONE_ROOT);
    return true;
}

static bool test_blocking_roundtrip() {
    entt::registry src;
    auto e = src.create();
    src.emplace<Blocking>(e, false, true);   // blocks_move=false、blocks_sight=true

    std::stringstream ss;
    zone_io::save(src, ss);
    entt::registry dst;
    zone_io::load(dst, ss);

    bool ok = false;
    for (auto en : dst.view<Blocking>()) {
        auto& b = dst.get<Blocking>(en);
        ok = (!b.blocks_move && b.blocks_sight);
    }
    CHECK("blocking round-trip", ok);
    return true;
}

static bool test_area_terrain_roundtrip() {
    entt::registry src;
    auto m = src.create();
    auto& at = src.emplace<AreaTerrain>(m);
    at.tiles.alloc(3, 3);
    at.tiles.set(1, 2, Tile{7, TILE_WALKABLE});

    std::stringstream ss;
    zone_io::save(src, ss);
    entt::registry dst;
    zone_io::load(dst, ss);

    bool ok = false;
    for (auto e : dst.view<AreaTerrain>()) {
        auto& g = dst.get<AreaTerrain>(e);
        Tile t = g.tiles.getval(1, 2);
        ok = (g.tiles.sx == 3 && g.tiles.sy == 3
              && t.terrain == 7 && (t.flags & TILE_WALKABLE));
    }
    CHECK("area terrain grid round-trip", ok);
    return true;
}

static bool test_serialize_empty() {
    entt::registry src;
    std::stringstream ss;
    zone_io::save(src, ss);

    entt::registry dst;
    zone_io::load(dst, ss);
    CHECK("empty registry", dst.storage<entt::entity>().size() == 0u);
    return true;
}

// ---- 執行器 ----

int main() {
    struct { const char* name; bool(*fn)(); } cases[] = {
        { "zone_key_roundtrip",  test_zone_key_roundtrip  },
        { "zone_key_root",       test_zone_key_root       },
        { "serialize_roundtrip",        test_serialize_roundtrip        },
        { "serialize_orphans_removed",  test_serialize_orphans_removed  },
        { "zone_meta_placeholder",      test_zone_meta_placeholder_survives },
        { "zone_path_deterministic",    test_zone_path_deterministic    },
        { "save_load_root",             test_save_load_root             },
        { "world_config_persists",      test_world_config_persists      },
        { "world_dim_bounds",           test_world_dim_bounds           },
        { "position_roundtrip",         test_position_roundtrip         },
        { "tick_per_loaded_zone",       test_tick_runs_per_loaded_zone  },
        { "tick_system_order",          test_tick_system_order          },
        { "zone_layers_and_parent",     test_zone_layers_and_parent     },
        { "blocking_roundtrip",         test_blocking_roundtrip         },
        { "area_terrain_roundtrip",     test_area_terrain_roundtrip     },
        { "serialize_empty",            test_serialize_empty            },
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
