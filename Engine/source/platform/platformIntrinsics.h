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

#ifndef _PLATFORMINTRINSICS_H_
#define _PLATFORMINTRINSICS_H_

#ifndef _TORQUE_TYPES_H_
#  include "platform/types.h"
#endif

#if defined( TORQUE_COMPILER_VISUALC )
#  include "platform/platformIntrinsics.visualc.h"
#elif defined ( TORQUE_COMPILER_GCC )
#  include "platform/platformIntrinsics.gcc.h"
#else
#  error No intrinsics implemented for compiler.
#endif

// This carried a "TODO: 64bit safe" from the original authors. It reinterpreted
// the pointer slot as a volatile U32 and truncated both comparands, so on a
// 64-bit target the compare-and-swap operated on half a pointer -- it would
// have succeeded against a stale value whose low word happened to match.
// Dispatches on pointer width; both U32 and U64 overloads exist in
// platformIntrinsics.gcc.h.

template< typename T >
inline bool dCompareAndSwap( T* volatile& refPtr, T* oldPtr, T* newPtr )
{
#if defined(TORQUE_CPU_X86_64) || defined(TORQUE_CPU_ARM64)
   return dCompareAndSwap( *reinterpret_cast< volatile U64* >( &refPtr ),
                           ( U64 ) ( uintptr_t ) oldPtr,
                           ( U64 ) ( uintptr_t ) newPtr );
#else
   return dCompareAndSwap( *reinterpret_cast< volatile U32* >( &refPtr ),
                           ( U32 ) ( uintptr_t ) oldPtr,
                           ( U32 ) ( uintptr_t ) newPtr );
#endif
}

// Test-And-Set

inline bool dTestAndSet( volatile U32& ref )
{
   return dCompareAndSwap( ref, 0, 1 );
}
inline bool dTestAndSet( volatile U64& ref )
{
   return dCompareAndSwap( ref, 0, 1 );
}

#endif // _PLATFORMINTRINSICS_H_
