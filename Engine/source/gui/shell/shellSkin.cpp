#include "platform/platform.h"
#include "gui/shell/shellSkin.h"

#include "core/stringTable.h"
#include "core/strings/stringFunctions.h"
#include "gfx/gfxDevice.h"

ShellSkin::ShellSkin()
{
   mBase = NULL;
   mCount = 0;
}

void ShellSkin::clear()
{
   for ( U32 i = 0; i < mCount; i++ )
   {
      mSuffix[ i ] = NULL;
      mTexture[ i ] = NULL;
   }

   mCount = 0;
}

void ShellSkin::setBase( StringTableEntry base )
{
   if ( base == mBase )
      return;

   clear();
   mBase = base;
}

GFXTexHandle ShellSkin::get( const char *suffix )
{
   if ( !mBase || !mBase[ 0 ] )
      return GFXTexHandle();

   if ( !suffix )
      suffix = "";

   StringTableEntry key = StringTable->insert( suffix );

   for ( U32 i = 0; i < mCount; i++ )
      if ( mSuffix[ i ] == key )
         return mTexture[ i ];

   if ( mCount == MaxParts )
      return GFXTexHandle();

   const U32 slot = mCount++;
   mSuffix[ slot ] = key;

   // A missing part is not an error: one stem serves several controls, and
   // each uses only the suffixes it knows about.
   //
   // The name is built from the interned copy rather than from what the caller
   // passed.  avar formats into one static buffer, so a caller that reached it
   // through avar would have its suffix overwritten by this very call.
   mTexture[ slot ].set( avar( "%s%s", mBase, key ), &GFXDefaultPersistentProfile,
                         avar( "%s() - mTexture (line %d)", __FUNCTION__, __LINE__ ) );

   return mTexture[ slot ];
}
