#pragma once
#include "../zone_key.h"

// 每個 zone 的 placeholder entity 都持有的身分 component。
// 保證 zone 永遠至少有一個非孤兒的 entity（見 zone_io.h），
// 並讓已載入的 registry 知道自己代表哪個 ZoneKey 以及它的 parent。
struct ZoneMeta {
    ZoneKey self{ZONE_ROOT};
    ZoneKey parent{ZONE_ROOT};   // 直接的 parent zone；ZONE_ROOT = 掛在 root 之下

    template<class Archive>
    void serialize(Archive& ar) { ar(self, parent); }
};
