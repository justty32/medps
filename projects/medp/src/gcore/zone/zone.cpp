#include "zone.h"
#include "../world/world.h"
#include <stdexcept>
#include <string>

std::unique_ptr<Zone> make_zone(ZoneKind kind) {
    switch (kind) {
        case ZoneKind::Plain: return std::make_unique<Zone>();
        case ZoneKind::World: return std::make_unique<World>();
    }
    throw std::runtime_error(
        "make_zone: 未知 ZoneKind（存檔損毀？）: " +
        std::to_string(static_cast<unsigned>(kind)));
}
