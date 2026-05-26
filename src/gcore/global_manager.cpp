#include "global_manager.h"
#include "serialize/zone_io.h"
#include "components/zone_meta.h"
#include "components/child_zone_summary.h"
#include <cstdio>

entt::registry* GlobalManager::get(ZoneKey key) {
    auto it = loaded_.find(key);
    return (it != loaded_.end()) ? it->second.get() : nullptr;
}

ZoneResolution GlobalManager::resolve(const CrossZoneRef& ref) {
    entt::registry* reg = (ref.zone == ZONE_ROOT) ? &root : get(ref.zone);
    return ZoneResolution{ reg, ref.local_entity };
}

std::filesystem::path GlobalManager::zone_path(ZoneKey key) const {
    char buf[17];
    std::snprintf(buf, sizeof(buf), "%016llx", static_cast<unsigned long long>(key));
    return zones_dir / (std::string(buf) + ".bin");
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
    if (inserted)
        zone_io::load(*it->second, zone_path(key));
    return *it->second;
}

void GlobalManager::unload(ZoneKey key) {
    auto it = loaded_.find(key);
    if (it == loaded_.end()) return;
    zone_io::save(*it->second, zone_path(key));
    loaded_.erase(it);
}

std::vector<ZoneKey> GlobalManager::children(ZoneKey parent) {
    std::vector<ZoneKey> out;
    if (auto* preg = parent_registry(parent))
        for (auto e : preg->view<ChildZoneSummary>())
            out.push_back(preg->get<ChildZoneSummary>(e).key);
    return out;
}

std::filesystem::path GlobalManager::root_path() const {
    return zones_dir / "root.bin";
}

void GlobalManager::save_all() {
    zone_io::save(root, root_path());
    for (auto& [key, reg] : loaded_)
        zone_io::save(*reg, zone_path(key));
}

void GlobalManager::load_root() {
    zone_io::load(root, root_path());
}
