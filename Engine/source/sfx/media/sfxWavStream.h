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

#ifndef _SFXWAVSTREAM_H_
#define _SFXWAVSTREAM_H_

#ifndef _SFXFILESTREAM_H_
   #include "sfx/sfxFileStream.h"
#endif
#ifndef _TVECTOR_H_
   #include "core/util/tVector.h"
#endif
#include "core/util/safeDelete.h"


/// An SFXFileStream that loads sample data from a WAV file.
class SFXWavStream : public SFXFileStream,
                     public IPositionable< U32 >
{
   public:

      typedef SFXFileStream Parent;

   protected:

      /// The file position of the start of
      /// the PCM data for fast reset().
      U32 mDataStart;

      /// HH: Added for Tribes 2 compatibility.
      /// @name IMA ADPCM
      ///
      /// Most of Tribes 2's audio is 4-bit IMA ADPCM, which arrives as
      /// independent blocks of nibbles rather than samples.  We decode a block
      /// at a time and present the result as the 16 bit PCM that mFormat
      /// describes, so the position this stream reports is a decoded one and
      /// no longer the file's.
      /// @{

      /// Compressed bytes in the data chunk.
      U32 mDataLength;

      /// Compressed bytes per block, or zero when the file is already PCM.
      U32 mBlockAlign;

      /// Sample frames a whole block decodes to.
      U32 mSamplesPerBlock;

      /// Decoded read position in bytes.
      U32 mPosition;

      /// Which block mBlock holds, or -1 for none.
      S32 mBlockIndex;

      /// Sample frames mBlock holds, which is short of mSamplesPerBlock only
      /// for a truncated final block.
      U32 mBlockSamples;

      /// One block, decoded.
      Vector< U8 > mBlock;

      /// One block, as read.
      Vector< U8 > mEncoded;

      bool _isCompressed() const { return mBlockAlign != 0; }

      /// Read and decode a block, unless it is the one we already hold.
      bool _decodeBlock( U32 index );

      U32 _readCompressed( U8 *buffer, U32 bytes );

      /// @}

      // SFXFileStream
      virtual bool _readHeader();
      virtual void _close();

   public:

      ///
      static SFXWavStream* create( Stream *stream );

      ///
	   SFXWavStream();

      ///
      SFXWavStream( const SFXWavStream& cloneFrom );

      /// Destructor.
      virtual ~SFXWavStream();

      // SFXStream
      virtual void reset();
      virtual U32 read( U8 *buffer, U32 length );
      virtual bool isEOS() const;
      virtual SFXStream* clone() const
      {
         SFXWavStream* stream = new SFXWavStream( *this );
         if( !stream->mStream )
            SAFE_DELETE( stream );
         return stream;
      }

      // IPositionable
      virtual U32 getPosition() const;
      virtual void setPosition( U32 offset );
};

#endif  // _SFXWAVSTREAM_H_
