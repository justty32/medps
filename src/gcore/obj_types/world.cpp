#include "world.h"


template<>
void BinFSR::write<BigMap::Tile>(BigMap::Tile* t, BinFSR::ostream_t& fs) {
	BinFSR::write<int>(&(t->terrain), fs);
}

template<>
void BinFSR::read<BigMap::Tile>(BigMap::Tile* t, BinFSR::istream_t& fs) {
	BinFSR::read<int>(&(t->terrain), fs);
}

void BigMap::TileMap::Save(BinFSR::ostream_t& fs)
{
	this->Component::Save(fs);
	this->tdarray<BigMap::Tile>::Save(fs);
}

void BigMap::TileMap::Load(BinFSR::istream_t& fs)
{
	this->Component::Load(fs);
	this->tdarray<BigMap::Tile>::Load(fs);
}
