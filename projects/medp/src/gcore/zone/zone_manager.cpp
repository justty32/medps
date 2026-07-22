#include "zone_manager.h"
#include "../serialize/zone_io.h"
#include <cereal/archives/portable_binary.hpp>
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <string>

ZoneManager::ZoneManager(std::filesystem::path dir)
    : dir_(std::move(dir)) {
    const auto manifest = dir_ / "manifest.bin";
    if (std::filesystem::exists(manifest)) {
        {
            std::ifstream ifs{manifest, std::ios::binary};
            cereal::PortableBinaryInputArchive ar{ifs};
            ar(next_id_);
        }
        if (!load(ZONE_ROOT))
            throw std::runtime_error(
                "ZoneManager: 存檔目錄有 manifest 但缺 root.bin（存檔損毀）: " + dir_.string());
    } else {
        if (std::filesystem::exists(dir_))
            for (const auto& e : std::filesystem::directory_iterator(dir_))
                if (e.path().extension() == ".bin")
                    throw std::runtime_error(
                        "ZoneManager: 目錄有 zone 檔卻無 manifest，不當新世界處理（疑似損毀）: " +
                        e.path().string());
        emplace_zone(ZONE_ROOT, ZONE_ROOT, ZoneKind::Plain);   // 全新世界；root 的 parent 是它自己
    }
}

Zone* ZoneManager::get(ZoneId id) {
    auto it = zones_.find(id);
    return (it != zones_.end()) ? it->second.get() : nullptr;
}

Zone& ZoneManager::emplace_zone(ZoneId id, ZoneId parent, ZoneKind kind) {
    auto [it, inserted] = zones_.try_emplace(id);
    if (inserted) {
        it->second = make_zone(kind);
        it->second->id     = id;
        it->second->parent = parent;
    }
    return *it->second;
}

Zone& ZoneManager::create_child(ZoneId parent, ZoneKind kind) {
    if (!zones_.count(parent))
        throw std::runtime_error(
            "ZoneManager::create_child: parent 未載入: parent=" + std::to_string(parent));
    const ZoneId id = next_id_++;
    write_manifest();
    if (std::filesystem::exists(path(id)))
        throw std::runtime_error(
            "ZoneManager::create_child: 配發的 id 在磁碟上已有檔案（manifest 損毀/回退？）: id=" +
            std::to_string(id));
    return emplace_zone(id, parent, kind);
}

void ZoneManager::destroy(ZoneId id) {
    if (id == ZONE_ROOT) return;
    zones_.erase(id);
    std::filesystem::remove(path(id));
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

void ZoneManager::write_manifest() {
    std::filesystem::create_directories(dir_);
    const auto tmp = dir_ / "manifest.bin.tmp";
    {
        std::ofstream ofs{tmp, std::ios::binary};
        cereal::PortableBinaryOutputArchive ar{ofs};
        ar(next_id_);
    }
    // rename 原子替換：撕裂寫入不會留下半套 manifest
    std::filesystem::rename(tmp, dir_ / "manifest.bin");
}

void ZoneManager::save_all() {
    for (auto& [id, zone] : zones_)
        write(*zone);
    write_manifest();
}

bool ZoneManager::load(ZoneId id) {
    if (zones_.count(id)) return true;   // 已在記憶體中

    auto p = path(id);
    if (!std::filesystem::exists(p)) return false;

    std::ifstream ifs{p, std::ios::binary};
    auto zone = zone_io::load(ifs);      // kind 由檔頭 tag 決定子類；id / parent 由檔案內容還原
    if (zone->id != id)
        throw std::runtime_error(
            "ZoneManager::load: 檔案內容 id 與請求不符（存檔損毀）: 請求=" + std::to_string(id) +
            " 檔內=" + std::to_string(zone->id));
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
