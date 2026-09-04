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

#ifndef _TYPESPOSIX_H_
#define _TYPESPOSIX_H_


#include <stdint.h>

#define FN_CDECL     ///< Calling convention

/// Allocation and pointer-difference size type. Used to overload operator new
/// and to size every allocation in the engine.
///
/// This was `unsigned int` -- 32 bits regardless of target. On a 64-bit build
/// that silently truncates every allocation size and every pointer arithmetic
/// result that passes through it, across the 55 files that name the type.
typedef uintptr_t dsize_t;


/** Platform dependent file date-time structure.  The defination of this structure
  * will likely be different for each OS platform.
  */
/// Was S32, which truncates a 64-bit time_t and breaks outright in 2038.
/// Spelled as the fixed-width type rather than S64 so this header does not
/// depend on being included after the S*/U* typedefs in types.gcc.h.
typedef int64_t FileTime;


#ifndef NULL
#  define NULL (0)
#endif


#endif //_TYPESPOSIX_H_
