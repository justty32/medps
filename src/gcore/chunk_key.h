#pragma once
#include "zone_key.h"

// chunk 是儲存/檔案單位（方案 B）。同一個 ZoneType 與 z 的數個邏輯 zone
// 共用一個 chunk，定址方式是把 zone 的 (x,y) 對各型別的 chunk 邊長做整數除法。
// 邏輯定址（ZoneKey）維持不變 --
// 這只是把磁碟上的 blob 分組，以減少檔案數量並讓 streaming 更平順。
// 參見 ChunkedFolderZoneStore 與 work/design/registry_chunking_investigation.md。

inline int16_t chunk_side(ZoneType type) {
    switch (type) {
        case ZoneType::Region: return zone_scale::REGION_CHUNK;  // 5×5
        case ZoneType::Area:   return zone_scale::AREA_CHUNK;     // 1:1
        default:               return 1;                         // World / root（世界 / 根）
    }
}

// 擁有 `key` 的那個 chunk。ZONE_ROOT 與任何 1:1 型別自成一個 chunk。
inline ZoneKey chunk_key_of(ZoneKey key) {
    if (key == ZONE_ROOT) return ZONE_ROOT;
    const ZoneType t = zone_key_type(key);
    const int16_t  n = chunk_side(t);
    if (n <= 1) return key;
    auto fdiv = [](int16_t a, int16_t b) -> int16_t {           // 向下取整除法
        int q = a / b;
        if ((a % b != 0) && ((a < 0) != (b < 0))) --q;
        return static_cast<int16_t>(q);
    };
    return make_zone_key(t, fdiv(zone_key_x(key), n), fdiv(zone_key_y(key), n), zone_key_z(key));
}
