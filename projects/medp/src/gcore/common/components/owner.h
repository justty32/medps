#pragma once
#include <cstdint>

// actor 的陣營歸屬。存的是穩定的 faction id，不是 entt handle——
// faction 實體屬 root（見 zone.h：root 放陣營/神祇/具名角色），跨 registry
// 不能持有對方的 entt::entity，故以零語意 uint64 id 參照（比照 zone id 的做法）。
// 0 = 無主/中立（未歸屬的遺跡、流民、蠻族據點…）。actor 兩大家族共用的身分元件之一。
struct Owner {
    uint64_t faction = 0;

    template <class Archive>
    void serialize(Archive& ar) { ar(faction); }
};
