#pragma once

// Grid position of an actor within its zone (the zone's map is a tdarray grid).
// Integer tile coordinates; zone layering is encoded in the ZoneKey, not here.
// (Has .x/.y so it also satisfies tdarray's is_coor concept.)
struct Position {
    int x{};
    int y{};

    template<class Archive>
    void serialize(Archive& ar) { ar(x, y); }
};
