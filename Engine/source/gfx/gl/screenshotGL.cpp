#include "platform/platform.h"
#include "gfx/gl/screenshotGL.h"

#include "gfx/gfxDevice.h"
#include "gfx/gfxTarget.h"
#include "gfx/gl/gfxGLWindowTarget.h"
#include "gfx/bitmap/gBitmap.h"
#include "gfx/gl/tGL/tGL.h"


GBitmap* ScreenShotGL::_captureBackBuffer()
{
   GFXTarget *target = GFX->getActiveRenderTarget();
   if ( !target )
      return NULL;

   const Point2I size = target->getSize();
   if ( size.x < 1 || size.y < 1 )
      return NULL;

   GBitmap *bitmap = new GBitmap( size.x, size.y, false, GFXFormatR8G8B8 );

   // The scene lives in the window target's framebuffer, not the window's own.
   if ( GFXGLWindowTarget *wt = dynamic_cast< GFXGLWindowTarget* >( target ) )
   {
      glBindFramebuffer( GL_READ_FRAMEBUFFER, wt->getBackBufferFBO() );
      glReadBuffer( GL_COLOR_ATTACHMENT0 );
   }
   else
   {
      glReadBuffer( GL_BACK );
   }
   glPixelStorei( GL_PACK_ALIGNMENT, 1 );
   glReadPixels( 0, 0, size.x, size.y, GL_RGB, GL_UNSIGNED_BYTE, bitmap->getWritableBits() );

   // glReadPixels hands rows back bottom up relative to how GBitmap stores them.
   const bool needsFlip = true;

   const U32 stride = size.x * 3;
   U8 *scratch = new U8[ stride ];
   U8 *top = bitmap->getWritableBits();
   U8 *bottom = top + stride * ( size.y - 1 );

   for ( ; needsFlip && top < bottom; top += stride, bottom -= stride )
   {
      dMemcpy( scratch, top, stride );
      dMemcpy( top, bottom, stride );
      dMemcpy( bottom, scratch, stride );
   }

   delete [] scratch;

   return bitmap;
}
