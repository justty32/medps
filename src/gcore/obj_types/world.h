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

	struct Position {
		int x, y;
	};

	class MapEntity : public Obj {
		OBJ_INIT_DEF(_ID_BEGIN + 11, TileMap, Component)
	public:
		void Save(BinFSR::ostream_t& fs) override;
		void Load(BinFSR::istream_t& fs) override;
		Position pos;
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

struct Entity { int id; std::string name; int owner; };
struct Positioned: Entity{ int x, y; };
struct Settlement: Positioned{ };
struct City: Settlement{};
struct Village: Settlement{};
struct Military: Positioned{};
struct Unit: Military{};
struct Hero: Military{};
struct ArmyStack: Military{};
struct Tile{};
struct Map{};

struct Tile {
	int terrain;
	std::map<int, int> improvements; // include river, road, farm, mine...
	int unit_stack; // points to unit stack
	int city; // points to city
	int special_place; // points to special place
};
struct Map {
	tdarray<Tile> tiles;
	std::list<Obj*> unit_stacks;
	std::list<Obj*> cities;
	std::list<Obj*> special_places;
};
struct Entity { };
struct Positioned : Entity {
	int x, y;
};
struct Unit {
	int hp, maxhp, atk, def, move;
	int unit_stack;
};
struct Hero: Unit { };
struct UnitStack: Positioned{
	int units[6];
	int hero;
	int faction;
};
struct City : Positioned {
	int population, happiness;
	int level, def, hp;
	int gen_food, gen_gold, gen_prod, gen_sci;
	int st_food, st_gold, st_prod, st_sci;
};
