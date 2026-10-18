//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#include "platform/platform.h"

#include "console/console.h"
#include "console/consoleTypes.h"
#include "gfx/bitmap/gBitmap.h"
#include "gui/core/guiControl.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxTextureHandle.h"
#include "gfx/gfxDrawUtil.h"
#include "console/engineAPI.h"
#include "math/mMathFn.h"
#include "core/resource.h"


class GuiChunkedBitmapCtrl : public GuiControl
{
private:
   typedef GuiControl Parent;
   enum { ChunkSize = 256 };

   void loadChunks(const char *name);
   void renderRegion(const Point2I &offset, const Point2I &extent);

protected:
   StringTableEntry mBitmapName;

   /// The bitmap cut into the exe's chunks, each a texture of its own.
   Vector<GFXTexHandle> mChunks;
   Point2I mBitmapSize;

   bool  mUseVariable;
   bool  mTile;

public:
   //creation methods
   DECLARE_CONOBJECT(GuiChunkedBitmapCtrl);
   DECLARE_CATEGORY( "Gui Images" );
   
   GuiChunkedBitmapCtrl();
   static void initPersistFields();

   //Parental methods
   bool onWake();
   void onSleep();

   void setBitmap(const char *name);

   void onRender(Point2I offset, const RectI &updateRect);
};

IMPLEMENT_CONOBJECT(GuiChunkedBitmapCtrl);

ConsoleDocClass( GuiChunkedBitmapCtrl,
   "@brief This is a control that will render a specified bitmap or a bitmap specified in a referenced variable.\n\n"

   "This control allows you to either set a bitmap with the \"bitmap\" field or with the setBitmap method.  You can also choose "
   "to reference a variable in the \"variable\" field such as \"$image\" and then set \"useVariable\" to true.  This will cause it to "
   "synchronize the variable with the bitmap displayed (if the variable holds a valid image).  You can then change the variable and "
   "effectively changed the displayed image.\n\n"

   "@tsexample\n"
   "$image = \"anotherbackground.png\";\n"
   "new GuiChunkedBitmapCtrl(ChunkedBitmap)\n"
   "{\n"
   "   bitmap = \"background.png\";\n"
   "   variable = \"$image\";\n"
   "   useVariable = false;\n"
   "}\n\n"
   "// This will result in the control rendering \"background.png\"\n"
   "// If we now set the useVariable to true it will now render \"anotherbackground.png\"\n"
   "ChunkedBitmap.useVariable = true;\n"
   "@endtsexample\n\n"

   "@see GuiControl::variable\n\n"

   "@ingroup GuiImages\n"
);


void GuiChunkedBitmapCtrl::initPersistFields()
{
   addGroup("GuiChunkedBitmapCtrl");		
   addField( "bitmap",        TypeFilename,  Offset( mBitmapName, GuiChunkedBitmapCtrl ), "This is the bitmap to render to the control." );
   addField( "useVariable",   TypeBool,      Offset( mUseVariable, GuiChunkedBitmapCtrl ), "This decides whether to use the \"bitmap\" file "
	                                                                                      "or a bitmap stored in \"variable\"");
   addField( "tile",          TypeBool,      Offset( mTile, GuiChunkedBitmapCtrl ), "This is no longer in use");
   endGroup("GuiChunkedBitmapCtrl");
   Parent::initPersistFields();
}

DefineEngineMethod( GuiChunkedBitmapCtrl, setBitmap, void, (const char* filename),,
   "@brief Set the image rendered in this control.\n\n"
   "@param filename The image name you want to set\n"
   "@tsexample\n"
   "ChunkedBitmap.setBitmap(\"images/background.png\");"
   "@endtsexample\n\n")
{
   object->setBitmap( filename );
}

GuiChunkedBitmapCtrl::GuiChunkedBitmapCtrl()
{
   mBitmapName = StringTable->insert("");
   mUseVariable = false;
   mTile = false;
   mBitmapSize.set( 0, 0 );
}

void GuiChunkedBitmapCtrl::setBitmap(const char *name)
{
   bool awake = mAwake;
   if(awake)
      onSleep();

   mBitmapName = StringTable->insert(name);
   if(awake)
      onWake();
   setUpdate();
}

bool GuiChunkedBitmapCtrl::onWake()
{
   if(!Parent::onWake())
      return false;

   if ( mChunks.empty() )
   {
      if ( mUseVariable )
         loadChunks( mConsoleVariable && mConsoleVariable[ 0 ] ? Con::getVariable( mConsoleVariable ) : NULL );
      else
         loadChunks( mBitmapName );
   }

   return true;
}

void GuiChunkedBitmapCtrl::onSleep()
{
   mChunks.clear();
   mBitmapSize.set( 0, 0 );
   Parent::onSleep();
}

void GuiChunkedBitmapCtrl::loadChunks(const char *name)
{
   mChunks.clear();
   mBitmapSize.set( 0, 0 );

   if ( !name || !name[ 0 ] )
      return;

   // Tribes 2 names its bitmaps from beneath textures/.
   Torque::Path path( name );
   if ( !path.getPath().startsWith( "textures" ) )
      path.setPath( Torque::Path::Join( "textures", '/', path.getPath() ) );

   Resource<GBitmap> bitmap = GBitmap::load( path );
   if ( bitmap == NULL )
      return;

   const U32 width = bitmap->getWidth();
   const U32 height = bitmap->getHeight();
   const U32 bpp = bitmap->getBytesPerPixel();

   // The exe cuts the bitmap into chunks of up to 256 pixels and pads each
   // out to a power of two with white. A chunk's far edges are filtered
   // against that white, which is why they come out lighter than the art.
   for ( U32 y = 0; y < height; y += ChunkSize )
   {
      for ( U32 x = 0; x < width; x += ChunkSize )
      {
         const U32 chunkWidth = getMin( U32( ChunkSize ), width - x );
         const U32 chunkHeight = getMin( U32( ChunkSize ), height - y );

         GBitmap *chunk = new GBitmap( getNextPow2( chunkWidth ), getNextPow2( chunkHeight ), false, bitmap->getFormat() );
         dMemset( chunk->getWritableBits(), 0xFF, chunk->getWidth() * chunk->getHeight() * bpp );

         for ( U32 row = 0; row < chunkHeight; row++ )
            dMemcpy( chunk->getAddress( 0, row ), bitmap->getAddress( x, y + row ), chunkWidth * bpp );

         mChunks.push_back( GFXTexHandle( chunk, &GFXDefaultGUIProfile, true,
            avar( "%s() - chunk (line %d)", __FUNCTION__, __LINE__ ) ) );
      }
   }

   mBitmapSize.set( width, height );
}

void GuiChunkedBitmapCtrl::renderRegion(const Point2I &offset, const Point2I &extent)
{
   const U32 width = mBitmapSize.x;
   const U32 height = mBitmapSize.y;
   if ( !width || !height )
      return;

   // The exe places each chunk by truncating its scaled edges, in double
   // precision from a float scale. The last chunk gets no special case, so a
   // scale that lands a hair under the extent leaves the last column undrawn.
   const F32 widthScale = F32( extent.x ) / F32( width );
   const F32 heightScale = F32( extent.y ) / F32( height );

   GFXDrawUtil *draw = GFX->getDrawUtil();
   draw->clearBitmapModulation();

   U32 chunk = 0;
   for ( U32 y = 0; y < height; y += ChunkSize )
   {
      const U32 chunkHeight = getMin( U32( ChunkSize ), height - y );
      const S32 top = S32( F64( y ) * heightScale + offset.y );
      const S32 tall = S32( F64( y + chunkHeight ) * heightScale + offset.y - top );

      for ( U32 x = 0; x < width; x += ChunkSize )
      {
         const U32 chunkWidth = getMin( U32( ChunkSize ), width - x );
         const S32 left = S32( F64( x ) * widthScale + offset.x );
         const S32 wide = S32( F64( x + chunkWidth ) * widthScale + offset.x - left );

         // Clamped, as the exe clamps its chunks.
         draw->drawBitmapStretchSR( mChunks[ chunk++ ], RectI( left, top, wide, tall ),
            RectI( 0, 0, chunkWidth, chunkHeight ), GFXBitmapFlip_None, GFXTextureFilterLinear, false );
      }
   }
}


void GuiChunkedBitmapCtrl::onRender(Point2I offset, const RectI &updateRect)
{
   // With no bitmap the exe draws the control as a plain one, fill and all.
   if ( mChunks.empty() )
   {
      Parent::onRender( offset, updateRect );
      return;
   }

   renderRegion( offset, getExtent() );
   renderChildControls(offset, updateRect);
}
