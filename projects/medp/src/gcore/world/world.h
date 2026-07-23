#pragma once
#include "../zone/zone.h"
#include "world_gen.h"

// 第一個 Zone 子類：世界層地圖（三層願景的最上層，時間模型純回合——tick 分派未動工）。
// 只放 worldgen 與世界尺度資料；全局實體仍歸 root，別把全局邏輯堆進來。
// worldgen 配方（WorldGenParams）＋terrain 常數＋演算法都在 world_gen.{h,cpp}。
struct World : Zone {
    WorldGenParams gen;

    static constexpr ZoneKind KIND = ZoneKind::World;
    ZoneKind kind() const override { return KIND; }

    void save_extra(cereal::PortableBinaryOutputArchive& ar) override { ar(gen); }
    void load_extra(cereal::PortableBinaryInputArchive& ar) override { ar(gen); }

    // 依 gen 重建 layers[0]；薄殼，實作委派給 world_gen::generate（見 world_gen.cpp）。
    void generate();
};
