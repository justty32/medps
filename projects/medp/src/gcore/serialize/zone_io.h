#pragma once
#include <istream>
#include <memory>
#include <ostream>
#include <cereal/archives/portable_binary.hpp>
#include <cereal/types/map.hpp>
#include "../zone/zone.h"
#include "registry_io.h"

// 一個完整 Zone 的 save/load。
//
// Zone 有兩塊資料，走兩條不同的路：
//   1. kind / id / parent / layers / 子類 extra —— 純資料，直接交給 cereal
//   2. reg —— entity 與 component，得走 EnTT snapshot（registry_io）
//
// 檔案格式就是這兩塊依序接在一起。第一塊開頭是 uint8 的 ZoneKind tag：
// load 必須先讀它才知道要 new 哪個子類，因此 load 是「建構＋讀入」一體、
// 回傳 unique_ptr<Zone>；未知 tag 由 make_zone throw（fail-fast）。
// 子類專屬資料經 save_extra/load_extra 掛鉤寫在第一塊尾端。
//
// 注意 cereal archive 是在解構時才把緩衝寫出去的，所以第一塊必須用大括號
// 限制生存期，確保它先完整落地，registry_io 才接著往同一個 stream 寫。
namespace zone_io {

inline void save(Zone& z, std::ostream& os) {
    {
        cereal::PortableBinaryOutputArchive ar{os};
        auto kind = static_cast<uint8_t>(z.kind());
        ar(kind, z.id, z.parent, z.layers);
        z.save_extra(ar);
    }
    registry_io::save(z.reg, os);
}

inline std::unique_ptr<Zone> load(std::istream& is) {
    std::unique_ptr<Zone> z;
    {
        cereal::PortableBinaryInputArchive ar{is};
        uint8_t kind{};
        ar(kind);
        z = make_zone(static_cast<ZoneKind>(kind));
        ar(z->id, z->parent, z->layers);
        z->load_extra(ar);
    }
    registry_io::load(z->reg, is);
    return z;
}

} // namespace zone_io
