#pragma once
#include <unordered_map>
#include <memory>
#include <vector>
#include <filesystem>
#include <entt.hpp>
#include "zone_key.h"
#include "components/cross_zone_ref.h"

// Result of resolving a CrossZoneRef against a GlobalManager.
//   reg == nullptr           -> target zone is not loaded
//   reg != nullptr && !valid -> zone loaded but the entity is stale/gone
//   valid()                  -> fully usable (reg + entity)
struct ZoneResolution {
    entt::registry* reg{nullptr};
    entt::entity    entity{entt::null};

    bool valid() const { return reg && reg->valid(entity); }
    explicit operator bool() const { return valid(); }
};

class GlobalManager {
public:
    entt::registry        root;                 // ZONE_ROOT, always live
    std::filesystem::path zones_dir{"zones"};   // on-disk location for zone files

    // nullptr if not loaded
    entt::registry* get(ZoneKey key);

    // resolve a cross-zone reference to {registry, entity}. Only consults
    // already-loaded zones (does no disk IO); ZONE_ROOT maps to `root`.
    ZoneResolution resolve(const CrossZoneRef& ref);

    // create a brand-new zone under `parent`, seed its ZoneMeta placeholder, and
    // register a ChildZoneSummary stub in the parent registry (if parent loaded).
    // Idempotent: re-creating an existing zone won't duplicate the stub.
    entt::registry& create(ZoneKey key, ZoneKey parent);

    // deserialize a zone from its derived path; if already loaded returns existing.
    entt::registry& load(ZoneKey key);

    // serialize a zone to its derived path and drop it from memory.
    void unload(ZoneKey key);

    // list direct child zones of a (loaded) parent WITHOUT loading them.
    // returns empty if the parent zone is not loaded.
    std::vector<ZoneKey> children(ZoneKey parent);

    // deterministic on-disk path for a zone (no clash: ZoneKey is unique).
    std::filesystem::path zone_path(ZoneKey key) const;

    // fixed path of the root registry (the top of a whole-game save).
    std::filesystem::path root_path() const;

    // ---- whole-game save / load ----

    // checkpoint: write root + every currently-loaded zone to disk (no eviction).
    void save_all();

    // open a game: load the root registry. Sub-zones stay on disk until streamed
    // in on demand via load()/create().
    void load_root();

private:
    entt::registry* parent_registry(ZoneKey parent);          // ZONE_ROOT -> &root
    void ensure_child_stub(entt::registry& parent, ZoneKey child);

    std::unordered_map<ZoneKey, std::unique_ptr<entt::registry>> loaded_;
};
