#pragma once

#if _MSC_VER
#define COMPILER_IS_MSVC
#elif defined(__GNUC__)
#define COMPILER_IS_GCC
#elif defined(__llvm__) || defined(__clang__)
#define COMPILER_IS_CLANG
#else
#define COMPILER_IS_UNKNOWN
#endif