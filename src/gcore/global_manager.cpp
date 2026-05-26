#include "global_manager.h"
#include "serialize/zone_io.h"
#include "components/zone_meta.h"
#include "components/child_zone_summary.h"
#include <sstream>

GlobalManager::GlobalManager()
    : GlobalManager(std::make_unique<FolderZoneStore>("zones")) {}

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
