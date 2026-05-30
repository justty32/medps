#include "global_manager.h"
#include "serialize/zone_io.h"
#include "components/zone_meta.h"
#include <sstream>
#include <cassert>

GlobalManager::GlobalManager()
    : GlobalManager(std::make_unique<FolderZoneStore>("zones")) {}

GlobalManager::GlobalManager(std::unique_ptr<ZoneStore> store)
    : store_(std::move(store)) {}

entt::registry* GlobalManager::get(ZoneKey key) {
    auto it = loaded_.find(key);
    return (it != loaded_.end()) ? it->second.get() : nullptr;
}

void GlobalManager::write_zone(ZoneKey key, entt::registry& reg) {
    std::ostringstream oss;
    zone_io::save(reg, oss);
    store_->write(key, oss.str());
}

entt::registry& GlobalManager::create(ZoneKey key, ZoneKey parent) {
    auto [it, inserted] = loaded_.emplace(key, std::make_unique<entt::registry>());
    auto& reg = *it->second;
    if (inserted) {
        auto e = reg.create();                       // placeholder；可在 orphans() 中存活
        reg.emplace<ZoneMeta>(e, key, parent);
    }
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

WorldConfig& GlobalManager::init_world(int16_t world_dim_x, int16_t world_dim_y,
                                       int16_t world_dim_z) {
    assert(zone_scale::valid_world_dim(world_dim_x)
           && zone_scale::valid_world_dim(world_dim_y)
           && "world_dim overflows the 16-bit ZoneKey x/y tile grid");
    auto v = root.view<WorldConfig>();
    entt::entity e = v.empty() ? root.create() : v.front();
    auto& cfg = root.get_or_emplace<WorldConfig>(e);
    cfg.world_dim_x = world_dim_x;
    cfg.world_dim_y = world_dim_y;
    cfg.world_dim_z = world_dim_z;
    return cfg;
}

WorldConfig GlobalManager::world_config() const {
    auto v = root.view<const WorldConfig>();
    if (v.empty()) return WorldConfig{};
    return v.get<const WorldConfig>(v.front());
}

void GlobalManager::add_zone_system(ZoneSystem sys) {
    zone_systems_.push_back(std::move(sys));
}

void GlobalManager::tick() {
    for (auto& [key, reg] : loaded_)
        for (auto& sys : zone_systems_)
            sys(*reg);
}
