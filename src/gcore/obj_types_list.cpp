#pragma once
#include "util.h"
#include "obj_types.h"

void Obj::_init_all_subtypes() {
	OBJ_TYPE_LIST_REGISTER_CLASS(Obj);
	OBJ_TYPE_LIST_REGISTER_CLASS(Scene);
	OBJ_TYPE_LIST_REGISTER_CLASS(Component);
	OBJ_TYPE_LIST_REGISTER_CLASS(ComponentManager);
	OBJ_TYPE_LIST_REGISTER_CLASS(BigMap::TileMap);
	OBJ_TYPE_LIST_REGISTER_CLASS(BigMap::MapEntity);
}

