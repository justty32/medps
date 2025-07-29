#include "scene.hpp"
#include <fstream>

void Scene::Save(std::ostrstream& fs)
{
	this->base_class_t::Save(fs);
	size_t size = objs.size();
	BinFSR::write(&size, fs);
	int tid = 0, badtid = -1;
	for (int i = 0; i < size; i++) {
		if (objs[i] == nullptr) {
			BinFSR::write(&badtid, fs);
		}
		else {
			tid = objs[i]->GetTypeID();
			BinFSR::write(&tid, fs);
			objs[i]->Save(fs);
		}
	}
}

void Scene::Load(std::istrstream& fs)
{
	this->base_class_t::Load(fs);
	size_t size = 0;
	BinFSR::read(&size, fs);
	if (size == 0)
		return;
	objs.resize(size);
	int tid = 0;
	for (int i = 0; i < size; i++) {
		BinFSR::read(&tid, fs);
		if (tid < 0) {
			objs[i] = nullptr;
			empty_id_pool.push_back(i);
		}
		else {
			objs[i] = Obj::_g_default_constructors[tid]();
			objs[i]->Load(fs);
		}
	}
}
