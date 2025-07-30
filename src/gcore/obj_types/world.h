#pragma once

#include "../util.h"

namespace BigMap {
	constexpr int _ID_BEGIN = 200000;

	struct Tile {
		int terrain;
	};
	class TileMap : public Component, public tdarray<Tile> {
		OBJ_INIT_DEF(_ID_BEGIN + 10, TileMap, Component)
	public:
		void Save(BinFSR::ostream_t& fs) override;
		void Load(BinFSR::istream_t& fs) override;
	};

	class MapEntity : public Component {
		OBJ_INIT_DEF(_ID_BEGIN + 11, TileMap, Component)

	};
}

struct BigMapTile {
	int terrain;
	int biome;
	int resource_type;
	int resource_amount;
	int resource_regen_amount;
	int buildable_type;
	int move_attr;
};