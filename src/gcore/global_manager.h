#pragma once
#include <unordered_map>
#include <memory>
#include <vector>
#include <functional>
#include <entt.hpp>
#include "zone_key.h"
#include "components/cross_zone_ref.h"
#include "serialize/zone_store.h"

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
    entt::registry root;   // ZONE_ROOT, always live

    // defaults to a chunked folder backend at "zones" (zones packed per chunk
    // file; Plan B). Inject another ZoneStore to change where/how zones persist.
    GlobalManager();
    explicit GlobalManager(std::unique_ptr<ZoneStore> store);

    // nullptr if not loaded
    entt::registry* get(ZoneKey key);

    // resolve a cross-zone reference to {registry, entity}. Only consults
    // already-loaded zones (does no disk IO); ZONE_ROOT maps to `root`.
    ZoneResolution resolve(const CrossZoneRef& ref);

    // create a brand-new zone under `parent`, seed its ZoneMeta placeholder, and
    // register a ChildZoneSummary stub in the parent registry (if parent loaded).
    // Idempotent: re-creating an existing zone won't duplicate the stub.
    entt::registry& create(ZoneKey key, ZoneKey parent);

    // deserialize a zone from the store; if already loaded returns existing.
    entt::registry& load(ZoneKey key);

    // serialize a zone to the store and drop it from memory.
    void unload(ZoneKey key);

    // load `key` plus every persisted zone sharing its storage chunk, each as
    // its own registry (folder backend = just load(key)). Warms a whole chunk
    // file so crossing zone boundaries inside it costs no extra disk reads.
    void prefetch(ZoneKey key);

    // Elder-Scrolls-style open-world streaming: keep a (2*radius+1)^2 window of
    // zones loaded around `center` (same ZoneType + z) -- load in-window zones
    // that exist on disk, unload same-layer loaded zones that fell outside the
    // window. Call as the focus (player's zone) moves. Suggested radius: Region
    // ~2 (5x5, like ES uGridsToLoad); Area 0 (one heavy local map, like an interior).
    void stream_around(ZoneKey center, int radius);

    // list direct child zones of a (loaded) parent WITHOUT loading them.
    // returns empty if the parent zone is not loaded.
    std::vector<ZoneKey> children(ZoneKey parent);

    // ---- systems ----

    // a per-zone system: runs against one zone's registry.
    using ZoneSystem = std::function<void(entt::registry&)>;

    // register a per-zone system; tick() runs them in registration order.
    void add_zone_system(ZoneSystem sys);

    // advance one step: run every registered per-zone system against each
    // LOADED zone (root is excluded -- it holds global entities, not map actors;
    // cross-zone systems will be handled separately).
    void tick();

    // ---- whole-game save / load ----

    // checkpoint: write root + every currently-loaded zone to the store (no eviction).
    void save_all();

    // open a game: load the root registry. Sub-zones stay in the store until
    // streamed in on demand via load()/create().
    void load_root();

    ZoneStore& store() { return *store_; }

private:
    entt::registry* parent_registry(ZoneKey parent);          // ZONE_ROOT -> &root
    void ensure_child_stub(entt::registry& parent, ZoneKey child);
    void write_zone(ZoneKey key, entt::registry& reg);        // reg -> bytes -> store

    std::unique_ptr<ZoneStore>                                   store_;
    std::unordered_map<ZoneKey, std::unique_ptr<entt::registry>> loaded_;
    std::vector<ZoneSystem>                                      zone_systems_;
};
