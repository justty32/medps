#pragma once
#include "utildef.h"
#include "mydef.h"
#include "bin_fwr.hpp"
#include <map>
#include <functional>
#include <fstream>

// use at every Obj's subtypes's declaration
// ex:
// class Adder : public Obj {
//     OBJ_INIT_DEF(Adder)
//     ...
#define OBJ_INIT_DEF(TYPE_ID, CLASS, BASE_CLASS)\
	using base_class_t = BASE_CLASS;\
	friend class Obj;\
public:\
	inline static int GetTypeID() { return TYPE_ID; }

// use at _init_subtypes() or _init_all_subtypes()
// ex. OBJ_TYPE_LIST_REGISTER_CLASS(Obj)
#define OBJ_TYPE_LIST_REGISTER_CLASS(CLASS)\
	Obj::_g_default_constructors[CLASS::GetTypeID()] = []() -> Obj* { return new CLASS; };

// if the obj type doesn't have any vars to save, use this
#define OBJ_DEFINE_DEFAULT_SAVELOAD()\
	inline void Save(std::ostrstream& fs) override { this->base_class_t::Save(fs); /*...*/}\
	inline void Load(std::istrstream& fs) override { this->base_class_t::Load(fs); /*...*/}

class Obj;
class Scene;

template<typename T> requires std::is_base_of_v<Obj, T>
int GetObjTypeID() { return T::GetTypeID(); }

// ex.
// class SomeObjType : public Obj {
//     OBJ_INIT_DEF(<unique number>, SomeObjType, Obj)
// public:
//     void Save(std::ostrstream& fs);
//     void Load(std::istrstream& fs);
class Obj {
public:
	int id = 0;
	Scene* scene = nullptr;
	static int GetTypeID() { return 0; }
	virtual void Save(std::ostrstream& fs);
	virtual void Load(std::istrstream& fs);
	static std::map<int, std::function<Obj*()>> _g_default_constructors; // {type, constructor}
private:
	// defined at class_list.cpp
	// call all subtypes's _init
	static inline void _init_all_subtypes();
};
