//-----------------------------------------------------------------------------
// Fills the OpenAL dispatch table from the linked openal-soft.
//
// This replaced the win32 and macOS LoadOAL implementations, which dlopened the
// driver by name at runtime. openal-soft is fetched and linked by CMake now, so
// the entries are just the linked symbols and nothing here is platform aware.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "sfx/openal/LoadOAL.h"

ALboolean LoadOAL10Library( char* szOALFullPathName, LPOPENALFNTABLE table )
{
   TORQUE_UNUSED( szOALFullPathName );

   if( !table )
      return AL_FALSE;

   table->alEnable               = alEnable;
   table->alDisable              = alDisable;
   table->alIsEnabled            = alIsEnabled;
   table->alGetBoolean           = alGetBoolean;
   table->alGetInteger           = alGetInteger;
   table->alGetFloat             = alGetFloat;
   table->alGetDouble            = alGetDouble;
   table->alGetBooleanv          = alGetBooleanv;
   table->alGetIntegerv          = alGetIntegerv;
   table->alGetFloatv            = alGetFloatv;
   table->alGetDoublev           = alGetDoublev;
   table->alGetString            = alGetString;
   table->alGetError             = alGetError;
   table->alIsExtensionPresent   = alIsExtensionPresent;
   table->alGetProcAddress       = alGetProcAddress;
   table->alGetEnumValue         = alGetEnumValue;
   table->alListeneri            = alListeneri;
   table->alListenerf            = alListenerf;
   table->alListener3f           = alListener3f;
   table->alListenerfv           = alListenerfv;
   table->alGetListeneri         = alGetListeneri;
   table->alGetListenerf         = alGetListenerf;
   table->alGetListener3f        = alGetListener3f;
   table->alGetListenerfv        = alGetListenerfv;
   table->alGenSources           = alGenSources;
   table->alDeleteSources        = alDeleteSources;
   table->alIsSource             = alIsSource;
   table->alSourcei              = alSourcei;
   table->alSourcef              = alSourcef;
   table->alSource3f             = alSource3f;
   table->alSourcefv             = alSourcefv;
   table->alGetSourcei           = alGetSourcei;
   table->alGetSourcef           = alGetSourcef;
   table->alGetSourcefv          = alGetSourcefv;
   table->alSourcePlayv          = alSourcePlayv;
   table->alSourceStopv          = alSourceStopv;
   table->alSourcePlay           = alSourcePlay;
   table->alSourcePause          = alSourcePause;
   table->alSourceStop           = alSourceStop;
   table->alSourceRewind         = alSourceRewind;
   table->alGenBuffers           = alGenBuffers;
   table->alDeleteBuffers        = alDeleteBuffers;
   table->alIsBuffer             = alIsBuffer;
   table->alBufferData           = alBufferData;
   table->alGetBufferi           = alGetBufferi;
   table->alGetBufferf           = alGetBufferf;
   table->alSourceQueueBuffers   = alSourceQueueBuffers;
   table->alSourceUnqueueBuffers = alSourceUnqueueBuffers;
   table->alDistanceModel        = alDistanceModel;
   table->alDopplerFactor        = alDopplerFactor;
   table->alDopplerVelocity      = alDopplerVelocity;
   table->alcGetString           = alcGetString;
   table->alcGetIntegerv         = alcGetIntegerv;
   table->alcOpenDevice          = alcOpenDevice;
   table->alcCloseDevice         = alcCloseDevice;
   table->alcCreateContext       = alcCreateContext;
   table->alcMakeContextCurrent  = alcMakeContextCurrent;
   table->alcProcessContext      = alcProcessContext;
   table->alcGetCurrentContext   = alcGetCurrentContext;
   table->alcGetContextsDevice   = alcGetContextsDevice;
   table->alcSuspendContext      = alcSuspendContext;
   table->alcDestroyContext      = alcDestroyContext;
   table->alcGetError            = alcGetError;
   table->alcIsExtensionPresent  = alcIsExtensionPresent;
   table->alcGetProcAddress      = alcGetProcAddress;
   table->alcGetEnumValue        = alcGetEnumValue;

   return AL_TRUE;
}

ALvoid UnloadOAL10Library()
{
   // Nothing to unload: openal-soft is linked, not opened at runtime.
}
