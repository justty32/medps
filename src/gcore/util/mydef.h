#pragma once

// ex. IS_MACRO_EMPTY() == true
// ex. IS_MACRO_EMPTY(qwer) == false
#define IS_MACRO_EMPTY(...) (sizeof( (char*)(""  #__VA_ARGS__ )) == 0 )

#define CAT_WITH_COMMA(A,B) A##,##B

// ex. FUNC_MAKE_CONSTEXPR_ARR_RECURSIZE(foo, 4, int, 3)
//     constexpr std::array<int, 4> arr = foo();
//     arr = {3,3,3,3}
#define FUNC_MAKE_CONSTEXPR_ARR_RECURSIZE(FUNC_NAME, SIZE, TVAL, VALUE)\
	template<size_t s = SIZE, int i = 0>\
	constexpr std::array<TVAL, s> FUNC_NAME() { \
		using arr_t = std::array<TVAL, s>; \
		if constexpr (i == s) { \
			return arr_t{}; \
		} \
		else { \
			arr_t narr = FUNC_NAME<s, i + 1>();\
			narr[i] = VALUE ;\
			return narr;\
		}\
	}

// ex. template<int i> constexpr int cal(){ return i * 2; }
//     FUNC_MAKE_CONSTEXPR_ARR_RECURSIZE(foo, 4, int, cal)
//     constexpr std::array<int, 4> arr = foo();
//     arr = {0,2,4,6};
#define FUNC_MAKE_CONSTEXPR_ARR_RECURSIZE_CAL(FUNC_NAME, SIZE, TVAL, CAL_FUNC)\
	template<size_t s = SIZE, int i = 0>\
	constexpr std::array<TVAL, s> FUNC_NAME() { \
		using arr_t = std::array<TVAL, s>; \
		if constexpr (i == s) { \
			return arr_t{}; \
		} \
		else { \
			arr_t narr = FUNC_NAME<s, i + 1>();\
			narr[i] = CAL_FUNC<i>() ;\
			return narr;\
		}\
	}

// example : LET_CONCEPT_BE_CHECKABLE(is_intte, std::is_integral<T>)
#define LET_CONCEPT_BE_CHECKABLE(STRUCT_NAME, CONCEPT)\
	template<typename T> \
	struct STRUCT_NAME {\
		template<CONCEPT T>\
		constexpr static bool func() { return true; } \
		template<typename T> \
		constexpr static bool func() { return false; } \
		constexpr static bool value = func<T>(); };

// ex : LET_CONCEPT_BE_CHECKABLE(std::is_integral<T>)
//		std::is_integral_v<int> == true
#define LET_CONCEPT_BE_CHECKABLE_V(CONCEPT)\
	LET_CONCEPT_BE_CHECKABLE(CONCEPT##_s, CONCEPT);\
	template<typename T>\
	constexpr bool CONCEPT##_v = CONCEPT##_s<T>::value;

