#pragma once
#include <strstream>
#include <vector>

// binary stream write/read
namespace BinFSR {
	template<typename T>
	inline void read(T* t, std::istrstream& fs) {
		fs.read((char*)t, sizeof(T));
	}
	template<typename T>
	inline void write(T* t, std::ostrstream& fs) {
		fs.write((char*)t, sizeof(T));
	}
	template<typename T>
	void read(std::vector<T>* vec, std::istrstream& fs) {
		size_t size = 0;
		read(&size, fs);
		vec->resize(size);
		for (int i = 0; i < size; i++)
			read(&(vec->at(i)), fs);
	}
	template<typename T>
	void write(std::vector<T>* vec, std::ostrstream& fs) {
		size_t size = vec->size();
		write(&size, fs);
		for (int i = 0; i < size; i++)
			write(&(vec->at(i)), fs);
	}
}
