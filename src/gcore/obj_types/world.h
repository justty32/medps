#pragma once

#include "../util.h"

class World : public Scene {
	OBJ_INIT_DEF(1001, World, Scene)
public:
	virtual void Save(std::ostrstream& fs);
	virtual void Load(std::istrstream& fs);
};


// space : map, 
// time : event
// obj : c

struct BigMapTile {
	int terrain;
	int biome;
	int resource_type;
	int resource_amount;
	int resource_regen_amount;
	int buildable_type;
	int move_attr;
};