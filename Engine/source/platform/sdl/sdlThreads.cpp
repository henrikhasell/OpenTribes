#include "platform/platform.h"
#include "platform/threads/thread.h"
#include "platform/threads/mutex.h"
#include "platform/threads/semaphore.h"

#include <SDL3/SDL.h>

//-----------------------------------------------------------------------------
// Mutex

class PlatformMutexData
{
public:
   SDL_Mutex* mutex;
   PlatformMutexData() : mutex(SDL_CreateMutex()) {}
   ~PlatformMutexData() { if (mutex) SDL_DestroyMutex(mutex); }
};

Mutex::Mutex()
{
   mData = new PlatformMutexData;
}

Mutex::~Mutex()
{
   delete mData;
}

bool Mutex::lock(bool block)
{
   if (!mData->mutex)
      return false;

   if (block)
   {
      SDL_LockMutex(mData->mutex);
      return true;
   }

   return SDL_TryLockMutex(mData->mutex);
}

void Mutex::unlock()
{
   if (mData->mutex)
      SDL_UnlockMutex(mData->mutex);
}

//-----------------------------------------------------------------------------
// Semaphore

class PlatformSemaphore
{
public:
   SDL_Semaphore* semaphore;
   PlatformSemaphore(S32 initialCount) : semaphore(SDL_CreateSemaphore(initialCount)) {}
   ~PlatformSemaphore() { if (semaphore) SDL_DestroySemaphore(semaphore); }
};

Semaphore::Semaphore(S32 initialCount)
{
   mData = new PlatformSemaphore(initialCount);
}

Semaphore::~Semaphore()
{
   delete mData;
}

bool Semaphore::acquire(bool block, S32 timeoutMS)
{
   if (!mData->semaphore)
      return false;

   if (!block)
      return SDL_TryWaitSemaphore(mData->semaphore);

   if (timeoutMS < 0)
   {
      SDL_WaitSemaphore(mData->semaphore);
      return true;
   }

   return SDL_WaitSemaphoreTimeout(mData->semaphore, timeoutMS);
}

void Semaphore::release()
{
   if (mData->semaphore)
      SDL_SignalSemaphore(mData->semaphore);
}

//-----------------------------------------------------------------------------
// Thread

class PlatformThreadData
{
public:
   ThreadRunFunction       mRunFunc;
   void*                   mRunArg;
   Thread*                 mThread;
   SDL_Thread*             mSDLThread;
   Semaphore               mGateway;
   U32                     mThreadId;

   PlatformThreadData()
      : mRunFunc(0), mRunArg(0), mThread(0), mSDLThread(0), mGateway(1), mThreadId(0) {}
};

ThreadManager::MainThreadId ThreadManager::smMainThreadId;

static int SDLCALL sdlThreadEntry(void* data)
{
   PlatformThreadData* threadData = static_cast<PlatformThreadData*>(data);

   threadData->mThreadId = ThreadManager::getCurrentThreadId();
   threadData->mGateway.acquire();

   if (threadData->mRunFunc)
      threadData->mRunFunc(threadData->mRunArg);
   else
      threadData->mThread->run(threadData->mRunArg);

   threadData->mGateway.release();

   Thread* thread = threadData->mThread;
   if (thread->autoDelete)
      delete thread;

   return 0;
}

Thread::Thread(ThreadRunFunction func, void* arg, bool start_thread, bool autodelete)
{
   AssertFatal(!start_thread, "Thread::Thread - starting from the constructor is not supported");

   mData = new PlatformThreadData;
   mData->mRunFunc = func;
   mData->mRunArg  = arg;
   mData->mThread  = this;

   shouldStop  = false;
   autoDelete  = autodelete;
}

Thread::~Thread()
{
   stop();
   if (isAlive())
      join();

   delete mData;
}

void Thread::start(void* arg)
{
   if (isAlive())
      return;

   shouldStop = false;

   if (arg)
      mData->mRunArg = arg;

   mData->mSDLThread = SDL_CreateThread(sdlThreadEntry, "TorqueThread", mData);
}

bool Thread::join()
{
   if (!mData->mSDLThread)
      return false;

   SDL_WaitThread(mData->mSDLThread, NULL);
   mData->mSDLThread = NULL;
   return true;
}

void Thread::run(void* arg)
{
   if (mData->mRunFunc)
      mData->mRunFunc(arg);
}

bool Thread::isAlive()
{
   if (!mData->mSDLThread)
      return false;

   // The gateway is held for the lifetime of the thread body.
   if (mData->mGateway.acquire(false))
   {
      mData->mGateway.release();
      return false;
   }
   return true;
}

U32 Thread::getId()
{
   return mData->mThreadId;
}

void Thread::_setName(const char* name)
{
   // SDL names a thread at creation; there is no rename after the fact.
   TORQUE_UNUSED(name);
}

U32 ThreadManager::getCurrentThreadId()
{
   return U32(SDL_GetCurrentThreadID());
}

bool ThreadManager::compare(U32 threadId_1, U32 threadId_2)
{
   return threadId_1 == threadId_2;
}
