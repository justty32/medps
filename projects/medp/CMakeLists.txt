cmake_minimum_required(VERSION 3.14)

set(PROJ medp)
project(${PROJ}_proj)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Source files are UTF-8 (they carry CJK comments). Tell MSVC so explicitly,
# otherwise it decodes them as the system code page (e.g. cp950) and warns C4819.
if(MSVC)
	add_compile_options(/utf-8)
endif()

#set(BUILD_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(BUILD_PATH ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${BUILD_PATH}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${BUILD_PATH}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${BUILD_PATH}")
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${BUILD_PATH}")
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${BUILD_PATH}")
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${BUILD_PATH}")
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${BUILD_PATH}")
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${BUILD_PATH}")
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${BUILD_PATH}")

if("${CMAKE_BUILD_TYPE}" STREQUAL "")
	set(CMAKE_BUILD_TYPE Debug)
endif()

set(SRC "${CMAKE_CURRENT_SOURCE_DIR}/src")

file(GLOB_RECURSE src
    "${SRC}/*.cpp" "${SRC}/*.hpp" "${SRC}/*.h" "${SRC}/*.c"
)
# The core simulation must stay godot-free; gbind/ (the GDExtension facade)
# is built separately and is the only place allowed to include godot-cpp.
list(FILTER src EXCLUDE REGEX "/gbind/")
source_group(TREE ${SRC} FILES ${src})

add_library(${PROJ} SHARED ${src})
add_library("${PROJ}_static" STATIC ${src})

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_directories(${PROJ} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/lib")

# 測試現為平級獨立專案 projects/tests，自行 find/link 本專案的 medp_static，
# 不再由此 add_subdirectory。

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

file(REMOVE_RECURSE "${BUILD_PATH}/data")
file(COPY "${PROJECT_SOURCE_DIR}/data" DESTINATION "${BUILD_PATH}")

# ---- optional Godot 4 GDExtension facade ----
# OFF by default: godot-cpp's first build is slow (generates bindings + compiles
# the binding lib). Enable with -DMEDP_BUILD_GDEXTENSION=ON.
option(MEDP_BUILD_GDEXTENSION "Build the Godot 4 GDExtension facade (src/gbind)" OFF)
if(MEDP_BUILD_GDEXTENSION)
    set(GODOT_CPP_DIR "C:/code/mine/pas/projects/godot-cpp"
        CACHE PATH "Path to the godot-cpp checkout")
    add_subdirectory("${GODOT_CPP_DIR}" "${CMAKE_BINARY_DIR}/godot-cpp-build")

    file(GLOB_RECURSE gbind_src "${SRC}/gbind/*.cpp" "${SRC}/gbind/*.h")
    add_library(medp_gdext SHARED ${gbind_src})
    set_target_properties(medp_gdext PROPERTIES CXX_STANDARD 20 OUTPUT_NAME "medp_gdext")
    target_include_directories(medp_gdext PRIVATE "${SRC}")
    target_link_libraries(medp_gdext PRIVATE "${PROJ}_static" godot-cpp)
endif()