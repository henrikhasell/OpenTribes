// HH: Added for Tribes 2 compatibility.
#ifndef _SHELLSKIN_H_
#define _SHELLSKIN_H_

#ifndef _GUITYPES_H_
#include "gui/core/guiTypes.h"
#endif

/// A set of textures sharing a stem.
///
/// A shell control does not always have one bitmap; it has a stem.  The
/// profile carries `bitmapBase` -- "gui/launch_btn" -- and the control appends
/// the suffixes it knows about, "" and "_rol" and "_act".  A suffix that
/// resolves to nothing stays null and simply does not draw, which is what lets
/// one stem serve controls that use different parts of it.
class ShellSkin
{
   enum { MaxParts = 8 };

   StringTableEntry mBase;
   StringTableEntry mSuffix[ MaxParts ];
   GFXTexHandle     mTexture[ MaxParts ];
   U32              mCount;

public:
   ShellSkin();

   /// Point the skin at a stem.  Does nothing if it is already there, so a
   /// control can call this from onRender without paying for it.
   void setBase( StringTableEntry base );

   /// The texture for a suffix, loading it the first time it is asked for.
   GFXTexHandle get( const char *suffix );

   void clear();
};

#endif // _SHELLSKIN_H_
