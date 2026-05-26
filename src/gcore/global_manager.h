#pragma once
#include <unordered_map>
#include <memory>
#include <filesystem>
#include <entt.hpp>
#include "zone_key.h"

class GlobalManager {
public:
    entt::registry root;

    // nullptr if not loaded
    entt::registry* get(ZoneKey key);

    // create a brand-new zone with a placeholder (ZoneMeta) entity and register
    // its save path. If the zone is already loaded, returns the existing one.
    entt::registry& create(ZoneKey key, const std::filesystem::path& path);

    // deserialize from path and add to loaded; if already loaded returns existing
    entt::registry& load(ZoneKey key, const std::filesystem::path& path);

    // serialize to disk and remove from loaded
    void unload(ZoneKey key);

    entt::registry& get_or_load(ZoneKey key, const std::filesystem::path& path);

private:
    std::unordered_map<ZoneKey, std::unique_ptr<entt::registry>> loaded_;
    std::unordered_map<ZoneKey, std::filesystem::path>           paths_;
};
