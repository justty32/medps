#pragma once

// 在 zone grid 上每個 tick 的移動步進（整數 tile）。
// movement system 的示範 component；之後再針對真正的回合模型細修。
struct Velocity {
    int dx{};
    int dy{};

    template<class Archive>
    void serialize(Archive& ar) { ar(dx, dy); }
};
