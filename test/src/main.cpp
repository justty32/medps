#include <gcore/zone_key.h>
#include <gcore/components/cross_zone_ref.h>
#include <gcore/components/zone_meta.h>
#include <gcore/components/child_zone_summary.h>
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
