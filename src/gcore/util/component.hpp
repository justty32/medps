#pragma once
#include "obj.hpp"

class ComponentManager;
class Component : public Obj {
	OBJ_INIT_DEF(11, Component, Obj)
public:
	void Save(BinFSR::ostream_t& fs) override;
	void Load(BinFSR::istream_t& fs) override;
	ComponentManager* comp_mg;
};

// will inherit with Obj
class ComponentManager : public Obj {
	OBJ_INIT_DEF(12, ComponentManager, Obj)
public:
	void Save(BinFSR::ostream_t& fs) override;
	void Load(BinFSR::istream_t& fs) override;
	std::map<int, Component*> comps;
	inline Component* GetComp(int type_id) {
		if (auto it = comps.find(type_id); it != comps.end()) return it->second;
		else return nullptr;
	}
	template<typename T> requires std::is_base_of_v<Component, T>
	T* GetComp() { return dynamic_cast<T*>(GetComp(T::GetTypeID())); }
	inline bool HasComp(int type_id) { return comps.find(type_id) != comps.end(); }
	template<typename T> requires std::is_base_of_v<Component, T>
	inline bool HasComp() { return HasComp(T::GetTypeID()); }
	inline void AddComp(Component* comp) {
		if (auto it = comps.find(comp->GetTypeIDV()); it != comps.end()) {
			Component* old = it->second;
			delete old;
			it->second = comp;
		}
		else {
			comps[comp->GetTypeIDV()] = comp;
		}
	}
	inline Component* NewComp(int type_id) {
		if (auto it = Obj::_g_default_constructors.find(type_id);
			it != Obj::_g_default_constructors.end()) {
			Component* comp = dynamic_cast<Component*>(it->second());
			AddComp(comp);
			return comp;
		}
		else {
			return nullptr;
		}
	}
	template<typename T> requires std::is_base_of_v<Component, T>
	inline T* NewComp() { return NewComp(T::GetTypeID()); }
	inline Component* RemoveComp(int type_id) {
		auto it = comps.find(type_id);
		if (it == comps.end()) return nullptr;
		Component* c = it->second;
		comps.erase(it);
		return c;
	}
	template<typename T> requires std::is_base_of_v<Component, T>
	inline T* RemoveComp() { return dynamic_cast<T*>(RemoveComp(T::GetTypeID())); }
	inline void DeleteComp(int type_id) {
		Component* c = RemoveComp(type_id);
		if (c != nullptr) delete c;
	}
	template<typename T> requires std::is_base_of_v<Component, T>
	inline void DeleteComp() { DeleteComp(T::GetTypeID()); }
};

