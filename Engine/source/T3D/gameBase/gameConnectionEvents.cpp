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
#include "app/game.h"
#include "T3D/gameBase/gameConnection.h"
#include "T3D/gameBase/gameConnectionEvents.h"
#include "T3D/gameBase/dataBlocksT2.h"
#include "T3D/fx/explosion.h"
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
/// No sequence name is on the wire: retail's client finds one by looking the
/// state's own name up in the shape.
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
      image->stateSequence[i] = image->stateName[i];
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

         if( const char *shape = T2Layout::lastShapeFile() )
         {
            if( ShapeBaseData *sb = dynamic_cast< ShapeBaseData * >( mObj ) )
               sb->shapeName = StringTable->insert( shape );
            else if( ShapeBaseImageData *si = dynamic_cast< ShapeBaseImageData * >( mObj ) )
               si->shapeName = StringTable->insert( shape );
            else if( DataBlockT2 *db = dynamic_cast< DataBlockT2 * >( mObj ) )
               db->shapeName = StringTable->insert( shape );
               // An explosion names its shape first as well, and loads it by
               // the whole path rather than by the bare name.
            else if( ExplosionData *ex = dynamic_cast< ExplosionData * >( mObj ) )
               ex->dtsFileName = StringTable->insert(
                  T2Layout::resourcePath( shape, "shapes/" ) );
         }

         // HH: Added for Tribes 2 compatibility.
         if( ExplosionData *ex = dynamic_cast< ExplosionData * >( mObj ) )
         {
            U32 faceViewer;
            if( T2Layout::lastDataBlockWord( T2Layout::T2DbFaceViewer,
                                             &faceViewer ) )
               ex->faceViewer = faceViewer != 0;
         }

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

