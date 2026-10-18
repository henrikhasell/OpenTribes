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

// HH: Replaced for Tribes 2 compatibility. Retail's precipitation.cc, from
// PrecipitationData::initPersistFields (0x006801c0) to Precipitation::
// updateStorm (0x00683360).

#include "platform/platform.h"
#include "T3D/fx/precipitation.h"

#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "core/stream/bitStream.h"
#include "core/stream/fileStream.h"
#include "environment/sky.h"
#include "gfx/gfxVertexBuffer.h"
#include "materials/shaderData.h"
#include "math/mRandom.h"
#include "renderInstance/renderPassManager.h"
#include "scene/sceneManager.h"
#include "scene/sceneRenderState.h"
#include "sfx/sfxSource.h"
#include "sfx/sfxSystem.h"
#include "sfx/sfxTrack.h"
#include "sfx/sfxTypes.h"
#include "T3D/gameBase/gameConnection.h"
#include "T3D/gameBase/t2Layout.h"
#include "T3D/player.h"

IMPLEMENT_CO_NETOBJECT_V1(Precipitation);
IMPLEMENT_CO_DATABLOCK_V1(PrecipitationData);

ConsoleDocClass( Precipitation,
   "@brief Tribes 2's rain, snow and sand.\n\n"
   "Drops spawn in boxes about the camera and fall a set distance every "
   "frame, drawn as camera-facing quads cut from a 4x4 sheet of flakes.\n\n"
   "@ingroup FX\n" );

ConsoleDocClass( PrecipitationData,
   "@brief What a Precipitation falls as, and the boxes it spawns in.\n\n"
   "@ingroup FX\n" );

bool Precipitation::smPrecipitationOn = true;
bool Precipitation::smPrePause = false;

const RenderInstType Precipitation::RIT_Precipitation( "Precipitation" );

// Retail loads the sheet as a BitmapTexture, the type its texture manager
// filters GL_LINEAR with no mip levels (0x0044b85f), which is why a far flake
// stays a crisp speck rather than fading into its neighbours on the sheet.
GFX_ImplementTextureProfile( PrecipitationTextureProfile,
                             GFXTextureProfile::DiffuseMap,
                             GFXTextureProfile::PreserveSize | GFXTextureProfile::Static | GFXTextureProfile::NoMipmap,
                             GFXTextureProfile::None );

static F32 readF32( BitStream *stream )
{
   const U32 word = stream->readInt( 32 );
   F32 value;
   dMemcpy( &value, &word, sizeof( value ) );
   return value;
}

static void writeF32( BitStream *stream, F32 value )
{
   U32 word;
   dMemcpy( &word, &value, sizeof( word ) );
   stream->writeInt( word, 32 );
}

static void readColor( BitStream *stream, ColorF *color )
{
   ColorI c;
   c.red   = stream->readInt( 8 );
   c.green = stream->readInt( 8 );
   c.blue  = stream->readInt( 8 );
   c.alpha = stream->readInt( 8 );
   *color = c;
}

static void writeColor( BitStream *stream, const ColorF &color )
{
   const ColorI c = color;
   stream->writeInt( c.red,   8 );
   stream->writeInt( c.green, 8 );
   stream->writeInt( c.blue,  8 );
   stream->writeInt( c.alpha, 8 );
}

/// Retail rounds up with the FPU in round-to-plus-infinity, then takes one.
static S32 ceilIndex( F32 value )
{
   return S32( mCeil( value ) ) - 1;
}

//--------------------------------------------------------------------------
// PrecipitationData
//--------------------------------------------------------------------------

PrecipitationData::PrecipitationData()
{
   soundProfile     = NULL;
   type             = Rain;
   maxSize          = 1.0f;
   sizeX            = 1.0f;
   sizeY            = 1.0f;
   materialList     = StringTable->EmptyString();

   movingBoxPer     = 0.0f;
   divHeightVal     = 1.0f;
   sizeBigBox       = 0.0f;
   topBoxSpeed      = 0.0f;
   frontBoxSpeed    = 0.0f;
   topBoxDrawPer    = 0.0f;
   bottomDrawHeight = 0.0f;
   skipIfPer        = 0.0f;
   bottomSpeedPer   = 1.0f;
   frontSpeedPer    = 1.0f;
   frontRadiusPer   = 1.0f;
}

void PrecipitationData::initPersistFields()
{
   addField( "soundProfile",     TypeSFXTrackName, Offset( soundProfile, PrecipitationData ),
      "Looped where the precipitation is, while it shows." );
   addField( "type",             TypeS32,    Offset( type, PrecipitationData ),
      "0 rain, 1 snow, 2 sand." );
   addField( "maxSize",          TypeF32,    Offset( maxSize, PrecipitationData ), "" );
   addField( "materialList",     TypeString, Offset( materialList, PrecipitationData ),
      "A .dml under textures/; the first texture it names is the sheet of drops." );
   addField( "sizeX",            TypeF32,    Offset( sizeX, PrecipitationData ),
      "Half the width of a drop, in metres." );
   addField( "sizeY",            TypeF32,    Offset( sizeY, PrecipitationData ),
      "Half the height of a drop, in metres." );
   addField( "movingBoxPer",     TypeF32,    Offset( movingBoxPer, PrecipitationData ), "" );
   addField( "divHeightVal",     TypeF32,    Offset( divHeightVal, PrecipitationData ), "" );
   addField( "sizeBigBox",       TypeF32,    Offset( sizeBigBox, PrecipitationData ), "" );
   addField( "topBoxSpeed",      TypeF32,    Offset( topBoxSpeed, PrecipitationData ), "" );
   addField( "frontBoxSpeed",    TypeF32,    Offset( frontBoxSpeed, PrecipitationData ), "" );
   addField( "topBoxDrawPer",    TypeF32,    Offset( topBoxDrawPer, PrecipitationData ), "" );
   addField( "bottomDrawHeight", TypeF32,    Offset( bottomDrawHeight, PrecipitationData ), "" );
   addField( "skipIfPer",        TypeF32,    Offset( skipIfPer, PrecipitationData ), "" );
   addField( "bottomSpeedPer",   TypeF32,    Offset( bottomSpeedPer, PrecipitationData ), "" );
   addField( "FrontSpeedPer",    TypeF32,    Offset( frontSpeedPer, PrecipitationData ), "" );
   addField( "FrontRadiusPer",   TypeF32,    Offset( frontRadiusPer, PrecipitationData ), "" );

   Parent::initPersistFields();
}

bool PrecipitationData::onAdd()
{
   if( !Parent::onAdd() )
      return false;

   // Retail's onAdd (0x00680350) clamps both sizes to (0, 20].
   if( sizeX <= 0.0f || sizeX > 20.0f )
   {
      Con::errorf( "PrecipitationData(%s)::onAdd: sizeX must be in the range [0 >, 20]", getName() );
      sizeX = 1.0f;
   }
   if( sizeY <= 0.0f || sizeY > 20.0f )
   {
      Con::errorf( "PrecipitationData(%s)::onAdd: sizeY must be in the range [0 >, 20]", getName() );
      sizeY = 1.0f;
   }

   return true;
}

bool PrecipitationData::preload( bool server, String &errorStr )
{
   if( !Parent::preload( server, errorStr ) )
      return false;

   if( !server && soundProfile && !sfxResolve( &soundProfile, errorStr ) )
      Con::errorf( "PrecipitationData::preload: %s", errorStr.c_str() );

   return true;
}

// The whole of what retail's packData (0x00680450) writes: no parent field
// comes first, which is why neither half calls up to its parent.
void PrecipitationData::packData( BitStream *stream, U32 )
{
   if( stream->writeFlag( soundProfile != NULL ) )
      stream->writeInt( soundProfile->getId() - DataBlockObjectIdFirst,
                        DataBlockObjectIdBitSize );

   stream->writeInt( type, 32 );
   writeF32( stream, maxSize );
   stream->writeString( materialList );
   writeF32( stream, sizeX );
   writeF32( stream, sizeY );
   writeF32( stream, movingBoxPer );
   writeF32( stream, divHeightVal );
   writeF32( stream, sizeBigBox );
   writeF32( stream, topBoxSpeed );
   writeF32( stream, frontBoxSpeed );
   writeF32( stream, topBoxDrawPer );
   writeF32( stream, bottomDrawHeight );
   writeF32( stream, skipIfPer );
   writeF32( stream, bottomSpeedPer );
   writeF32( stream, frontSpeedPer );
   writeF32( stream, frontRadiusPer );
}

bool PrecipitationData::unpackData( BitStream *stream, U32 )
{
   if( stream->readFlag() )
   {
      const U32 id = stream->readInt( DataBlockObjectIdBitSize ) + DataBlockObjectIdFirst;
      *reinterpret_cast< U32* >( &soundProfile ) = ( id << 1 ) | 0x1;
   }
   else
      soundProfile = NULL;

   type         = stream->readInt( 32 );
   maxSize      = readF32( stream );
   materialList = stream->readSTString();
   sizeX        = readF32( stream );
   sizeY        = readF32( stream );
   movingBoxPer     = readF32( stream );
   divHeightVal     = readF32( stream );
   sizeBigBox       = readF32( stream );
   topBoxSpeed      = readF32( stream );
   frontBoxSpeed    = readF32( stream );
   topBoxDrawPer    = readF32( stream );
   bottomDrawHeight = readF32( stream );
   skipIfPer        = readF32( stream );
   bottomSpeedPer   = readF32( stream );
   frontSpeedPer    = readF32( stream );
   frontRadiusPer   = readF32( stream );

   return true;
}

//--------------------------------------------------------------------------
// Precipitation
//--------------------------------------------------------------------------

Precipitation::Precipitation()
{
   mTypeMask |= EnvironmentObjectType;
   mNetFlags.set( Ghostable | ScopeAlways );

   mDataBlock = NULL;
   dMemset( mDrops, 0, sizeof( mDrops ) );

   mPercentage  = 1.0f;
   mColors[0].set( -1.0f, 0.0f, 0.0f, 1.0f );
   mColors[1].set( -1.0f, 0.0f, 0.0f, 1.0f );
   mColors[2].set( -1.0f, 0.0f, 0.0f, 1.0f );
   mNumColors   = 0;
   mOffsetSpeed = 0.25f;
   mMinVelocity = -1.0f;
   mMaxVelocity = -1.0f;
   mMaxNumDrops = -1;
   mMaxRadius   = -1;

   mNumDrops  = 0;
   mNumActive = 0;

   mStormLastMs = 0;
   mStormTarget = -1.0f;
   mStormTime   = 0.0f;
   mStormRate   = 0.0f;
   mStormDir    = 0;
   mStormTicks  = 0.0f;
   mStormShow   = true;

   mFirstFrame  = false;
   mNotRendered = true;
   mTime        = 0.0f;
   mLastTime    = 0.0f;
   mSpawnOffset.zero();
   mLastCamPos.zero();
   mCamVelFracX = 0.0f;
   mCamVelFracY = 0.0f;
   mBoxMin.zero();
   mBoxMax.zero();
   mAvgSpeed    = 0.0f;
   mBoxTopDelta = 0.0f;
   mWind.set( 0.0f, 0.0f, 1.0f );
   mFrameOffset.zero();

   mAmbientSound = NULL;
   mModelViewSC  = NULL;
}

Precipitation::~Precipitation()
{
   SFX_DELETE( mAmbientSound );
}

void Precipitation::initPersistFields()
{
   addField( "percentage",  TypeF32,    Offset( mPercentage, Precipitation ),
      "Share of maxNumDrops that falls, 0 to 1." );
   addField( "color1",      TypeColorF, Offset( mColors[0], Precipitation ),
      "Drops are tinted one of up to three colours; a negative red ends the list." );
   addField( "color2",      TypeColorF, Offset( mColors[1], Precipitation ), "" );
   addField( "color3",      TypeColorF, Offset( mColors[2], Precipitation ), "" );
   addField( "offsetSpeed", TypeF32,    Offset( mOffsetSpeed, Precipitation ),
      "How far the sky's wind pushes a drop sideways per metre it falls." );
   addField( "minVelocity", TypeF32,    Offset( mMinVelocity, Precipitation ),
      "Slowest fall, in metres a frame." );
   addField( "maxVelocity", TypeF32,    Offset( mMaxVelocity, Precipitation ),
      "Fastest fall, in metres a frame." );
   addField( "maxNumDrops", TypeS32,    Offset( mMaxNumDrops, Precipitation ),
      "At most 2000." );
   addField( "maxRadius",   TypeS32,    Offset( mMaxRadius, Precipitation ),
      "How far from the camera drops spawn, in metres." );

   Parent::initPersistFields();
}

void Precipitation::consoleInit()
{
   Con::addVariable( "$pref::precipitationOn", TypeBool, &smPrecipitationOn,
      "Draw and sound precipitation at all.\n\n@ingroup FX" );
   Con::addVariable( "$pref::prePause", TypeBool, &smPrePause,
      "Hold every drop where it is.\n\n@ingroup FX" );
}

// Retail's validation (0x00680e50), run from both onAdd and onNewDataBlock: a
// mission that leaves a field at its default gets the type's own.
void Precipitation::validate()
{
   mNumColors = 0;
   for( S32 i = 0; i < MaxColors && mColors[i].red >= 0.0f; ++i )
      ++mNumColors;

   const S32 kind = mDataBlock ? mDataBlock->type : PrecipitationData::Rain;

   if( mNumColors == 0 )
   {
      if( kind == PrecipitationData::Rain )
         mColors[0].set( 0.6f, 0.6f, 0.6f, 1.0f );
      else if( kind == PrecipitationData::Snow )
         mColors[0].set( 1.0f, 1.0f, 1.0f, 1.0f );
      else
         mColors[0].set( 0.9f, 0.8f, 0.5f, 1.0f );
      mNumColors = 1;
   }

   // Retail has sand falling at snow's slowest, not a speed of its own.
   if( mMinVelocity == -1.0f )
      mMinVelocity = kind == PrecipitationData::Rain ? 1.25f : 0.25f;

   if( mMaxVelocity == -1.0f )
   {
      if( kind == PrecipitationData::Rain )
         mMaxVelocity = 4.0f;
      else if( kind == PrecipitationData::Snow )
         mMaxVelocity = 1.5f;
      else
         mMaxVelocity = 1.0f;
   }

   if( F32( mMaxRadius ) == -1.0f )
      mMaxRadius = kind == PrecipitationData::Snow ? 125 : 80;

   if( mMaxNumDrops > MaxDrops || mMaxNumDrops < 0 )
      mMaxNumDrops = MaxDrops;
}

// Retail's table (0x006834a0): sixteen cells, four to a row.
void Precipitation::initTexCoords()
{
   for( S32 row = 0; row < 4; ++row )
      for( S32 col = 0; col < 4; ++col )
         mTexCoords[row * 4 + col].set( col * 0.25f, row * 0.25f );
}

// Retail (0x006816a0) loads the .dml as a material list and draws from its
// first texture alone. A line names a path under the .dml's own directory.
void Precipitation::loadMaterialList()
{
   mTexture = NULL;

   if( !mDataBlock || !mDataBlock->materialList || !mDataBlock->materialList[0] )
      return;

   const String dml = T2Layout::resourcePath( mDataBlock->materialList, "textures/" );

   FileStream stream;
   if( !stream.open( dml, Torque::FS::File::Read ) )
   {
      Con::warnf( "Precipitation material list is missing: %s", dml.c_str() );
      return;
   }

   char line[1024];
   stream.readLine( (U8 *)line, sizeof( line ) );
   stream.close();

   char *end = line + dStrlen( line );
   while( end != line && ( end[-1] == '\r' || end[-1] == ' ' || end[-1] == '\t' ) )
      *--end = 0;

   if( !line[0] )
      return;

   const String::SizeType slash = dml.find( '/', 0, String::Right );
   const String dir = slash == String::NPos ? String() : dml.substr( 0, slash + 1 );
   const String name = dir + line;

   mTexture.set( name, &PrecipitationTextureProfile, "Precipitation" );
   if( !mTexture.isValid() )
      Con::warnf( "Precipitation texture is missing: %s", name.c_str() );
}

void Precipitation::startSound()
{
   if( mAmbientSound || !mDataBlock || !mDataBlock->soundProfile || !smPrecipitationOn )
      return;

   mAmbientSound = SFX->createSource( mDataBlock->soundProfile, &getTransform() );
   if( mAmbientSound )
      mAmbientSound->play();
}

void Precipitation::stopSound()
{
   SFX_DELETE( mAmbientSound );
}

bool Precipitation::onAdd()
{
   if( !Parent::onAdd() )
      return false;

   if( mPercentage > 1.0f || mPercentage < 0.0f )
   {
      Con::errorf( "Precipitation::onAdd - Percentage is invalid. <= 1.0 or >= 0.0" );
      mPercentage = 1.0f;
   }

   if( isClientObject() )
   {
      // A ghost's datablock arrives before it is added, and GameBase only
      // calls onNewDataBlock for a server object.
      GameBaseData *data = getDataBlock();
      if( data && !onNewDataBlock( data, false ) )
         return false;

      mFirstFrame = true;
      for( S32 i = 0; i < mMaxNumDrops && i < MaxDrops; ++i )
         mDrops[i].respawn = true;

      loadMaterialList();
      startSound();

      mNumDrops  = S32( F32( mMaxNumDrops ) * mPercentage );
      mNumActive = mNumDrops;
      initTexCoords();
   }
   else
   {
      // Retail names every one Precipitation (0x00680dd9), the name
      // weather.cs's testStorm() addresses it by.
      assignName( "Precipitation" );
   }

   setGlobalBounds();
   resetWorldBox();
   addToScene();

   validate();
   return true;
}

void Precipitation::onRemove()
{
   stopSound();
   removeFromScene();
   Parent::onRemove();
}

bool Precipitation::onNewDataBlock( GameBaseData *dptr, bool reload )
{
   mDataBlock = dynamic_cast< PrecipitationData* >( dptr );
   if( !mDataBlock || !Parent::onNewDataBlock( dptr, reload ) )
      return false;

   validate();
   mAvgSpeed = ( mMinVelocity + mMaxVelocity ) * 0.5f * 0.65f;

   scriptOnNewDataBlock();
   return true;
}

void Precipitation::processTick( const Move *move )
{
   Parent::processTick( move );
   mStormTicks += 1.0f;
}

void Precipitation::advanceTime( F32 dt )
{
   Parent::advanceTime( dt );
   if( !mNotRendered )
      mTime += dt;
}

//--------------------------------------------------------------------------
// Scripts
//--------------------------------------------------------------------------

void Precipitation::setPercentage( F32 pct )
{
   if( pct > 1.0f || pct < 0.0f )
      return;

   mPercentage = pct;
   mNumDrops = S32( F32( mMaxNumDrops ) * mPercentage );
   setMaskBits( PercentageMask );
}

void Precipitation::stormPrecipitation( F32 pct, F32 seconds )
{
   mStormTime = seconds;
   if( mStormTarget >= 0.0f )
   {
      mStormDir = mStormTarget > pct ? 2 : 1;
      mPercentage = mStormTarget;
   }
   else
      mStormDir = mPercentage > pct ? 2 : 1;

   mStormTarget = pct;
   setMaskBits( StormMask );
}

void Precipitation::stormShow( bool show )
{
   mStormShow = show;
   setMaskBits( StormShowMask );
}

DefineEngineMethod( Precipitation, setPercentage, void, ( F32 percentage ), ,
   "precipitation.setPercentage(percentage <1.0 to 0.0>)" )
{
   object->setPercentage( percentage );
}

DefineEngineMethod( Precipitation, stormPrecipitation, void, ( F32 percentage, F32 seconds ), ,
   "precipitation.stormPrecipitation(Percentage <0 to 1>, Time<sec>)" )
{
   object->stormPrecipitation( percentage, seconds );
}

DefineEngineMethod( Precipitation, stormShow, void, ( bool show ), ,
   "precipitation.stormShow(bool)" )
{
   object->stormShow( show );
}

//--------------------------------------------------------------------------
// The wire
//--------------------------------------------------------------------------

// The 28 nodes of sT2RcNodes_Precipitation, GameBase's five first. Retail's
// packUpdate is 0x00681000 and its unpackUpdate 0x00681260.
U32 Precipitation::packUpdate( NetConnection *, U32 mask, BitStream *stream, bool )
{
   if( stream->writeFlag( ( mask & PercentageMask ) && mDataBlock ) )
      stream->writeInt( mDataBlock->getId() - DataBlockObjectIdFirst,
                        DataBlockObjectIdBitSize );

   if( stream->writeFlag( mask & StormMask ) )
      stream->writeFlag( false );

   if( stream->writeFlag( mask & InitMask ) )
   {
      writeF32( stream, mPercentage );
      stream->writeInt( mNumColors, 32 );
      for( S32 i = 0; i < mNumColors; ++i )
         writeColor( stream, mColors[i] );

      writeF32( stream, mOffsetSpeed );
      writeF32( stream, mMinVelocity );
      writeF32( stream, mMaxVelocity );
      stream->writeInt( mMaxNumDrops, 32 );
      stream->writeInt( mMaxRadius, 32 );

      // A storm already under way, for a client that joins halfway through.
      if( stream->writeFlag( mStormTicks * 0.03125f < mStormTime ) )
      {
         writeF32( stream, mStormTicks );
         writeF32( stream, mStormTarget );
         writeF32( stream, mStormTime );
      }
   }

   if( stream->writeFlag( mask & StormShowMask ) )
      stream->writeInt( mStormShow ? 1 : 0, 8 );

   if( stream->writeFlag( mask & StormMask ) )
   {
      writeF32( stream, mStormTarget );
      writeF32( stream, mStormTime );
      mStormTicks = 0.0f;
   }

   if( stream->writeFlag( mask & PercentageMask ) )
      writeF32( stream, mPercentage );

   return 0;
}

bool Precipitation::unpackUpdate( BitStream *stream, bool isInitial )
{
   if( stream->readFlag() )
   {
      const U32 id = stream->readInt( DataBlockObjectIdBitSize ) + DataBlockObjectIdFirst;
      PrecipitationData *data = NULL;
      if( Sim::findObject( id, data ) && data )
         setDataBlock( data );
      else
         Con::errorf( "Precipitation: no datablock %u", id );
   }

   // GameBase's target: a sensor group that means nothing to precipitation.
   if( stream->readFlag() && stream->readFlag() )
      stream->readInt( 9 );

   if( stream->readFlag() )
   {
      mPercentage = readF32( stream );

      mNumColors = stream->readInt( 32 );
      if( mNumColors < 0 || mNumColors > MaxColors )
      {
         Con::errorf( "Precipitation: %d colours on the wire", mNumColors );
         return false;
      }
      for( S32 i = 0; i < mNumColors; ++i )
         readColor( stream, &mColors[i] );

      mOffsetSpeed = readF32( stream );
      mMinVelocity = readF32( stream );
      mMaxVelocity = readF32( stream );
      mMaxNumDrops = stream->readInt( 32 );
      mMaxRadius   = stream->readInt( 32 );

      if( stream->readFlag() )
      {
         mStormTicks  = readF32( stream );
         mStormTarget = readF32( stream );
         mStormTime   = readF32( stream );

         mStormRate = ( mStormTarget - mPercentage ) / ( 32.0f * mStormTime );
         mStormDir  = mPercentage > mStormTarget ? 2 : 1;

         // Catch up with the storm by the ticks it has already run. Retail
         // negates a falling storm's rate here though it is negative already.
         const F32 rate = mStormDir == 1 ? mStormRate : -mStormRate;
         mPercentage += rate * mStormTicks;
         mStormShow = true;
      }
   }

   if( stream->readFlag() )
   {
      mStormShow = stream->readInt( 8 ) != 0;
      if( !mStormShow )
         mPercentage = 0.0f;
   }

   if( stream->readFlag() )
   {
      mStormTarget = readF32( stream );
      mStormTime   = readF32( stream );
      if( mStormTime != 0.0f )
         startStorm();
   }

   if( stream->readFlag() )
   {
      mPercentage = readF32( stream );
      mNumDrops = S32( F32( mMaxNumDrops ) * mPercentage );
   }

   return true;
}

// Retail's 0x00683300.
void Precipitation::startStorm()
{
   mStormRate = ( mStormTarget - mPercentage ) / ( 32.0f * mStormTime );
   mStormDir  = mPercentage > mStormTarget ? 2 : 1;
   mStormShow = true;
}

// Retail's 0x00683360: the percentage moves at its rate per 32ms of sim
// time, and the drops that respawn follow it.
void Precipitation::updateStorm()
{
   const U32 now = Sim::getCurrentTime();
   F32 elapsed = 1.0f;
   if( mStormLastMs != 0 )
      elapsed = F32( now - mStormLastMs ) * 0.03125f;
   mStormLastMs = now;

   mPercentage += mStormRate * elapsed;

   if( ( mStormDir == 1 && mPercentage >= mStormTarget ) ||
       ( mStormDir == 2 && mPercentage <= mStormTarget ) )
   {
      mPercentage  = mStormTarget;
      mStormDir    = 0;
      mStormLastMs = 0;
   }

   mNumActive = S32( F32( mMaxNumDrops ) * mPercentage );
}

//--------------------------------------------------------------------------
// The boxes
//--------------------------------------------------------------------------

Sky *Precipitation::findSky()
{
   if( mSky.isNull() )
   {
      Vector< SceneObject* > found;
      getContainer()->findObjectList( EnvironmentObjectType, &found );
      for( U32 i = 0; i < found.size(); ++i )
         if( Sky *sky = dynamic_cast< Sky* >( found[i] ) )
         {
            mSky = sky;
            break;
         }
   }

   return static_cast< Sky* >( mSky.getObject() );
}

// Retail's 0x00682910. The box drops live in follows the camera, stretched
// the way the player is running; how fast the player is moving decides which
// box a drop respawns in, which is what outTopChance tells respawnDrop.
void Precipitation::updateBox( const Point3F &camPos, F32 *outVelZ, F32 *outTopChance )
{
   GameConnection *conn = GameConnection::getConnectionToServer();
   if( !conn )
      return;

   const F32 radius = F32( mMaxRadius );

   mWind.set( 0.0f, 0.0f, 1.0f );
   Sky *sky = findSky();
   if( sky && sky->mEffectPrecip )
   {
      const Point3F skyWind = sky->getWindVelocity();
      mWind.x = skyWind.x * mOffsetSpeed;
      mWind.y = skyWind.y * mOffsetSpeed;
   }

   mSpawnOffset.set( radius * mWind.x, radius * mWind.y, 0.0f );

   // A camera that jumps further than half the box takes its drops with it.
   mFrameOffset.zero();
   const Point3F moved = camPos - mLastCamPos;
   if( moved.len() > radius * 0.5f )
      mFrameOffset = moved;
   mLastCamPos = camPos;

   const F32 divHeight = mDataBlock->divHeightVal;
   F32 bottom = camPos.z - radius / divHeight;

   ShapeBase *control = dynamic_cast< ShapeBase* >( conn->getControlObject() );
   if( control && control->getWaterCoverage() != 0.0f )
      bottom = control->getLiquidHeight() + mDataBlock->sizeY;

   Player *player = dynamic_cast< Player* >( control );
   if( !player )
   {
      mBoxMin.set( camPos.x - radius, camPos.y - radius, bottom );
      mBoxMax.set( camPos.x + radius, camPos.y + radius, camPos.z + radius / divHeight );
      return;
   }

   Point3F vel = player->getVelocity();

   const F32 maxShift = mDataBlock->movingBoxPer;
   if( vel.x != 0.0f )
      mCamVelFracX = mClampF( vel.x * 0.025f, -maxShift, maxShift );
   if( vel.y != 0.0f )
      mCamVelFracY = mClampF( vel.y * 0.025f, -maxShift, maxShift );

   *outVelZ = vel.z;

   // Running shortens the box overhead, so more of the drops land in view.
   F32 height = radius;
   const F32 horizontal = mSqrt( vel.x * vel.x + vel.y * vel.y );
   if( horizontal > 1.0f )
      height = radius * mClampF( 40.0f / horizontal, 0.01f, 1.0f );

   F32 top = height / divHeight;
   if( top > mBoxTopDelta && top - mBoxTopDelta > 5.0f )
      top += 5.0f;
   mBoxTopDelta = top;

   if( mDataBlock->sizeBigBox != 0.0f )
   {
      mBoxMin.set( camPos.x - ( radius - radius * mCamVelFracX ),
                   camPos.y - ( radius - radius * mCamVelFracY ),
                   bottom );
      mBoxMax.set( camPos.x + radius + radius * mCamVelFracX,
                   camPos.y + radius + radius * mCamVelFracY,
                   camPos.z + top );
   }
   else
   {
      mBoxMin.set( camPos.x - radius, camPos.y - radius, camPos.z - radius / divHeight );
      mBoxMax.set( camPos.x + radius, camPos.y + radius, camPos.z + radius / divHeight );
   }

   const F32 speed = vel.len();
   const F32 roll = gRandGen.randF();
   const F32 topSpeed = mDataBlock->topBoxSpeed;
   const F32 frontSpeed = mDataBlock->frontBoxSpeed;

   if( speed < topSpeed )
      *outTopChance = 1.0f;
   else if( speed < frontSpeed )
      *outTopChance = roll < 0.25f ? 1.0f
         : 1.0f - ( speed - topSpeed ) / ( frontSpeed - topSpeed ) + vel.z / speed;
   else
      *outTopChance = roll < 0.25f ? 1.0f : vel.z / speed;

   if( speed != 0.0f )
      vel /= speed;
   else
      vel.zero();

   mSpawnOffset.set( F32( mAbs( S32( radius * mCamVelFracX ) ) ) * vel.x,
                     F32( mAbs( S32( radius * mCamVelFracY ) ) ) * vel.y,
                     radius * vel.z );

   // Spawn upwind by as far as a drop drifts on its way down.
   if( mAvgSpeed != 0.0f && mWind.z != 0.0f && ( mWind.x != 0.0f || mWind.y != 0.0f ) )
   {
      const F32 fall = radius / ( mAvgSpeed * mWind.z );
      mSpawnOffset.x += mWind.x * fall;
      mSpawnOffset.y += mWind.y * fall;
      mSpawnOffset.z = 0.0f;
   }
}

// The respawn half of retail's 0x00681a40.
void Precipitation::respawnDrop( Drop &drop, const Point3F &camPos, F32 topChance )
{
   const F32 maxRadius = F32( mMaxRadius );
   const F32 spreadX = ( 1.0f - mFabs( mCamVelFracX ) ) * maxRadius;
   const F32 spreadY = ( 1.0f - mFabs( mCamVelFracY ) ) * maxRadius;
   const F32 spread = getMin( spreadX, spreadY );

   const F32 rollBox    = gRandGen.randF();
   const F32 rollColour = gRandGen.randF();
   const F32 rollSpeed  = gRandGen.randF();

   Point2F dir;
   dir.y = gRandGen.randF() * 2.0f - 1.0f;
   dir.x = gRandGen.randF() * 2.0f - 1.0f;
   dir.normalize();

   drop.speed = -( mMaxVelocity - mMinVelocity ) * rollSpeed - mMinVelocity;

   bool top = rollBox > 1.0f - topChance;
   if( !top && gRandGen.randF() < mDataBlock->topBoxDrawPer && topChance < -0.4f )
      top = true;

   F32 z, reach;
   if( top )
   {
      // Falling fast, the drops overhead come at you faster still.
      if( topChance < -0.4f )
         drop.speed *= 2.5f * mDataBlock->frontSpeedPer;

      if( mFirstFrame )
         z = ( gRandGen.randF() * 2.0f - 1.0f ) * ( mBoxMax.z - 5.0f );
      else
         z = mBoxMax.z - 5.0f;
      drop.box = TopBox;

      F32 scale = rollColour < 0.2f ? rollSpeed : rollColour;
      scale = mClampF( scale, 0.01f, 1.0f );
      reach = spread * scale;
   }
   else if( -topChance < rollBox )
   {
      z = rollSpeed * mBoxMax.z;
      reach = spread * mDataBlock->frontRadiusPer;
      drop.speed *= mDataBlock->frontSpeedPer;
      drop.box = FrontBox;
   }
   else
   {
      z = mBoxMin.z + mDataBlock->bottomDrawHeight;
      drop.box = BottomBox;
      reach = spread * rollSpeed;
      drop.speed *= mDataBlock->bottomSpeedPer;
   }

   dir *= reach;
   drop.color = ceilIndex( F32( mNumColors ) * rollColour );

   drop.spawn.set( camPos.x + mSpawnOffset.x + dir.x,
                   camPos.y + mSpawnOffset.y + dir.y,
                   z );
   drop.pos = drop.spawn;
   drop.fadeHeight = z - camPos.z + 20.0f;
   drop.drift = mWind;

   // Retail's 0x00683520 rounds a sliver of rolls up past one, so the
   // sixteenth cell of the sheet turns up about once in 25,000 drops.
   F32 rollTex = gRandGen.randF();
   if( rollTex > 0.99995f && rollTex < 0.99999f )
      rollTex = 1.01f;
   drop.tex = ceilIndex( rollTex * 15.0f );

   drop.visible = true;
   drop.respawn = false;
}

//--------------------------------------------------------------------------
// Drawing
//--------------------------------------------------------------------------

bool Precipitation::initRender()
{
   if( mShader )
      return true;

   ShaderData *shaderData = NULL;
   if( !Sim::findObject( "PrecipitationShader", shaderData ) || !shaderData )
   {
      shaderData = new ShaderData();
      shaderData->setField( "OGLVertexShaderFile", "shaders/common/gl/precipV.glsl" );
      shaderData->setField( "OGLPixelShaderFile", "shaders/common/gl/precipP.glsl" );
      shaderData->setDataField( StringTable->insert( "samplerNames" ), "0", "diffuseMap" );
      shaderData->setField( "pixVersion", "2.0" );
      if( !shaderData->registerObject( "PrecipitationShader" ) )
      {
         delete shaderData;
         return false;
      }
      Sim::getRootGroup()->addObject( shaderData );
   }

   mShader = shaderData->getShader();
   if( !mShader )
      return false;

   mShaderConsts = mShader->allocConstBuffer();
   mModelViewSC = mShader->getShaderConstHandle( "$modelview" );

   // What retail's 0x00681a40 enables: blending over what is there and an
   // alpha test at a tenth, which the shader takes since GL has none. Depth
   // is left as retail leaves it, read and written.
   GFXStateBlockDesc desc;
   desc.setCullMode( GFXCullNone );
   desc.setZReadWrite( true, true );
   desc.setBlend( true, GFXBlendSrcAlpha, GFXBlendInvSrcAlpha );
   desc.samplersDefined = true;
   desc.samplers[0] = GFXSamplerStateDesc::getWrapLinear();
   mStateBlock = GFX->createStateBlock( desc );

   return true;
}

void Precipitation::prepRenderImage( SceneRenderState *state )
{
   if( !smPrecipitationOn )
   {
      stopSound();
      return;
   }

   if( !mStormShow )
      return;

   startSound();

   // Retail draws once a frame, and moving the drops is part of drawing them.
   if( !state->isDiffusePass() )
      return;

   ObjectRenderInst *ri = state->getRenderPass()->allocInst< ObjectRenderInst >();
   ri->renderDelegate.bind( this, &Precipitation::renderObject );
   // Retail sorts it BeginSort, ahead of the sorted translucent images, water
   // among them. Not ObjectTranslucent: two bins take that type, and drawing
   // is what moves the drops.
   ri->type = RIT_Precipitation;
   state->getRenderPass()->addInst( ri );
}

void Precipitation::renderObject( ObjectRenderInst *, SceneRenderState *state,
                                  BaseMatInstance *overrideMat )
{
   if( overrideMat || !mDataBlock )
      return;

   if( mStormDir != 0 )
      updateStorm();

   // Retail's sand (0x00682650) is a path of its own, and no stock mission
   // uses it.
   if( mDataBlock->type == PrecipitationData::Sand )
      return;

   const MatrixF &camera = state->getCameraTransform();
   Point3F right, up;
   camera.getColumn( 0, &right );
   camera.getColumn( 2, &up );

   renderDrops( state->getCameraPosition(), right, up );
   mNotRendered = false;
}

// Retail's 0x00681a40: move every drop, respawn the ones that left the box,
// and draw each as a quad facing the camera.
void Precipitation::renderDrops( const Point3F &camPos, const Point3F &right,
                                 const Point3F &up )
{
   const Point3F a = right * mDataBlock->sizeX;
   const Point3F b = up * mDataBlock->sizeY;

   F32 topChance = 1.0f;
   F32 velZ = 0.0f;
   F32 rainCatchUp = 0.0f;
   mCamVelFracX = 0.0f;
   mCamVelFracY = 0.0f;

   updateBox( camPos, &velZ, &topChance );

   // Rain falling past a falling player is drawn at least as fast as him.
   if( velZ < 0.0f && mDataBlock->type == PrecipitationData::Rain )
      rainCatchUp = ( mTime - mLastTime ) * velZ - mMinVelocity;

   const bool canDraw = initRender() && mTexture.isValid();

   // The count is the wire's maxNumDrops times its percentage, neither of
   // them checked against the array.
   const S32 numDrops = mClamp( mNumDrops, 0, (S32)MaxDrops );

   GFXVertexBufferHandle< GFXVertexPCT > vb;
   GFXVertexPCT *verts = NULL;
   U32 quads = 0;
   if( canDraw && numDrops > 0 )
   {
      vb.set( GFX, numDrops * 6, GFXBufferTypeVolatile );
      verts = vb.lock();
   }

   for( S32 i = 0; i < numDrops; ++i )
   {
      Drop &drop = mDrops[i];

      if( i >= mNumActive && !drop.visible )
         continue;

      F32 fall = drop.speed;
      if( smPrePause )
         fall = 0.0f;
      else if( rainCatchUp < 0.0f && fall > rainCatchUp )
         fall = rainCatchUp;

      drop.pos.x += mFrameOffset.x + drop.drift.x * fall;
      drop.pos.y += mFrameOffset.y + drop.drift.y * fall;
      drop.pos.z += mFrameOffset.z + fall;

      const bool inside = !drop.respawn &&
         drop.pos.x >= mBoxMin.x && drop.pos.x < mBoxMax.x &&
         drop.pos.y >= mBoxMin.y && drop.pos.y < mBoxMax.y &&
         drop.pos.z >= mBoxMin.z && drop.pos.z < mBoxMax.z;

      if( !inside )
      {
         if( i < mNumActive )
            respawnDrop( drop, camPos, topChance );
         else
         {
            drop.visible = false;
            continue;
         }
      }

      if( !drop.visible || !verts )
         continue;

      // Retail indexes with both rolls unchecked; a roll of exactly zero
      // reads the entry before the table.
      const Point2F &uv = mTexCoords[ mClamp( drop.tex, 0, TexCells - 1 ) ];
      const ColorF &tint = mColors[ mClamp( drop.color, 0, MaxColors - 1 ) ];

      // A drop spawned overhead fades in as it comes down to the camera.
      F32 alpha = 1.0f;
      if( drop.box == TopBox )
         alpha = 1.0f - ( drop.pos.z - camPos.z ) / drop.fadeHeight;

      const ColorI colour( ColorF( mClampF( tint.red, 0.0f, 1.0f ),
                                   mClampF( tint.green, 0.0f, 1.0f ),
                                   mClampF( tint.blue, 0.0f, 1.0f ),
                                   mClampF( alpha, 0.0f, 1.0f ) ) );

      // Retail's corners, in its order. A flake faces the camera; a raindrop
      // is a streak drawn up from below it along the way it drifts.
      Point3F corner[4];
      if( mDataBlock->type == PrecipitationData::Snow )
      {
         corner[0] = drop.pos + a + b;
         corner[1] = drop.pos + a - b;
         corner[2] = drop.pos - a - b;
         corner[3] = drop.pos - a + b;
      }
      else
      {
         const Point3F streak = drop.drift * ( mDataBlock->sizeY * 2.0f );
         corner[1] = drop.pos + a - b;
         corner[2] = drop.pos - a - b;
         corner[0] = corner[1] + streak;
         corner[3] = corner[2] + streak;
      }

      const Point2F cornerUV[4] =
      {
         Point2F( uv.x,         uv.y ),
         Point2F( uv.x,         uv.y + 0.25f ),
         Point2F( uv.x + 0.25f, uv.y + 0.25f ),
         Point2F( uv.x + 0.25f, uv.y ),
      };

      static const U32 sQuad[6] = { 0, 1, 2, 0, 2, 3 };
      for( U32 k = 0; k < 6; ++k )
      {
         GFXVertexPCT &v = verts[ quads * 6 + k ];
         v.point    = corner[ sQuad[k] ];
         v.texCoord = cornerUV[ sQuad[k] ];
         v.color    = colour;
      }
      ++quads;
   }

   if( verts )
   {
      vb.unlock();

      if( quads )
      {
         // The drops are in world space and the object sits at the origin, so
         // the world matrix the scene set, which carries the camera, is the
         // whole transform.
         GFX->setStateBlock( mStateBlock );
         GFX->setShader( mShader );
         MatrixF mvp = GFX->getProjectionMatrix() * GFX->getViewMatrix() * GFX->getWorldMatrix();
         mShaderConsts->setSafe( mModelViewSC, mvp );
         GFX->setShaderConstBuffer( mShaderConsts );
         GFX->setTexture( 0, mTexture );
         GFX->setVertexBuffer( vb );
         GFX->drawPrimitive( GFXTriangleList, 0, quads * 2 );
      }
   }

   mLastTime = mTime;
   mFirstFrame = false;
}
