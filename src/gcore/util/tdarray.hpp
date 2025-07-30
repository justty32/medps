#pragma once

#include "utildef.h"
#include "bin_fwr.hpp"
#include "mydef.h"

#include <functional>
#include <vector>
#include <type_traits>
#include <fstream>

template<typename T>
concept is_coor = std::is_same_v<T, std::pair<int, int>>
	|| std::is_same_v<T, std::tuple<int, int>>
	|| requires(T t) { t.x; t.y; };
LET_CONCEPT_BE_CHECKABLE_V(is_coor);

template<typename T>
class tdarray {
private:
	template<is_coor T>
	inline constexpr int bx(T t) {
		if constexpr (std::is_same_v<T, std::pair<int, int>>) return t.first;
		else if constexpr (std::is_same_v<T, std::tuple<int, int>>) return std::get<0>(t);
		else return t.x;
	}
	template<is_coor T>
	inline constexpr int by(T t) {
		if constexpr (std::is_same_v<T, std::pair<int, int>>) return t.second;
		else if constexpr (std::is_same_v<T, std::tuple<int, int>>) return std::get<1>(t);
		else return t.y;
	}
	std::vector<T> vec;
public:
	size_t sx = 0, sy = 0;
	tdarray() {}
	tdarray(size_t sizex, size_t sizey): tdarray(){
		alloc(sizex, sizey);
	}
	void Save(BinFSR::ostream_t& fs) {
		BinFSR::write(&sx, fs);
		BinFSR::write(&sy, fs);
		BinFSR::write(&vec, fs);
	}
	void Load(BinFSR::istream_t& fs) {
		BinFSR::read(&sx, fs);
		BinFSR::read(&sy, fs);
		vec.resize(sx * sy);
		BinFSR::read(&vec, fs);
	}
	inline bool usable() { return vec.size() > 0 && sx > 0 && sy > 0 && sx * sy == vec.size(); }
	inline bool unusable() { return !usable(); }
	bool alloc(size_t sizex, size_t sizey){
		if (sizex == 0 || sizey == 0)
			return true;
		clear();
		vec.resize(sizex * sizey);
		sx = sizex;
		sy = sizey;
		return false;
	}
	inline void clear() { vec.clear(); sx = sy = 0; }
	inline bool out(int x, int y){
		if (x < 0 || x >= sx || y < 0 || y >= sy) return true;
		if (x * sy + y >= vec.size()) return true;
		return false;
	}
	template<is_coor T> inline bool out(T c){
		if (bx(c) < 0 || bx(c) >= sx
			|| by(c) < 0 || by(c) >= sy) return true;
		if (bx(c) * sy + by(c) >= vec.size()) return true;
		return false;
	}
	inline T& get(int x, int y){ return vec[x*sy+y]; }
	template<is_coor TC> T& get(TC c){ return vec[bx(c) * sy + by(c)]; }
	T* getptr(int x, int y){
		if (unusable() || out(x,y)) return nullptr;
		return &(vec[x*sy+y]);
	}
	template<is_coor TC> T* getptr(TC c){
		if (unusable() || out(c)) return nullptr;
		return &(vec[bx(c) * sy + by(c)]);
	}
	inline T getval(int x, int y){ return vec[x*sy + y]; }
	template<is_coor TC> T getval(TC c){
		return vec[bx(c) * sy + by(c)];
	}
	T getval(int x, int y, T default_v){
		if (unusable() || out(x,y)) return default_v;
		return vec[x*sy + y];
	}
	template<is_coor TC> T getval(TC c, T default_v){
		if (unusable() || out(c)) return default_v;
		return vec[bx(c) * sy + by(c)];
	}
	inline T& getref(int x, int y){ return vec[x*sy + y]; }
	template<is_coor TC> T& getref(TC c){
		return vec[bx(c) * sy + by(c)];
	}
	template<typename Tinvoke>
	requires std::invocable<Tinvoke, T&> || std::invocable<Tinvoke, T>
	bool each(Tinvoke func) {
		if (unusable()) return true;
		for(int i = 0 ; i < vec.size() ; i++) func(vec[i]);
		return false;
	}
	template<typename Tinvoke>
	requires (std::invocable<Tinvoke, T&> 
		&& std::is_same<std::invoke_result_t<Tinvoke, T&>, bool>::value)
		|| (std::invocable<Tinvoke, T>
		&& std::is_same<std::invoke_result_t<Tinvoke, T>, bool>::value)
	bool eachb(Tinvoke func) {
		if (unusable()) return true;
		for(int i = 0 ; i < vec.size() ;i++)
			if (func(vec[i])) return true;
		return false;
	}
	template<typename Tinvoke>
	requires std::invocable<Tinvoke, T&, int, int> 
		|| std::invocable<Tinvoke, T, int, int>
	bool eachxy(Tinvoke func) {
		if (unusable()) return true;
		for(int i = 0; i < sx; i++)
			for(int j = 0; j < sy; j++)
				func(vec[i*sy+j], i, j);
		return false;
	}
	template<typename Tinvoke>
	requires (std::invocable<Tinvoke, T&, int, int>
		&& (std::is_same<std::invoke_result_t<Tinvoke, T&, int, int>, bool>::value)
		|| (std::invocable<Tinvoke, T, int, int>)
		&& std::is_same<std::invoke_result_t<Tinvoke, T, int, int>, bool>::value)
	bool eachxyb(Tinvoke func) {
		if (unusable()) return true;
		for(int i = 0; i < sx; i++)
			for(int j = 0; j < sy; j++)
				if(func(vec[i*sy+j], i, j)) return true;
		return false;
	}
	bool set(int x, int y, T value){
		if (unusable() || out(x,y)) return true;
		vec[x*sy + y] = value;
		return false;
	}
	template<is_coor TC> bool set(TC c, T value){
		if (unusable() || out(c)) return true;
		vec[bx(c) * sy + by(c)] = value;
		return false;
	}
	bool set_all(T value){
		if (unusable()) return true;
		for(int i = 0 ; i < vec.size() ;i++) vec[i] = value;
		return false;
	}
	tdarray<T> copy_to(){
		if (unusable()) return tdarray<T>();
		tdarray<T> ta = tdarray<T>(sx, sy);
		if constexpr (std::is_trivially_copyable<T>::value) {
			std::copy(vec.begin(), vec.end(), ta.vec.begin());
		}
		else {
			for(int i = 0 ; i < vec.size() ;i++) ta.vec[i] = vec[i];
		}
		return ta;
	}
	void copy_from(tdarray<T>& target){
		if (target.unusable()) return;
		clear();
		alloc(target.sx, target.sy);
		if constexpr (std::is_trivially_copyable<T>::value) {
			std::copy(target.vec.begin(), target.vec.end(), vec.begin());
		}
		else {
			for(int i = 0 ; i < vec.size() ;i++) vec[i] = target.vec[i];
		}
	}

	template<typename T>
	auto operator [](T c) {
		if constexpr (is_coor_v<T>) return getref(c);
		else return &(vec[c * sy]);
	}
};

