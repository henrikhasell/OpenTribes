//-----------------------------------------------------------------------------
// Async update threads, replacing platformWin32/winAsync.cpp and its macOS
// counterpart.
//
// The Win32 version used an auto-reset event plus a waitable timer. An SDL
// semaphore is the same thing: signalling releases it, waiting acquires and
// resets it, and the periodic variant just waits with a timeout instead of
// blocking indefinitely.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "platform/async/asyncUpdate.h"

#include <SDL3/SDL.h>

AsyncUpdateThread::AsyncUpdateThread( String name, AsyncUpdateList* updateList )
   : Parent( 0, 0, false, false ),
     mUpdateList( updateList ),
     mName( name )
{
   mUpdateEvent = SDL_CreateSemaphore( 0 );
}

AsyncUpdateThread::~AsyncUpdateThread()
{
   if( mUpdateEvent )
      SDL_DestroySemaphore( static_cast< SDL_Semaphore* >( mUpdateEvent ) );
}

void AsyncUpdateThread::_waitForEventAndReset()
{
   if( mUpdateEvent )
      SDL_WaitSemaphore( static_cast< SDL_Semaphore* >( mUpdateEvent ) );
}

void AsyncUpdateThread::triggerUpdate()
{
   if( mUpdateEvent )
      SDL_SignalSemaphore( static_cast< SDL_Semaphore* >( mUpdateEvent ) );
}

AsyncPeriodicUpdateThread::AsyncPeriodicUpdateThread( String name,
                                                      AsyncUpdateList* updateList,
                                                      U32 intervalMS )
   : Parent( name, updateList )
{
   // Stores the interval rather than a timer handle; the wait below applies it.
   mUpdateTimer = reinterpret_cast< void* >( static_cast< uintptr_t >( intervalMS ) );
}

AsyncPeriodicUpdateThread::~AsyncPeriodicUpdateThread()
{
}

void AsyncPeriodicUpdateThread::_waitForEventAndReset()
{
   const U32 intervalMS = U32( reinterpret_cast< uintptr_t >( mUpdateTimer ) );

   if( mUpdateEvent )
      SDL_WaitSemaphoreTimeout( static_cast< SDL_Semaphore* >( mUpdateEvent ), intervalMS );
}
