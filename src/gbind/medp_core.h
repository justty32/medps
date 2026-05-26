#pragma once
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/string.hpp>

namespace medp_gd {

// Smoke-test facade: a RefCounted that GDScript can `new` and call.
// Proves the toolchain (godot-cpp build -> bindings -> link medp_static ->
// .dll -> Godot loads -> GDScript calls) works end to end.
class MedpCore : public godot::RefCounted {
    GDCLASS(MedpCore, godot::RefCounted)

protected:
    static void _bind_methods();

public:
    godot::String version() const;
};

} // namespace medp_gd
