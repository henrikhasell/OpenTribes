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

#include "platform/platform.h"
#include "core/dnet.h"
#include "core/stream/bitStream.h"
#include "console/netClassTable.h"
#include "console/consoleTypes.h"
#include "console/simBase.h"
#include "scene/pathManager.h"
#include "scene/sceneManager.h"
#include "sfx/sfxSystem.h"
#include "sfx/sfxDescription.h"
#include "sfx/audioT2.h"
#include "app/game.h"
#include "T3D/gameBase/gameConnection.h"
#include "T3D/gameBase/gameConnectionEvents.h"
#include "T3D/gameBase/dataBlocksT2.h"
#include "T3D/gameBase/processList.h"
#include "T3D/fx/explosion.h"
#include "T3D/debris.h"
#include "T3D/gameBase/t2Layout.h"
#include "T3D/shapeBase.h"
#include "T3D/fx/particleEmitter.h"

#define DebugChecksum 0xF00DBAAD


//#define DEBUG_SPEW


//--------------------------------------------------------------------------
IMPLEMENT_CO_CLIENTEVENT_V1(SimDataBlockEvent);
IMPLEMENT_CO_CLIENTEVENT_V1(Sim2DAudioEvent);
IMPLEMENT_CO_CLIENTEVENT_V1(Sim3DAudioEvent);
IMPLEMENT_CO_CLIENTEVENT_V1(SetMissionCRCEvent);

ConsoleDocClass( SimDataBlockEvent,
				"@brief Use by GameConnection to process incoming datablocks.\n\n"
				"Not intended for game development, internal use only, but does expose ClientReceivedDataBlock.\n\n "
				"@internal");

ConsoleDocClass( Sim2DAudioEvent,
				"@brief Use by GameConnection to send a 2D sound event over the network.\n\n"
				"Not intended for game development, internal use only, but does expose GameConnection::play2D.\n\n "
				"@internal");

ConsoleDocClass( Sim3DAudioEvent,
				"@brief Use by GameConnection to send a 3D sound event over the network.\n\n"
				"Not intended for game development, internal use only, but does expose GameConnection::play3D.\n\n "
				"@internal");

ConsoleDocClass( SetMissionCRCEvent,
				"@brief Use by GameConnection to send a 3D sound event over the network.\n\n"
				"Not intended for game development, internal use only, but does expose GameConnection::setMissionCRC.\n\n "
				"@internal");

//----------------------------------------------------------------------------

SimDataBlockEvent::SimDataBlockEvent(SimDataBlock* obj, U32 index, U32 total, U32 missionSequence)
{
   mObj = NULL;
   mIndex = index;
   mTotal = total;
   mMissionSequence = missionSequence;
   mProcess = false;

   if(obj)
   {
      id = obj->getId();
      AssertFatal(id >= DataBlockObjectIdFirst && id <= DataBlockObjectIdLast,
                  "Out of range event data block id... check simBase.h");
      
      #ifdef DEBUG_SPEW
      Con::printf("queuing data block: %d", mIndex);
      #endif
   }
}

SimDataBlockEvent::~SimDataBlockEvent()
{
   if( mObj )
      delete mObj;
}

#ifdef TORQUE_DEBUG_NET
const char *SimDataBlockEvent::getDebugName()
{
   SimObject *obj = Sim::findObject(id);
   static char buffer[256];
   dSprintf(buffer, sizeof(buffer), "%s [%s - %s]",
            getClassName(),
            obj ? obj->getName() : "",
            obj ? obj->getClassName() : "NONE");
   return buffer;
}
#endif // TORQUE_DEBUG_NET

void SimDataBlockEvent::notifyDelivered(NetConnection *conn, bool )
{
   // if the modified key for this event is not the current one,
   // we've already resorted and resent some blocks, so fall out.
   if(conn->isRemoved())
      return;
   
   GameConnection *gc = (GameConnection *) conn;
   if(gc->getDataBlockSequence() != mMissionSequence)
      return;

   U32 nextIndex = mIndex + DataBlockQueueCount;
   SimDataBlockGroup *g = Sim::getDataBlockGroup();

   if(mIndex == g->size() - 1)
   {
      gc->setDataBlockModifiedKey(gc->getMaxDataBlockModifiedKey());
      gc->sendConnectionMessage(GameConnection::DataBlocksDone, mMissionSequence);
   }

   if(g->size() <= nextIndex)
      return;

   SimDataBlock *blk = (SimDataBlock *) (*g)[nextIndex];
   gc->postNetEvent(new SimDataBlockEvent(blk, nextIndex, g->size(), mMissionSequence));
}

void SimDataBlockEvent::pack(NetConnection *conn, BitStream *bstream)
{
   SimDataBlock* obj;
   Sim::findObject(id,obj);
   GameConnection *gc = (GameConnection *) conn;
   if(bstream->writeFlag(gc->getDataBlockModifiedKey() < obj->getModifiedKey()))
   {
      if(obj->getModifiedKey() > gc->getMaxDataBlockModifiedKey())
         gc->setMaxDataBlockModifiedKey(obj->getModifiedKey());

      AssertFatal(obj,
                  "SimDataBlockEvent:: Data blocks cannot be deleted");
      bstream->writeInt(id - DataBlockObjectIdFirst,DataBlockObjectIdBitSize);

      S32 classId = obj->getClassId(conn->getNetClassGroup());
      bstream->writeClassId(classId, NetClassTypeDataBlock, conn->getNetClassGroup());
      bstream->writeInt(mIndex, DataBlockObjectIdBitSize);
      bstream->writeInt(mTotal, DataBlockObjectIdBitSize + 1);
      obj->packData(bstream, classId);
#ifdef TORQUE_DEBUG_NET
      bstream->writeInt(classId ^ DebugChecksum, 32);
#endif
   }
}

// HH: Added for Tribes 2 compatibility.
/// A state carries one string, its name, and no sequence. Retail's client has
/// none either: `stateSequence` is a field only a script fills (0x005f523d),
/// zeroed at 0x005f429b and never written by unpackData, so the lookup its
/// preload makes at 0x005f4ad1 finds nothing and no mounted image on a client
/// is animated. Leave it empty here for the same reason.
static void readT2ImageStates( ShapeBaseImageData *image )
{
   const Vector< T2Layout::ImageState > &states = T2Layout::lastImageStates();

   U32 count = states.size();
   if( count > ShapeBaseImageData::MaxStates )
   {
      Con::warnf( "T2 image: a datablock carries %d states and this engine "
                  "has room for %d; the rest are being dropped.", count,
                  ShapeBaseImageData::MaxStates );
      count = ShapeBaseImageData::MaxStates;
   }

   for( U32 i = 0; i < count; i++ )
   {
      const T2Layout::ImageState &state = states[i];
      if( state.name.isEmpty() )
         continue;

      image->stateName[i] = StringTable->insert( state.name.c_str() );
      image->stateTimeoutValue[i] = state.hasTimeout ? state.timeout : 0.0f;
      image->stateWaitForTimeout[i] = state.waitForTimeout;
      image->stateFire[i] = state.fire;

            // The repair gun's Deactivate is the only state in the game that clears
            // this.
      image->stateDirection[i] = state.direction;

            // The chaingun's Fire is the only state in the game that sets this, and
            // retail has it ejecting a shell and carrying a flash sequence alike.
      image->stateEjectShell[i] = state.ejectShell;
      image->stateSequenceRandomFlash[i] = state.ejectShell;
      if( state.sound >= 0 )
         *reinterpret_cast< U32 * >( &image->stateSound[i] ) =
            ( ( U32( state.sound ) + DataBlockObjectIdFirst ) << 1 ) | 0x1;

      image->stateLoaded[i] =
         ( ShapeBaseImageData::StateData::LoadedState ) state.loaded;
      image->stateSpin[i] =
         ( ShapeBaseImageData::StateData::SpinState ) state.spin;
      image->stateRecoil[i] =
         ( ShapeBaseImageData::StateData::RecoilState ) state.recoil;
   }

   // The ways out are states by number on the wire and states by name here,
   // so they wait until every state this image has is named.
   for( U32 i = 0; i < count; i++ )
   {
      const T2Layout::ImageState &state = states[i];

      const char *to[ T2Layout::ImageState::TransitionCount ];
      for( U32 t = 0; t < T2Layout::ImageState::TransitionCount; t++ )
      {
         const S32 which = state.transition[t];
         to[t] = which >= 0 && U32(which) < count ? image->stateName[which]
                                                  : NULL;
      }

      typedef T2Layout::ImageState S;
      image->stateTransitionNotLoaded[i]   = to[ S::NotLoaded ];
      image->stateTransitionLoaded[i]      = to[ S::Loaded ];
      image->stateTransitionNoAmmo[i]      = to[ S::NoAmmo ];
      image->stateTransitionAmmo[i]        = to[ S::Ammo ];
      image->stateTransitionNoTarget[i]    = to[ S::NoTarget ];
      image->stateTransitionTarget[i]      = to[ S::Target ];
      image->stateTransitionNotWet[i]      = to[ S::NotWet ];
      image->stateTransitionWet[i]         = to[ S::Wet ];
      image->stateTransitionTriggerUp[i]   = to[ S::TriggerUp ];
      image->stateTransitionTriggerDown[i] = to[ S::TriggerDown ];
      image->stateTransitionTimeout[i]     = to[ S::Timeout ];
   }
}


// HH: Added for Tribes 2 compatibility.
static void readT2Explosion( ExplosionData *ex )
{
   U32 word;
   if( T2Layout::lastDataBlockWord( T2Layout::T2DbFaceViewer, &word ) )
      ex->faceViewer = word != 0;
   if( T2Layout::lastDataBlockWord( T2Layout::T2DbPlaySpeed, &word ) )
      ex->playSpeed = F32( word ) * 0.05f;
   if( T2Layout::lastDataBlockWord( T2Layout::T2DbDelayMS, &word ) )
      ex->delayMS = word * TickMs;
   if( T2Layout::lastDataBlockWord( T2Layout::T2DbDelayVariance, &word ) )
      ex->delayVariance = word * TickMs;
   if( T2Layout::lastDataBlockWord( T2Layout::T2DbExplosionLifetime, &word ) )
      ex->lifetimeMS = word * TickMs;
   if( T2Layout::lastDataBlockWord( T2Layout::T2DbLifetimeVariance, &word ) )
      ex->lifetimeVariance = word * TickMs;
   if( T2Layout::lastDataBlockWord( T2Layout::T2DbExplosionOffset, &word ) )
      dMemcpy( &ex->offset, &word, sizeof( ex->offset ) );

   U32 x, y, z;
   if( T2Layout::lastDataBlockWord( T2Layout::T2DbExplosionScaleX, &x ) &&
       T2Layout::lastDataBlockWord( T2Layout::T2DbExplosionScaleY, &y ) &&
       T2Layout::lastDataBlockWord( T2Layout::T2DbExplosionScaleZ, &z ) )
      ex->explosionScale.set( F32( x ) * 0.01f, F32( y ) * 0.01f,
                              F32( z ) * 0.01f );

   const Vector<U32> &times = T2Layout::lastDataBlockValues(
      T2Layout::T2DbExplosionTime );
   const Vector<U32> &sizes = T2Layout::lastDataBlockValues(
      T2Layout::T2DbExplosionSize );

   for( U32 i = 0; i < times.size() && i < ExplosionData::EC_NUM_TIME_KEYS;
        i++ )
      ex->times[i] = F32( times[i] ) / 255.0f;

   for( U32 i = 0; i * 3 + 2 < sizes.size() &&
                   i < ExplosionData::EC_NUM_TIME_KEYS; i++ )
      ex->sizes[i].set( F32( sizes[i * 3 + 0] ) * 0.01f,
                        F32( sizes[i * 3 + 1] ) * 0.01f,
                        F32( sizes[i * 3 + 2] ) * 0.01f );

   if( T2Layout::lastDataBlockWord( T2Layout::T2DbExplosionSound, &word ) )
      *reinterpret_cast< U32 * >( &ex->soundProfile ) =
         ( ( word + DataBlockObjectIdFirst ) << 1 ) | 0x1;

   if( T2Layout::lastDataBlockWord( T2Layout::T2DbExplosionParticleEmitter, &word ) )
      ex->particleEmitterId = word + DataBlockObjectIdFirst;
   if( T2Layout::lastDataBlockWord( T2Layout::T2DbExplosionParticleDensity, &word ) )
      ex->particleDensity = word;

   F32 value;
   if( T2Layout::lastDataBlockField( T2Layout::T2DbExplosionParticleRadius, &value ) )
      ex->particleRadius = value;

   if( T2Layout::lastDataBlockWord( T2Layout::T2DbExplosionDebrisThetaMin, &word ) )
      ex->debrisThetaMin = F32( word );
   if( T2Layout::lastDataBlockWord( T2Layout::T2DbExplosionDebrisThetaMax, &word ) )
      ex->debrisThetaMax = F32( word );
   if( T2Layout::lastDataBlockWord( T2Layout::T2DbExplosionDebrisPhiMin, &word ) )
      ex->debrisPhiMin = F32( word );
   if( T2Layout::lastDataBlockWord( T2Layout::T2DbExplosionDebrisPhiMax, &word ) )
      ex->debrisPhiMax = F32( word );
   if( T2Layout::lastDataBlockWord( T2Layout::T2DbExplosionDebrisNum, &word ) )
      ex->debrisNum = word;
   if( T2Layout::lastDataBlockWord( T2Layout::T2DbExplosionDebrisNumVariance, &word ) )
      ex->debrisNumVariance = word;
   if( T2Layout::lastDataBlockWord( T2Layout::T2DbExplosionDebrisVelocity, &word ) )
      ex->debrisVelocity = F32( word ) * 0.1f;
   if( T2Layout::lastDataBlockWord( T2Layout::T2DbExplosionDebrisVelocityVariance, &word ) )
      ex->debrisVelocityVariance = F32( word ) * 0.1f;

   if( T2Layout::lastDataBlockWord( T2Layout::T2DbExplosionShakeCamera, &word ) )
      ex->shakeCamera = word != 0;

   if( T2Layout::lastDataBlockField( T2Layout::T2DbExplosionShakeFreqX, &value ) )
      ex->camShakeFreq.x = value;
   if( T2Layout::lastDataBlockField( T2Layout::T2DbExplosionShakeFreqY, &value ) )
      ex->camShakeFreq.y = value;
   if( T2Layout::lastDataBlockField( T2Layout::T2DbExplosionShakeFreqZ, &value ) )
      ex->camShakeFreq.z = value;
   if( T2Layout::lastDataBlockField( T2Layout::T2DbExplosionShakeAmpX, &value ) )
      ex->camShakeAmp.x = value;
   if( T2Layout::lastDataBlockField( T2Layout::T2DbExplosionShakeAmpY, &value ) )
      ex->camShakeAmp.y = value;
   if( T2Layout::lastDataBlockField( T2Layout::T2DbExplosionShakeAmpZ, &value ) )
      ex->camShakeAmp.z = value;
   if( T2Layout::lastDataBlockField( T2Layout::T2DbExplosionShakeDuration, &value ) )
      ex->camShakeDuration = value;
   if( T2Layout::lastDataBlockField( T2Layout::T2DbExplosionShakeRadius, &value ) )
      ex->camShakeRadius = value;
   if( T2Layout::lastDataBlockField( T2Layout::T2DbExplosionShakeFalloff, &value ) )
      ex->camShakeFalloff = value;

   // ExplosionData::onAdd turns these ids into the datablocks they name.
   if( T2Layout::lastDataBlockWord( T2Layout::T2DbExplosionDebris, &word ) )
      ex->debrisIDList[0] = word + DataBlockObjectIdFirst;

   static const U32 sEmitters[ExplosionData::EC_NUM_EMITTERS] =
   {
      T2Layout::T2DbExplosionEmitter0, T2Layout::T2DbExplosionEmitter1,
      T2Layout::T2DbExplosionEmitter2, T2Layout::T2DbExplosionEmitter3,
   };
   for( U32 i = 0; i < ExplosionData::EC_NUM_EMITTERS; i++ )
      if( T2Layout::lastDataBlockWord( sEmitters[i], &word ) )
         ex->emitterIDList[i] = word + DataBlockObjectIdFirst;

   static const U32 sSubs[ExplosionData::EC_MAX_SUB_EXPLOSIONS] =
   {
      T2Layout::T2DbSubExplosion0, T2Layout::T2DbSubExplosion1,
      T2Layout::T2DbSubExplosion2, T2Layout::T2DbSubExplosion3,
      T2Layout::T2DbSubExplosion4,
   };
   for( U32 i = 0; i < ExplosionData::EC_MAX_SUB_EXPLOSIONS; i++ )
      if( T2Layout::lastDataBlockWord( sSubs[i], &word ) )
         ex->explosionIDList[i] = word + DataBlockObjectIdFirst;
}

// HH: Added for Tribes 2 compatibility.
static void readT2Debris( DebrisData *db )
{
   U32 word;
   F32 value;

   if( T2Layout::lastDataBlockField( T2Layout::T2DbDebrisElasticity, &value ) )
      db->elasticity = value;
   if( T2Layout::lastDataBlockField( T2Layout::T2DbDebrisFriction, &value ) )
      db->friction = value;
   if( T2Layout::lastDataBlockWord( T2Layout::T2DbDebrisNumBounces, &word ) )
      db->numBounces = word;
   if( T2Layout::lastDataBlockWord( T2Layout::T2DbDebrisBounceVariance, &word ) )
      db->bounceVariance = word;
   if( T2Layout::lastDataBlockField( T2Layout::T2DbDebrisMinSpinSpeed, &value ) )
      db->minSpinSpeed = value;
   if( T2Layout::lastDataBlockField( T2Layout::T2DbDebrisMaxSpinSpeed, &value ) )
      db->maxSpinSpeed = value;
   if( T2Layout::lastDataBlockWord( T2Layout::T2DbDebrisExplodeOnMaxBounce, &word ) )
      db->explodeOnMaxBounce = word != 0;
   if( T2Layout::lastDataBlockWord( T2Layout::T2DbDebrisStaticOnMaxBounce, &word ) )
      db->staticOnMaxBounce = word != 0;
   if( T2Layout::lastDataBlockWord( T2Layout::T2DbDebrisSnapOnMaxBounce, &word ) )
      db->snapOnMaxBounce = word != 0;
   if( T2Layout::lastDataBlockField( T2Layout::T2DbDebrisLifetime, &value ) )
      db->lifetime = value;
   if( T2Layout::lastDataBlockField( T2Layout::T2DbDebrisLifetimeVariance, &value ) )
      db->lifetimeVariance = value;
   if( T2Layout::lastDataBlockField( T2Layout::T2DbDebrisVelocity, &value ) )
      db->velocity = value;
   if( T2Layout::lastDataBlockField( T2Layout::T2DbDebrisVelocityVariance, &value ) )
      db->velocityVariance = value;
   if( T2Layout::lastDataBlockWord( T2Layout::T2DbDebrisFade, &word ) )
      db->fade = word != 0;
   if( T2Layout::lastDataBlockWord( T2Layout::T2DbDebrisUseRadiusMass, &word ) )
      db->useRadiusMass = word != 0;
   if( T2Layout::lastDataBlockField( T2Layout::T2DbDebrisBaseRadius, &value ) )
      db->baseRadius = value;
   if( T2Layout::lastDataBlockField( T2Layout::T2DbDebrisGravModifier, &value ) )
      db->gravModifier = value;
   if( T2Layout::lastDataBlockField( T2Layout::T2DbDebrisTerminalVelocity, &value ) )
      db->terminalVelocity = value;
   if( T2Layout::lastDataBlockWord( T2Layout::T2DbDebrisIgnoreWater, &word ) )
      db->ignoreWater = word != 0;

   // The texture, then the shape.
   const Vector< String > &strings = T2Layout::lastStrings();
   if( strings.size() > 0 && strings[0].isNotEmpty() )
      db->textureName = StringTable->insert( strings[0].c_str() );
   if( strings.size() > 1 && strings[1].isNotEmpty() )
      db->shapeName = StringTable->insert(
         T2Layout::resourcePath( strings[1].c_str(), "shapes/" ) );

   // DebrisData::onAdd turns these ids into the datablocks they name.
   if( T2Layout::lastDataBlockWord( T2Layout::T2DbDebrisEmitter0, &word ) )
      db->emitterIDList[0] = word + DataBlockObjectIdFirst;
   if( T2Layout::lastDataBlockWord( T2Layout::T2DbDebrisEmitter1, &word ) )
      db->emitterIDList[1] = word + DataBlockObjectIdFirst;
   if( T2Layout::lastDataBlockWord( T2Layout::T2DbDebrisExplosion, &word ) )
      db->explosionId = word + DataBlockObjectIdFirst;
}

void SimDataBlockEvent::unpack(NetConnection *cptr, BitStream *bstream)
{
   if(bstream->readFlag())
   {
      mProcess = true;
      id = bstream->readInt(DataBlockObjectIdBitSize) + DataBlockObjectIdFirst;
      S32 classId = bstream->readClassId(NetClassTypeDataBlock, cptr->getNetClassGroup());
      mIndex = bstream->readInt(DataBlockObjectIdBitSize);
      mTotal = bstream->readInt(DataBlockObjectIdBitSize + 1);
      
      SimObject* ptr;
      if( Sim::findObject( id, ptr ) )
      {
         // An object with the given ID already exists.  Make sure it has the right class.
         
         AbstractClassRep* classRep = AbstractClassRep::findClassRep( cptr->getNetClassGroup(), NetClassTypeDataBlock, classId );
         if( classRep && dStrcmp( classRep->getClassName(), ptr->getClassName() ) != 0 )
         {
            Con::warnf( "A '%s' datablock with id: %d already existed. "
                        "Clobbering it with new '%s' datablock from server.",
                        ptr->getClassName(), id, classRep->getClassName() );
            ptr->deleteObject();
            ptr = NULL;
         }
      }
      
      if( !ptr )
         ptr = ( SimObject* ) ConsoleObject::create( cptr->getNetClassGroup(), NetClassTypeDataBlock, classId );
         
      mObj = dynamic_cast< SimDataBlock* >( ptr );
      if( mObj != NULL )
      {
         #ifdef DEBUG_SPEW
         Con::printf(" - SimDataBlockEvent: unpacking event of type: %s", mObj->getClassName());
         #endif
         
         if( !mObj->unpackData( bstream, classId ) )
         {
            cptr->setLastError( "Could not read a %s datablock, %d of %d.",
               mObj->getClassName(), mIndex, mTotal );
            return;
         }

         // A ShapeBaseData names its own shape as it unpacks.
         if( const char *shape = T2Layout::lastShapeFile() )
         {
            if( ShapeBaseImageData *si = dynamic_cast< ShapeBaseImageData * >( mObj ) )
               si->shapeName = StringTable->insert( shape );
            else if( DataBlockT2 *db = dynamic_cast< DataBlockT2 * >( mObj ) )
               db->shapeName = StringTable->insert( shape );
            else if( ExplosionData *ex = dynamic_cast< ExplosionData * >( mObj ) )
               ex->dtsFileName = StringTable->insert(
                  T2Layout::resourcePath( shape, "shapes/" ) );
         }

         // HH: Added for Tribes 2 compatibility.
         if( ExplosionData *ex = dynamic_cast< ExplosionData * >( mObj ) )
            readT2Explosion( ex );
         else if( DebrisData *db = dynamic_cast< DebrisData * >( mObj ) )
            readT2Debris( db );

         if( AudioProfile *ap = dynamic_cast< AudioProfile * >( mObj ) )
            ap->readT2Fields();
         else if( AudioDescription *ad = dynamic_cast< AudioDescription * >( mObj ) )
            ad->readT2Fields();

            // HH: Added for Tribes 2 compatibility.
            // A weapon wants mount node 0 and a pack node 1.
         U32 mountPoint;
         if( ShapeBaseImageData *si = dynamic_cast< ShapeBaseImageData * >( mObj ) )
         {
            if( T2Layout::lastDataBlockWord( T2Layout::T2DbImageMountPoint,
                                             &mountPoint ) )
            {
               if( mountPoint < SceneObject::NumMountPoints )
                  si->mountPoint = mountPoint;
               else
                  Con::warnf( "T2 image: datablock %d reads mount point %u, "
                              "which is past the %d this engine has -- it is "
                              "being left at 0.", id, mountPoint,
                              SceneObject::NumMountPoints );
            }

            // HH: Added for Tribes 2 compatibility.
            U32 cloakable;
            if( T2Layout::lastDataBlockWord( T2Layout::T2DbImageCloakable,
                                             &cloakable ) )
               si->cloakable = cloakable != 0;
         }

               // HH: Added for Tribes 2 compatibility.
               // The body carries an image's states and nothing was reading them.
         if( ShapeBaseImageData *si = dynamic_cast< ShapeBaseImageData * >( mObj ) )
            readT2ImageStates( si );

         // HH: Added for Tribes 2 compatibility.
         if( ParticleEmitterData *pe = dynamic_cast< ParticleEmitterData * >( mObj ) )
         {
            pe->dataBlockIds = T2Layout::lastDataBlockIds();
            pe->particleString = NULL;
         }
      }
      else
      {
         #ifdef DEBUG_SPEW
         Con::printf(" - SimDataBlockEvent: INVALID PACKET!  Could not create class with classID: %d", classId);
         #endif
         
         delete ptr;
         cptr->setLastError("No datablock class %d (%s), %d of %d.", classId,
            classId >= 0 && classId < (S32)NetClassTable::DataBlockCount
               ? NetClassTable::DataBlocks[classId] : "out of range",
            mIndex, mTotal);
      }

#ifdef TORQUE_DEBUG_NET
      U32 checksum = bstream->readInt(32);
      AssertISV( (checksum ^ DebugChecksum) == (U32)classId,
         avar("unpack did not match pack for event of class %s.",
            mObj->getClassName()) );
#endif

   }
}

void SimDataBlockEvent::write(NetConnection *cptr, BitStream *bstream)
{
   if(bstream->writeFlag(mProcess))
   {
      bstream->writeInt(id - DataBlockObjectIdFirst,DataBlockObjectIdBitSize);
      S32 classId = mObj->getClassId(cptr->getNetClassGroup());
      bstream->writeClassId(classId, NetClassTypeDataBlock, cptr->getNetClassGroup());
      bstream->writeInt(mIndex, DataBlockObjectIdBitSize);
      bstream->writeInt(mTotal, DataBlockObjectIdBitSize + 1);
      mObj->packData(bstream, classId);
   }
}

void SimDataBlockEvent::process(NetConnection *cptr)
{
   if(mProcess)
   {
      // HH: Modified so that the loading bar moves as the datablocks arrive.
      if(Con::isFunction("ClientReceivedDataBlock"))
         Con::executef("ClientReceivedDataBlock", Con::getIntArg(mIndex),
            Con::getIntArg(mTotal));

      String &errorBuffer = NetConnection::getErrorBuffer();
                     
      // Register the datablock object if this is a new DB
      // and not for a modified datablock event.
         
      if( !mObj->isProperlyAdded() )
      {
         // This is a fresh datablock object.
         // Perform preload on datablock and register
         // the object.

         GameConnection* conn = dynamic_cast< GameConnection* >( cptr );
         if( conn )
            conn->preloadDataBlock( mObj );
         
         if( mObj->registerObject(id) )
         {
                  // HH: Added for Tribes 2 compatibility.
                  // onAdd clears every sequence index, so preload has to run after it.
            if( dynamic_cast< ShapeBaseImageData * >( mObj ) )
               mObj->preload( false, NetConnection::getErrorBuffer() );

            cptr->addObject( mObj );
            mObj = NULL;
         }
      }
      else
      {
         // This is an update to an existing datablock.  Preload
         // to finish this.

         mObj->preload( false, errorBuffer );
         mObj = NULL;
      }
   }
}


//----------------------------------------------------------------------------


Sim2DAudioEvent::Sim2DAudioEvent(SFXProfile *profile)
{
   mProfile = profile;
}

void Sim2DAudioEvent::pack(NetConnection *, BitStream *bstream)
{
   bstream->writeInt( mProfile->getId() - DataBlockObjectIdFirst, DataBlockObjectIdBitSize);
}

void Sim2DAudioEvent::write(NetConnection *, BitStream *bstream)
{
   bstream->writeInt( mProfile->getId() - DataBlockObjectIdFirst, DataBlockObjectIdBitSize);
}

void Sim2DAudioEvent::unpack(NetConnection *, BitStream *bstream)
{
   SimObjectId id = bstream->readInt(DataBlockObjectIdBitSize) + DataBlockObjectIdFirst;
   Sim::findObject(id, mProfile);
}

void Sim2DAudioEvent::process(NetConnection *)
{
   if (mProfile)
      SFX->playOnce( mProfile );
}

//----------------------------------------------------------------------------

static F32 SoundPosAccuracy = 0.5;
static S32 SoundRotBits = 8;

Sim3DAudioEvent::Sim3DAudioEvent(SFXProfile *profile,const MatrixF* mat)
{
   mProfile = profile;
   if (mat)
      mTransform = *mat;
}

void Sim3DAudioEvent::pack(NetConnection *con, BitStream *bstream)
{
   bstream->writeInt(mProfile->getId() - DataBlockObjectIdFirst, DataBlockObjectIdBitSize);

   // If the sound has cone parameters, the orientation is
   // transmitted as well.
   SFXDescription* ad = mProfile->getDescription();
   if ( bstream->writeFlag( ad->mConeInsideAngle || ad->mConeOutsideAngle ) ) 
   {
      QuatF q(mTransform);
      q.normalize();

      // LH - we can get a valid quat that's very slightly over 1 in and so
      // this fails (barely) check against zero.  So use some error-
      AssertFatal((1.0 - ((q.x * q.x) + (q.y * q.y) + (q.z * q.z))) >= (0.0 - 0.001),
                  "QuatF::normalize() is broken in Sim3DAudioEvent");

      bstream->writeFloat(q.x,SoundRotBits);
      bstream->writeFloat(q.y,SoundRotBits);
      bstream->writeFloat(q.z,SoundRotBits);
      bstream->writeFlag(q.w < 0.0);
   }

   Point3F pos;
   mTransform.getColumn(3,&pos);
   bstream->writeCompressedPoint(pos,SoundPosAccuracy);
}

void Sim3DAudioEvent::write(NetConnection *con, BitStream *bstream)
{
   // Just do the normal pack...
   pack(con,bstream);
}

void Sim3DAudioEvent::unpack(NetConnection *con, BitStream *bstream)
{
   SimObjectId id = bstream->readInt(DataBlockObjectIdBitSize) + DataBlockObjectIdFirst;
   Sim::findObject(id, mProfile);

   if (bstream->readFlag()) {
      QuatF q;
      q.x = bstream->readFloat(SoundRotBits);
      q.y = bstream->readFloat(SoundRotBits);
      q.z = bstream->readFloat(SoundRotBits);
      F32 value = ((q.x * q.x) + (q.y * q.y) + (q.z * q.z));
// #ifdef __linux
      // Hmm, this should never happen, but it does...
      if ( value > 1.f )
         value = 1.f;
// #endif
      q.w = mSqrt(1.f - value);
      if (bstream->readFlag())
         q.w = -q.w;
      q.setMatrix(&mTransform);
   }
   else
      mTransform.identity();

   Point3F pos;
   bstream->readCompressedPoint(&pos,SoundPosAccuracy);
   mTransform.setColumn(3, pos);
}

void Sim3DAudioEvent::process(NetConnection *)
{
   if (mProfile)
      SFX->playOnce( mProfile, &mTransform );
}

