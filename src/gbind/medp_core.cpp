#include "medp_core.h"

using namespace godot;

void medp_gd::MedpCore::_bind_methods() {
    ClassDB::bind_method(D_METHOD("version"), &MedpCore::version);
}

String medp_gd::MedpCore::version() const {
    return String("medp core 0.1 (entt v3.16.0 + cereal v1.3.2)");
}
