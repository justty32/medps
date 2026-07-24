cmake_minimum_required(VERSION 3.14)

set(PROJ medp_game)
project(${PROJ}_proj)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(MSVC)
    add_compile_options(/utf-8)
else()
    # GCC/MinGW：明確指定原始碼與執行期字串都是 UTF-8，
    # 避免 Windows 系統字碼頁（CP950/CP936）污染字串常數。
    add_compile_options(-finput-charset=utf-8 -fexec-charset=utf-8)
endif()

if("${CMAKE_BUILD_TYPE}" STREQUAL "")
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(BUILD_PATH "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${BUILD_PATH}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${BUILD_PATH}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${BUILD_PATH}")

set(MEDP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../medp"
    CACHE PATH "Path to the sibling medp project")
set(MEDP_LIB_DIR "${MEDP_DIR}/build/bin"
    CACHE PATH "Directory containing the pre-built medp_static library")

find_library(MEDP_STATIC_LIB
    NAMES medp_static libmedp_static
    PATHS "${MEDP_LIB_DIR}"
    NO_DEFAULT_PATH)
if(NOT MEDP_STATIC_LIB)
    message(FATAL_ERROR
        "找不到 medp_static。請先建置 medp：\n"
        "  cmake -S ${MEDP_DIR} -B ${MEDP_DIR}/build && cmake --build ${MEDP_DIR}/build")
endif()

find_library(TCOD_STATIC_LIB
    NAMES tcod libtcod
    PATHS "${MEDP_LIB_DIR}"
    NO_DEFAULT_PATH)
if(NOT TCOD_STATIC_LIB)
    message(FATAL_ERROR
        "找不到 libtcod 靜態庫（應隨 medp 建置產出於 ${MEDP_LIB_DIR}）。")
endif()

set(SRC "${CMAKE_CURRENT_SOURCE_DIR}/src")
file(GLOB_RECURSE src "${SRC}/*.cpp" "${SRC}/*.hpp" "${SRC}/*.h" "${SRC}/*.c")
source_group(TREE ${SRC} FILES ${src})

add_executable(${PROJ} ${src})

target_include_directories(${PROJ} PRIVATE
    "${MEDP_DIR}/src"
    "${MEDP_DIR}/include")

target_link_libraries(${PROJ} "${MEDP_STATIC_LIB}" "${TCOD_STATIC_LIB}")

# Windows 系統庫
if(WIN32)
    target_link_libraries(${PROJ} winmm)
endif()

set_property(TARGET ${PROJ} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${BUILD_PATH}")

string(TOLOWER "${CMAKE_SYSTEM_NAME}" SYSTEM_NAME)
string(TOLOWER "${CMAKE_BUILD_TYPE}" BUILD_TYPE)
if(NOT DEFINED BITS)
    set(BITS 32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(BITS 64)
    endif()
endif()

set_property(TARGET ${PROJ} PROPERTY
    OUTPUT_NAME "${PROJ}.${SYSTEM_NAME}.${BUILD_TYPE}.${BITS}")
