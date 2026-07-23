#include "world.h"

// 薄殼：真正的 worldgen 在 world_gen.cpp（只認配方＋一張 grid，不依賴 World）。
// World 負責挑 layer——目前世界圖是單層 layers[0]——並把自己的 id 傳去供錯誤定位。
void World::generate() { world_gen::generate(gen, layers[0], id); }
