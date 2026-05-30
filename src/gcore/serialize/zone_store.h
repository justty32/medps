#pragma once
#include <string>
#include <vector>
#include <optional>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <cstdio>
#include "../zone_key.h"

// 序列化後 zone 位元組的儲存後端，以 ZoneKey 為鍵。
// 將 GlobalManager 與 zone「存放位置」解耦（檔案資料夾、單一
// pack 檔、內嵌 DB……）。zone_io 負責 registry<->位元組；ZoneStore
// 負責位元組<->儲存。ZONE_ROOT 與其他 key 一樣儲存。
struct ZoneStore {
    virtual ~ZoneStore() = default;

    virtual void                       write(ZoneKey key, const std::string& bytes) = 0;
    virtual std::optional<std::string> read(ZoneKey key) = 0;
    virtual bool                       has(ZoneKey key) = 0;

    // 將待寫入的資料提交至持久化儲存。對 folder 後端而言是 no-op；
    // 對單一檔案 / DB 後端則是真正的 commit 點。
    virtual void flush() {}

    // 與 `key` 儲存在同一個 storage group（chunk / pack page）的所有 key，
    // 若 `key` 存在則一併包含。動到其中一個會讓其餘的載入成本很低，因此
    // 呼叫端可以一次 prefetch 整個 group。預設後端每個單位只存一個
    // zone，所以當 `key` 存在時這就只是 {key}。
    virtual std::vector<ZoneKey> group_of(ZoneKey key) {
        if (has(key)) return { key };
        return {};
    }
};

// 在一個目錄下每個 zone 一個檔案（原本的行為）。
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
