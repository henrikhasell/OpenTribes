// HH: Added for Tribes 2 compatibility.
#ifndef _SHELLSKIN_H_
#define _SHELLSKIN_H_

#ifndef _GUITYPES_H_
#include "gui/core/guiTypes.h"
#endif

class GBitmap;

S32 shellSliceGrid( GBitmap *bitmap, RectI *out, S32 cols, S32 rows );

class ShellSkin
{
   enum { MaxParts = 9 };

   StringTableEntry mBase;
   StringTableEntry mSuffix[ MaxParts ];
   GFXTexHandle     mTexture[ MaxParts ];
   U32              mCount;

public:
   ShellSkin();

   void setBase( StringTableEntry base );

   GFXTexHandle get( const char *suffix );

   void clear();
};

#endif // _SHELLSKIN_H_
