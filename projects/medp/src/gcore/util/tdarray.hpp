#pragma once

#include "mydef.h"

#include <functional>
#include <vector>
#include <type_traits>
#include <cereal/types/vector.hpp>

// ---- tdarray<T> — 2D 陣列模板 ---------------------------------------------
// 單一 std::vector 打平存放，row-major：索引 = x*sy + y（x 是「列」，y 是「行」）。
// 已 cereal 化（serialize 存 sx, sy, vec），是 AreaTerrain 等 grid component 的底層容器。
//
// 使用前必知的三個慣例：
// 1. 回傳 bool 的操作（alloc/out/set/each…）一律「true = 失敗 / 越界 / 提早中斷」，
//    與直覺相反，呼叫端請注意。
// 2. 座標參數凡是接受 is_coor 的 overload，可傳 pair<int,int>、tuple<int,int>，
//    或任何有 .x/.y 成員的型別（例如 components/position.h 的 Position）。
// 3. 取值家族的差異：get/getref = 未檢查、回傳參照（兩者等價）；
//    getptr = 有檢查、越界回 nullptr；getval = 回傳複本，可帶越界時的 default 值。

// 可當座標用的型別：pair、tuple，或具 .x/.y 成員者。
template<typename T>
concept is_coor = std::is_same_v<T, std::pair<int, int>>
	|| std::is_same_v<T, std::tuple<int, int>>
	|| requires(T t) { t.x; t.y; };
LET_CONCEPT_BE_CHECKABLE_V(is_coor);

template<typename T>
class tdarray {
private:
	// bx/by：把任意 is_coor 型別統一解出 x / y 分量。
	template<is_coor TC>
	inline constexpr int bx(TC t) {
		if constexpr (std::is_same_v<TC, std::pair<int, int>>) return t.first;
		else if constexpr (std::is_same_v<TC, std::tuple<int, int>>) return std::get<0>(t);
		else return t.x;
	}
	template<is_coor TC>
	inline constexpr int by(TC t) {
		if constexpr (std::is_same_v<TC, std::pair<int, int>>) return t.second;
		else if constexpr (std::is_same_v<TC, std::tuple<int, int>>) return std::get<1>(t);
		else return t.y;
	}
	std::vector<T> vec;   // 打平的儲存區，大小恆為 sx*sy（usable 時）
public:
	size_t sx = 0, sy = 0;   // 維度；alloc/clear 之外不要直接改
	tdarray() {}
	tdarray(size_t sizex, size_t sizey): tdarray(){
		alloc(sizex, sizey);
	}
	template<class Archive>
	void serialize(Archive& ar) { ar(sx, sy, vec); }
	// usable = 已配置且 sx*sy 與實際大小一致；多數操作先以 unusable() 防衛。
	inline bool usable() { return vec.size() > 0 && sx > 0 && sy > 0 && sx * sy == vec.size(); }
	inline bool unusable() { return !usable(); }
	// 配置 sizex*sizey；會先 clear 舊內容。回傳 true = 失敗（任一維為 0）。
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
	// out：true = 座標越界。
	inline bool out(int x, int y){
		if (x < 0 || x >= sx || y < 0 || y >= sy) return true;
		if (x * sy + y >= vec.size()) return true;
		return false;
	}
	template<is_coor TC> inline bool out(TC c){
		if (bx(c) < 0 || bx(c) >= sx
			|| by(c) < 0 || by(c) >= sy) return true;
		if (bx(c) * sy + by(c) >= vec.size()) return true;
		return false;
	}
	// get / getref：未檢查、回傳參照（等價 API，見檔頭慣例 3）。
	inline T& get(int x, int y){ return vec[x*sy+y]; }
	template<is_coor TC> T& get(TC c){ return vec[bx(c) * sy + by(c)]; }
	// getptr：有檢查，未配置或越界回 nullptr。
	T* getptr(int x, int y){
		if (unusable() || out(x,y)) return nullptr;
		return &(vec[x*sy+y]);
	}
	template<is_coor TC> T* getptr(TC c){
		if (unusable() || out(c)) return nullptr;
		return &(vec[bx(c) * sy + by(c)]);
	}
	// getval：回傳複本；帶 default_v 的版本在未配置/越界時回傳 default_v。
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
	// ---- 遍歷家族 ----
	// each   ：對每格呼叫 func(T&)。回傳 true = 容器不可用（沒跑）。
	// eachb  ：func 回傳 bool，回傳 true 時提早中斷；整體回傳 true = 中斷或不可用。
	// eachxy ：func(T&, x, y)，依 x 外層、y 內層的順序走訪。
	// eachxyb：eachxy 的可中斷版本。
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
	// set：有檢查的寫入；回傳 true = 失敗（未配置或越界）。
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
	// copy_to / copy_from：深複製（trivially copyable 時走 std::copy 快路徑）。
	// copy_from 會先 clear 再改成 target 的維度。
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

	// operator[]：傳座標型別 = getref（單格參照）；傳整數 = 第 c 列的起始「指標」，
	// 供 arr[x][y] 兩段式索引使用。注意整數版不做越界檢查。
	template<typename TC>
	auto operator [](TC c) {
		if constexpr (is_coor_v<TC>) return getref(c);
		else return &(vec[c * sy]);
	}
};

