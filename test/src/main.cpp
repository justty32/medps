#include <gcore/gcore.h>
#include <iostream>
#include <strstream>
#include <spanstream>
#include <sstream>

#define MAX_COLUMNS 20

#define HAHA(...) using k = std::tuple<__VA_ARGS__>;

int main(void)
{
	HAHA(int, int)
}