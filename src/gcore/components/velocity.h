#pragma once

// Per-tick movement step on the zone grid (integer tiles).
// Demo component for the movement system; refine for the real turn model later.
struct Velocity {
    int dx{};
    int dy{};

    template<class Archive>
    void serialize(Archive& ar) { ar(dx, dy); }
};
