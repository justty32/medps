#include <gcore/zone_key.h>
#include <gcore/components/cross_zone_ref.h>
#include <gcore/components/zone_meta.h>
#include <gcore/serialize/zone_io.h>

#include <cassert>
#include <sstream>
#include <cstdio>

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
