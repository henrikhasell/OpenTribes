#include "platform/platform.h"
#include "gui/shell/shellFieldCtrl.h"

#include "console/console.h"
#include "console/engineAPI.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"

IMPLEMENT_CONOBJECT( ShellFieldCtrl );

ConsoleDocClass( ShellFieldCtrl,
   "@brief The frame a Tribes 2 shell panel is drawn in.\n\n"

   "Nine pieces off the profile's stem -- corners, edges and a fill -- stretched "
   "over whatever the control was sized to.\n\n"

   "@ingroup GuiShell"
);

const char *ShellFieldCtrl::smSuffix[ PartCount ] =
{
   "_TL", "_TM", "_TR",
   "_ML", "_MM", "_MR",
   "_BL", "_BM", "_BR"
};

bool ShellFieldCtrl::parts( GFXTexHandle *out )
{
   mSkin.setBase( mProfile->mBitmapBase );

   bool complete = true;

   for ( U32 i = 0; i < PartCount; i++ )
   {
      out[ i ] = mSkin.get( smSuffix[ i ] );

      if ( !out[ i ] )
         complete = false;
   }

   return complete;
}

bool ShellFieldCtrl::onWake()
{
   if ( !Parent::onWake() )
      return false;

   GFXTexHandle part[ PartCount ];

   if ( parts( part ) )
   {
      setMinExtent( Point2I(
         S32( part[ TopLeft ]->getBitmapWidth() ) + S32( part[ TopRight ]->getBitmapWidth() ),
         S32( part[ BotLeft ]->getBitmapHeight() ) + 10 ) );

      resize( getPosition(), getExtent() );
   }

   return true;
}

void ShellFieldCtrl::onSleep()
{
   Parent::onSleep();
   mSkin.clear();
}

void ShellFieldCtrl::onRender( Point2I offset, const RectI &updateRect )
{
   GFXTexHandle part[ PartCount ];

   if ( parts( part ) )
   {
      GFXDrawUtil *draw = GFX->getDrawUtil();
      draw->clearBitmapModulation();

      const S32 left = S32( part[ TopLeft ]->getBitmapWidth() );
      const S32 right = S32( part[ TopRight ]->getBitmapWidth() );
      const S32 span = getWidth() - left - right;

      const S32 bottom = S32( part[ BotLeft ]->getBitmapHeight() );
      const S32 top = getMin( S32( part[ TopLeft ]->getBitmapHeight() ), getHeight() - bottom );
      const S32 middle = getHeight() - top - bottom;

      Point2I at = offset;

      draw->drawBitmapStretch( part[ TopLeft ], RectI( at.x, at.y, left, top ) );
      at.x += left;

      if ( span > 0 )
         draw->drawBitmapStretch( part[ TopMiddle ], RectI( at.x, at.y, span, top ) );

      at.x += span;
      draw->drawBitmapStretch( part[ TopRight ], RectI( at.x, at.y, right, top ) );

      at.x = offset.x;
      at.y += top;

      if ( middle > 0 )
      {
         const S32 midLeft = S32( part[ MidLeft ]->getBitmapWidth() );

         draw->drawBitmapStretch( part[ MidLeft ], RectI( at.x, at.y, midLeft, middle ) );
         at.x += midLeft;

         if ( span > 0 )
            draw->drawBitmapStretch( part[ MidMiddle ], RectI( at.x, at.y, span, middle ) );

         at.x += span;
         draw->drawBitmapStretch( part[ MidRight ],
            RectI( at.x, at.y, S32( part[ MidRight ]->getBitmapWidth() ), middle ) );

         at.x = offset.x;
      }

      at.y += middle;

      draw->drawBitmap( part[ BotLeft ], at );
      at.x += S32( part[ BotLeft ]->getBitmapWidth() );

      if ( span > 0 )
         draw->drawBitmapStretch( part[ BotMiddle ],
            RectI( at.x, at.y, span, S32( part[ BotMiddle ]->getBitmapHeight() ) ) );

      at.x += span;
      draw->drawBitmap( part[ BotRight ], at );
   }

   renderChildControls( offset, updateRect );
}
