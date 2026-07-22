#include "zone_manager.h"
#include "../serialize/zone_io.h"
#include <cstdio>
#include <fstream>

ZoneManager::ZoneManager(std::filesystem::path dir)
    : dir_(std::move(dir)) {
    create(ZONE_ROOT, ZONE_ROOT);   // root 永遠存在，parent 是它自己
}

Zone* ZoneManager::get(ZoneId id) {
    auto it = zones_.find(id);
    return (it != zones_.end()) ? it->second.get() : nullptr;
}

Zone& ZoneManager::create(ZoneId id, ZoneId parent) {
    auto [it, inserted] = zones_.try_emplace(id);
    if (inserted) {
        it->second = std::make_unique<Zone>();
        it->second->id     = id;
        it->second->parent = parent;
    }
    return *it->second;
}

void ZoneManager::destroy(ZoneId id) {
    if (id == ZONE_ROOT) return;
    zones_.erase(id);
}

std::filesystem::path ZoneManager::path(ZoneId id) const {
    if (id == ZONE_ROOT) return dir_ / "root.bin";
    char buf[17];
    std::snprintf(buf, sizeof(buf), "%016llx", static_cast<unsigned long long>(id));
    return dir_ / (std::string(buf) + ".bin");
}

void ZoneManager::write(Zone& z) {
    std::filesystem::create_directories(dir_);
    std::ofstream ofs{path(z.id), std::ios::binary};
    zone_io::save(z, ofs);
}

void ZoneManager::save_all() {
    for (auto& [id, zone] : zones_)
        write(*zone);
}

bool ZoneManager::load(ZoneId id) {
    if (zones_.count(id)) return true;   // 已在記憶體中

    auto p = path(id);
    if (!std::filesystem::exists(p)) return false;

    auto zone = std::make_unique<Zone>();
    std::ifstream ifs{p, std::ios::binary};
    zone_io::load(*zone, ifs);           // id / parent 由檔案內容還原
    zones_.emplace(id, std::move(zone));
    return true;
}

void ZoneManager::unload(ZoneId id) {
    if (id == ZONE_ROOT) return;
    auto it = zones_.find(id);
    if (it == zones_.end()) return;
    write(*it->second);
    zones_.erase(it);
}

void ZoneManager::add_zone_system(ZoneSystem sys) {
    systems_.push_back(std::move(sys));
}

void ZoneManager::tick() {
    for (auto& [id, zone] : zones_)
        for (auto& sys : systems_)
            sys(*zone);
}
