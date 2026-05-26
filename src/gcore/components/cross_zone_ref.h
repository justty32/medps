#pragma once
#include <entt.hpp>
#include "../zone_key.h"

struct CrossZoneRef {
    ZoneKey      zone{ZONE_ROOT};
    entt::entity local_entity{entt::null};

    template<class Archive>
    void save(Archive& ar) const {
        using id_t = std::underlying_type_t<entt::entity>;
        ar(zone, static_cast<id_t>(local_entity));
    }

    template<class Archive>
    void load(Archive& ar) {
        using id_t = std::underlying_type_t<entt::entity>;
        id_t raw{};
        ar(zone, raw);
        local_entity = static_cast<entt::entity>(raw);
    }
};
