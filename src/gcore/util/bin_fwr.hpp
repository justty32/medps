#pragma once
#include <sstream>
#include <vector>

// binary stream write/read
namespace BinFSR {
	using istream_t = std::istringstream;
	using ostream_t = std::ostringstream;
	template<typename T>
	inline void read(T* t, istream_t& fs) {
		fs.read((char*)t, sizeof(T));
	}
	template<typename T>
	inline void write(T* t, ostream_t& fs) {
		fs.write((char*)t, sizeof(T));
	}
	template<typename T>
	void read(std::vector<T>* vec, istream_t& fs) {
		size_t size = 0;
		read(&size, fs);
		vec->resize(size);
		for (int i = 0; i < size; i++)
			read(&(vec->at(i)), fs);
	}
	template<typename T>
	void write(std::vector<T>* vec, ostream_t& fs) {
		size_t size = vec->size();
		write(&size, fs);
		for (int i = 0; i < size; i++)
			write(&(vec->at(i)), fs);
	}
}
