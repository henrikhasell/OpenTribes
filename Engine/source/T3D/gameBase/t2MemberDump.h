// HH: Added for Tribes 2 compatibility.
#ifndef _T2MEMBERDUMP_H_
#define _T2MEMBERDUMP_H_

#ifndef _MMATRIX_H_
#include "math/mMatrix.h"
#endif
#ifndef _MQUAT_H_
#include "math/mQuat.h"
#endif
#ifndef _MRECT_H_
#include "math/mRect.h"
#endif
#ifndef _COLOR_H_
#include "core/color.h"
#endif
#ifndef _TORQUE_STRING_H_
#include "core/util/str.h"
#endif

/// What a class put a Tribes 2 body into, so a value check can look at it.
///
/// The value check reads a class's state back through console reflection, and
/// for a ghost that is the wrong instrument.  A `SceneObject` registers
/// `locked`, `canSave`, `isRenderEnabled` and `mountNode`; what a Tribes 2
/// reader fills is `mObjToWorld`, `mCRC`, `mT2SkinBase`, `mTerrFileName` --
/// members with no console field at all.  On one session that left 956 engine
/// fields and 222 parser names uncompared against 43 that matched, over seven
/// classes of the fifty-three.  Reflection cannot see what the port decodes,
/// so the port has to say.
///
/// A class overrides dumpT2Members() beside its readT2Fields() and writes
/// what that reader filed:
///
///     void InteriorInstance::dumpT2Members( T2Dump& d ) const
///     {
///        Parent::dumpT2Members( d );
///        d.put( "crc", mCRC );
///        d.put( "interiorFile", mInteriorFileName );
///     }
///
/// Three things about this that are not obvious.
///
/// **The name is an assertion, not a label.**  These are the second reader's
/// field names, because a name is the key the two readings are joined on and
/// a dump keyed on engine member names would compare against nothing at all.
/// So `d.put("crc", mCRC)` claims that this member is that field.  A wrong
/// claim shows up as a disagreement and gets looked at, which is the right
/// way for it to fail -- and worth keeping in mind, because those names are a
/// hypothesis transcribed from a retail field list and have been wrong
/// before.
///
/// **A dump that falls behind its reader loses coverage quietly.**  Nothing
/// here can tell that `readT2Fields` fills a member this does not print; the
/// field simply goes on being counted among the ones the parser names and the
/// engine does not.  That is why the counts are reported rather than only the
/// verdict: the gap is the measurement.
///
/// **Structured values are flattened here rather than in the comparison.**  A
/// transform is `transform.elements.0` through `.15`, a point is `.x .y .z`,
/// a colour `.r .g .b`.  `valuecheck.py` flattens the second reader's nested
/// JSON the same way, so the two meet on the dotted name and the comparison
/// stays a scalar one.
class T2Dump
{
public:
   /// Where a line goes.  Kept as a sink rather than a stream so that the
   /// engine side of the check stays the only thing that knows the file
   /// format -- this header is included by a hundred classes and should not
   /// drag a FileStream in with it.
   struct Sink
   {
      virtual ~Sink() {}
      virtual void line( const char* name, const char* value ) = 0;
   };

   T2Dump( Sink& sink ) : mSink( sink ) {}

   void put( const char* name, const char* v ) { mSink.line( name, v ? v : "" ); }
   void put( const char* name, const String& v ) { put( name, v.c_str() ); }

   void put( const char* name, bool v ) { put( name, v ? "1" : "0" ); }
   void put( const char* name, S32 v ) { num( name, "%d", double( v ) ); }
   void put( const char* name, U32 v ) { num( name, "%u", double( v ) ); }
   void put( const char* name, F32 v ) { num( name, "%g", double( v ) ); }

   void put( const char* name, const Point2F& p )
   {
      sub( name, "x", p.x ); sub( name, "y", p.y );
   }

   void put( const char* name, const Point3F& p )
   {
      sub( name, "x", p.x ); sub( name, "y", p.y ); sub( name, "z", p.z );
   }

   void put( const char* name, const QuatF& q )
   {
      sub( name, "x", q.x ); sub( name, "y", q.y );
      sub( name, "z", q.z ); sub( name, "w", q.w );
   }

   void put( const char* name, const ColorF& c )
   {
      sub( name, "r", c.red ); sub( name, "g", c.green );
      sub( name, "b", c.blue ); sub( name, "a", c.alpha );
   }

   /// A ColorI, whose components the second reader reports as the bytes they
   /// arrived as rather than as fractions.
   void put( const char* name, const ColorI& c )
   {
      sub( name, "r", F32( c.red ) ); sub( name, "g", F32( c.green ) );
      sub( name, "b", F32( c.blue ) );
   }

   void put( const char* name, const RectI& r )
   {
      sub( name, "x", F32( r.point.x ) ); sub( name, "y", F32( r.point.y ) );
      sub( name, "w", F32( r.extent.x ) ); sub( name, "h", F32( r.extent.y ) );
   }

   /// A transform, as both of the two shapes the second reader reports one
   /// in: the sixteen elements, and the position and rotation apart.  Which
   /// of the two it gives varies by class, so giving both means the
   /// comparison joins on whichever arrived instead of on neither.
   void put( const char* name, const MatrixF& m )
   {
      // MatrixF::idx is (col + row*4) and getColumn(3) reads m[3], m[7],
      // m[11], so Torque's sixteen floats are already in the order the second
      // reader reports them in -- row-major, translation in the last column.
      // Checked rather than assumed: a transposed dump would make a correct
      // transform disagree in twelve of sixteen places and a mirrored one
      // agree, and a mirrored transform read as right is exactly the fault
      // this check exists for.
      char key[128];
      for( U32 i = 0; i < 16; i++ )
      {
         dSprintf( key, sizeof( key ), "%s.elements.%u", name, i );
         num( key, "%g", double( m( i / 4, i % 4 ) ) );
      }

      Point3F pos;
      m.getColumn( 3, &pos );
      char nested[128];
      dSprintf( nested, sizeof( nested ), "%s.position", name );
      put( nested, pos );

      MatrixF rot( m );
      rot.setPosition( Point3F::Zero );
      dSprintf( nested, sizeof( nested ), "%s.rotation", name );
      put( nested, QuatF( rot ) );
   }

   /// One element of a list the second reader reports as a JSON array, named
   /// as valuecheck.py flattens one: `materialNames.3`.
   /// @{
   void at( const char* name, U32 i, const char* v )
   {
      char key[128];
      dSprintf( key, sizeof( key ), "%s.%u", name, i );
      put( key, v );
   }

   void at( const char* name, U32 i, F32 v )
   {
      char key[128];
      dSprintf( key, sizeof( key ), "%s.%u", name, i );
      put( key, v );
   }

   void at( const char* name, U32 i, S32 v )
   {
      char key[128];
      dSprintf( key, sizeof( key ), "%s.%u", name, i );
      put( key, v );
   }

   void at( const char* name, U32 i, U32 v )
   {
      char key[128];
      dSprintf( key, sizeof( key ), "%s.%u", name, i );
      put( key, v );
   }
   /// @}

   /// The name a nested value of a list element takes:
   /// `fogVolumes.1.visibleDistance`.
   static void nest( char* out, U32 size, const char* name, U32 i,
                     const char* field )
   {
      dSprintf( out, size, "%s.%u.%s", name, i, field );
   }

private:
   void num( const char* name, const char* fmt, double v )
   {
      char buf[64];
      // Printed through one width for every number, so that a reading can be
      // compared as text when it is not a float -- valuecheck.py falls back
      // to a string compare and a value printed two ways would differ there
      // for no reason.
      if( !dStrcmp( fmt, "%g" ) )
         dSprintf( buf, sizeof( buf ), "%g", v );
      else if( !dStrcmp( fmt, "%u" ) )
         dSprintf( buf, sizeof( buf ), "%u", U32( v ) );
      else
         dSprintf( buf, sizeof( buf ), "%d", S32( v ) );
      mSink.line( name, buf );
   }

   void sub( const char* name, const char* field, F32 v )
   {
      char key[128];
      dSprintf( key, sizeof( key ), "%s.%s", name, field );
      num( key, "%g", double( v ) );
   }

   Sink& mSink;
};

#endif
