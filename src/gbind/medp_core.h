#pragma once
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/string.hpp>

namespace medp_gd {

// Smoke-test facade：一個 GDScript 可以 `new` 並呼叫的 RefCounted。
// 用來驗證整條工具鏈（godot-cpp build -> bindings -> link medp_static ->
// .dll -> Godot 載入 -> GDScript 呼叫）能從頭到尾正常運作。
class MedpCore : public godot::RefCounted {
    GDCLASS(MedpCore, godot::RefCounted)

protected:
    static void _bind_methods();

public:
    godot::String version() const;
};

} // namespace medp_gd
