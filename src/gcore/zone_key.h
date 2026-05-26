#pragma once
#include <cstdint>

enum class ZoneType : uint16_t {
    Invalid = 0,   // reserved: collides with ZONE_ROOT (0). real zone types start from 1.
    // TODO: real types
};

using ZoneKey = uint64_t;
constexpr ZoneKey ZONE_ROOT = 0;   // = make_zone_key(ZoneType::Invalid, 0, 0, 0)

// bits 63-48: ZoneType (16) | bits 47-32: x (16) | bits 31-16: y (16) | bits 15-0: z (16)
inline ZoneKey make_zone_key(ZoneType type, int16_t x, int16_t y, int16_t z) {
    return (static_cast<uint64_t>(type)                        << 48)
         | (static_cast<uint64_t>(static_cast<uint16_t>(x))   << 32)
         | (static_cast<uint64_t>(static_cast<uint16_t>(y))   << 16)
         |  static_cast<uint64_t>(static_cast<uint16_t>(z));
}

inline ZoneType zone_key_type(ZoneKey k) { return static_cast<ZoneType>(k >> 48); }
inline int16_t  zone_key_x   (ZoneKey k) { return static_cast<int16_t>(k >> 32); }
inline int16_t  zone_key_y   (ZoneKey k) { return static_cast<int16_t>(k >> 16); }
inline int16_t  zone_key_z   (ZoneKey k) { return static_cast<int16_t>(k);       }
