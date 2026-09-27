#include "platform/platform.h"
#include "gui/shell/shellWindowCtrl.h"

#include "console/console.h"
#include "console/engineAPI.h"
#include "core/strings/stringFunctions.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"
#include "gfx/bitmap/gBitmap.h"
#include "gfx/gFont.h"
#include "gui/shell/shellSkin.h"

IMPLEMENT_CONOBJECT( ShellWindowCtrl );

ConsoleDocClass( ShellWindowCtrl,
   "@brief A Tribes 2 shell window.\n\n"

   "A titled tab across the top -- one cap drawn at each end, the second of "
   "them mirrored, with the middle stretched between -- over a body that is a "
   "gradient, a fill, and the gradient again upside down.\n\n"

   "@ingroup GuiShell"
);

ShellWindowCtrl::ShellWindowCtrl()
{
   mWindowBase = StringTable->insert( "gui/window" );

   mSliced = false;
   mTitleHeight = 20;
   mMinBodyHeight = 2;
}

bool ShellWindowCtrl::onWake()
{
   if ( !Parent::onWake() )
      return false;

   const char *stem = mProfile->mBitmapBase;

   if ( !stem || !stem[ 0 ] )
      stem = "gui/window";

   {
      char name[ 256 ];

      dSprintf( name, sizeof( name ), "%s_fieldfill", stem );
      mFillTexture.set( name, &GFXDefaultPersistentProfile, avar( "%s() - fill", __FUNCTION__ ) );

      dSprintf( name, sizeof( name ), "%s_fieldgrade", stem );
      mGradeTexture.set( name, &GFXDefaultPersistentProfile, avar( "%s() - grade", __FUNCTION__ ) );

      dSprintf( name, sizeof( name ), "%s_titleTab", mWindowBase );
      mTitleTexture.set( name, &GFXDefaultPersistentProfile, avar( "%s() - title", __FUNCTION__ ) );

      dSprintf( name, sizeof( name ), "%s_close", mWindowBase );
      mCloseTexture.set( name, &GFXDefaultPersistentProfile, avar( "%s() - close", __FUNCTION__ ) );

      dSprintf( name, sizeof( name ), "%s_corner", mWindowBase );
      mCornerTexture.set( name, &GFXDefaultPersistentProfile, avar( "%s() - corner", __FUNCTION__ ) );

      mSliced = mTitleTexture &&
         shellSliceGrid( mTitleTexture->getBitmap(), mTitleCell, 1, 2 ) == 2;

      if ( mCloseTexture &&
           shellSliceGrid( mCloseTexture->getBitmap(), mCloseCell, 3, 1 ) != 1 )
         mCloseTexture = NULL;
   }

   if ( mSliced )
   {
      mTitleHeight = mTitleCell[ TitleCap ].extent.y;

      Point2I minExtent = getMinExtent();

      minExtent.x = getMax( minExtent.x, mTitleCell[ TitleCap ].extent.x * 2 );
      minExtent.y = getMax( minExtent.y, mTitleHeight + mMinBodyHeight );

      setMinExtent( minExtent );
      resize( getPosition(), getExtent() );
   }

   return true;
}

void ShellWindowCtrl::onSleep()
{
   Parent::onSleep();

   mTitleTexture = NULL;
   mCloseTexture = NULL;
   mCornerTexture = NULL;
   mFillTexture = NULL;
   mGradeTexture = NULL;

   mSliced = false;
}

void ShellWindowCtrl::onRender( Point2I offset, const RectI &updateRect )
{
   GFXDrawUtil *draw = GFX->getDrawUtil();
   draw->clearBitmapModulation();

   const S32 cap = mSliced ? mTitleCell[ TitleCap ].extent.x : 0;
   const S32 span = getWidth() - cap * 2;

   if ( mSliced )
   {
      Point2I at = offset;

      draw->drawBitmapSR( mTitleTexture, at, mTitleCell[ TitleCap ] );
      at.x += cap;

      if ( span > 0 )
         draw->drawBitmapStretchSR( mTitleTexture,
            RectI( at.x, at.y, span, mTitleHeight ), mTitleCell[ TitleMiddle ] );

      at.x += span;

      draw->drawBitmapSR( mTitleTexture, at, mTitleCell[ TitleCap ], GFXBitmapFlip_X );

      if ( mCloseTexture )
      {
         const RectI &close = mCloseCell[ 0 ];

         draw->drawBitmapSR( mCloseTexture,
            Point2I( offset.x + getWidth() - cap - close.extent.x,
                     offset.y + ( mTitleHeight - close.extent.y ) / 2 ),
            close );
      }

      const char *text = getText();

      if ( text && text[ 0 ] && span > 0 && mProfile->mFont )
      {
         RectI clip( offset.x + cap, offset.y, span, mTitleHeight );

         if ( clip.intersect( updateRect ) )
         {
            GFX->setClipRect( clip );

            GFont *font = mProfile->mFont;

            Point2I where( offset.x + cap + ( span - S32( font->getStrWidth( text ) ) ) / 2,
                           offset.y + ( mTitleHeight - S32( font->getHeight() ) ) / 2 );

            draw->setBitmapModulation( mProfile->mFontColor );
            draw->drawText( font, where, text, mProfile->mFontColors );
            draw->clearBitmapModulation();
         }
      }
   }

   const S32 bodyTop = offset.y + ( mSliced ? mTitleHeight : 0 );
   const S32 body = getHeight() - ( mSliced ? mTitleHeight : 0 );

   if ( mGradeTexture && body > 0 )
   {
      const S32 grade = S32( mGradeTexture->getBitmapHeight() );
      const bool roomy = body >= grade * 2;

      const S32 top = roomy ? grade : body / 2;
      const S32 foot = roomy ? grade : body - top;
      const S32 fill = body - top - foot;

      S32 y = bodyTop;

      draw->drawBitmapStretch( mGradeTexture, RectI( offset.x, y, getWidth(), top ) );
      y += top;

      if ( fill > 0 && mFillTexture )
         draw->drawBitmapStretch( mFillTexture, RectI( offset.x, y, getWidth(), fill ) );

      y += fill;

      draw->drawBitmapStretch( mGradeTexture, RectI( offset.x, y, getWidth(), foot ),
                               GFXBitmapFlip_Y );
   }

   renderChildControls( offset, updateRect );

   if ( mCornerTexture )
   {
      draw->clearBitmapModulation();
      draw->drawBitmap( mCornerTexture,
         Point2I( offset.x + getWidth() - S32( mCornerTexture->getBitmapWidth() ),
                  offset.y + getHeight() - S32( mCornerTexture->getBitmapHeight() ) ) );
   }
}
