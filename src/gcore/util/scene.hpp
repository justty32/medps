#pragma once
#include "obj.hpp"
#include <vector>
#include <list>
#include <filesystem>

class Scene : public Obj{
	OBJ_INIT_DEF(1, Scene, Obj)
public:
	std::vector<Obj*> objs;
	std::list<int> empty_id_pool;
	void Save(BinFSR::ostream_t& fs) override;
	void Load(BinFSR::istream_t& fs) override;
	// new obj
	inline Obj* NewObj(int type_id) {
		Obj* obj = Obj::_g_default_constructors[type_id]();
		AddObj(obj);
		return obj;
	}
	template<typename T>
	requires std::is_base_of_v<T, Obj>
	inline T* NewObj() { return (T*)NewObj(T::GetTypeID()); }
	// add obj
	template<typename T>
	requires std::is_base_of_v<T, Obj>
	inline int AddObj(T* obj) {
		if (empty_id_pool.empty() == false) {
			obj->id = empty_id_pool.front();
			empty_id_pool.pop_front();
			objs[obj->id] = obj;
		}
		else {
			obj->id = static_cast<int>(objs.size());
			objs.push_back(obj);
		}
		obj->scene = this;
		return obj->id;
	}
	// delete obj, but that id remains empty
	// and add id to pool for reuse
	inline void DeleteObj(int id) {
		delete objs[id];
		objs[id] = nullptr;
		empty_id_pool.push_back(id);
	}
	// get obj
	template<typename T = Obj>
	requires std::is_base_of_v<T, Obj>
	inline T* GetObj(int id) {
		if (id < 0 || id >= objs.size()) return nullptr;
		return (T*)objs[id]; 
	}
};