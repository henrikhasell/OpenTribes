// HH: Added for Tribes 2 compatibility.
#ifndef _T2AFFINE_H_
#define _T2AFFINE_H_

#ifndef _T2FIELDS_H_
#include "T3D/gameBase/t2Fields.h"
#endif
#ifndef _MMATRIX_H_
#include "math/mMatrix.h"
#endif
#ifndef _MQUAT_H_
#include "math/mQuat.h"
#endif
#ifndef _MMATHFN_H_
#include "math/mMathFn.h"
#endif

/// The transform a Tribes 2 affine transform sends, rebuilt off the cursor.
///
/// Six machine words -- a position and three terms of a quaternion -- and then
/// a flag.  The fourth term's magnitude is recovered from the other three
/// rather than sent; the flag is its sign, and set means negative.
///
/// That last sentence is the whole reason this function exists.  It was
/// open-coded seven times in this tree and five of the seven dropped the flag
/// and took the positive root, four of them with a comment asserting the sign
/// was not available to a reader.  It was not, until T2Fields::takeFlag
/// landed.  A dropped sign is not a missing rotation but the mirrored one, and
/// no recorded session or second reader can see the difference: both readings
/// consume the same bits.  Two classes standing on one map read the same nine
/// words to different rotations for a week.
///
/// Which way the sign goes is settled three ways, none of them a majority
/// vote:
///
///   - the write side in this tree, which is the definition rather than a
///     reading of it: BitStream::writeAffineTransform ends
///     `writeFlag(q.w < 0.0)`, so the flag *is* the sign of w by
///     construction (core/stream/bitStream.cpp, and writeQuat does the same
///     at a lower precision);
///   - the read side beside it: BitStream::readAffineTransform recovers the
///     magnitude and then `if(readFlag()) q.w = -q.w;`;
///   - t2-demo-parser, which reads the retail Tribes 2 disassembly rather
///     than Torque, and whose BitStream.readAffineTransform computes the same
///     magnitude and negates on the same flag.
///
/// The cursor cannot call BitStream::readAffineTransform itself -- it reads
/// the layout, not the stream, and the words arrive as separate nodes -- which
/// is why the formula has to live somewhere, and why it should live in exactly
/// one place.
inline void t2AffineMatrix( const Point3F &pos, const Point3F &axis,
                            bool negative, MatrixF &out )
{
   QuatF q;
   q.x = axis.x;
   q.y = axis.y;
   q.z = axis.z;
   q.w = mSqrt( 1.0f - getMin( q.x * q.x + q.y * q.y + q.z * q.z, 1.0f ) );

   if ( negative )
      q.w = -q.w;

   q.setMatrix( &out );
   out.setColumn( 3, pos );
}

/// Take a flag the layout may not name.
///
/// T2Layout::ghostNode returns NoNode for a field its layout has no name for,
/// and T2Fields::takeFlag reports a node that is not a flag against the
/// layout -- formatting a message and interning it before it finds out the
/// message has been said already.  On a name that never resolves that is a
/// dSprintf and a string-table insert for every body, so the guard belongs on
/// this side of the call rather than inside the complaint.
inline bool takeT2Flag( T2Fields &f, U32 node, bool &out )
{
   if ( node == U32( T2Reader::NoNode ) )
      return false;

   return f.takeFlag( node, out );
}

#endif
