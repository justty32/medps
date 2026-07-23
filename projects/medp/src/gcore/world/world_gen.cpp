#include "world_gen.h"
#include <stdexcept>
#include <string>
#include <libtcod/mersenne.hpp>
#include <libtcod/noise.hpp>

namespace {

// 一張 0~1 的 2D FBM 場。RNG 用 Mersenne Twister 鎖 seed，取樣結果決定性。
struct Field {
    TCODRandom rng;
    TCODNoise  noise;
    float      scale;
    float      octaves;

    Field(uint32_t seed, float scale, int32_t octaves)
        : rng{seed, TCOD_RNG_MT},
          noise{2, TCOD_NOISE_DEFAULT_HURST, TCOD_NOISE_DEFAULT_LACUNARITY, &rng, TCOD_NOISE_SIMPLEX},
          scale{scale},
          octaves{static_cast<float>(octaves)} {}

    float at(int x, int y) {
        const float f[2] = {x * scale, y * scale};
        return (noise.getFbm(f, octaves) + 1.0f) * 0.5f;  // -1~1 → 0~1
    }
};

}  // namespace

void world_gen::generate(const WorldGenParams& gen, tdarray<Tile>& grid, uint64_t zone_id) {
    if (gen.width <= 0 || gen.height <= 0)
        throw std::runtime_error(
            "world_gen::generate: 尺寸非正: " + std::to_string(gen.width) + "x" +
            std::to_string(gen.height) + " (zone id=" + std::to_string(zone_id) + ")");

    // 高度/溫度/濕度三張獨立場，seed 派生錯開。
    Field height{gen.seed,     gen.noise_scale, gen.octaves};
    Field temp  {gen.seed + 1, gen.noise_scale, gen.octaves};
    Field humid {gen.seed + 2, gen.noise_scale, gen.octaves};

    grid.alloc(static_cast<uint32_t>(gen.width), static_cast<uint32_t>(gen.height));

    grid.eachxy([&](Tile& t, int x, int y) {
        if (height.at(x, y) <= gen.sea_level) {
            t = Tile{TERRAIN_OCEAN, 0};
            return;
        }
        uint32_t biome = TERRAIN_GRASSLAND;
        if (temp.at(x, y) < 0.3f)        biome = TERRAIN_TUNDRA;
        else if (humid.at(x, y) >= 0.4f) biome = TERRAIN_FOREST;
        t = Tile{biome, TILE_WALKABLE};
    });
}
