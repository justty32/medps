#include <gcore/zone_key.h>
#include <gcore/chunk_key.h>
#include <gcore/serialize/chunked_zone_store.h>
#include <gcore/components/cross_zone_ref.h>
#include <gcore/components/zone_meta.h>
#include <gcore/components/child_zone_summary.h>
#include <gcore/components/position.h>
#include <gcore/components/velocity.h>
#include <gcore/components/owner.h>
#include <gcore/systems/movement.h>
#include <gcore/serialize/zone_io.h>
#include <gcore/serialize/zone_store.h>
#include <gcore/global_manager.h>

#include <cassert>
#include <sstream>
#include <cstdio>
#include <algorithm>
#include <vector>
#include <filesystem>
#include <memory>

static bool contains(const std::vector<ZoneKey>& v, ZoneKey k) {
    return std::find(v.begin(), v.end(), k) != v.end();
}

// ---- helpers ----

static void pass(const char* name) { std::printf("  [PASS] %s\n", name); }
static void fail(const char* name) { std::printf("  [FAIL] %s\n", name); }

#define CHECK(name, expr) do { if (expr) pass(name); else { fail(name); return false; } } while(0)

// ---- tests ----

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
    src.emplace<CrossZoneRef>(e1, ZoneKey{42}, entt::entity{7});

    auto e2 = src.create();
    // e2 has no CrossZoneRef

    auto e3 = src.create();
    src.emplace<CrossZoneRef>(e3, ZONE_ROOT, entt::null);

    std::stringstream ss;
    zone_io::save(src, ss);

    entt::registry dst;
    zone_io::load(dst, ss);

    // e1 should have CrossZoneRef with zone=42, local=7
    auto view = dst.view<CrossZoneRef>();
    int count = 0;
    bool e1_ok = false, e3_ok = false;
    for (auto e : view) {
        auto& ref = view.get<CrossZoneRef>(e);
        if (ref.zone == ZoneKey{42} && ref.local_entity == entt::entity{7}) e1_ok = true;
        if (ref.zone == ZONE_ROOT   && ref.local_entity == entt::null)      e3_ok = true;
        ++count;
    }

    CHECK("component count", count == 2);
    CHECK("e1 data",         e1_ok);
    CHECK("e3 data",         e3_ok);
    return true;
}

static bool test_serialize_orphans_removed() {
    // entities without any component-in-AllComponents are dropped by loader.orphans()
    entt::registry src;
    auto e_empty = src.create();        // no component, will be orphan after load
    auto e_with  = src.create();
    src.emplace<CrossZoneRef>(e_with, ZoneKey{1}, entt::entity{2});

    std::stringstream ss;
    zone_io::save(src, ss);

    entt::registry dst;
    zone_io::load(dst, ss);

    // snapshot_loader preserves entity values; orphans() destroys e_empty
    CHECK("orphan removed",  !dst.valid(e_empty));
    CHECK("non-orphan kept",  dst.valid(e_with));
    return true;
}

static bool test_zone_meta_placeholder_survives() {
    // a zone whose only entity is a ZoneMeta placeholder must survive round-trip
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

static bool test_resolve_root() {
    GlobalManager gm;
    auto e = gm.root.create();

    CrossZoneRef ref{ZONE_ROOT, e};
    auto res = gm.resolve(ref);

    CHECK("root reg",    res.reg == &gm.root);
    CHECK("root entity", res.entity == e);
    CHECK("root valid",  res.valid());
    return true;
}

static bool test_resolve_unloaded_zone() {
    GlobalManager gm;
    CrossZoneRef ref{make_zone_key(ZoneType{1}, 5, 6, 0), entt::entity{0}};
    auto res = gm.resolve(ref);

    CHECK("unloaded reg null", res.reg == nullptr);
    CHECK("unloaded invalid",  !res.valid());
    return true;
}

static bool test_resolve_loaded_zone() {
    GlobalManager gm;
    auto key = make_zone_key(ZoneType{1}, 5, 6, 0);
    auto& zone = gm.create(key, ZONE_ROOT);

    auto e = zone.create();
    zone.emplace<ZoneMeta>(e, key);

    CrossZoneRef ref{key, e};
    auto res = gm.resolve(ref);

    CHECK("loaded reg",     res.reg == &zone);
    CHECK("loaded valid",   res.valid());

    // stale entity in a loaded zone: reg present, but not valid
    CrossZoneRef stale{key, entt::entity{9999}};
    auto sres = gm.resolve(stale);
    CHECK("stale reg present", sres.reg == &zone);
    CHECK("stale not valid",   !sres.valid());
    return true;
}

static bool test_child_index_flat() {
    GlobalManager gm;
    auto a1 = make_zone_key(ZoneType{1}, 1, 0, 0);
    auto a2 = make_zone_key(ZoneType{1}, 2, 0, 0);

    gm.create(a1, ZONE_ROOT);
    gm.create(a2, ZONE_ROOT);
    gm.create(a1, ZONE_ROOT);   // idempotent: must not duplicate stub

    auto kids = gm.children(ZONE_ROOT);
    CHECK("two children",  kids.size() == 2);
    CHECK("has a1",        contains(kids, a1));
    CHECK("has a2",        contains(kids, a2));
    return true;
}

static bool test_child_index_hierarchy() {
    GlobalManager gm;
    auto province = make_zone_key(ZoneType{1}, 0, 0, 0);
    auto area     = make_zone_key(ZoneType{2}, 3, 4, 0);

    gm.create(province, ZONE_ROOT);
    gm.create(area, province);

    auto root_kids     = gm.children(ZONE_ROOT);
    auto province_kids = gm.children(province);

    CHECK("root has province",     contains(root_kids, province));
    CHECK("province has area",     contains(province_kids, area));
    CHECK("area not under root",  !contains(root_kids, area));
    return true;
}

static bool test_child_overview_without_loading() {
    // overview a child that exists but is not loaded: parent's stub still lists it
    GlobalManager gm;
    auto province = make_zone_key(ZoneType{1}, 7, 0, 0);
    auto area     = make_zone_key(ZoneType{2}, 8, 0, 0);
    gm.create(province, ZONE_ROOT);
    gm.create(area, province);

    // simulate area not being loaded by checking parent's index independently
    auto kids = gm.children(province);
    CHECK("area listed", contains(kids, area));
    // area registry exists here because create() loads it; the point is that
    // children() reads ONLY the parent's stub, never the area registry.
    return true;
}

static bool test_child_summary_roundtrip() {
    // a parent zone's child stubs must survive serialization
    entt::registry parent;
    auto ph = parent.create();
    parent.emplace<ZoneMeta>(ph, ZoneKey{100}, ZONE_ROOT);
    auto stub = parent.create();
    parent.emplace<ChildZoneSummary>(stub, ZoneKey{200});

    std::stringstream ss;
    zone_io::save(parent, ss);

    entt::registry dst;
    zone_io::load(dst, ss);

    bool found = false;
    for (auto e : dst.view<ChildZoneSummary>())
        if (dst.get<ChildZoneSummary>(e).key == ZoneKey{200}) found = true;
    CHECK("child stub survived", found);
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

    {   // session 1: build a game and checkpoint it
        GlobalManager gm{std::make_unique<FolderZoneStore>(dir)};
        auto e = gm.root.create();
        gm.root.emplace<ZoneMeta>(e, ZONE_ROOT, ZONE_ROOT);
        gm.create(child, ZONE_ROOT);   // registers a child stub in root
        gm.save_all();
    }

    bool ok = true;
    {   // session 2: reopen and verify root + its child index persisted
        GlobalManager gm2{std::make_unique<FolderZoneStore>(dir)};
        gm2.load_root();

        auto kids = gm2.children(ZONE_ROOT);
        if (!contains(kids, child)) ok = false;
        // child zone is NOT loaded yet (streamed on demand)
        if (gm2.get(child) != nullptr) ok = false;
    }

    std::filesystem::remove_all(dir);
    CHECK("root + child index persisted, child not auto-loaded", ok);
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

static bool test_owner_resolves_to_root_faction() {
    GlobalManager gm;
    // faction is a global entity in root
    auto faction = gm.root.create();

    // a unit lives in an area zone, owned by that faction (cross-zone ref)
    auto areaKey = make_zone_key(ZoneType{2}, 1, 1, 0);
    auto& area = gm.create(areaKey, ZONE_ROOT);
    auto unit = area.create();
    area.emplace<Position>(unit, 5, 6);
    area.emplace<Owner>(unit, CrossZoneRef{ZONE_ROOT, faction});

    // resolve the unit's owner back to the root faction entity
    auto& own = area.get<Owner>(unit);
    auto res = gm.resolve(own.faction);

    CHECK("owner resolves to root", res.reg == &gm.root);
    CHECK("owner entity is faction", res.entity == faction);
    CHECK("owner valid",             res.valid());
    return true;
}

static bool test_owner_roundtrip() {
    // Owner (wrapping a CrossZoneRef) must survive serialization
    entt::registry src;
    auto e = src.create();
    src.emplace<Owner>(e, CrossZoneRef{ZONE_ROOT, entt::entity{77}});

    std::stringstream ss;
    zone_io::save(src, ss);

    entt::registry dst;
    zone_io::load(dst, ss);

    bool ok = false;
    for (auto en : dst.view<Owner>()) {
        auto& o = dst.get<Owner>(en);
        ok = (o.faction.zone == ZONE_ROOT && o.faction.local_entity == entt::entity{77});
    }
    CHECK("owner ref survived", ok);
    return true;
}

static bool test_tick_runs_per_loaded_zone() {
    GlobalManager gm;
    gm.add_zone_system(systems::movement);

    // two loaded zones, each with a moving actor
    auto& z1 = gm.create(make_zone_key(ZoneType{1}, 1, 0, 0), ZONE_ROOT);
    auto e1 = z1.create();
    z1.emplace<Position>(e1, 0, 0);
    z1.emplace<Velocity>(e1, 1, 2);

    auto& z2 = gm.create(make_zone_key(ZoneType{1}, 2, 0, 0), ZONE_ROOT);
    auto e2 = z2.create();
    z2.emplace<Position>(e2, 10, 10);
    z2.emplace<Velocity>(e2, -1, 0);

    // an actor in root should NOT be ticked by per-zone systems
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
    // systems run in registration order against the same zone
    GlobalManager gm;
    auto& z = gm.create(make_zone_key(ZoneType{1}, 3, 0, 0), ZONE_ROOT);
    auto e = z.create();
    z.emplace<Position>(e, 0, 0);

    gm.add_zone_system([](entt::registry& r){
        r.view<Position>().each([](Position& p){ p.x += 1; });   // first: +1
    });
    gm.add_zone_system([](entt::registry& r){
        r.view<Position>().each([](Position& p){ p.x *= 10; });  // then: *10
    });

    gm.tick();
    CHECK("order is +1 then *10", z.get<Position>(e).x == 10);
    return true;
}

static bool test_zone_layers_and_parent() {
    using namespace zone_scale;
    // an Area built from world-tile (7,8) + region-local (3,4), underground layer
    auto a = area_key(7, 8, 3, 4, zlayer::Underground);
    CHECK("area type",  zone_key_type(a) == ZoneType::Area);
    CHECK("area gx",    zone_key_x(a) == 7 * REGION_DIM + 3);
    CHECK("area gy",    zone_key_y(a) == 8 * REGION_DIM + 4);

    auto r = parent_of(a);                       // Area -> Region by integer division
    CHECK("parent is region", zone_key_type(r) == ZoneType::Region);
    CHECK("region wx",        zone_key_x(r) == 7);
    CHECK("region wy",        zone_key_y(r) == 8);
    CHECK("z preserved",      zone_key_z(r) == zlayer::Underground);

    auto w = parent_of(r);                       // Region -> World
    CHECK("parent is world",  zone_key_type(w) == ZoneType::World);
    CHECK("world parent root", parent_of(w) == ZONE_ROOT);
    return true;
}

static bool test_chunk_key_grouping() {
    using namespace zone_scale;
    auto r_a = region_key(10, 10);
    auto r_b = region_key(12, 11);   // same 5×5 chunk as r_a
    auto r_c = region_key(20, 20);   // different chunk
    CHECK("same chunk",   chunk_key_of(r_a) == chunk_key_of(r_b));
    CHECK("diff chunk",   chunk_key_of(r_a) != chunk_key_of(r_c));
    CHECK("chunk coord",  zone_key_x(chunk_key_of(r_a)) == 10 / REGION_CHUNK);
    // Area is 1:1 -> its own chunk
    auto a = area_key(3, 4, 1, 2);
    CHECK("area self-chunk", chunk_key_of(a) == a);
    return true;
}

static bool test_chunked_store_packs_zones() {
    auto dir = std::filesystem::temp_directory_path() / "medps_test_chunk";
    std::filesystem::remove_all(dir);

    auto r_a = region_key(10, 10);
    auto r_b = region_key(12, 11);   // lands in the same chunk as r_a
    bool ok = (chunk_key_of(r_a) == chunk_key_of(r_b));   // precondition of this test

    {   // write two zones that share a chunk
        ChunkedFolderZoneStore s{dir};
        s.write(r_a, "alpha");
        s.write(r_b, "bravo");
        s.flush();
    }

    // both zones packed into ONE chunk file (the whole point of Plan B)
    int chunk_files = 0;
    if (std::filesystem::exists(dir))
        for (auto& e : std::filesystem::directory_iterator(dir))
            if (e.path().extension() == ".chunk") ++chunk_files;
    if (chunk_files != 1) ok = false;

    {   // reopen: each zone round-trips independently
        ChunkedFolderZoneStore s2{dir};
        auto a = s2.read(r_a);
        auto b = s2.read(r_b);
        if (!a || *a != "alpha") ok = false;
        if (!b || *b != "bravo") ok = false;
        s2.write(r_a, "ALPHA2");          // overwrite one zone in the chunk
    }
    {   // partial update must not clobber the other zone in the same chunk
        ChunkedFolderZoneStore s3{dir};
        auto a = s3.read(r_a);
        auto b = s3.read(r_b);
        if (!a || *a != "ALPHA2") ok = false;
        if (!b || *b != "bravo")  ok = false;
        if (s3.has(region_key(99, 99))) ok = false;   // absent zone -> not present
    }

    std::filesystem::remove_all(dir);
    CHECK("two zones share one chunk file, independent round-trip + partial update", ok);
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

// ---- runner ----

int main() {
    struct { const char* name; bool(*fn)(); } cases[] = {
        { "zone_key_roundtrip",  test_zone_key_roundtrip  },
        { "zone_key_root",       test_zone_key_root       },
        { "serialize_roundtrip",        test_serialize_roundtrip        },
        { "serialize_orphans_removed",  test_serialize_orphans_removed  },
        { "zone_meta_placeholder",      test_zone_meta_placeholder_survives },
        { "resolve_root",               test_resolve_root               },
        { "resolve_unloaded_zone",      test_resolve_unloaded_zone      },
        { "resolve_loaded_zone",        test_resolve_loaded_zone        },
        { "child_index_flat",           test_child_index_flat           },
        { "child_index_hierarchy",      test_child_index_hierarchy      },
        { "child_overview",             test_child_overview_without_loading },
        { "child_summary_roundtrip",    test_child_summary_roundtrip    },
        { "zone_path_deterministic",    test_zone_path_deterministic    },
        { "save_load_root",             test_save_load_root             },
        { "position_roundtrip",         test_position_roundtrip         },
        { "owner_resolves_faction",     test_owner_resolves_to_root_faction },
        { "owner_roundtrip",            test_owner_roundtrip            },
        { "tick_per_loaded_zone",       test_tick_runs_per_loaded_zone  },
        { "tick_system_order",          test_tick_system_order          },
        { "zone_layers_and_parent",     test_zone_layers_and_parent     },
        { "chunk_key_grouping",         test_chunk_key_grouping         },
        { "chunked_store_packs_zones",  test_chunked_store_packs_zones  },
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
