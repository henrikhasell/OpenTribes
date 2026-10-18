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

#include "sfx/openal/sfxALDevice.h"
#include "sfx/openal/sfxALBuffer.h"
#include "platform/async/asyncUpdate.h"

#include <chrono>


//-----------------------------------------------------------------------------

SFXALDevice::SFXALDevice(  SFXProvider *provider, 
                           const OPENALFNTABLE &openal, 
                           String name, 
                           bool useHardware, 
                           S32 maxBuffers )
   :  Parent( name, provider, useHardware, maxBuffers ),
      mOpenAL( openal ), 
      mDevice( NULL ), 
      mContext( NULL ),
      mFeeder( NULL )
{
   mMaxBuffers = getMax( maxBuffers, 8 );

   // TODO: The OpenAL device doesn't set the primary buffer
   // $pref::SFX::frequency or $pref::SFX::bitrate!

   mDevice = mOpenAL.alcOpenDevice( name );
   mOpenAL.alcGetError( mDevice );
   if( mDevice ) 
   {
      mContext = mOpenAL.alcCreateContext( mDevice, NULL );

      if( mContext ) 
         mOpenAL.alcMakeContextCurrent( mContext );

      U32 err = mOpenAL.alcGetError( mDevice );
      
      if( err != ALC_NO_ERROR )
         Con::errorf( "SFXALDevice - Initialization Error: %s", mOpenAL.alcGetString( mDevice, err ) );
   }

   AssertFatal( mDevice != NULL && mContext != NULL, "Failed to create OpenAL device and/or context!" );

   // Start the update thread.
   
   if( !Con::getBoolVariable( "$_forceAllMainThread" ) )
   {
      SFXInternal::gUpdateThread = new AsyncPeriodicUpdateThread
         ( "OpenAL Update Thread", SFXInternal::gBufferUpdateList,
           Con::getIntVariable( "$pref::SFX::updateInterval", SFXInternal::DEFAULT_UPDATE_INTERVAL ) );
      SFXInternal::gUpdateThread->start();
   }

   // HH: Added for Tribes 2 compatibility.
   mFeeder = new Feeder( this );
   mFeeder->start();
}

//-----------------------------------------------------------------------------

SFXALDevice::~SFXALDevice()
{
   // HH: Added for Tribes 2 compatibility.
   if( mFeeder )
   {
      mFeeder->stop();
      mFeeder->join();
      SAFE_DELETE( mFeeder );
   }

   _releaseAllResources();

   mOpenAL.alcMakeContextCurrent( NULL );
	mOpenAL.alcDestroyContext( mContext );
	mOpenAL.alcCloseDevice( mDevice );
}

//-----------------------------------------------------------------------------

// HH: Added for Tribes 2 compatibility.
// Retail's provider, Miles Fast 2D, only flags a started 3D sample. Its own
// 30 Hz timer loads the first buffer, and the mixer takes the sample up on its
// next fragment, but a stop is immediate. So the tick-long jet pulses on empty
// energy lose their fronts, or vanish, whatever the frame rate.
void SFXALDevice::Feeder::run( void* arg )
{
   using namespace std::chrono;
   while( !checkForStop() )
   {
      const S64 now = duration_cast< nanoseconds >( steady_clock::now().time_since_epoch() ).count();
      const S64 next = ( now * 30 / 1000000000 + 1 ) * 1000000000 / 30;
      Platform::sleep( U32( ( next - now + 999999 ) / 1000000 ) );
      mDevice->_feed();
   }
}

void SFXALDevice::_holdStart( SFXALVoice* voice )
{
   mFeedMutex.lock();
   if( !mFeedQueue.contains( voice ) )
      mFeedQueue.push_back( voice );
   mFeedMutex.unlock();
}

void SFXALDevice::_dropStart( SFXALVoice* voice )
{
   mFeedMutex.lock();
   mFeedQueue.remove( voice );
   mFeedMutex.unlock();
}

bool SFXALDevice::_isStartHeld( const SFXALVoice* voice )
{
   mFeedMutex.lock();
   const bool held = mFeedQueue.contains( const_cast< SFXALVoice* >( voice ) );
   mFeedMutex.unlock();
   return held;
}

void SFXALDevice::_feed()
{
   mFeedMutex.lock();
   for( U32 i = 0; i < mFeedQueue.size(); ++ i )
      mOpenAL.alSourcePlay( mFeedQueue[ i ]->mSourceName );
   mFeedQueue.clear();
   mFeedMutex.unlock();
}

//-----------------------------------------------------------------------------

SFXBuffer* SFXALDevice::createBuffer( const ThreadSafeRef< SFXStream >& stream, SFXDescription* description )
{
   AssertFatal( stream, "SFXALDevice::createBuffer() - Got null stream!" );
   AssertFatal( description, "SFXALDevice::createBuffer() - Got null description!" );

   SFXALBuffer* buffer = SFXALBuffer::create(   mOpenAL, 
                                                stream,
                                                description, 
                                                mUseHardware );
   if ( !buffer )
      return NULL;

   _addBuffer( buffer );
   return buffer;
}

//-----------------------------------------------------------------------------

SFXVoice* SFXALDevice::createVoice( bool is3D, SFXBuffer *buffer )
{
   // Don't bother going any further if we've 
   // exceeded the maximum voices.
   if ( mVoices.size() >= mMaxBuffers )
      return NULL;

   AssertFatal( buffer, "SFXALDevice::createVoice() - Got null buffer!" );

   SFXALBuffer* alBuffer = dynamic_cast<SFXALBuffer*>( buffer );
   AssertFatal( alBuffer, "SFXALDevice::createVoice() - Got bad buffer!" );

   SFXALVoice* voice = SFXALVoice::create( this, alBuffer );
   if ( !voice )
      return NULL;

   _addVoice( voice );
	return voice;
}

//-----------------------------------------------------------------------------

void SFXALDevice::setListener( U32 index, const SFXListenerProperties& listener )
{
   if( index != 0 )
      return;

   // Torque and OpenAL are both right handed 
   // systems, so no coordinate flipping is needed.

   const MatrixF &transform = listener.getTransform();
   Point3F pos, tupple[2];
   transform.getColumn( 3, &pos );
   transform.getColumn( 1, &tupple[0] );
   transform.getColumn( 2, &tupple[1] );

   const VectorF &velocity = listener.getVelocity();

   mOpenAL.alListenerfv( AL_POSITION, pos );
   mOpenAL.alListenerfv( AL_VELOCITY, velocity );
   mOpenAL.alListenerfv( AL_ORIENTATION, (const F32 *)&tupple[0] );
}

//-----------------------------------------------------------------------------

void SFXALDevice::setDistanceModel( SFXDistanceModel model )
{
   switch( model )
   {
      case SFXDistanceModelLinear:
         mOpenAL.alDistanceModel( AL_LINEAR_DISTANCE_CLAMPED );
         if( mRolloffFactor != 1.0f )
            _setRolloffFactor( 1.0f ); // No rolloff on linear.
         break;
         
      case SFXDistanceModelLogarithmic:
         mOpenAL.alDistanceModel( AL_INVERSE_DISTANCE_CLAMPED );
         if( mUserRolloffFactor != mRolloffFactor )
            _setRolloffFactor( mUserRolloffFactor );
         break;
         
      default:
         AssertWarn( false, "SFXALDevice::setDistanceModel - distance model not implemented" );
   }
   
   mDistanceModel = model;
}

//-----------------------------------------------------------------------------

void SFXALDevice::setDopplerFactor( F32 factor )
{
   mOpenAL.alDopplerFactor( factor );
}

//-----------------------------------------------------------------------------

void SFXALDevice::_setRolloffFactor( F32 factor )
{
   mRolloffFactor = factor;
   
   for( U32 i = 0, num = mVoices.size(); i < num; ++ i )
      mOpenAL.alSourcef( ( ( SFXALVoice* ) mVoices[ i ] )->mSourceName, AL_ROLLOFF_FACTOR, factor );
}

//-----------------------------------------------------------------------------

void SFXALDevice::setRolloffFactor( F32 factor )
{
   if( mDistanceModel == SFXDistanceModelLinear && factor != 1.0f )
      Con::errorf( "SFXALDevice::setRolloffFactor - rolloff factor <> 1.0f ignored in linear distance model" );
   else
      _setRolloffFactor( factor );
      
   mUserRolloffFactor = factor;
}
