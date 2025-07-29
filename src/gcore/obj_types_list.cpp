#pragma once
#include "util.h"
#include "obj_types.h"

void Obj::_init_all_subtypes() {
	OBJ_TYPE_LIST_REGISTER_CLASS(Obj);
	OBJ_TYPE_LIST_REGISTER_CLASS(Scene);
}

