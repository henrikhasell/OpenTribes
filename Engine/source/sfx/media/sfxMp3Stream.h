// HH: Added for Tribes 2 compatibility.

#ifndef _SFXMP3STREAM_H_
#define _SFXMP3STREAM_H_

#ifndef _SFXFILESTREAM_H_
   #include "sfx/sfxFileStream.h"
#endif
#include "core/util/safeDelete.h"


/// An SFXFileStream that decodes sample data from an mp3 file.
///
/// Tribes 2's five music tracks are the only mp3 it ships, and each is four
/// minutes of 44.1kHz stereo -- about 45MB decoded -- so they are meant to be
/// streamed rather than held.  Decoding is dr_mp3's, driven through the
/// Torque stream the same way SFXVorbisStream drives vorbisfile.
class SFXMp3Stream : public SFXFileStream,
                     public IPositionable< U32 >
{
   public:

      typedef SFXFileStream Parent;

   protected:

      /// The dr_mp3 decoder, or NULL when closed.  Held untyped so that
      /// dr_mp3, which is one translation unit's worth of implementation,
      /// stays out of every file that includes this one.
      void *mMp3;

      /// Decoded read position in bytes.
      U32 mPosition;

      bool _openMp3();

      // SFXFileStream
      virtual bool _readHeader();
      virtual void _close();

   public:

      ///
      static SFXMp3Stream* create( Stream *stream );

      ///
      SFXMp3Stream();

      ///
      SFXMp3Stream( const SFXMp3Stream& cloneFrom );

      virtual ~SFXMp3Stream();

      // SFXStream
      virtual void reset();
      virtual U32 read( U8 *buffer, U32 length );
      virtual bool isEOS() const;
      virtual SFXStream* clone() const
      {
         SFXMp3Stream* stream = new SFXMp3Stream( *this );
         if( !stream->mMp3 )
            SAFE_DELETE( stream );
         return stream;
      }

      // IPositionable
      virtual U32 getPosition() const;
      virtual void setPosition( U32 offset );
};

#endif // _SFXMP3STREAM_H_
