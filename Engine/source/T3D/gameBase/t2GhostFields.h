#ifndef _T2GHOSTFIELDS_H_
#define _T2GHOSTFIELDS_H_

namespace T2Protocol
{
   enum T2GhostField
   {
      T2FieldPosition,              ///< a compressed point, world space
      T2FieldYaw,                   ///< readFloat(bits) turns of heading
   };

   /// place. The reader checks both before it believes a row.
   struct T2GhostFieldRow
   {
      const char   *cls;
      S16           node;
      U8            op;
      U8            bits;
      T2GhostField  field;
   };

   static const T2GhostFieldRow sT2GhostFields[] =
   {
      { "Player", 25, OpPoint, 0, T2FieldPosition },
      { "Player", 19, OpRaw,   7, T2FieldYaw      },
   };
}

#endif
