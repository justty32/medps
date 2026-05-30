#pragma once
#include "../zone_key.h"

// 存在 PARENT zone 的 registry 裡的輕量 stub，每個直接 child zone 一個。
// 讓已載入的 parent 不必載入子 zone 即可列舉 / 概覽其 children。
// 可視需要把額外的 component（地圖位置、icon、已探索 flag...）掛到同一個
// stub entity 上。
struct ChildZoneSummary {
    ZoneKey key{ZONE_ROOT};

    template<class Archive>
    void serialize(Archive& ar) { ar(key); }
};
