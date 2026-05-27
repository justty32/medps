#include "global_manager.h"
#include "serialize/zone_io.h"
#include "serialize/chunked_zone_store.h"
#include "components/zone_meta.h"
#include "components/child_zone_summary.h"
#include <sstream>

GlobalManager::GlobalManager()
    : GlobalManager(std::make_unique<ChunkedFolderZoneStore>("zones")) {}

GlobalManager::GlobalManager(std::unique_ptr<ZoneStore> store)
    : store_(std::move(store)) {}

entt::registry* GlobalManager::get(ZoneKey key) {
    auto it = loaded_.find(key);
    return (it != loaded_.end()) ? it->second.get() : nullptr;
}

ZoneResolution GlobalManager::resolve(const CrossZoneRef& ref) {
    entt::registry* reg = (ref.zone == ZONE_ROOT) ? &root : get(ref.zone);
    return ZoneResolution{ reg, ref.local_entity };
}

void GlobalManager::write_zone(ZoneKey key, entt::registry& reg) {
    std::ostringstream oss;
    zone_io::save(reg, oss);
    store_->write(key, oss.str());
}

entt::registry* GlobalManager::parent_registry(ZoneKey parent) {
    return (parent == ZONE_ROOT) ? &root : get(parent);
}

void GlobalManager::ensure_child_stub(entt::registry& parent, ZoneKey child) {
    for (auto e : parent.view<ChildZoneSummary>())
        if (parent.get<ChildZoneSummary>(e).key == child) return;  // already indexed
    auto stub = parent.create();
    parent.emplace<ChildZoneSummary>(stub, child);
}

entt::registry& GlobalManager::create(ZoneKey key, ZoneKey parent) {
    auto [it, inserted] = loaded_.emplace(key, std::make_unique<entt::registry>());
    auto& reg = *it->second;
    if (inserted) {
        auto e = reg.create();                       // placeholder; survives orphans()
        reg.emplace<ZoneMeta>(e, key, parent);
    }
    if (auto* preg = parent_registry(parent))        // index into parent if it's loaded
        ensure_child_stub(*preg, key);
    return reg;
}

entt::registry& GlobalManager::load(ZoneKey key) {
    auto [it, inserted] = loaded_.emplace(key, std::make_unique<entt::registry>());
    if (inserted) {
        if (auto bytes = store_->read(key)) {
            std::istringstream iss{*bytes};
            zone_io::load(*it->second, iss);
        }
    }
    return *it->second;
}

void GlobalManager::unload(ZoneKey key) {
    auto it = loaded_.find(key);
    if (it == loaded_.end()) return;
    write_zone(key, *it->second);
    loaded_.erase(it);
}

void GlobalManager::prefetch(ZoneKey key) {
    for (ZoneKey k : store_->group_of(key))
        load(k);                       // idempotent: skips already-loaded
}

void GlobalManager::stream_around(ZoneKey center, int radius) {
    const ZoneType t  = zone_key_type(center);
    const int16_t  z  = zone_key_z(center);
    const int16_t  cx = zone_key_x(center);
    const int16_t  cy = zone_key_y(center);

    // load every persisted zone inside the window (skip off-world negatives;
    // a zone not on disk is simply absent -- generation-on-demand is game logic)
    for (int dy = -radius; dy <= radius; ++dy)
        for (int dx = -radius; dx <= radius; ++dx) {
            int nx = cx + dx, ny = cy + dy;
            if (nx < 0 || ny < 0) continue;
            ZoneKey nk = make_zone_key(t, static_cast<int16_t>(nx),
                                          static_cast<int16_t>(ny), z);
            if (store_->has(nk)) load(nk);
        }

    // evict loaded same-layer zones that fell outside the window
    std::vector<ZoneKey> victims;
    for (auto& [k, reg] : loaded_) {
        if (zone_key_type(k) != t || zone_key_z(k) != z) continue;
        int dx = zone_key_x(k) - cx, dy = zone_key_y(k) - cy;
        if (dx < -radius || dx > radius || dy < -radius || dy > radius)
            victims.push_back(k);
    }
    for (ZoneKey k : victims) unload(k);
}

std::vector<ZoneKey> GlobalManager::children(ZoneKey parent) {
    std::vector<ZoneKey> out;
    if (auto* preg = parent_registry(parent))
        for (auto e : preg->view<ChildZoneSummary>())
            out.push_back(preg->get<ChildZoneSummary>(e).key);
    return out;
}

void GlobalManager::save_all() {
    write_zone(ZONE_ROOT, root);
    for (auto& [key, reg] : loaded_)
        write_zone(key, *reg);
    store_->flush();
}

void GlobalManager::load_root() {
    if (auto bytes = store_->read(ZONE_ROOT)) {
        std::istringstream iss{*bytes};
        zone_io::load(root, iss);
    }
}

void GlobalManager::add_zone_system(ZoneSystem sys) {
    zone_systems_.push_back(std::move(sys));
}

void GlobalManager::tick() {
    for (auto& [key, reg] : loaded_)
        for (auto& sys : zone_systems_)
            sys(*reg);
}
