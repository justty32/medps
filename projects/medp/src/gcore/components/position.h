#pragma once

// actor 在其 zone 內的 grid 位置（zone 的地圖是一個 tdarray grid）。
// 整數 tile 座標；zone 的分層編碼在 ZoneKey 裡，不在這裡。
// （具有 .x/.y，因此也滿足 tdarray 的 is_coor concept。）
struct Position {
    int x{};
    int y{};

    template<class Archive>
    void serialize(Archive& ar) { ar(x, y); }
};
