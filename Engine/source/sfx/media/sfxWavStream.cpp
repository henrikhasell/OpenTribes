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

#include "sfx/media/sfxWavStream.h"
#include "core/stream/stream.h"
#include "core/strings/stringFunctions.h"


/// WAV File-header
struct WAVFileHdr
{
   U8  id[4];
   U32 size;
   U8  type[4];
};

//// WAV Fmt-header
struct WAVFmtHdr
{
   U16 format;
   U16 channels;
   U32 samplesPerSec;
   U32 bytesPerSec;
   U16 blockAlign;
   U16 bitsPerSample;
};

/// WAV FmtEx-header
struct WAVFmtExHdr
{
   U16 size;
   U16 samplesPerBlock;
};

/// WAV Smpl-header
struct WAVSmplHdr
{
   U32 manufacturer;
   U32 product;
   U32 samplePeriod;
   U32 note;
   U32 fineTune;
   U32 SMPTEFormat;
   U32 SMPTEOffest;
   U32 loops;
   U32 samplerData;

   struct
   {
      U32 identifier;
      U32 type;
      U32 start;
      U32 end;
      U32 fraction;
      U32 count;
   } loop[1];
};

/// WAV Chunk-header
struct WAVChunkHdr
{
   U8  id[4];
   U32 size;
};


// HH: Added for Tribes 2 compatibility.
//
// The IMA ADPCM step and index tables, which every decoder of it shares.
static const S32 sAdpcmIndexTable[ 16 ] =
{
   -1, -1, -1, -1, 2, 4, 6, 8,
   -1, -1, -1, -1, 2, 4, 6, 8
};

static const S32 sAdpcmStepTable[ 89 ] =
{
       7,     8,     9,    10,    11,    12,    13,    14,    16,    17,
      19,    21,    23,    25,    28,    31,    34,    37,    41,    45,
      50,    55,    60,    66,    73,    80,    88,    97,   107,   118,
     130,   143,   157,   173,   190,   209,   230,   253,   279,   307,
     337,   371,   408,   449,   494,   544,   598,   658,   724,   796,
     876,   963,  1060,  1166,  1282,  1411,  1552,  1707,  1878,  2066,
    2272,  2499,  2749,  3024,  3327,  3660,  4026,  4428,  4871,  5358,
    5894,  6484,  7132,  7845,  8630,  9493, 10442, 11487, 12635, 13899,
   15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

static inline S16 decodeAdpcmNibble( U8 nibble, S32 &predictor, S32 &index )
{
   const S32 step = sAdpcmStepTable[ index ];

   // Shift each term rather than rounding the whole difference once.  The two
   // are not the same for a step the shifts truncate, and this is the one the
   // reference decoder takes.
   S32 difference = step >> 3;
   if( nibble & 1 )
      difference += step >> 2;
   if( nibble & 2 )
      difference += step >> 1;
   if( nibble & 4 )
      difference += step;
   if( nibble & 8 )
      difference = -difference;

   predictor += difference;
   if( predictor > 32767 )
      predictor = 32767;
   else if( predictor < -32768 )
      predictor = -32768;

   index += sAdpcmIndexTable[ nibble ];
   if( index > 88 )
      index = 88;
   else if( index < 0 )
      index = 0;

   return S16( predictor );
}


SFXWavStream* SFXWavStream::create( Stream *stream )
{
   SFXWavStream *sfxStream = new SFXWavStream();
   if ( sfxStream->open( stream, true ) )
      return sfxStream;

   delete sfxStream;
   return NULL;
}

SFXWavStream::SFXWavStream()
   : mDataStart( -1 ),
     mDataLength( 0 ),
     mBlockAlign( 0 ),
     mSamplesPerBlock( 0 ),
     mPosition( 0 ),
     mBlockIndex( -1 ),
     mBlockSamples( 0 )
{
}

SFXWavStream::SFXWavStream( const SFXWavStream& cloneFrom )
   : Parent( cloneFrom ),
     mDataStart( cloneFrom.mDataStart ),
     // HH: Modified for Tribes 2 compatibility.
     mDataLength( cloneFrom.mDataLength ),
     mBlockAlign( cloneFrom.mBlockAlign ),
     mSamplesPerBlock( cloneFrom.mSamplesPerBlock ),
     mPosition( 0 ),
     mBlockIndex( -1 ),
     mBlockSamples( 0 ),
     mBlock( cloneFrom.mBlock.size() ),
     mEncoded( cloneFrom.mEncoded.size() )
{
}

SFXWavStream::~SFXWavStream()
{
   // We must call close from our own destructor
   // and not the base class... as it causes a
   // pure virtual runtime assertion.
   close();
}

void SFXWavStream::_close()
{
   mDataStart = -1;

   // HH: Modified for Tribes 2 compatibility.
   mDataLength = 0;
   mBlockAlign = 0;
   mSamplesPerBlock = 0;
   mPosition = 0;
   mBlockIndex = -1;
   mBlockSamples = 0;
}

bool SFXWavStream::_readHeader()
{
   // We read the wav chunks to gather than header info
   // and find the start and end position of the data chunk. 
   mDataStart = -1;

   WAVFileHdr fileHdr;
   mStream->read( 4, &fileHdr.id[0] );
   mStream->read( &fileHdr.size );
   mStream->read( 4, &fileHdr.type[0] );

   fileHdr.size=((fileHdr.size+1)&~1)-4;

   WAVChunkHdr chunkHdr;
   mStream->read( 4, &chunkHdr.id[0] );
   mStream->read( &chunkHdr.size );

   // Unread chunk data rounded up to nearest WORD.
   S32 chunkRemaining = chunkHdr.size + ( chunkHdr.size & 1 );

   WAVFmtHdr   fmtHdr;
   WAVFmtExHdr fmtExHdr;
   WAVSmplHdr  smplHdr;

   // HH: Added for Tribes 2 compatibility.
   // A compressed file's true sample count, which its final block overruns.
   U32 factSamples = 0;

   dMemset(&fmtHdr, 0, sizeof(fmtHdr));

   while ((fileHdr.size!=0) && (mStream->getStatus() != Stream::EOS))
   {
      // WAV format header chunk.
      if ( !dStrncmp( (const char*)chunkHdr.id, "fmt ", 4 ) )
      {
         mStream->read(&fmtHdr.format);
         mStream->read(&fmtHdr.channels);
         mStream->read(&fmtHdr.samplesPerSec);
         mStream->read(&fmtHdr.bytesPerSec);
         mStream->read(&fmtHdr.blockAlign);
         mStream->read(&fmtHdr.bitsPerSample);

         if ( fmtHdr.format == 0x0001 )
         {
            mFormat.set( fmtHdr.channels, fmtHdr.bitsPerSample * fmtHdr.channels, fmtHdr.samplesPerSec );
            chunkRemaining -= sizeof( WAVFmtHdr );
         }
         else
         {
            mStream->read(sizeof(WAVFmtExHdr), &fmtExHdr);
            // HH: Modified for Tribes 2 compatibility.
            chunkRemaining -= sizeof(WAVFmtHdr) + sizeof(WAVFmtExHdr);

            if( fmtHdr.format == 0x0011 && fmtHdr.channels > 0 && fmtHdr.channels <= 2 )
            {
               // We hand out the decoded samples, so the format we advertise
               // is 16 bit PCM rather than the four bits on disk.
               mFormat.set( fmtHdr.channels, 16 * fmtHdr.channels, fmtHdr.samplesPerSec );

               mBlockAlign = fmtHdr.blockAlign;
               mSamplesPerBlock = fmtExHdr.samplesPerBlock;

               const U32 preamble = 4 * fmtHdr.channels;
               if( mBlockAlign <= preamble )
                  mBlockAlign = 0;
               else if( !mSamplesPerBlock )
                  mSamplesPerBlock = 1 + ( ( mBlockAlign - preamble ) * 2 ) / fmtHdr.channels;
            }
         }
      }

      // WAV data chunk
      else if (!dStrncmp((const char*)chunkHdr.id,"data",4))
      {
         // TODO: Handle these other formats in a more graceful manner!

         if (fmtHdr.format==0x0001)
         {
            mDataStart = mStream->getPosition();
            mStream->setPosition( mDataStart + chunkHdr.size );
            chunkRemaining -= chunkHdr.size;
            mSamples = chunkHdr.size / mFormat.getBytesPerSample();
         }
         // HH: Modified for Tribes 2 compatibility.
         else if( _isCompressed() )
         {
            mDataStart = mStream->getPosition();
            mDataLength = chunkHdr.size;
            mStream->setPosition( mDataStart + chunkHdr.size );
            chunkRemaining -= chunkHdr.size;

            const U32 channels = mFormat.getChannels();
            const U32 preamble = 4 * channels;
            const U32 tail = mDataLength % mBlockAlign;

            mSamples = ( mDataLength / mBlockAlign ) * mSamplesPerBlock;
            if( tail >= preamble )
               mSamples += 1 + ( ( tail - preamble ) * 2 ) / channels;

            if( factSamples && factSamples < mSamples )
               mSamples = factSamples;

            mBlock.setSize( mSamplesPerBlock * channels * sizeof( S16 ) );
            mEncoded.setSize( mBlockAlign );
         }
         else
         {
            // HH: Modified for Tribes 2 compatibility.
            // TODO: decode MP3 (0x0055), which no shipped wav uses.
            static U16 sWarnedFormat = 0;
            if( fmtHdr.format != sWarnedFormat )
            {
               sWarnedFormat = fmtHdr.format;
               Con::errorf( "SFXWavStream: wave format 0x%04x is not decoded, "
                  "so these sounds are silent.", fmtHdr.format );
            }
            return false;
         }
      }

      // HH: Added for Tribes 2 compatibility.
      // Sample count, which a compressed file carries because its blocks hold
      // a whole number of samples and its sound does not.
      else if (!dStrncmp((const char*)chunkHdr.id,"fact",4))
      {
         mStream->read( &factSamples );
         chunkRemaining -= sizeof( factSamples );
      }

      // WAV sample header
      else if (!dStrncmp((const char*)chunkHdr.id,"smpl",4))
      {
         // this struct read is NOT endian safe but it is ok because
         // we are only testing the loops field against ZERO
         mStream->read(sizeof(WAVSmplHdr), &smplHdr);

         // This has never been hooked up and its usefulness is
         // dubious.  Do we really want the audio file overriding
         // the SFXDescription setting?    
         //mLooping = ( smplHdr.loops ? true : false );

         chunkRemaining -= sizeof(WAVSmplHdr);
      }

      // either we have unread chunk data or we found an unknown chunk type
      // loop and read up to 1K bytes at a time until we have
      // read to the end of this chunk
      AssertFatal(chunkRemaining >= 0, "AudioBuffer::readWAV: remaining chunk data should never be less than zero.");
      if ( chunkRemaining > 0 )
      {
         U32 pos = mStream->getPosition();
         mStream->setPosition( pos + chunkRemaining );
         chunkRemaining = 0;
      }

      const U32 consumed = ((chunkHdr.size+1)&~1)+8;
      if (fileHdr.size <= consumed)
         break;
      fileHdr.size -= consumed;

      // read next chunk header...
      if (!mStream->read(4, &chunkHdr.id[0]) || !mStream->read(&chunkHdr.size))
         break;

      // HH: Added for Tribes 2 compatibility.
      // 181 of the shipped voice files declare a data chunk shorter than the
      // audio they hold, so what follows it is samples, not a chunk.
      if (dStrncmp((const char*)chunkHdr.id,"data",4) &&
          chunkHdr.size > mStream->getStreamSize() - mStream->getPosition())
         break;

      // unread chunk data rounded up to nearest WORD
      chunkRemaining = chunkHdr.size + (chunkHdr.size&1);
   }

   return ( mDataStart != -1 );
}

void SFXWavStream::reset()
{
   AssertFatal( mStream, "SFXWavStream::reset() - Stream is null!" );
   AssertFatal( mDataStart != -1, "SFXWavStream::seek() - Data start offset is invalid!" );

   // HH: Modified for Tribes 2 compatibility.
   if( _isCompressed() )
   {
      mPosition = 0;
      return;
   }

   mStream->setPosition( mDataStart );
}


// HH: Added for Tribes 2 compatibility.
bool SFXWavStream::_decodeBlock( U32 index )
{
   if( mBlockIndex == S32( index ) )
      return mBlockSamples > 0;

   mBlockIndex = -1;
   mBlockSamples = 0;

   const U32 offset = index * mBlockAlign;
   if( offset >= mDataLength )
      return false;

   const U32 channels = mFormat.getChannels();
   const U32 preamble = 4 * channels;
   const U32 size = getMin( mBlockAlign, mDataLength - offset );
   if( size < preamble )
      return false;

   mStream->setPosition( mDataStart + offset );
   if( !mStream->read( size, mEncoded.address() ) )
      return false;

   // Each channel opens the block with the sample to predict from and the
   // step to predict it with.
   S32 predictor[ 2 ];
   S32 stepIndex[ 2 ];
   const U8 *in = mEncoded.address();

   for( U32 c = 0; c < channels; ++ c )
   {
      predictor[ c ] = S16( U16( in[ 0 ] ) | ( U16( in[ 1 ] ) << 8 ) );
      stepIndex[ c ] = in[ 2 ] <= 88 ? in[ 2 ] : 88;
      in += 4;
   }

   S16 *out = ( S16* ) mBlock.address();
   for( U32 c = 0; c < channels; ++ c )
      out[ c ] = S16( predictor[ c ] );

   U32 frames = 1;

   // The nibbles that follow come a word of each channel at a time, low
   // nibble first.
   const U32 dataBytes = size - preamble;
   for( U32 consumed = 0;
        consumed + preamble <= dataBytes && frames + 8 <= mSamplesPerBlock;
        consumed += preamble )
   {
      for( U32 c = 0; c < channels; ++ c )
         for( U32 b = 0; b < 4; ++ b )
         {
            const U8 byte = in[ consumed + c * 4 + b ];

            out[ ( frames + b * 2 ) * channels + c ] =
               decodeAdpcmNibble( byte & 0xf, predictor[ c ], stepIndex[ c ] );
            out[ ( frames + b * 2 + 1 ) * channels + c ] =
               decodeAdpcmNibble( byte >> 4, predictor[ c ], stepIndex[ c ] );
         }

      frames += 8;
   }

   mBlockIndex = index;
   mBlockSamples = frames;

   return true;
}


// HH: Added for Tribes 2 compatibility.
U32 SFXWavStream::_readCompressed( U8 *buffer, U32 bytes )
{
   const U32 frameSize = mFormat.getBytesPerSample();
   const U32 length = mSamples * frameSize;

   bytes -= bytes % frameSize;
   if( mPosition >= length )
      return 0;
   if( mPosition + bytes > length )
      bytes = length - mPosition;

   U32 done = 0;
   while( done < bytes )
   {
      const U32 frame = ( mPosition + done ) / frameSize;
      const U32 inBlock = frame % mSamplesPerBlock;

      if( !_decodeBlock( frame / mSamplesPerBlock ) || inBlock >= mBlockSamples )
         break;

      const U32 copy = getMin( ( mBlockSamples - inBlock ) * frameSize, bytes - done );
      dMemcpy( buffer + done, mBlock.address() + inBlock * frameSize, copy );

      done += copy;
   }

   mPosition += done;
   return done;
}


// HH: Added for Tribes 2 compatibility.
bool SFXWavStream::isEOS() const
{
   if( _isCompressed() )
      return mPosition >= mSamples * mFormat.getBytesPerSample();

   return Parent::isEOS();
}

U32 SFXWavStream::getPosition() const
{
   AssertFatal( mStream, "SFXWavStream::getPosition() - Stream is null!" );

   // HH: Modified for Tribes 2 compatibility.
   if( _isCompressed() )
      return mPosition;

   return ( mStream->getPosition() - mDataStart );
}

void SFXWavStream::setPosition( U32 offset )
{
   AssertFatal( mStream, "SFXWavStream::setPosition() - Stream is null!" );

   offset -= offset % mFormat.getBytesPerSample();
   const U32 dataLength = mSamples * mFormat.getBytesPerSample();
   if( offset > dataLength )
      offset = dataLength;

   AssertFatal( mDataStart != -1, "SFXWavStream::getPosition() - Data start offset is invalid!" );

   // HH: Modified for Tribes 2 compatibility.
   if( _isCompressed() )
   {
      mPosition = offset;
      return;
   }

   U32 byte = mDataStart + offset;

   mStream->setPosition( byte );
}

U32 SFXWavStream::read( U8 *buffer, U32 bytes )
{
   AssertFatal( mStream, "SFXWavStream::seek() - Stream is null!" );

   // HH: Modified for Tribes 2 compatibility.
   if( _isCompressed() )
      return _readCompressed( buffer, bytes );

   // Read in even sample chunks.
   
   bytes -= bytes % mFormat.getBytesPerSample();

   // Read the data and determine how much we've read.
   // FileStreams apparently report positions past
   // the actual stream length, so manually cap the
   // numbers here.
   
   const U32 oldPosition = mStream->getPosition();
   mStream->read( bytes, buffer );
   U32 newPosition = mStream->getPosition();
   const U32 maxPosition = getDataLength() + mDataStart;
   if( newPosition > maxPosition )
      newPosition = maxPosition;
      
   const U32 numBytesRead = newPosition - oldPosition;

   // TODO: Is it *just* 16 bit samples that needs to 
   // be flipped?  What about 32 bit samples?
   #ifdef TORQUE_BIG_ENDIAN

      // We need to endian-flip 16-bit data.
      if ( getFormat().getBytesPerChannel() == 2 ) 
      {
         U16 *ds = (U16*)buffer;
         U16 *de = (U16*)(buffer+bytes);
         while (ds<de)
         {
            *ds = convertLEndianToHost(*ds);
            ds++;
         }
      }

   #endif

   return numBytesRead;
}
