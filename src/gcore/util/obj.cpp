#include "obj.hpp"

std::map<int, std::function<Obj*()>> Obj::_g_default_constructors;

void Obj::Save(std::ostrstream& fs)
{
    BinFSR::write(&id, fs);
}

void Obj::Load(std::istrstream& fs)
{
    BinFSR::read(&id, fs);
}
