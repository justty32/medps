#pragma once
#include <istream>
#include <ostream>
#include <entt.hpp>
#include <cereal/archives/portable_binary.hpp>
#include "entt_cereal_archive.h"
#include "all_components.h"

// 單一 registry 的 snapshot save/load。
// registry <-> 位元組；要存到哪裡（檔案、pack、DB）不歸這裡管。
namespace registry_io {

namespace detail {

    template<typename... Cs>
    void save_impl(entt::registry& reg, output_archive& out, entt::type_list<Cs...>) {
        auto snap = entt::snapshot{reg};
        snap.get<entt::entity>(out);
        (snap.get<Cs>(out), ...);
    }

    template<typename... Cs>
    void load_impl(entt::registry& reg, input_archive& in, entt::type_list<Cs...>) {
        auto loader = entt::snapshot_loader{reg};
        loader.get<entt::entity>(in);
        (loader.get<Cs>(in), ...);
        // orphans() 會銷毀所有最終沒有任何 component 的 entity。
        // 未登記在 AllComponents 的 component 不只資料會掉，只帶這種 component
        // 的 entity 本身也會在這裡一併消失。
        loader.orphans();
    }

} // namespace detail

inline void save(entt::registry& reg, std::ostream& os) {
    cereal::PortableBinaryOutputArchive cereal_out{os};
    output_archive out{cereal_out};
    detail::save_impl(reg, out, AllComponents{});
}

inline void load(entt::registry& reg, std::istream& is) {
    cereal::PortableBinaryInputArchive cereal_in{is};
    input_archive in{cereal_in};
    detail::load_impl(reg, in, AllComponents{});
}

} // namespace registry_io
