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
};

// 在一個目錄下每個 zone 一個檔案（原本的行為）。
class FolderZoneStore : public ZoneStore {
public:
    // dir 即所有 zone 檔的根資料夾，可任意指定（相對 / 絕對 / 多層皆可）；
    // 它就是檔名的 folder prefix。前端載入存檔時把該局存檔的資料夾路徑傳進來即可。
    // 注意：GlobalManager() 無參數建構子內建用 FolderZoneStore("zones")（相對於工作
    // 目錄）只是個方便的預設；要換位置就走 GlobalManager(unique_ptr<ZoneStore>) 注入。
    explicit FolderZoneStore(std::filesystem::path dir) : dir_(std::move(dir)) {}

    // key → 檔案路徑：dir_/<16 碼 hex key>.bin；ZONE_ROOT 特例為 dir_/root.bin。
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
