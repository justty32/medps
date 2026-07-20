#pragma once
#include <filesystem>
#include <fstream>
#include <entt.hpp>
#include <cereal/archives/portable_binary.hpp>
#include "entt_cereal_archive.h"
#include "all_components.h"

namespace zone_io {

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
        // 慣例：剛建立的 zone 必須保留一個帶有至少一個 component 的
        // 佔位 entity，否則它會在這裡被清掉。
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

inline void save(entt::registry& reg, const std::filesystem::path& path) {
    if (path.has_parent_path())
        std::filesystem::create_directories(path.parent_path());
    std::ofstream ofs{path, std::ios::binary};
    save(reg, ofs);
}

inline void load(entt::registry& reg, const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) return;
    std::ifstream ifs{path, std::ios::binary};
    load(reg, ifs);
}

} // namespace zone_io
