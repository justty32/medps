cmake_minimum_required(VERSION 3.14)

set(PROJ medp_test)
project(${PROJ}_proj)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if("${CMAKE_BUILD_TYPE}" STREQUAL "")
	set(CMAKE_BUILD_TYPE Debug)
endif()

set(SRC "${CMAKE_CURRENT_SOURCE_DIR}/src")

file(GLOB_RECURSE src
    "${SRC}/*.cpp" "${SRC}/*.hpp" "${SRC}/*.h" "${SRC}/*.c"
)
source_group(TREE ${SRC} FILES ${src})

add_executable(${PROJ} ${src})

include_directories("${PROJECT_SOURCE_DIR}/../src")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_directories(${PROJ} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/lib")
target_link_libraries(${PROJ} medp_static)

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