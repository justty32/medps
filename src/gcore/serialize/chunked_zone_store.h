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

// Plan B 儲存：每個 CHUNK（一塊邏輯 zone 的集合）一個檔案，而非每個 zone 一個。
// 一個 chunk 檔是一份 cereal PortableBinary 的 `map<ZoneKey, bytes>`。每個邏輯
// zone 保有自己獨立的 blob（由 zone_io 產生）；這裡只是把那些 blob 在磁碟上
// 分組——減少檔案數量（Region 5×5 ÷25），同時不動到 registry /
// entity-namespace / CrossZoneRef 的語意。Area 維持 1:1。
//
// Write-through：每次 write() 都會立刻持久化其 chunk，因此 unload()
//（不會呼叫 flush()）也能持久保存——與 FolderZoneStore 一致。
// 所以 flush() 是 no-op。一個記憶體內快取避免每次存取都重讀 chunk
// 檔；指向它的參照保持穩定（unordered_map）。
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

    // 持久化在 key 所屬 chunk 檔中的每個 zone（prefetch 單位）。
    std::vector<ZoneKey> group_of(ZoneKey key) override {
        Chunk& chunk = load_chunk(chunk_key_of(key));
        std::vector<ZoneKey> out;
        out.reserve(chunk.size());
        for (auto& [k, blob] : chunk) out.push_back(k);
        return out;
    }

private:
    using Chunk = std::map<ZoneKey, std::string>;   // 有序 -> 檔案內容具決定性

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
        out(chunk);   // 在 `out` 解構時（`ofs` 關閉前）flush
    }

    std::filesystem::path                  dir_;
    std::unordered_map<ZoneKey, Chunk>     cache_;
};
