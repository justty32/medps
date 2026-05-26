#include "global_manager.h"
#include "serialize/zone_io.h"
#include "components/zone_meta.h"

entt::registry* GlobalManager::get(ZoneKey key) {
    auto it = loaded_.find(key);
    return (it != loaded_.end()) ? it->second.get() : nullptr;
}

entt::registry& GlobalManager::create(ZoneKey key, const std::filesystem::path& path) {
    auto [it, inserted] = loaded_.emplace(key, std::make_unique<entt::registry>());
    if (inserted) {
        paths_[key] = path;
        // placeholder entity: keeps the zone alive across save/load (orphans()).
        auto e = it->second->create();
        it->second->emplace<ZoneMeta>(e, key);
    }
    return *it->second;
}

entt::registry& GlobalManager::load(ZoneKey key, const std::filesystem::path& path) {
    auto [it, inserted] = loaded_.emplace(key, std::make_unique<entt::registry>());
    if (inserted) {
        paths_[key] = path;
        zone_io::load(*it->second, path);
        // NOTE: load() is for zones with an existing save file. For a brand-new
        // zone use create(), which seeds the required ZoneMeta placeholder so the
        // zone survives snapshot_loader::orphans() (see zone_io.h).
    }
    return *it->second;
}

void GlobalManager::unload(ZoneKey key) {
    auto it = loaded_.find(key);
    if (it == loaded_.end()) return;
    if (auto pit = paths_.find(key); pit != paths_.end())
        zone_io::save(*it->second, pit->second);
    loaded_.erase(it);
}

entt::registry& GlobalManager::get_or_load(ZoneKey key, const std::filesystem::path& path) {
    if (auto* reg = get(key)) return *reg;
    return load(key, path);
}
