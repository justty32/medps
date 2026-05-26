#pragma once
#include <entt.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <type_traits>

using entt_id_t = std::underlying_type_t<entt::entity>;

struct output_archive {
    cereal::PortableBinaryOutputArchive& ar;

    void operator()(entt_id_t v)                { ar(v); }
    void operator()(entt::entity e)             { ar(static_cast<entt_id_t>(e)); }
    template<typename T>
    void operator()(entt::entity e, const T& c) { ar(static_cast<entt_id_t>(e), c); }
    template<typename T>
    void operator()(const T& v)                 { ar(v); }  // component data (separate call)
};

struct input_archive {
    cereal::PortableBinaryInputArchive& ar;

    void operator()(entt_id_t& v) { ar(v); }
    void operator()(entt::entity& e) {
        entt_id_t v{};
        ar(v);
        e = static_cast<entt::entity>(v);
    }
    template<typename T>
    void operator()(entt::entity& e, T& c) {
        entt_id_t v{};
        ar(v, c);
        e = static_cast<entt::entity>(v);
    }
    template<typename T>
    void operator()(T& v) { ar(v); }  // component data (separate call)
};
