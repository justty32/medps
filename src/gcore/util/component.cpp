#include "component.hpp"

void Component::Save(std::ostrstream& fs)
{
	this->base_class_t::Save(fs);
}

void Component::Load(std::istrstream& fs)
{
	this->base_class_t::Load(fs);
}

void ComponentManager::Save(std::ostrstream& fs)
{
	this->base_class_t::Save(fs);
	size_t comps_num = comps.size();
	BinFSR::write(&comps_num, fs);
	for (auto& [k, v] : comps) {
		BinFSR::write(&k, fs);
		v->Save(fs);
	}
}

void ComponentManager::Load(std::istrstream& fs)
{
	this->base_class_t::Load(fs);
	size_t comps_num = 0;
	BinFSR::read(&comps_num, fs);
	int type_id = 0;
	for (int i = 0; i < comps_num; i++) {
		BinFSR::read(&type_id, fs);
		Component* c = NewComp(type_id);
		c->Load(fs);
	}
}
