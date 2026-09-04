//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#ifndef _TYPESGCC_H
#define _TYPESGCC_H


// For additional information on GCC predefined macros
// http://gcc.gnu.org/onlinedocs/gcc-3.0.2/cpp.html


//--------------------------------------
// Types
typedef signed long long    S64;
typedef unsigned long long  U64;


//--------------------------------------
// Compiler Version
#define TORQUE_COMPILER_GCC (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)


//--------------------------------------
// Identify the compiler string

#if defined(__MINGW32__)
#  define TORQUE_COMPILER_STRING "GCC (MinGW)"
#  define TORQUE_COMPILER_MINGW
#elif defined(__CYGWIN__)
#  define TORQUE_COMPILER_STRING "GCC (Cygwin)"
#  define TORQUE_COMPILER_MINGW
#else
#  define TORQUE_COMPILER_STRING "GCC "
#endif


//--------------------------------------
// Identify the Operating System
// The Win32, PS3 and macOS branches went with their platform layers. Detection
// now uses __linux__ rather than the bare `linux` macro, which only exists in
// GNU dialects and vanishes under a strict -std=c++NN.
#if defined(__linux__)
#  define TORQUE_OS_STRING "Linux"
#  define TORQUE_OS_LINUX
#  include "platform/types.posix.h"

#elif defined(__OpenBSD__)
#  define TORQUE_OS_STRING "OpenBSD"
#  define TORQUE_OS_OPENBSD
#  include "platform/types.posix.h"

#elif defined(__FreeBSD__)
#  define TORQUE_OS_STRING "FreeBSD"
#  define TORQUE_OS_FREEBSD
#  include "platform/types.posix.h"

#else
#  error "GCC: Unsupported Operating System"
#endif


//--------------------------------------
// Identify the CPU
//
// There was no x86_64 case here at all. `i386` is not defined on a 64-bit
// target, so every 64-bit build fell through to the #error below -- the
// original tree could not be compiled for x86_64 even in principle.
//
// TORQUE_SUPPORTS_NASM is deliberately not defined on any target: the .asm
// sources it gated (math/mMathSSE.cpp, math/mMathAMD.cpp) are 32-bit.
#if defined(__x86_64__) || defined(_M_X64)
#  define TORQUE_CPU_STRING "x86_64"
#  define TORQUE_CPU_X86_64
#  define TORQUE_LITTLE_ENDIAN
#  define TORQUE_SUPPORTS_GCC_INLINE_X86_ASM

#elif defined(__i386__) || defined(i386)
#  define TORQUE_CPU_STRING "Intel x86"
#  define TORQUE_CPU_X86
#  define TORQUE_LITTLE_ENDIAN
#  define TORQUE_SUPPORTS_GCC_INLINE_X86_ASM

#elif defined(__aarch64__)
#  define TORQUE_CPU_STRING "ARM64"
#  define TORQUE_CPU_ARM64
#  define TORQUE_LITTLE_ENDIAN

#else
#  error "GCC: Unsupported Target CPU"
#endif

#ifndef Offset
/// Offset macro:
/// Calculates the location in memory of a given member x of class cls from the
/// start of the class.  Need several definitions to account for various
/// flavors of GCC.

// now, for each compiler type, define the Offset macros that should be used.
// The Engine code usually uses the Offset macro, but OffsetNonConst is needed
// when a variable is used in the indexing of the member field (see
// TSShapeConstructor::initPersistFields for an example)

// compiler is non-GCC, or gcc < 3
#if (__GNUC__ < 3)
#define Offset(x, cls) _Offset_Normal(x, cls)
#define OffsetNonConst(x, cls) _Offset_Normal(x, cls)

// compiler is GCC 3 with minor version less than 4
#elif defined(TORQUE_COMPILER_GCC) && (__GNUC__ == 3) && (__GNUC_MINOR__ < 4)
#define Offset(x, cls) _Offset_Variant_1(x, cls)
#define OffsetNonConst(x, cls) _Offset_Variant_1(x, cls)

// compiler is GCC 3 with minor version greater than 4
#elif defined(TORQUE_COMPILER_GCC) && (__GNUC__ == 3) && (__GNUC_MINOR__ >= 4)
#include <stddef.h>
#define Offset(x, cls) _Offset_Variant_2(x, cls)
#define OffsetNonConst(x, cls) _Offset_Variant_1(x, cls)

// compiler is GCC 4
#elif defined(TORQUE_COMPILER_GCC) && (__GNUC__ == 4)
#include <stddef.h>
#define Offset(x, cls) _Offset_Normal(x, cls)
#define OffsetNonConst(x, cls) _Offset_Variant_1(x, cls)

#endif
#endif

#endif // INCLUDED_TYPES_GCC_H

