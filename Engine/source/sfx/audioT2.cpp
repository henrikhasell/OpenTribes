// HH: Added for Tribes 2 compatibility.

#include "platform/platform.h"
#include "sfx/audioT2.h"

#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "console/sim.h"
#include "console/simSet.h"
#include "core/module.h"
#include "core/strings/stringFunctions.h"
#include "core/util/path.h"
#include "T3D/gameBase/t2Layout.h"
#include "math/mMathFn.h"
#include "math/mMatrix.h"
#include "sfx/sfxDescription.h"
#include "sfx/sfxResource.h"
#include "sfx/sfxSource.h"
#include "sfx/sfxSystem.h"


IMPLEMENT_CO_DATABLOCK_V1( AudioDescription );
IMPLEMENT_CO_DATABLOCK_V1( AudioProfile );

ConsoleDocClass( AudioDescription,
   "@brief Tribes 2's name for an SFXDescription.\n\n"
   "Tribes 2 declares its sound descriptions as AudioDescription, with the "
   "same volume, isLooping and is3D that SFXDescription has.  Its `type` is a "
   "mixer channel, which is resolved to an SFXSource group on add.\n\n"
   "@ingroup SFX" );

ConsoleDocClass( AudioProfile,
   "@brief Tribes 2's name for an SFXProfile.\n\n"
   "Tribes 2 declares its sounds as AudioProfile, with the same filename, "
   "description and preload that SFXProfile has.  Without this class none of "
   "them were built, so a shell button had no sound to play.\n\n"
   "@ingroup SFX" );


//-----------------------------------------------------------------------------
// Mixer channels.
//-----------------------------------------------------------------------------

namespace AudioChannels
{
   struct Description
   {
      Channel channel;
      const char* global;
      const char* group;
   };

   static const Description sChannels[] =
   {
      { Default, "$DefaultAudioType", "AudioChannelDefault" },
      { Gui,     "$GuiAudioType",     "AudioChannelGui"     },
      { Effect,  "$EffectAudioType",  "AudioChannelEffect"  },
      { Music,   "$MusicAudioType",   "AudioChannelMusic"   },
      { Voice,   "$VoiceAudioType",   "AudioChannelVoice"   },
      { Chat,    "$ChatAudioType",    "AudioChannelChat"    },
      { Radio,   "$RadioAudioType",   "AudioChannelRadio"   },
   };

   static SimObjectPtr< SFXSource > sGroups[ Count ];
   static SimObjectPtr< SFXSource > sMaster;

   /// Build a description that puts its sources under \a group.
   static SFXDescription* createDescription( const char* name, SFXSource* group )
   {
      SFXDescription* description = new SFXDescription;
      description->mSourceGroup = group;

      if( !description->registerObject( name ) )
      {
         delete description;
         return NULL;
      }

      return description;
   }

   /// Build a source that stands in for a channel, and start it so that the
   /// sounds added to it are allowed to play.
   static SFXSource* createGroup( const char* name, SFXDescription* description )
   {
      SFXSource* group = new SFXSource;
      group->setDataField( StringTable->insert( "description" ), NULL,
                           description->getName() );

      if( !group->registerObject( name ) )
      {
         delete group;
         return NULL;
      }

      group->play();
      return group;
   }

   void create()
   {
      if( sMaster )
         return;

      SFXDescription* masterDescription = createDescription( "AudioChannelMasterDescription", NULL );
      if( !masterDescription )
         return;

      sMaster = createGroup( "AudioChannelMaster", masterDescription );
      if( !sMaster )
         return;

      SFXDescription* channelDescription = createDescription( "AudioChannelDescription", sMaster );
      if( !channelDescription )
         return;

      for( U32 i = 0; i < Count; ++ i )
      {
         const Description& description = sChannels[ i ];

         sGroups[ description.channel ] = createGroup( description.group, channelDescription );
         Con::setIntVariable( description.global, description.channel );
      }
   }

   SFXSource* group( S32 channel )
   {
      if( channel < 0 || channel >= Count )
         return NULL;

      return sGroups[ channel ];
   }

   SFXSource* master()
   {
      return sMaster;
   }
}

MODULE_BEGIN( AudioT2 )

   MODULE_INIT_AFTER( Sim )

   MODULE_INIT
   {
      AudioChannels::create();
   }

MODULE_END;


//-----------------------------------------------------------------------------
// Datablocks.
//-----------------------------------------------------------------------------

void AudioDescription::readT2Fields()
{
   U32 word;
   F32 value;

   if( T2Layout::lastDataBlockWord( T2Layout::T2DbAudioVolume, &word ) )
      mVolume = F32( word ) / 63.0f;

   if( T2Layout::lastDataBlockWord( T2Layout::T2DbAudioLooping, &word ) )
      mIsLooping = word != 0;

   if( T2Layout::lastDataBlockWord( T2Layout::T2DbAudio3D, &word ) )
      mIs3D = word != 0;

   if( T2Layout::lastDataBlockField( T2Layout::T2DbAudioReferenceDistance,
                                     &value ) )
      mMinDistance = value;

   if( T2Layout::lastDataBlockField( T2Layout::T2DbAudioMaxDistance, &value ) )
      mMaxDistance = value;

   if( T2Layout::lastDataBlockWord( T2Layout::T2DbAudioConeInsideAngle,
                                    &word ) )
      mConeInsideAngle = word;

   if( T2Layout::lastDataBlockWord( T2Layout::T2DbAudioConeOutsideAngle,
                                    &word ) )
      mConeOutsideAngle = word;

   if( T2Layout::lastDataBlockWord( T2Layout::T2DbAudioConeOutsideVolume,
                                    &word ) )
      mConeOutsideVolume = F32( word ) / 63.0f;

   if( T2Layout::lastDataBlockWord( T2Layout::T2DbAudioChannel, &word ) )
   {
      char type[ 16 ];
      dSprintf( type, sizeof( type ), "%u", word );
      setDataField( StringTable->insert( "type" ), NULL, type );
   }
}


void AudioProfile::readT2Fields()
{
   const Vector< String > &strings = T2Layout::lastStrings();
   if( strings.size() && strings.first().isNotEmpty() )
   {
      String file = strings.first();
      if( !file.endsWith( ".wav" ) && !file.endsWith( ".WAV" ) )
         file += ".wav";

      mFilename = file;
   }

   U32 word;
   if( T2Layout::lastDataBlockWord( T2Layout::T2DbAudioDescription, &word ) )
      *reinterpret_cast< U32 * >( &mDescription ) =
         ( ( word + DataBlockObjectIdFirst ) << 1 ) | 0x1;

   mPreload = false;
}


bool AudioDescription::onAdd()
{
   static const char* sType = StringTable->insert( "type" );

   const char* type = getDataField( sType, NULL );
   if( type && type[ 0 ] )
   {
      SFXSource* group = AudioChannels::group( dAtoi( type ) );
      if( group )
         mSourceGroup = group;
      else
         Con::warnf( "AudioDescription::onAdd - %s names no mixer channel '%s'",
            getName(), type );
   }

   return Parent::onAdd();
}


//-----------------------------------------------------------------------------
// Sources.
//-----------------------------------------------------------------------------

/// Resolve the one argument every alx function takes: a source we made
/// earlier, or a profile to make one from.
static SFXSource* findSource( const char* name )
{
   return dynamic_cast< SFXSource* >( Sim::findObject( name ) );
}

static SFXTrack* findTrack( const char* name )
{
   return dynamic_cast< SFXTrack* >( Sim::findObject( name ) );
}

/// Build the throwaway profile that alxCreateSource( description, filename )
/// asks for.
static SFXProfile* createProfile( SFXDescription* description, const char* filename )
{
   SFXProfile* profile = new SFXProfile( description, StringTable->insert( filename ), true );
   if( !profile->registerObject() )
   {
      delete profile;
      return NULL;
   }

   profile->setAutoDelete( true );
   return profile;
}

static MatrixF positionAt( const char* x, const char* y, const char* z )
{
   MatrixF transform;
   transform.set( EulerF( 0, 0, 0 ), Point3F( dAtof( x ), dAtof( y ), dAtof( z ) ) );
   return transform;
}

ConsoleFunction( alxCreateSource, S32, 2, 6,
   "( AudioProfile profile | ( AudioDescription description, string filename ) "
   "[, float x, float y, float z ] )\n"
   "Create a stopped source and return a handle to it.\n\n"
   "@ingroup SFX" )
{
   AudioChannels::create();

   SFXTrack* track = findTrack( argv[ 1 ] );
   U32 firstCoord = 2;

   if( !track )
   {
      SFXDescription* description = dynamic_cast< SFXDescription* >( Sim::findObject( argv[ 1 ] ) );
      if( !description || argc < 3 )
      {
         Con::errorf( "alxCreateSource - no sound or description '%s'", argv[ 1 ] );
         return 0;
      }

      track = createProfile( description, argv[ 2 ] );
      if( !track )
         return 0;

      firstCoord = 3;
   }

   SFXSource* source;
   if( argc > firstCoord + 2 )
   {
      const MatrixF transform = positionAt( argv[ firstCoord ],
                                            argv[ firstCoord + 1 ],
                                            argv[ firstCoord + 2 ] );
      source = SFX->createSource( track, &transform );
   }
   else
      source = SFX->createSource( track );

   return source ? source->getId() : 0;
}

ConsoleFunction( alxPlay, S32, 2, 5,
   "( AudioSource handle | ( AudioProfile profile [, float x, float y, float z ] ) )\n"
   "Play a source created earlier, or create a play-once source for a sound "
   "and play that.\n\n"
   "@ingroup SFX" )
{
   AudioChannels::create();

   SFXSource* source = findSource( argv[ 1 ] );
   if( source )
   {
      source->play();
      return source->getId();
   }

   SFXTrack* track = findTrack( argv[ 1 ] );
   if( !track )
   {
      Con::errorf( "alxPlay - no source or sound '%s'", argv[ 1 ] );
      return 0;
   }

   if( argc > 4 )
   {
      const MatrixF transform = positionAt( argv[ 2 ], argv[ 3 ], argv[ 4 ] );
      source = SFX->playOnce( track, &transform );
   }
   else
      source = SFX->playOnce( track );

   return source ? source->getId() : 0;
}

DefineEngineFunction( alxStop, void, ( const char* handle ),,
   "Stop the source named by @a handle and release it.\n\n"
   "@ingroup SFX" )
{
   SFXSource* source = findSource( handle );
   if( source )
      SFX->stopAndDeleteSource( source );
}

DefineEngineFunction( alxStopAll, void, (),,
   "Stop every playing source.\n\n"
   "@ingroup SFX" )
{
   SimSet* sources = Sim::getSFXSourceSet();
   if( !sources )
      return;

   // Stopping a play-once source deletes it, so collect before stopping.  A
   // channel group is a source too, and stopping one would silence that
   // channel for good, so the groups are the only sources left alone.
   Vector< SimObjectPtr< SFXSource > > playing;
   for( SimSet::iterator iter = sources->begin(); iter != sources->end(); ++ iter )
   {
      SFXSource* source = dynamic_cast< SFXSource* >( *iter );
      if( !source || source == AudioChannels::master() )
         continue;

      if( source->getSourceGroup() != AudioChannels::master() )
         playing.push_back( source );
   }

   for( U32 i = 0; i < playing.size(); ++ i )
      if( playing[ i ] )
         playing[ i ]->stop();
}

DefineEngineFunction( alxSourcef, void, ( const char* handle, const char* name, F32 value ),,
   "Set a float property of a source.  AL_GAIN and AL_GAIN_LINEAR set its "
   "volume, AL_PITCH its pitch.\n\n"
   "@ingroup SFX" )
{
   SFXSource* source = findSource( handle );
   if( !source )
   {
      Con::errorf( "alxSourcef - no source '%s'", handle );
      return;
   }

   if( dStrstr( name, "PITCH" ) )
      source->setPitch( value );
   else if( dStrstr( name, "GAIN" ) )
      source->setVolume( value );
   else
      Con::warnf( "alxSourcef - '%s' is not a property we set", name );
}

DefineEngineFunction( alxIsEnabled, bool, (),,
   "Whether a sound device was opened.\n\n"
   "@ingroup SFX" )
{
   return SFX && SFX->hasDevice();
}


//-----------------------------------------------------------------------------
// Channels and the listener.
//-----------------------------------------------------------------------------

DefineEngineFunction( alxSetChannelVolume, void, ( S32 channel, F32 volume ),,
   "Set the volume of one mixer channel, such as $GuiAudioType.\n\n"
   "@ingroup SFX" )
{
   AudioChannels::create();

   SFXSource* group = AudioChannels::group( channel );
   if( group )
      group->setVolume( mClampF( volume, 0.f, 1.f ) );
   else
      Con::errorf( "alxSetChannelVolume - %i is not a mixer channel", channel );
}

DefineEngineFunction( alxGetChannelVolume, F32, ( S32 channel ),,
   "The volume of one mixer channel.\n\n"
   "@ingroup SFX" )
{
   AudioChannels::create();

   SFXSource* group = AudioChannels::group( channel );
   return group ? group->getVolume() : 0.f;
}

DefineEngineFunction( alxListenerf, void, ( const char* name, F32 value ),,
   "Set a float property of the listener.  AL_GAIN and AL_GAIN_LINEAR set the "
   "master volume.\n\n"
   "@ingroup SFX" )
{
   AudioChannels::create();

   if( !dStrstr( name, "GAIN" ) )
   {
      Con::warnf( "alxListenerf - '%s' is not a property we set", name );
      return;
   }

   if( AudioChannels::master() )
      AudioChannels::master()->setVolume( mClampF( value, 0.f, 1.f ) );
}


//-----------------------------------------------------------------------------
// Sound files.
//-----------------------------------------------------------------------------

DefineEngineFunction( alxGetWaveLen, S32, ( const char* filename ),,
   "The length of a sound file in milliseconds, or 0 if it cannot be read.\n\n"
   "@ingroup SFX" )
{
   Resource< SFXResource > resource = SFXResource::load( filename );
   return resource != NULL ? resource->getDuration() : 0;
}


//-----------------------------------------------------------------------------
// Music.
//-----------------------------------------------------------------------------

static SimObjectPtr< SFXSource > sMusicSource;
static bool sMusicStopping = false;

/// Tribes 2 spells a music path the way Windows does and roots it at the game
/// directory rather than the mod, so it reaches us as `base\music\x.mp3`.
static String musicPath( const char* filename )
{
   String path( filename );
   path.replace( '\\', '/' );
   return path;
}

static void stopMusic()
{
   if( !sMusicSource )
      return;

   sMusicStopping = true;
   SFX->stopAndDeleteSource( sMusicSource );
   sMusicSource = NULL;
   sMusicStopping = false;
}

DefineEngineFunction( _alxMusicStatus, void, ( const char* handle, const char* status ),,
   "@internal" )
{
   if( sMusicStopping || dStricmp( status, "Stopped" ) != 0 )
      return;

   sMusicSource = NULL;
   Con::executef( "finishedMusicStream", "false" );
}

DefineEngineFunction( alxPlayMusic, bool, ( const char* filename ),,
   "Stream a music track, replacing the one playing.\n\n"
   "@ingroup SFX" )
{
   AudioChannels::create();
   stopMusic();

   SFXDescription* description;
   if( !Sim::findObject( "AudioChannelMusicDescription", description ) )
   {
      description = new SFXDescription;
      description->mIsStreaming = true;
      description->mIs3D = false;
      description->mIsLooping = false;
      description->mSourceGroup = AudioChannels::group( AudioChannels::Music );

      if( !description->registerObject( "AudioChannelMusicDescription" ) )
      {
         delete description;
         return false;
      }
   }

   SFXProfile* profile = createProfile( description, musicPath( filename ).c_str() );
   if( !profile )
      return false;

   sMusicSource = SFX->createSource( profile );
   if( !sMusicSource )
   {
      Con::errorf( "alxPlayMusic - could not stream '%s'", filename );
      return false;
   }

   sMusicSource->setDataField( StringTable->insert( "statusCallback" ), NULL, "_alxMusicStatus" );
   sMusicSource->play();

   // A track that reaches its end is never played again -- the script starts
   // the next one -- so let the system reap it rather than deleting it from
   // inside its own status callback.
   SFX->deleteWhenStopped( sMusicSource );

   return true;
}

DefineEngineFunction( alxStopMusic, void, (),,
   "Stop the music track.\n\n"
   "@ingroup SFX" )
{
   stopMusic();
}


//-----------------------------------------------------------------------------
// Settings Tribes 2 asks for that we have no equivalent of.
//
// Each of these configured a Miles or EAX feature that the OpenAL backend
// either does for us or does not do at all.  They are answered rather than
// implemented so that the scripts calling them run to the end; a missing
// function aborts the rest of initAudio().
//-----------------------------------------------------------------------------

DefineEngineFunction( alxEnableEnvironmental, void, ( bool enable ),,
   "@internal" )
{
   TORQUE_UNUSED( enable );
}

DefineEngineFunction( alxDisableOuterFalloffs, void, ( bool disable ),,
   "@internal" )
{
   TORQUE_UNUSED( disable );
}

DefineEngineFunction( alxSetInnerFalloffScale, void, ( F32 scale ),,
   "@internal" )
{
   TORQUE_UNUSED( scale );
}

DefineEngineFunction( alxEnableForceFeedback, void, ( bool enable ),,
   "@internal" )
{
   TORQUE_UNUSED( enable );
}

DefineEngineFunction( alxContexti, void, ( const char* name, S32 value ),,
   "@internal" )
{
   TORQUE_UNUSED( name );
   TORQUE_UNUSED( value );
}

DefineEngineFunction( alxGetContexti, S32, ( const char* name ),,
   "@internal" )
{
   TORQUE_UNUSED( name );
   return 0;
}

DefineEngineFunction( alxGetContextstr, const char*, ( const char* name, S32 index ),,
   "@internal" )
{
   TORQUE_UNUSED( name );
   TORQUE_UNUSED( index );

   // The only provider we have is the one we opened, and the scripts ask for
   // its name to decide whether it is a hardware mixer.  None of the names
   // they know is ours, which is the answer we want: no EAX, no falloff
   // fiddling.
   return SFX && SFX->hasDevice() ? "OpenAL" : "";
}


//-----------------------------------------------------------------------------
// Voice capture, which we do not have.
//-----------------------------------------------------------------------------

DefineEngineFunction( alxCaptureInit, bool, (),,
   "@internal" )
{
   return false;
}

DefineEngineFunction( alxCaptureDestroy, void, (),,
   "@internal" )
{
}

DefineEngineFunction( alxCaptureStart, void, (),,
   "@internal" )
{
}

DefineEngineFunction( alxCaptureStop, void, (),,
   "@internal" )
{
}

DefineEngineFunction( alxSetCaptureGainScale, void, ( F32 scale ),,
   "@internal" )
{
   TORQUE_UNUSED( scale );
}


//-----------------------------------------------------------------------------
// Driver selection.
//
// Tribes 2 picks a Miles or DirectSound driver by name and gates the rest of
// its audio setup on the answer, so we accept whatever it asks for as long as
// a device is open.
//-----------------------------------------------------------------------------

DefineEngineFunction( audioDetect, void, (),,
   "@internal" )
{
   AudioChannels::create();
}

DefineEngineFunction( audioSetDriver, bool, ( const char* driver ),,
   "@internal" )
{
   TORQUE_UNUSED( driver );

   AudioChannels::create();
   return SFX && SFX->hasDevice();
}
