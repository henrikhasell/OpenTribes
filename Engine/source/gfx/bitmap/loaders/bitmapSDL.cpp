#include "platform/platform.h"

#include "console/console.h"
#include "core/stream/stream.h"
#include "gfx/bitmap/gBitmap.h"
#include "gfx/bitmap/pngUtils.h"

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include "zlib/zlib.h"


//-----------------------------------------------------------------------------
// SDL_IOStream over a Torque Stream, so SDL3_image reads and writes through the
// virtual filesystem rather than opening paths itself.
//-----------------------------------------------------------------------------

static Sint64 SDLCALL _ioSize( void *userdata )
{
   return Sint64( static_cast< Stream* >( userdata )->getStreamSize() );
}

static Sint64 SDLCALL _ioSeek( void *userdata, Sint64 offset, SDL_IOWhence whence )
{
   Stream *stream = static_cast< Stream* >( userdata );

   Sint64 target = offset;
   if ( whence == SDL_IO_SEEK_CUR )
      target += stream->getPosition();
   else if ( whence == SDL_IO_SEEK_END )
      target += stream->getStreamSize();

   if ( target < 0 || !stream->setPosition( U32( target ) ) )
      return -1;

   return Sint64( stream->getPosition() );
}

static size_t SDLCALL _ioRead( void *userdata, void *ptr, size_t size, SDL_IOStatus *status )
{
   Stream *stream = static_cast< Stream* >( userdata );

   // Torque's read is all-or-nothing, so clamp to what is left rather than
   // letting a short tail read fail outright.
   const U32 remaining = stream->getStreamSize() - stream->getPosition();
   const U32 count = getMin( U32( size ), remaining );

   if ( count == 0 )
   {
      *status = SDL_IO_STATUS_EOF;
      return 0;
   }

   if ( !stream->read( count, ptr ) )
   {
      *status = SDL_IO_STATUS_ERROR;
      return 0;
   }

   return count;
}

static size_t SDLCALL _ioWrite( void *userdata, const void *ptr, size_t size, SDL_IOStatus *status )
{
   if ( !static_cast< Stream* >( userdata )->write( U32( size ), ptr ) )
   {
      *status = SDL_IO_STATUS_ERROR;
      return 0;
   }

   return size;
}

static bool SDLCALL _ioFlush( void *userdata, SDL_IOStatus *status )
{
   TORQUE_UNUSED( userdata );
   TORQUE_UNUSED( status );
   return true;
}

static bool SDLCALL _ioClose( void *userdata )
{
   // The Stream outlives the SDL_IOStream; closing it is the caller's business.
   TORQUE_UNUSED( userdata );
   return true;
}

static SDL_IOStream *_openStream( Stream &stream )
{
   SDL_IOStreamInterface iface;
   SDL_INIT_INTERFACE( &iface );

   iface.size  = _ioSize;
   iface.seek  = _ioSeek;
   iface.read  = _ioRead;
   iface.write = _ioWrite;
   iface.flush = _ioFlush;
   iface.close = _ioClose;

   return SDL_OpenIO( &iface, &stream );
}


//-----------------------------------------------------------------------------
// PNG chunk writing
//-----------------------------------------------------------------------------

static void _writeU32BE( U8 *out, U32 value )
{
   out[0] = U8( value >> 24 );
   out[1] = U8( value >> 16 );
   out[2] = U8( value >> 8 );
   out[3] = U8( value );
}

static bool _writeChunk( Stream &stream, const char *type, const U8 *data, U32 length )
{
   U8 header[8];
   _writeU32BE( header, length );
   dMemcpy( header + 4, type, 4 );

   uLong crc = crc32( 0L, Z_NULL, 0 );
   crc = crc32( crc, header + 4, 4 );
   if ( length )
      crc = crc32( crc, data, length );

   U8 trailer[4];
   _writeU32BE( trailer, U32( crc ) );

   bool ok = stream.write( sizeof( header ), header );
   if ( length )
      ok &= stream.write( length, data );
   ok &= stream.write( sizeof( trailer ), trailer );

   return ok;
}


//-----------------------------------------------------------------------------
// Surface <-> GBitmap
//-----------------------------------------------------------------------------

/// GBitmap stores channels in memory order, which is what the RGB24 and RGBA32
/// byte-order formats give us.
static SDL_PixelFormat _sdlFormatFor( GFXFormat format )
{
   switch ( format )
   {
      case GFXFormatR8G8B8:    return SDL_PIXELFORMAT_RGB24;
      case GFXFormatR8G8B8A8:
      case GFXFormatR8G8B8X8:  return SDL_PIXELFORMAT_RGBA32;
      default:                 return SDL_PIXELFORMAT_UNKNOWN;
   }
}

static bool _surfaceToBitmap( SDL_Surface *surface, GBitmap *bitmap )
{
   const SDL_PixelFormatDetails *details = SDL_GetPixelFormatDetails( surface->format );
   if ( !details )
      return false;

   const bool hasAlpha = details->Amask != 0 || SDL_SurfaceHasColorKey( surface );
   const GFXFormat format = hasAlpha ? GFXFormatR8G8B8A8 : GFXFormatR8G8B8;

   SDL_Surface *converted = SDL_ConvertSurface( surface, _sdlFormatFor( format ) );
   if ( !converted )
   {
      Con::errorf( "bitmapSDL - SDL_ConvertSurface failed: %s", SDL_GetError() );
      return false;
   }

   bitmap->allocateBitmap( U32( converted->w ), U32( converted->h ), false, format );

   const U32 rowBytes = bitmap->getWidth() * bitmap->getBytesPerPixel();
   const U8 *src = static_cast< const U8* >( converted->pixels );
   U8 *dst = bitmap->getWritableBits();

   for ( S32 y = 0; y < converted->h; ++y, src += converted->pitch, dst += rowBytes )
      dMemcpy( dst, src, rowBytes );

   SDL_DestroySurface( converted );
   return true;
}

/// Wraps the bitmap's pixels without copying; valid only while the bitmap is.
static SDL_Surface *_bitmapToSurface( GBitmap *bitmap )
{
   const SDL_PixelFormat format = _sdlFormatFor( bitmap->getFormat() );
   if ( format == SDL_PIXELFORMAT_UNKNOWN )
   {
      Con::errorf( "bitmapSDL - cannot write a bitmap in format %d", bitmap->getFormat() );
      return NULL;
   }

   return SDL_CreateSurfaceFrom( S32( bitmap->getWidth() ), S32( bitmap->getHeight() ),
                                 format, bitmap->getWritableBits(),
                                 S32( bitmap->getWidth() * bitmap->getBytesPerPixel() ) );
}


//-----------------------------------------------------------------------------
// Greyscale PNG
//
// The .uft font caches store their glyph sheets as 8-bit greyscale PNG, and
// GFont::addBitmap asserts the sheets are GFXFormatA8. A decoder that promotes
// everything to RGB turns the sheet opaque, which draws every run of text on a
// black box. Both directions are handled here so the caches keep working and
// newly written ones can be read back.
//-----------------------------------------------------------------------------

static bool _isGreyscalePng( Stream &stream )
{
   const U32 start = stream.getPosition();

   U8 header[26];
   const bool read = stream.read( sizeof( header ), header );
   stream.setPosition( start );

   if ( !read || dMemcmp( header, "\x89PNG\r\n\x1a\n", 8 ) != 0 )
      return false;

   // IHDR bit depth at 24, colour type at 25; 0 is greyscale without alpha.
   return header[24] == 8 && header[25] == 0;
}

static bool _surfaceToAlphaBitmap( SDL_Surface *surface, GBitmap *bitmap )
{
   SDL_Surface *converted = SDL_ConvertSurface( surface, SDL_PIXELFORMAT_RGBA32 );
   if ( !converted )
   {
      Con::errorf( "bitmapSDL - SDL_ConvertSurface failed: %s", SDL_GetError() );
      return false;
   }

   bitmap->allocateBitmap( U32( converted->w ), U32( converted->h ), false, GFXFormatA8 );

   const U8 *src = static_cast< const U8* >( converted->pixels );
   U8 *dst = bitmap->getWritableBits();

   for ( S32 y = 0; y < converted->h; ++y, src += converted->pitch )
      for ( S32 x = 0; x < converted->w; ++x )
         *dst++ = src[ x * 4 ]; // grey, so any colour channel carries the value

   SDL_DestroySurface( converted );
   return true;
}

/// Minimal 8-bit greyscale encoder. SDL3_image writes RGB or RGBA whatever it
/// is given, and a round trip through those would not come back as A8.
static bool _writeGreyPNG( GBitmap *bitmap, Stream &stream )
{
   const U32 width = bitmap->getWidth();
   const U32 height = bitmap->getHeight();

   // Each row is prefixed with its filter type, 0 meaning none.
   const U32 rawSize = height * ( width + 1 );
   U8 *raw = new U8[ rawSize ];

   for ( U32 y = 0; y < height; ++y )
   {
      U8 *row = raw + y * ( width + 1 );
      *row++ = 0;
      dMemcpy( row, bitmap->getAddress( 0, y ), width );
   }

   uLongf deflatedSize = compressBound( rawSize );
   U8 *deflated = new U8[ deflatedSize ];
   const int zErr = compress2( deflated, &deflatedSize, raw, rawSize, Z_DEFAULT_COMPRESSION );

   delete [] raw;

   if ( zErr != Z_OK )
   {
      delete [] deflated;
      Con::errorf( "bitmapSDL - deflate failed writing greyscale PNG (%d)", zErr );
      return false;
   }

   bool ok = stream.write( 8, "\x89PNG\r\n\x1a\n" );

   U8 ihdr[13];
   _writeU32BE( ihdr + 0, width );
   _writeU32BE( ihdr + 4, height );
   ihdr[8]  = 8; // bit depth
   ihdr[9]  = 0; // colour type: greyscale
   ihdr[10] = 0; // deflate
   ihdr[11] = 0; // adaptive filtering
   ihdr[12] = 0; // no interlace

   ok &= _writeChunk( stream, "IHDR", ihdr, sizeof( ihdr ) );
   ok &= _writeChunk( stream, "IDAT", deflated, U32( deflatedSize ) );
   ok &= _writeChunk( stream, "IEND", NULL, 0 );

   delete [] deflated;
   return ok;
}


//-----------------------------------------------------------------------------
// Registration
//-----------------------------------------------------------------------------

static bool sReadSDL( Stream &stream, GBitmap *bitmap )
{
   const bool greyscale = _isGreyscalePng( stream );

   SDL_IOStream *io = _openStream( stream );
   if ( !io )
      return false;

   // Let SDL sniff the contents rather than trusting the extension -- the
   // registration that routed us here only matched the file name.
   SDL_Surface *surface = IMG_Load_IO( io, false );
   SDL_CloseIO( io );

   if ( !surface )
   {
      Con::errorf( "bitmapSDL - decode failed: %s", SDL_GetError() );
      return false;
   }

   const bool ok = greyscale ? _surfaceToAlphaBitmap( surface, bitmap )
                             : _surfaceToBitmap( surface, bitmap );
   SDL_DestroySurface( surface );

   return ok;
}

static bool sWritePNG( GBitmap *bitmap, Stream &stream, U32 compressionLevel )
{
   TORQUE_UNUSED( compressionLevel ); // SDL3_image picks its own

   if ( bitmap->getFormat() == GFXFormatA8 )
      return _writeGreyPNG( bitmap, stream );

   SDL_Surface *surface = _bitmapToSurface( bitmap );
   if ( !surface )
      return false;

   SDL_IOStream *io = _openStream( stream );
   const bool ok = io && IMG_SavePNG_IO( surface, io, false );

   if ( io )
      SDL_CloseIO( io );
   SDL_DestroySurface( surface );

   return ok;
}

static bool sWriteJPG( GBitmap *bitmap, Stream &stream, U32 compressionLevel )
{
   SDL_Surface *surface = _bitmapToSurface( bitmap );
   if ( !surface )
      return false;

   SDL_IOStream *io = _openStream( stream );
   const bool ok = io && IMG_SaveJPG_IO( surface, io, false, S32( compressionLevel ) );

   if ( io )
      SDL_CloseIO( io );
   SDL_DestroySurface( surface );

   return ok;
}

static bool sWriteBMP( GBitmap *bitmap, Stream &stream, U32 compressionLevel )
{
   TORQUE_UNUSED( compressionLevel );

   SDL_Surface *surface = _bitmapToSurface( bitmap );
   if ( !surface )
      return false;

   SDL_IOStream *io = _openStream( stream );
   const bool ok = io && SDL_SaveBMP_IO( surface, io, false );

   if ( io )
      SDL_CloseIO( io );
   SDL_DestroySurface( surface );

   return ok;
}

static bool sWriteUnsupported( GBitmap *bitmap, Stream &stream, U32 compressionLevel )
{
   TORQUE_UNUSED( bitmap );
   TORQUE_UNUSED( stream );
   TORQUE_UNUSED( compressionLevel );
   return false;
}

static struct _privateRegisterSDLImage
{
   void add( U32 priority, const char *ext, GBitmap::Registration::WriteFunc writeFunc,
             U32 defaultCompression = 0, const char *altExt = NULL )
   {
      GBitmap::Registration reg;

      reg.priority = priority;
      reg.extensions.push_back( ext );
      if ( altExt )
         reg.extensions.push_back( altExt );

      reg.readFunc = sReadSDL;
      reg.writeFunc = writeFunc;
      reg.defaultCompression = defaultCompression;

      GBitmap::sRegisterFormat( reg );
   }

   _privateRegisterSDLImage()
   {
      // Priorities decide the order sFindFile probes extensions for a name
      // given without one, so they match what the individual loaders used.
      add( 100, "png", sWritePNG, 6 );
      add( 50,  "jpg", sWriteJPG, 90, "jpeg" );
      add( 0,   "bmp", sWriteBMP );
      add( 0,   "tga", sWriteUnsupported );
      add( 0,   "gif", sWriteUnsupported );
   }
} sStaticRegisterSDLImage;


//-----------------------------------------------------------------------------
// DeferredPNGWriter
//
// libpng wrote rows straight out as they arrived. SDL3_image has no streaming
// writer, so rows accumulate into one bitmap and are encoded on end().
//-----------------------------------------------------------------------------

struct DeferredPNGWriterData
{
   GBitmap *bitmap;
   Stream  *stream;
   U32      row;
};

DeferredPNGWriter::DeferredPNGWriter()
   : mData( new DeferredPNGWriterData ), mActive( false )
{
   mData->bitmap = NULL;
   mData->stream = NULL;
   mData->row = 0;
}

DeferredPNGWriter::~DeferredPNGWriter()
{
   delete mData->bitmap;
   delete mData;
}

bool DeferredPNGWriter::begin( GFXFormat format, S32 width, S32 height, Stream &stream, U32 compressionLevel )
{
   TORQUE_UNUSED( compressionLevel );

   if ( format != GFXFormatA8 && _sdlFormatFor( format ) == SDL_PIXELFORMAT_UNKNOWN )
   {
      Con::errorf( "DeferredPNGWriter::begin - unsupported format %d", format );
      return false;
   }

   mData->bitmap = new GBitmap( U32( width ), U32( height ), false, format );
   mData->stream = &stream;
   mData->row = 0;
   mActive = true;

   return true;
}

void DeferredPNGWriter::append( GBitmap *bitmap, U32 rows )
{
   AssertFatal( mActive, "DeferredPNGWriter::append - not active" );

   const U32 rowBytes = mData->bitmap->getWidth() * mData->bitmap->getBytesPerPixel();
   const U32 count = getMin( rows, mData->bitmap->getHeight() - mData->row );

   for ( U32 i = 0; i < count; ++i, ++mData->row )
      dMemcpy( mData->bitmap->getAddress( 0, mData->row ), bitmap->getAddress( 0, i ), rowBytes );
}

void DeferredPNGWriter::end()
{
   AssertFatal( mActive, "DeferredPNGWriter::end - not active" );

   sWritePNG( mData->bitmap, *mData->stream, 0 );

   delete mData->bitmap;
   mData->bitmap = NULL;
   mActive = false;
}
