#include <gcore/gcore.h>
#include <iostream>
#include <strstream>

#define MAX_COLUMNS 20

int testt() {
	std::ofstream of;
	std::ostrstream ssof;
	std::vector<int> a;
	a.resize(1000000);
	for (int i = 0; i < a.size(); i++) {
		BinFSR::write(a.data() + i, ssof);
	}
	//BinFSR::write(&a, ssof);
	return 0;
}

int main(void)
{
	return testt();
}