#pragma once
#include <istream>
#include <ostream>
#include <cereal/archives/portable_binary.hpp>
#include <cereal/types/map.hpp>
#include "../zone/zone.h"
#include "registry_io.h"

// 一個完整 Zone 的 save/load。
//
// Zone 有兩塊資料，走兩條不同的路：
//   1. id / parent / layers —— 純資料，直接交給 cereal
//   2. reg                  —— entity 與 component，得走 EnTT snapshot（registry_io）
//
// 檔案格式就是這兩塊依序接在一起。注意 cereal archive 是在解構時才把緩衝
// 寫出去的，所以第一塊必須用大括號限制生存期，確保它先完整落地，
// registry_io 才接著往同一個 stream 寫。
namespace zone_io {

inline void save(Zone& z, std::ostream& os) {
    {
        cereal::PortableBinaryOutputArchive ar{os};
        ar(z.id, z.parent, z.layers);
    }
    registry_io::save(z.reg, os);
}

inline void load(Zone& z, std::istream& is) {
    {
        cereal::PortableBinaryInputArchive ar{is};
        ar(z.id, z.parent, z.layers);
    }
    registry_io::load(z.reg, is);
}

} // namespace zone_io
