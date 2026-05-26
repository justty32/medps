#pragma once
#include <string>
#include <optional>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <cstdio>
#include "../zone_key.h"

// Storage backend for serialized zone bytes, keyed by ZoneKey.
// Decouples GlobalManager from *where* zones live (folder of files, single
// pack file, embedded DB, ...). zone_io handles registry<->bytes; a ZoneStore
// handles bytes<->storage. ZONE_ROOT is stored like any other key.
struct ZoneStore {
    virtual ~ZoneStore() = default;

    virtual void                       write(ZoneKey key, const std::string& bytes) = 0;
    virtual std::optional<std::string> read(ZoneKey key) = 0;
    virtual bool                       has(ZoneKey key) = 0;

    // commit pending writes to durable storage. No-op for folder backend;
    // a real commit point for single-file / DB backends.
    virtual void flush() {}
};

// One file per zone under a directory (the original behavior).
class FolderZoneStore : public ZoneStore {
public:
    explicit FolderZoneStore(std::filesystem::path dir) : dir_(std::move(dir)) {}

    std::filesystem::path path(ZoneKey key) const {
        if (key == ZONE_ROOT) return dir_ / "root.bin";
        char buf[17];
        std::snprintf(buf, sizeof(buf), "%016llx", static_cast<unsigned long long>(key));
        return dir_ / (std::string(buf) + ".bin");
    }

    void write(ZoneKey key, const std::string& bytes) override {
        std::filesystem::create_directories(dir_);
        std::ofstream ofs{path(key), std::ios::binary};
        ofs.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    }

    std::optional<std::string> read(ZoneKey key) override {
        auto p = path(key);
        if (!std::filesystem::exists(p)) return std::nullopt;
        std::ifstream ifs{p, std::ios::binary};
        return std::string{std::istreambuf_iterator<char>(ifs),
                           std::istreambuf_iterator<char>()};
    }

    bool has(ZoneKey key) override { return std::filesystem::exists(path(key)); }

private:
    std::filesystem::path dir_;
};
