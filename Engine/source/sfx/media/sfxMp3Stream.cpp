// HH: Added for Tribes 2 compatibility.

// dr_mp3 is compiled here, ahead of any engine header, so that the engine's
// own macros cannot reach inside it.
#define DR_MP3_NO_STDIO
#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"

#include "sfx/media/sfxMp3Stream.h"
#include "core/stream/stream.h"


SFXMp3Stream* SFXMp3Stream::create( Stream *stream )
{
   SFXMp3Stream *sfxStream = new SFXMp3Stream();
   if( sfxStream->open( stream, true ) )
      return sfxStream;

   delete sfxStream;
   return NULL;
}

SFXMp3Stream::SFXMp3Stream()
   : mMp3( NULL ),
     mPosition( 0 )
{
}

SFXMp3Stream::SFXMp3Stream( const SFXMp3Stream& cloneFrom )
   : Parent( cloneFrom ),
     mMp3( NULL ),
     mPosition( 0 )
{
   if( mStream )
      _openMp3();
}

SFXMp3Stream::~SFXMp3Stream()
{
   // We must call close from our own destructor
   // and not the base class... as it causes a
   // pure virtual runtime assertion.
   close();
}

static size_t readFunc( void *userData, void *buffer, size_t bytes )
{
   Stream *stream = ( Stream* ) userData;

   const U32 start = stream->getPosition();
   stream->read( bytes, buffer );

   return stream->getPosition() - start;
}

static drmp3_bool32 seekFunc( void *userData, int offset, drmp3_seek_origin origin )
{
   Stream *stream = ( Stream* ) userData;

   S64 position;
   switch( origin )
   {
      case DRMP3_SEEK_SET:  position = offset; break;
      case DRMP3_SEEK_CUR:  position = S64( stream->getPosition() ) + offset; break;
      case DRMP3_SEEK_END:  position = S64( stream->getStreamSize() ) + offset; break;
      default:              return DRMP3_FALSE;
   }

   if( position < 0 || position > S64( stream->getStreamSize() ) )
      return DRMP3_FALSE;

   return stream->setPosition( U32( position ) ) ? DRMP3_TRUE : DRMP3_FALSE;
}

static drmp3_bool32 tellFunc( void *userData, drmp3_int64 *cursor )
{
   *cursor = ( ( Stream* ) userData )->getPosition();
   return DRMP3_TRUE;
}

bool SFXMp3Stream::_openMp3()
{
   mStream->setPosition( 0 );

   drmp3 *mp3 = ( drmp3* ) dMalloc( sizeof( drmp3 ) );
   if( !drmp3_init( mp3, readFunc, seekFunc, tellFunc, NULL, mStream, NULL ) )
   {
      dFree( mp3 );
      return false;
   }

   mMp3 = mp3;
   return true;
}

bool SFXMp3Stream::_readHeader()
{
   if( !_openMp3() )
      return false;

   // Counting the frames of a file with no length in it reads the whole thing,
   // but the length is what tells the system how long the track is and when it
   // has ended, and these five files are read once each at a mission's start.
   drmp3 *mp3 = ( drmp3* ) mMp3;

   const drmp3_uint64 frames = drmp3_get_pcm_frame_count( mp3 );
   if( !frames || !mp3->channels )
   {
      _close();
      return false;
   }

   mFormat.set( mp3->channels, 16 * mp3->channels, mp3->sampleRate );
   mSamples = frames;
   mPosition = 0;

   return drmp3_seek_to_pcm_frame( mp3, 0 ) == DRMP3_TRUE;
}

void SFXMp3Stream::_close()
{
   if( !mMp3 )
      return;

   drmp3_uninit( ( drmp3* ) mMp3 );
   dFree( mMp3 );

   mMp3 = NULL;
   mPosition = 0;
}

void SFXMp3Stream::reset()
{
   AssertFatal( mMp3, "SFXMp3Stream::reset() - Decoder is null!" );

   if( drmp3_seek_to_pcm_frame( ( drmp3* ) mMp3, 0 ) == DRMP3_TRUE )
      mPosition = 0;
}

U32 SFXMp3Stream::read( U8 *buffer, U32 bytes )
{
   AssertFatal( mMp3, "SFXMp3Stream::read() - Decoder is null!" );

   const U32 frameSize = mFormat.getBytesPerSample();
   const U32 length = mSamples * frameSize;

   bytes -= bytes % frameSize;
   if( mPosition >= length )
      return 0;
   if( mPosition + bytes > length )
      bytes = length - mPosition;

   const U32 read = drmp3_read_pcm_frames_s16( ( drmp3* ) mMp3, bytes / frameSize,
                                               ( drmp3_int16* ) buffer ) * frameSize;
   mPosition += read;

   return read;
}

bool SFXMp3Stream::isEOS() const
{
   return !mMp3 || mPosition >= mSamples * mFormat.getBytesPerSample();
}

U32 SFXMp3Stream::getPosition() const
{
   return mPosition;
}

void SFXMp3Stream::setPosition( U32 offset )
{
   AssertFatal( mMp3, "SFXMp3Stream::setPosition() - Decoder is null!" );

   const U32 frameSize = mFormat.getBytesPerSample();
   const U32 length = mSamples * frameSize;

   offset -= offset % frameSize;
   if( offset > length )
      offset = length;

   if( drmp3_seek_to_pcm_frame( ( drmp3* ) mMp3, offset / frameSize ) == DRMP3_TRUE )
      mPosition = offset;
}
