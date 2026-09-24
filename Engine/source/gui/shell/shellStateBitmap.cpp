#include "platform/platform.h"
#include "gui/shell/shellStateBitmap.h"

#include "console/console.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"
#include "gfx/bitmap/gBitmap.h"

ShellStateBitmap::ShellStateBitmap()
{
   mSource = NULL;
   mPieces = 0;
}

void ShellStateBitmap::update( GuiControlProfile *profile )
{
   // bitmapBase wins over bitmap.  A profile that names a family is the only
   // one that named anything: GuiControlProfile copies mBitmapName down from
   // GuiDefaultProfile, so every shell profile appears to carry a window frame
   // it never asked for.
   if ( !profile )
      return;

   load( ( profile->mBitmapBase && profile->mBitmapBase[ 0 ] )
            ? profile->mBitmapBase : profile->mBitmapName );
}

void ShellStateBitmap::load( const char *name )
{
   StringTableEntry source = ( name && name[ 0 ] ) ? StringTable->insert( name ) : NULL;

   if ( source == mSource )
      return;

   mSource = source;
   mTexture = NULL;
   mPieces = 0;

   if ( !source )
      return;

   if ( !mTexture.set( source, &GFXDefaultPersistentProfile,
                       avar( "%s() - mTexture (line %d)", __FUNCTION__, __LINE__ ) ) )
   {
      Con::errorf( "ShellStateBitmap::load - failed to load %s", source );
      mSource = NULL;
      return;
   }

   measure( mTexture->getBitmap() );
}

void ShellStateBitmap::measure( GBitmap *bitmap )
{
   if ( !bitmap || bitmap->getWidth() < 1 || bitmap->getHeight() < 1 )
      return;

   const S32 width = bitmap->getWidth();
   const S32 height = bitmap->getHeight();

   ColorI separator;
   if ( !bitmap->getColor( 0, 0, separator ) )
      return;

   S32 y = 0;

   for ( S32 piece = 0; piece < MaxPieces; piece++ )
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

      // A row that does not fit is the sheet saying it has no more rows.  Art
      // the size of the control it skins is one row of states and nothing
      // else, and that is not a failure -- it is a control drawn whole.
      if ( cellWidth < 3 || cellHeight < 3 || cellWidth * MaxStates > width + 1 )
         break;

      for ( S32 state = 0; state < MaxStates; state++ )
         mCell[ piece ][ state ] = RectI( cellWidth * state + 1, y + 1,
                                          cellWidth - 2, cellHeight - 2 );

      mPieces = piece + 1;

      y += cellHeight;
      if ( y >= height )
         break;
   }
}

void ShellStateBitmap::drawCell( S32 state, const Point2I &at ) const
{
   if ( !mPieces )
      return;

   if ( state < 0 )
      state = 0;
   else if ( state >= MaxStates )
      state = MaxStates - 1;

   GFX->getDrawUtil()->drawBitmapSR( mTexture, at, mCell[ 0 ][ state ] );
}

void ShellStateBitmap::draw( S32 state, const RectI &bounds ) const
{
   if ( !mPieces || bounds.extent.x < 1 || bounds.extent.y < 1 )
      return;

   if ( state < 0 )
      state = 0;
   else if ( state >= MaxStates )
      state = MaxStates - 1;

   GFXDrawUtil *draw = GFX->getDrawUtil();

   if ( mPieces < MaxPieces )
   {
      draw->drawBitmapStretchSR( mTexture, bounds, mCell[ 0 ][ state ] );
      return;
   }

   const RectI &left = mCell[ 0 ][ state ];
   const RectI &middle = mCell[ 1 ][ state ];
   const RectI &right = mCell[ 2 ][ state ];

   const S32 span = bounds.extent.x - left.extent.x - right.extent.x;

   // Too narrow to carry both caps: stretch the middle over the whole of it
   // rather than drawing caps that overlap.
   if ( span <= 0 )
   {
      draw->drawBitmapStretchSR( mTexture, bounds, middle );
      return;
   }

   // The caps are drawn at the size they were drawn at; only the middle gives.
   draw->drawBitmapSR( mTexture, bounds.point, left );

   draw->drawBitmapStretchSR( mTexture,
      RectI( bounds.point.x + left.extent.x, bounds.point.y, span, middle.extent.y ),
      middle );

   draw->drawBitmapSR( mTexture,
      Point2I( bounds.point.x + left.extent.x + span, bounds.point.y ), right );
}
