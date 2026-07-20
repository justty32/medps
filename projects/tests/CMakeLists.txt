cmake_minimum_required(VERSION 3.14)

set(PROJ medp_test)
project(${PROJ}_proj)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# 原始碼帶 CJK 註解，明確告訴 MSVC 以 UTF-8 解碼（否則 C4819）。
if(MSVC)
	add_compile_options(/utf-8)
endif()

if("${CMAKE_BUILD_TYPE}" STREQUAL "")
	set(CMAKE_BUILD_TYPE Debug)
endif()

set(BUILD_PATH "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${BUILD_PATH}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${BUILD_PATH}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${BUILD_PATH}")

# ---- 平級的 medp 核心專案：本測試獨立建置，link 它「已建好」的 medp_static ----
# 先 build projects/medp（產出 medp_static 到 projects/medp/build/bin），再 build 本專案。
set(MEDP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../medp"
	CACHE PATH "Path to the sibling medp project (headers under src/ + include/)")
set(MEDP_LIB_DIR "${MEDP_DIR}/build/bin"
	CACHE PATH "Directory containing the pre-built medp_static library")

find_library(MEDP_STATIC_LIB
	NAMES medp_static libmedp_static
	PATHS "${MEDP_LIB_DIR}"
	NO_DEFAULT_PATH)
if(NOT MEDP_STATIC_LIB)
	message(FATAL_ERROR
		"找不到 medp_static。請先建置 medp：\n"
		"  cmake -S ${MEDP_DIR} -B ${MEDP_DIR}/build && cmake --build ${MEDP_DIR}/build\n"
		"或以 -DMEDP_LIB_DIR=<路徑> 指定已建好的庫所在目錄（目前找的是 ${MEDP_LIB_DIR}）。")
endif()

set(SRC "${CMAKE_CURRENT_SOURCE_DIR}/src")

file(GLOB_RECURSE src
    "${SRC}/*.cpp" "${SRC}/*.hpp" "${SRC}/*.h" "${SRC}/*.c"
)
source_group(TREE ${SRC} FILES ${src})

add_executable(${PROJ} ${src})

# medp 的核心 header 在 ../medp/src；entt / cereal 第三方 header 在 ../medp/include。
target_include_directories(${PROJ} PRIVATE
	"${MEDP_DIR}/src"
	"${MEDP_DIR}/include")
target_link_libraries(${PROJ} "${MEDP_STATIC_LIB}")

if(CMAKE_GENERATOR MATCHES "Visual Studio")
	target_link_libraries(${PROJ} winmm)
endif()
set_property(TARGET ${PROJ} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${BUILD_PATH}")

string(TOLOWER "${CMAKE_SYSTEM_NAME}" SYSTEM_NAME)
string(TOLOWER "${CMAKE_BUILD_TYPE}" BUILD_TYPE)
if(NOT DEFINED BITS)
	set(BITS 32)
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(BITS 64)
	endif(CMAKE_SIZEOF_VOID_P EQUAL 8)
endif()

if(ANDROID)
	# Added the android abi after system name
	set(SYSTEM_NAME ${SYSTEM_NAME}.${ANDROID_ABI})
	# Android does not have the bits at the end if you look at the main godot repo build
	set_property(TARGET ${PROJ} PROPERTY OUTPUT_NAME "${PROJ}.${SYSTEM_NAME}.${BUILD_TYPE}")
else()
	set_property(TARGET ${PROJ} PROPERTY OUTPUT_NAME "${PROJ}.${SYSTEM_NAME}.${BUILD_TYPE}.${BITS}")
endif()
