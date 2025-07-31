#pragma once
#include <sstream>
#include <vector>
#include <map>
#include <unordered_map>
#include <array>

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

	template<typename T, size_t S>
	void read(std::array<T, S>* arr, istream_t& fs) {
		size_t size = 0;
		read(&size, fs);
		for (int i = 0; i < size && i < S; i++)
			read(&(arr->at(i)), fs);
	}
	template<typename T, size_t S>
	void write(std::array<T, S>* arr, ostream_t& fs) {
		size_t size = S;
		write(&size, fs);
		for (int i = 0; i < size; i++)
			write(&(arr->at(i)), fs);
	}

	template<typename Tk, typename Tv>
	void read(std::map<Tk, Tv>* m, istream_t& fs) {
		size_t size = 0;
		read(&size, fs);
		for (int i = 0; i < size; i++) {
			Tk k; Tv v;
			read(&k, fs);
			read(&v, fs);
			(*m)[k] = v;
		}
	}
	template<typename Tk, typename Tv>
	void write(std::map<Tk, Tv>* m, ostream_t& fs) {
		size_t size = m->size();
		write(&size, fs);
		for(auto& [k,v]: *m){
			write(&k, fs);
			write(&v, fs);
		}
	}

	template<typename Tk, typename Tv>
	void read(std::unordered_map<Tk, Tv>* m, istream_t& fs) {
		size_t size = 0;
		read(&size, fs);
		for (int i = 0; i < size; i++) {
			Tk k; Tv v;
			read(&k, fs);
			read(&v, fs);
			(*m)[k] = v;
		}
	}
	template<typename Tk, typename Tv>
	void write(std::unordered_map<Tk, Tv>* m, ostream_t& fs) {
		size_t size = m->size();
		write(&size, fs);
		for(auto& [k,v]: *m){
			write(&k, fs);
			write(&v, fs);
		}
	}

}
