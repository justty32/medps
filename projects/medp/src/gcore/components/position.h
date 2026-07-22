#pragma once

// actor 在其 zone 內的 grid 位置（zone 的地圖是 Zone::layers 那疊 tdarray）。
// 整數 tile 座標；「在哪個 zone」由 entity 所屬的 registry 決定，不記在這裡。
// z 是所在 layer，即 Zone::layers 的鍵：地面=0，往下為負。
// （具有 .x/.y，因此也滿足 tdarray 的 is_coor concept。）
struct Position {
    int x{};
    int y{};
    int z{};

    template<class Archive>
    void serialize(Archive& ar) { ar(x, y, z); }
};
