#pragma once
#include <map>
#include <unordered_map>
#include <string>
#include <vector>
#include <optional>
#include <filesystem>
#include <fstream>
#include <cstdio>
#include <cereal/archives/portable_binary.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/string.hpp>
#include "zone_store.h"
#include "../chunk_key.h"

// Plan B storage: one file per CHUNK (a block of logical zones), not per zone.
// A chunk file is a cereal PortableBinary `map<ZoneKey, bytes>`. Each logical
// zone keeps its own independent blob (produced by zone_io); this only groups
// those blobs on disk -- cutting file count (Region 5×5 ÷25) without touching
// the registry / entity-namespace / CrossZoneRef semantics. Area stays 1:1.
//
// Write-through: every write() persists its chunk immediately, so unload()
// (which does not call flush()) durably persists -- matching FolderZoneStore.
// flush() is therefore a no-op. An in-memory cache avoids re-reading a chunk
// file on every access; references into it stay stable (unordered_map).
class ChunkedFolderZoneStore : public ZoneStore {
public:
    explicit ChunkedFolderZoneStore(std::filesystem::path dir) : dir_(std::move(dir)) {}

    std::filesystem::path chunk_path(ZoneKey chunk) const {
        char buf[17];
        std::snprintf(buf, sizeof(buf), "%016llx", static_cast<unsigned long long>(chunk));
        return dir_ / (std::string(buf) + ".chunk");
    }

    void write(ZoneKey key, const std::string& bytes) override {
        ZoneKey ck = chunk_key_of(key);
        Chunk& chunk = load_chunk(ck);
        chunk[key] = bytes;
        persist(ck, chunk);
    }

    std::optional<std::string> read(ZoneKey key) override {
        Chunk& chunk = load_chunk(chunk_key_of(key));
        auto it = chunk.find(key);
        if (it == chunk.end()) return std::nullopt;
        return it->second;
    }

    bool has(ZoneKey key) override {
        Chunk& chunk = load_chunk(chunk_key_of(key));
        return chunk.find(key) != chunk.end();
    }

    // every zone persisted in key's chunk file (the prefetch unit).
    std::vector<ZoneKey> group_of(ZoneKey key) override {
        Chunk& chunk = load_chunk(chunk_key_of(key));
        std::vector<ZoneKey> out;
        out.reserve(chunk.size());
        for (auto& [k, blob] : chunk) out.push_back(k);
        return out;
    }

private:
    using Chunk = std::map<ZoneKey, std::string>;   // ordered -> deterministic file

    Chunk& load_chunk(ZoneKey ck) {
        if (auto it = cache_.find(ck); it != cache_.end()) return it->second;
        Chunk chunk;
        auto p = chunk_path(ck);
        if (std::filesystem::exists(p)) {
            std::ifstream ifs{p, std::ios::binary};
            cereal::PortableBinaryInputArchive in{ifs};
            in(chunk);
        }
        return cache_.emplace(ck, std::move(chunk)).first->second;
    }

    void persist(ZoneKey ck, const Chunk& chunk) {
        std::filesystem::create_directories(dir_);
        std::ofstream ofs{chunk_path(ck), std::ios::binary};
        cereal::PortableBinaryOutputArchive out{ofs};
        out(chunk);   // flushed when `out` is destroyed, before `ofs` closes
    }

    std::filesystem::path                  dir_;
    std::unordered_map<ZoneKey, Chunk>     cache_;
};
