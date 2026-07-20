#pragma once

// 站在某格上的離散物件（關閉的門、巨石、大型生物）逐 entity 的通行性。
// terrain 層級的通行性放在 AreaTerrain 的 tile flags；這個則是疊在 terrain 之上的阻擋。
struct Blocking {
    bool blocks_move{true};
    bool blocks_sight{false};

    template<class Archive>
    void serialize(Archive& ar) { ar(blocks_move, blocks_sight); }
};
