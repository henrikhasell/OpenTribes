#include "platform/platform.h"
#include "gui/shell/shellSkin.h"

#include "core/stringTable.h"
#include "core/strings/stringFunctions.h"
#include "gfx/gfxDevice.h"
#include "gfx/bitmap/gBitmap.h"

S32 shellSliceGrid( GBitmap *bitmap, RectI *out, S32 cols, S32 rows )
{
   if ( !bitmap || !out || cols < 1 || rows < 1 )
      return 0;

   const S32 width = bitmap->getWidth();
   const S32 height = bitmap->getHeight();

   ColorI separator;
   if ( width < 1 || height < 1 || !bitmap->getColor( 0, 0, separator ) )
      return 0;

   S32 y = 0;
   S32 found = 0;

   for ( S32 row = 0; row < rows; row++ )
   {
      ColorI color;

      // The rule that opens the row states how wide a cell is: it runs in the
      // separator colour exactly, and stops where the art begins.
      S32 cellWidth = 0;
      while ( cellWidth < width )
      {
         if ( !bitmap->getColor( cellWidth, y, color ) || color != separator )
            break;
         cellWidth++;
      }

      y++;

      // And column zero below it states how tall: the run that is not the
      // separator.
      S32 cellHeight = 0;
      while ( cellHeight + y < height )
      {
         if ( !bitmap->getColor( 0, cellHeight + y, color ) || color == separator )
            break;
         cellHeight++;
      }

      // A row that does not fit is the sheet saying it has no more rows.
      if ( cellWidth < 3 || cellHeight < 3 || cellWidth * cols > width + 1 )
         break;

      for ( S32 col = 0; col < cols; col++ )
         out[ row * cols + col ] = RectI( cellWidth * col + 1, y + 1,
                                          cellWidth - 2, cellHeight - 2 );

      found = row + 1;

      y += cellHeight;
      if ( y >= height )
         break;
   }

   return found;
}

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
