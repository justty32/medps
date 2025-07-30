#include "obj.hpp"

std::map<int, std::function<Obj*()>> Obj::_g_default_constructors;

void Obj::Save(BinFSR::ostream_t& fs)
{
    BinFSR::write(&id, fs);
}

void Obj::Load(BinFSR::istream_t& fs)
{
    BinFSR::read(&id, fs);
}
