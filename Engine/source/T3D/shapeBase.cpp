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
#include "T3D/shapeBase.h"

#include "core/dnet.h"
#include "sfx/sfxSystem.h"
#include "sfx/sfxSource.h"
#include "sfx/sfxTrack.h"
#include "sfx/sfxDescription.h"
#include "sfx/sfxProfile.h"
#include "T3D/sfx/sfx3DWorld.h"
#include "T3D/gameBase/gameConnection.h"
#include "T3D/gameBase/t2Layout.h"
#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "core/stream/bitStream.h"
#include "ts/tsPartInstance.h"
#include "ts/tsShapeInstance.h"
#include "ts/tsMaterialList.h"
#include "scene/sceneManager.h"
#include "scene/sceneRenderState.h"
#include "scene/sceneObjectLightingPlugin.h"
#include "T3D/fx/explosion.h"
#include "T3D/fx/particleEmitter.h"
#include "T3D/fx/cameraFXMgr.h"
#include "environment/waterBlock.h"
#include "T3D/debris.h"
#include "T3D/physicalZone.h"
#include "T3D/containerQuery.h"
#include "math/mathUtils.h"
#include "math/mMatrix.h"
#include "math/mTransform.h"
#include "math/mRandom.h"
#include "platform/profiler.h"
#include "gfx/gfxCubemap.h"
#include "gfx/gfxDrawUtil.h"
#include "gfx/gfxTransformSaver.h"
#include "renderInstance/renderPassManager.h"
#include "collision/earlyOutPolyList.h"
#include "core/resourceManager.h"
#include "scene/reflectionManager.h"
#include "gfx/sim/cubemapData.h"
#include "materials/materialManager.h"
// HH: Added for Tribes 2 compatibility.
#include "materials/materialDefinition.h"
#include "gui/3d/guiTSControl.h"
#include "materials/materialFeatureTypes.h"
#include "renderInstance/renderOcclusionMgr.h"
#include "core/stream/fileStream.h"

IMPLEMENT_CO_DATABLOCK_V1(ShapeBaseData);

ConsoleDocClass( ShapeBaseData,
   "@brief Defines properties for a ShapeBase object.\n\n"
   "@see ShapeBase\n"
   "@ingroup gameObjects\n"
);

IMPLEMENT_CALLBACK( ShapeBaseData, onEnabled, void, ( ShapeBase* obj, const char* lastState ), ( obj, lastState ),
   "@brief Called when the object damage state changes to Enabled.\n\n"
   "@param obj The ShapeBase object\n"
   "@param lastState The previous damage state\n" );

IMPLEMENT_CALLBACK( ShapeBaseData, onDisabled, void, ( ShapeBase* obj, const char* lastState ), ( obj, lastState ),
   "@brief Called when the object damage state changes to Disabled.\n\n"
   "@param obj The ShapeBase object\n"
   "@param lastState The previous damage state\n" );

IMPLEMENT_CALLBACK( ShapeBaseData, onDestroyed, void, ( ShapeBase* obj, const char* lastState ), ( obj, lastState ),
   "@brief Called when the object damage state changes to Destroyed.\n\n"
   "@param obj The ShapeBase object\n"
   "@param lastState The previous damage state\n" );

IMPLEMENT_CALLBACK( ShapeBaseData, onImpact, void, ( ShapeBase* obj, SceneObject *collObj, VectorF vec, F32 len ), ( obj, collObj, vec, len ),
   "@brief Called when we collide with another object beyond some impact speed.\n\n"
   "The Player class makes use of this callback when a collision speed is more than PlayerData::minImpactSpeed.\n"
   "@param obj The ShapeBase object\n"
   "@param collObj The object we collided with\n"
   "@param vec Collision impact vector\n"
   "@param len Length of the impact vector\n" );

IMPLEMENT_CALLBACK( ShapeBaseData, onCollision, void, ( ShapeBase* obj, SceneObject *collObj, VectorF vec, F32 len ), ( obj, collObj, vec, len ),
   "@brief Called when we collide with another object.\n\n"
   "@param obj The ShapeBase object\n"
   "@param collObj The object we collided with\n"
   "@param vec Collision impact vector\n"
   "@param len Length of the impact vector\n" );

IMPLEMENT_CALLBACK( ShapeBaseData, onDamage, void, ( ShapeBase* obj, F32 delta ), ( obj, delta ),
   "@brief Called when the object is damaged.\n\n"
   "@param obj The ShapeBase object\n"
   "@param obj The ShapeBase object\n"
   "@param delta The amount of damage received." );

IMPLEMENT_CALLBACK( ShapeBaseData, onTrigger, void, ( ShapeBase* obj, S32 index, bool state ), ( obj, index, state ),
   "@brief Called when a move trigger input changes state.\n\n"
   "@param obj The ShapeBase object\n"
   "@param index Index of the trigger that changed\n"
   "@param state New state of the trigger\n" );

IMPLEMENT_CALLBACK( ShapeBaseData, onEndSequence, void, ( ShapeBase* obj, S32 slot ), ( obj, slot ),
   "@brief Called when a thread playing a non-cyclic sequence reaches the end of the "
   "sequence.\n\n"
   "@param obj The ShapeBase object\n"
   "@param slot Thread slot that finished playing\n" );

IMPLEMENT_CALLBACK( ShapeBaseData, onForceUncloak, void, ( ShapeBase* obj, const char* reason ), ( obj, reason ),
   "@brief Called when the object is forced to uncloak.\n\n"
   "@param obj The ShapeBase object\n"
   "@param reason String describing why the object was uncloaked\n" );

//----------------------------------------------------------------------------
// Timeout for non-looping sounds on a channel
static SimTime sAudioTimeout = 500;
F32  ShapeBase::sWhiteoutDec = 0.007f;
F32  ShapeBase::sDamageFlashDec = 0.02f;
F32  ShapeBase::sFullCorrectionDistance = 0.5f;
F32  ShapeBase::sCloakSpeed = 0.5;
U32  ShapeBase::sLastRenderFrame = 0;

static const char *sDamageStateName[] =
{
   // Index by enum ShapeBase::DamageState
   "Enabled",
   "Disabled",
   "Destroyed"
};


//----------------------------------------------------------------------------

ShapeBaseData::ShapeBaseData()
 : shadowEnable( false ),
   shadowSize( 128 ),
   shadowMaxVisibleDistance( 80.0f ),
   shadowProjectionDistance( 10.0f ),
   shadowSphereAdjust( 1.0f ),
   shapeName( StringTable->insert("") ),
   cloakTexName( StringTable->insert("") ),
   mass( 1.0f ),
   drag( 0.0f ),
   density( 1.0f ),
   maxEnergy( 0.0f ),
   maxDamage( 1.0f ),
   disabledLevel( 1.0f ),
   destroyedLevel( 1.0f ),
   repairRate( 0.0033f ),
   eyeNode( -1 ),
   earNode( -1 ),
   cameraNode( -1 ),
   damageSequence( -1 ),
   hulkSequence( -1 ),
   cameraMaxDist( 0.0f ),
   cameraMinDist( 0.2f ),
   // HH: Modified for Tribes 2 compatibility.
   // Retail's constructor (0x5e3c30) gives 90, and most vehicle datablocks
   // leave it unset.
   cameraDefaultFov( 90.0f ),
   cameraMinFov( 5.0f ),
   cameraMaxFov( 120.f ),
   isInvincible( false ),
   renderWhenDestroyed( true ),
   debris( NULL ),
   // HH: Modified for Tribes 2 compatibility.
   // -1 is none, as in retail's constructor (0x5e3c30), since 0 is a real
   // datablock id.
   debrisID( -1 ),
   debrisShapeName( StringTable->insert("") ),
   explosion( NULL ),
   explosionID( -1 ),
   underwaterExplosion( NULL ),
   underwaterExplosionID( -1 ),
   cmdCategory( StringTable->insert("") ),
   cmdMiniIconName( StringTable->insert("") ),
   cmdIcon( NULL ),
   cmdIconID( -1 ),
   firstPersonOnly( false ),
   useEyePoint( false ),
   cubeDescId( 0 ),
   reflectorDesc( NULL ),
   observeThroughObject( false ),
   computeCRC( false ),
   inheritEnergyFromMount( false ),
   mCRC( 0 ),
   debrisDetail( -1 )
{      
   dMemset( mountPointNode, -1, sizeof( S32 ) * SceneObject::NumMountPoints );

   // HH: Added for Tribes 2 compatibility.
   // Tribes 2's ShapeBaseData constructor (0x005e3e5c) sets these.
   shieldEffectLifetimeMS = 300;
   shieldEffectScale.set( 1.0f, 1.0f, 1.0f );
}

struct ShapeBaseDataProto
{
   F32 mass;
   F32 drag;
   F32 density;
   F32 maxEnergy;
   F32 cameraMaxDist;
   F32 cameraMinDist;
   F32 cameraDefaultFov;
   F32 cameraMinFov;    
   F32 cameraMaxFov;    


   ShapeBaseDataProto()
   {
      mass = 1;
      drag = 0;
      density = 1;
      maxEnergy = 0;
      cameraMaxDist = 0;
      cameraMinDist = 0.2f;
      // HH: Modified for Tribes 2 compatibility.
      // A field the server leaves out takes this, and retail's prototype is
      // a default-constructed ShapeBaseData.
      cameraDefaultFov = 90.f;
      cameraMinFov = 5.0f;
      cameraMaxFov = 120.f;
   }
};

static ShapeBaseDataProto gShapeBaseDataProto;

ShapeBaseData::~ShapeBaseData()
{

}

// HH: Added for Tribes 2 compatibility.
// GameConnection::transmitDataBlocks packs and unpacks a server's own
// datablocks in place for its local client, and the body our write walk
// gives them carries defaults, not what the scripts set. One a remote server
// sent sits in its connection, and a re-send of it has to land, as it does in
// retail.
bool ShapeBaseData::isUnpackedInPlace()
{
   return isProperlyAdded() && !dynamic_cast<NetConnection *>(getGroup());
}

// HH: Added for Tribes 2 compatibility.
// The fields ShapeBaseData::unpackData (0x5e63e0) reads for every shape
// datablock, whatever its class.
bool ShapeBaseData::unpackData(BitStream *stream, U32 classId)
{
   if (!Parent::unpackData(stream, classId))
      return false;

   if (isUnpackedInPlace())
      return true;

   // computeCRC is the first field of the run, so a body without it is one
   // whose layout the run was not found in. It keeps naming its shape by the
   // first one it carries, as every datablock did before the run was known.
   U32 word;
   if (!T2Layout::lastDataBlockWord(T2Layout::T2DbComputeCRC, &word))
   {
      if (const char *shape = T2Layout::lastShapeFile())
         shapeName = StringTable->insert(shape);
      return true;
   }

   computeCRC = word != 0;
   if (T2Layout::lastDataBlockWord(T2Layout::T2DbShapeCRC, &word))
      mCRC = word;

   static const struct { U32 field; StringTableEntry ShapeBaseData::*member; }
      sStrings[] =
   {
      { T2Layout::T2DbShapeFile,       &ShapeBaseData::shapeName       },
      { T2Layout::T2DbDebrisShapeName, &ShapeBaseData::debrisShapeName },
      { T2Layout::T2DbCmdCategory,     &ShapeBaseData::cmdCategory     },
      { T2Layout::T2DbCmdMiniIconName, &ShapeBaseData::cmdMiniIconName },
   };

   for (U32 i = 0; i < sizeof(sStrings) / sizeof(sStrings[0]); i++)
   {
      const char *str = T2Layout::lastDataBlockString(sStrings[i].field);
      this->*sStrings[i].member = StringTable->insert(str ? str : "");
   }

   // Each is sent only where it differs from the prototype, and a clear flag
   // puts the prototype's value back.
   static const struct { U32 field; F32 ShapeBaseData::*member;
                         const F32 *proto; } sFields[] =
   {
      { T2Layout::T2DbMass,             &ShapeBaseData::mass,
        &gShapeBaseDataProto.mass             },
      { T2Layout::T2DbDrag,             &ShapeBaseData::drag,
        &gShapeBaseDataProto.drag             },
      { T2Layout::T2DbDensity,          &ShapeBaseData::density,
        &gShapeBaseDataProto.density          },
      { T2Layout::T2DbMaxEnergy,        &ShapeBaseData::maxEnergy,
        &gShapeBaseDataProto.maxEnergy        },
      { T2Layout::T2DbCameraMaxDist,    &ShapeBaseData::cameraMaxDist,
        &gShapeBaseDataProto.cameraMaxDist    },
      { T2Layout::T2DbCameraMinDist,    &ShapeBaseData::cameraMinDist,
        &gShapeBaseDataProto.cameraMinDist    },
      { T2Layout::T2DbCameraDefaultFov, &ShapeBaseData::cameraDefaultFov,
        &gShapeBaseDataProto.cameraDefaultFov },
      { T2Layout::T2DbCameraMinFov,     &ShapeBaseData::cameraMinFov,
        &gShapeBaseDataProto.cameraMinFov     },
      { T2Layout::T2DbCameraMaxFov,     &ShapeBaseData::cameraMaxFov,
        &gShapeBaseDataProto.cameraMaxFov     },
   };

   for (U32 i = 0; i < sizeof(sFields) / sizeof(sFields[0]); i++)
   {
      F32 value = *sFields[i].proto;
      T2Layout::lastDataBlockField(sFields[i].field, &value);
      this->*sFields[i].member = value;
   }

   // A clear flag is none, which retail stores as -1.
   static const struct { U32 field; S32 ShapeBaseData::*member; } sIds[] =
   {
      { T2Layout::T2DbCmdIcon,                  &ShapeBaseData::cmdIconID    },
      { T2Layout::T2DbShapeExplosion,           &ShapeBaseData::explosionID  },
      { T2Layout::T2DbShapeUnderwaterExplosion,
                                      &ShapeBaseData::underwaterExplosionID },
      { T2Layout::T2DbShapeDebris,              &ShapeBaseData::debrisID     },
   };

   for (U32 i = 0; i < sizeof(sIds) / sizeof(sIds[0]); i++)
   {
      U32 id;
      this->*sIds[i].member =
         T2Layout::lastDataBlockWord(sIds[i].field, &id)
            ? S32(id + DataBlockObjectIdFirst) : -1;
   }

   static const struct { U32 field; bool ShapeBaseData::*member; } sFlags[] =
   {
      { T2Layout::T2DbObserveThroughObject,
                                       &ShapeBaseData::observeThroughObject   },
      { T2Layout::T2DbIsInvincible,         &ShapeBaseData::isInvincible        },
      { T2Layout::T2DbRenderWhenDestroyed,  &ShapeBaseData::renderWhenDestroyed },
      { T2Layout::T2DbInheritEnergyFromMount,
                                       &ShapeBaseData::inheritEnergyFromMount },
      { T2Layout::T2DbFirstPersonOnly,      &ShapeBaseData::firstPersonOnly     },
      { T2Layout::T2DbUseEyePoint,          &ShapeBaseData::useEyePoint         },
   };

   for (U32 i = 0; i < sizeof(sFlags) / sizeof(sFlags[0]); i++)
      if (T2Layout::lastDataBlockWord(sFlags[i].field, &word))
         this->*sFlags[i].member = word != 0;

   U32 lifetime;
   if (T2Layout::lastDataBlockWord(T2Layout::T2DbShieldEffectLifetimeMS,
                                   &lifetime))
      shieldEffectLifetimeMS = S32(lifetime);

   // The scale is on the wire only when it is not the default.
   F32 scale[3];
   if (T2Layout::lastDataBlockField(T2Layout::T2DbShieldEffectScaleX, &scale[0]) &&
       T2Layout::lastDataBlockField(T2Layout::T2DbShieldEffectScaleY, &scale[1]) &&
       T2Layout::lastDataBlockField(T2Layout::T2DbShieldEffectScaleZ, &scale[2]))
      shieldEffectScale.set(scale[0], scale[1], scale[2]);

   return true;
}

bool ShapeBaseData::preload(bool server, String &errorStr)
{
   if (!Parent::preload(server, errorStr))
      return false;
   bool shapeError = false;

   // HH: Modified for Tribes 2 compatibility.
   // Retail (0x5e42b0) resolves these on server and client alike, before it
   // asks which it is, with -1 for none. The commander icon's failure prints
   // the debris message, as retail's does.
   if( !explosion && explosionID != -1 )
   {
      if( Sim::findObject( explosionID, explosion ) == false)
      {
         Con::errorf( ConsoleLogEntry::General, "ShapeBaseData::preload: Invalid packet, bad datablockId(explosion): 0x%x", explosionID );
      }
      AssertFatal(!(explosion && ((explosionID < DataBlockObjectIdFirst) || (explosionID > DataBlockObjectIdLast))),
         "ShapeBaseData::preload: invalid explosion data");
   }

   if( !underwaterExplosion && underwaterExplosionID != -1 )
   {
      if( Sim::findObject( underwaterExplosionID, underwaterExplosion ) == false)
      {
         Con::errorf( ConsoleLogEntry::General, "ShapeBaseData::preload: Invalid packet, bad datablockId(underwaterExplosion): 0x%x", underwaterExplosionID );
      }
      AssertFatal(!(underwaterExplosion && ((underwaterExplosionID < DataBlockObjectIdFirst) || (underwaterExplosionID > DataBlockObjectIdLast))),
         "ShapeBaseData::preload: invalid underwaterExplosion data");
   }

   if( !debris && debrisID != -1 )
   {
      if( Sim::findObject( debrisID, debris ) == false )
         Con::errorf( ConsoleLogEntry::General, "ShapeBaseData::preload: Invalid packet, bad datablockId(debris): 0x%x", debrisID );
      AssertFatal(!(debris && ((debrisID < DataBlockObjectIdFirst) || (debrisID > DataBlockObjectIdLast))),
         "ShapeBaseData::preload: invalid debris data");
   }

   if( !cmdIcon && cmdIconID != -1 )
   {
      if( Sim::findObject( cmdIconID, cmdIcon ) == false )
         Con::errorf( ConsoleLogEntry::General, "ShapeBaseData::preload: Invalid packet, bad datablockId(debris): 0x%x", cmdIconID );
   }

   // Resolve objects transmitted from server
   if (!server) {

      if( debrisShapeName && debrisShapeName[0] != '\0' && !bool(debrisShape) )
      {
         debrisShape = ResourceManager::get().load(T2Layout::resourcePath(debrisShapeName, "shapes/"));
         if( bool(debrisShape) == false )
         {
            errorStr = String::ToString("ShapeBaseData::load: Couldn't load shape \"%s\"", debrisShapeName);
            return false;
         }
         else
         {
            if(!server && !debrisShape->preloadMaterialList(debrisShape.getPath()) && NetConnection::filesWereDownloaded())
               shapeError = true;

            TSShapeInstance* pDummy = new TSShapeInstance(debrisShape, !server);
            delete pDummy;
         }
      }
   }

   //
   if (shapeName && shapeName[0]) {
      S32 i;

      // Resolve shapename
      mShape = ResourceManager::get().load(T2Layout::resourcePath(shapeName, "shapes/"));
      if (bool(mShape) == false)
      {
         errorStr = String::ToString("ShapeBaseData: Couldn't load shape \"%s\"",shapeName);
         return false;
      }
      if(!server && !mShape->preloadMaterialList(mShape.getPath()) && NetConnection::filesWereDownloaded())
         shapeError = true;

      if(computeCRC)
      {
         Con::printf("Validation required for shape: %s", shapeName);

         Torque::FS::FileNodeRef    fileRef = Torque::FS::GetFileNode(mShape.getPath());

         if (!fileRef)
            return false;

         if(server)
            mCRC = fileRef->getChecksum();
         else if(mCRC != fileRef->getChecksum())
         {
            errorStr = String::ToString("Shape \"%s\" does not match version on server.",shapeName);
            return false;
         }
      }
      // Resolve details and camera node indexes.
      static const String sCollisionStr( "collision-" );

      for (i = 0; i < mShape->details.size(); i++)
      {
         const String &name = mShape->names[mShape->details[i].nameIndex];

         if (name.compare( sCollisionStr, sCollisionStr.length(), String::NoCase ) == 0)
         {
            collisionDetails.push_back(i);
            collisionBounds.increment();

            mShape->computeBounds(collisionDetails.last(), collisionBounds.last());
            mShape->getAccelerator(collisionDetails.last());

            if (!mShape->bounds.isContained(collisionBounds.last()))
            {
               Con::warnf("Warning: shape %s collision detail %d (Collision-%d) bounds exceed that of shape.", shapeName, collisionDetails.size() - 1, collisionDetails.last());
               collisionBounds.last() = mShape->bounds;
            }
            else if (collisionBounds.last().isValidBox() == false)
            {
               Con::errorf("Error: shape %s-collision detail %d (Collision-%d) bounds box invalid!", shapeName, collisionDetails.size() - 1, collisionDetails.last());
               collisionBounds.last() = mShape->bounds;
            }

            // The way LOS works is that it will check to see if there is a LOS detail that matches
            // the the collision detail + 1 + MaxCollisionShapes (this variable name should change in
            // the future). If it can't find a matching LOS it will simply use the collision instead.
            // We check for any "unmatched" LOS's further down
            LOSDetails.increment();

            String   buff = String::ToString("LOS-%d", i + 1 + MaxCollisionShapes);
            U32 los = mShape->findDetail(buff);
            if (los == -1)
               LOSDetails.last() = i;
            else
               LOSDetails.last() = los;
         }
      }

      // Snag any "unmatched" LOS details
      static const String sLOSStr( "LOS-" );

      for (i = 0; i < mShape->details.size(); i++)
      {
         const String &name = mShape->names[mShape->details[i].nameIndex];

         if (name.compare( sLOSStr, sLOSStr.length(), String::NoCase ) == 0)
         {
            // See if we already have this LOS
            bool found = false;
            for (U32 j = 0; j < LOSDetails.size(); j++)
            {
               if (LOSDetails[j] == i)
               {
                     found = true;
                     break;
               }
            }

            if (!found)
               LOSDetails.push_back(i);
         }
      }

      debrisDetail = mShape->findDetail("Debris-17");
      eyeNode = mShape->findNode("eye");
      earNode = mShape->findNode( "ear" );
      if( earNode == -1 )
         earNode = eyeNode;
      cameraNode = mShape->findNode("cam");
      if (cameraNode == -1)
         cameraNode = eyeNode;

      // Resolve mount point node indexes
      for (i = 0; i < SceneObject::NumMountPoints; i++) {
         char fullName[256];
         dSprintf(fullName,sizeof(fullName),"mount%d",i);
         mountPointNode[i] = mShape->findNode(fullName);
      }

        // find the AIRepairNode - hardcoded to be the last node in the array...
      mountPointNode[AIRepairNode] = mShape->findNode("AIRepairNode");

      //
      hulkSequence = mShape->findSequence("Visibility");
      damageSequence = mShape->findSequence("Damage");

      //
      F32 w = mShape->bounds.len_y() / 2;
      if (cameraMaxDist < w)
         cameraMaxDist = w;
   }

   if(!server)
   {
/*
      // grab all the hud images
      for(U32 i = 0; i < NumHudRenderImages; i++)
      {
         if(hudImageNameFriendly[i] && hudImageNameFriendly[i][0])
            hudImageFriendly[i] = TextureHandle(hudImageNameFriendly[i], BitmapTexture);

         if(hudImageNameEnemy[i] && hudImageNameEnemy[i][0])
            hudImageEnemy[i] = TextureHandle(hudImageNameEnemy[i], BitmapTexture);
      }
*/
   }

   // Resolve CubeReflectorDesc.
   if ( cubeDescName.isNotEmpty() )
   {
      Sim::findObject( cubeDescName, reflectorDesc );
   }
   else if( cubeDescId > 0 )
   {
      Sim::findObject( cubeDescId, reflectorDesc );
   }

   return !shapeError;
}

bool ShapeBaseData::_setMass( void* object, const char* index, const char* data )
{
   ShapeBaseData* shape = reinterpret_cast< ShapeBaseData* >( object );

	F32 mass = dAtof(data);

	if (mass <= 0)
		mass = 0.01f;

	shape->mass = mass;

   return false;
}


void ShapeBaseData::initPersistFields()
{
   addGroup( "Shadows" );

      addField( "shadowEnable", TypeBool, Offset(shadowEnable, ShapeBaseData),
         "Enable shadows for this shape (currently unused, shadows are always enabled)." );
      addField( "shadowSize", TypeS32, Offset(shadowSize, ShapeBaseData),
         "Size of the projected shadow texture (must be power of 2)." );
      addField( "shadowMaxVisibleDistance", TypeF32, Offset(shadowMaxVisibleDistance, ShapeBaseData),
         "Maximum distance at which shadow is visible (currently unused)." );
      addField( "shadowProjectionDistance", TypeF32, Offset(shadowProjectionDistance, ShapeBaseData),
         "Maximum height above ground to project shadow. If the object is higher "
         "than this no shadow will be rendered." );
      addField( "shadowSphereAdjust", TypeF32, Offset(shadowSphereAdjust, ShapeBaseData),
         "Scalar applied to the radius of spot shadows (initial radius is based "
         "on the shape bounds but can be adjusted with this field)." );

   endGroup( "Shadows" );

   addGroup( "Render" );

      addField( "shapeFile", TypeShapeFilename, Offset(shapeName, ShapeBaseData),
         "The DTS or DAE model to use for this object." );

   endGroup( "Render" );

   addGroup( "Destruction", "Parameters related to the destruction effects of this object." );

      addField( "explosion", TYPEID< ExplosionData >(), Offset(explosion, ShapeBaseData),
         "%Explosion to generate when this shape is blown up." );
      addField( "underwaterExplosion", TYPEID< ExplosionData >(), Offset(underwaterExplosion, ShapeBaseData),
         "%Explosion to generate when this shape is blown up underwater." );
      addField( "debris", TYPEID< DebrisData >(), Offset(debris, ShapeBaseData),
         "%Debris to generate when this shape is blown up." );
      addField( "renderWhenDestroyed", TypeBool, Offset(renderWhenDestroyed, ShapeBaseData),
         "Whether to render the shape when it is in the \"Destroyed\" damage state." );
      addField( "debrisShapeName", TypeShapeFilename, Offset(debrisShapeName, ShapeBaseData),
         "The DTS or DAE model to use for auto-generated breakups. @note may not be functional." );

   endGroup( "Destruction" );

   addGroup( "Physics" );
   
      addProtectedField("mass", TypeF32, Offset(mass, ShapeBaseData), &_setMass, &defaultProtectedGetFn, "Shape mass.\nUsed in simulation of moving objects.\n"  );
      addField( "drag", TypeF32, Offset(drag, ShapeBaseData),
         "Drag factor.\nReduces velocity of moving objects." );
      addField( "density", TypeF32, Offset(density, ShapeBaseData),
         "Shape density.\nUsed when computing buoyancy when in water.\n" );

   endGroup( "Physics" );

   addGroup( "Damage/Energy" );

      addField( "maxEnergy", TypeF32, Offset(maxEnergy, ShapeBaseData),
         "Maximum energy level for this object." );
      addField( "maxDamage", TypeF32, Offset(maxDamage, ShapeBaseData),
         "Maximum damage level for this object." );
      addField( "disabledLevel", TypeF32, Offset(disabledLevel, ShapeBaseData),
         "Damage level above which the object is disabled.\n"
         "Currently unused." );
      addField( "destroyedLevel", TypeF32, Offset(destroyedLevel, ShapeBaseData),
         "Damage level above which the object is destroyed.\n"
         "When the damage level increases above this value, the object damage "
         "state is set to \"Destroyed\"." );
      addField( "repairRate", TypeF32, Offset(repairRate, ShapeBaseData),
         "Rate at which damage is repaired in damage units/tick.\n"
         "This value is subtracted from the damage level until it reaches 0." );
      addField( "inheritEnergyFromMount", TypeBool, Offset(inheritEnergyFromMount, ShapeBaseData),
         "Flag controlling whether to manage our own energy level, or to use "
         "the energy level of the object we are mounted to." );
      addField( "isInvincible", TypeBool, Offset(isInvincible, ShapeBaseData),
         "Invincible flag; when invincible, the object cannot be damaged or "
         "repaired." );
      // HH: Added for Tribes 2 compatibility.
      addField( "shieldEffectLifetimeMS", TypeS32, Offset(shieldEffectLifetimeMS, ShapeBaseData),
         "How long the flash lasts when a shield turns a hit away, in ms." );
      addField( "shieldEffectScale", TypePoint3F, Offset(shieldEffectScale, ShapeBaseData),
         "The shield flash's size along each axis, as a share of eight "
         "tenths of the shape's bounding sphere." );

   endGroup( "Damage/Energy" );

   addGroup( "Camera", "The settings used by the shape when it is the camera." );

      addField( "cameraMaxDist", TypeF32, Offset(cameraMaxDist, ShapeBaseData),
         "The maximum distance from the camera to the object.\n"
         "Used when computing a custom camera transform for this object.\n\n"
         "@see observeThroughObject" );
      addField( "cameraMinDist", TypeF32, Offset(cameraMinDist, ShapeBaseData),
         "The minimum distance from the camera to the object.\n"
         "Used when computing a custom camera transform for this object.\n\n"
         "@see observeThroughObject" );
      addField( "cameraDefaultFov", TypeF32, Offset(cameraDefaultFov, ShapeBaseData),
         "The default camera vertical FOV in degrees." );
      addField( "cameraMinFov", TypeF32, Offset(cameraMinFov, ShapeBaseData),
         "The minimum camera vertical FOV allowed in degrees." );
      addField( "cameraMaxFov", TypeF32, Offset(cameraMaxFov, ShapeBaseData),
         "The maximum camera vertical FOV allowed in degrees." );
      addField( "firstPersonOnly", TypeBool, Offset(firstPersonOnly, ShapeBaseData),
         "Flag controlling whether the view from this object is first person "
         "only." );
      addField( "useEyePoint", TypeBool, Offset(useEyePoint, ShapeBaseData),
         "Flag controlling whether the client uses this object's eye point to "
         "view from." );
      addField( "observeThroughObject", TypeBool, Offset(observeThroughObject, ShapeBaseData),
         "Observe this object through its camera transform and default fov.\n"
         "If true, when this object is the camera it can provide a custom camera "
         "transform and FOV (instead of the default eye transform)." );

   endGroup("Camera");

   addGroup( "Misc" );

      addField( "computeCRC", TypeBool, Offset(computeCRC, ShapeBaseData),
         "If true, verify that the CRC of the client's shape model matches the "
         "server's CRC for the shape model when loaded by the client." );

   endGroup( "Misc" );

   // HH: Added for Tribes 2 compatibility.
   addGroup( "Commander" );

      addField( "cmdCategory", TypeString, Offset(cmdCategory, ShapeBaseData),
         "The commander map's category for this shape." );
      addField( "cmdIcon", TYPEID< GameBaseData >(), Offset(cmdIcon, ShapeBaseData),
         "The CommanderIconData the commander map draws this shape with." );
      addField( "cmdMiniIconName", TypeString, Offset(cmdMiniIconName, ShapeBaseData),
         "The small icon the commander map lists this shape with." );

   endGroup( "Commander" );

   addGroup( "Reflection" );

      addField( "cubeReflectorDesc", TypeRealString, Offset( cubeDescName, ShapeBaseData ), 
         "References a ReflectorDesc datablock that defines performance and quality properties for dynamic reflections.\n");
      //addField( "reflectMaxRateMs", TypeS32, Offset( reflectMaxRateMs, ShapeBaseData ), "reflection will not be updated more frequently than this" );
      //addField( "reflectMaxDist", TypeF32, Offset( reflectMaxDist, ShapeBaseData ), "distance at which reflection is never updated" );
      //addField( "reflectMinDist", TypeF32, Offset( reflectMinDist, ShapeBaseData ), "distance at which reflection is always updated" );
      //addField( "reflectDetailAdjust", TypeF32, Offset( reflectDetailAdjust, ShapeBaseData ), "scale up or down the detail level for objects rendered in a reflection" );

   endGroup( "Reflection" );

   Parent::initPersistFields();
}

DefineEngineMethod( ShapeBaseData, checkDeployPos, bool, ( TransformF txfm ),,
   "@brief Check if there is the space at the given transform is free to spawn into.\n\n"

   "The shape's bounding box volume is used to check for collisions at the given world "
   "transform.  Only interior and static objects are checked for collision.\n"

   "@param txfm Deploy transform to check\n"
   "@return True if the space is free, false if there is already something in "
   "the way.\n"

   "@note This is a server side only check, and is not actually limited to spawning.\n")
{
   if (bool(object->mShape) == false)
      return false;

   MatrixF mat = txfm.getMatrix();

   Box3F objBox = object->mShape->bounds;
   Point3F boxCenter = (objBox.minExtents + objBox.maxExtents) * 0.5f;
   objBox.minExtents = boxCenter + (objBox.minExtents - boxCenter) * 0.9f;
   objBox.maxExtents = boxCenter + (objBox.maxExtents - boxCenter) * 0.9f;

   Box3F wBox = objBox;
   mat.mul(wBox);

   EarlyOutPolyList polyList;
   polyList.mNormal.set(0,0,0);
   polyList.mPlaneList.clear();
   polyList.mPlaneList.setSize(6);
   polyList.mPlaneList[0].set(objBox.minExtents,VectorF(-1,0,0));
   polyList.mPlaneList[1].set(objBox.maxExtents,VectorF(0,1,0));
   polyList.mPlaneList[2].set(objBox.maxExtents,VectorF(1,0,0));
   polyList.mPlaneList[3].set(objBox.minExtents,VectorF(0,-1,0));
   polyList.mPlaneList[4].set(objBox.minExtents,VectorF(0,0,-1));
   polyList.mPlaneList[5].set(objBox.maxExtents,VectorF(0,0,1));

   for (U32 i = 0; i < 6; i++)
   {
      PlaneF temp;
      mTransformPlane(mat, Point3F(1, 1, 1), polyList.mPlaneList[i], &temp);
      polyList.mPlaneList[i] = temp;
   }

   if (gServerContainer.buildPolyList(PLC_Collision, wBox, InteriorObjectType | StaticShapeObjectType, &polyList))
      return false;
   return true;
}


DefineEngineMethod(ShapeBaseData, getDeployTransform, TransformF, ( Point3F pos, Point3F normal ),,
   "@brief Helper method to get a transform from a position and vector (suitable for use with setTransform).\n\n"
   "@param pos Desired transform position\n"
   "@param normal Vector of desired direction\n"
   "@return The deploy transform\n" )
{
   normal.normalize();

   VectorF xAxis;
   if( mFabs(normal.z) > mFabs(normal.x) && mFabs(normal.z) > mFabs(normal.y))
      mCross( VectorF( 0, 1, 0 ), normal, &xAxis );
   else
      mCross( VectorF( 0, 0, 1 ), normal, &xAxis );

   VectorF yAxis;
   mCross( normal, xAxis, &yAxis );

   MatrixF testMat(true);
   testMat.setColumn( 0, xAxis );
   testMat.setColumn( 1, yAxis );
   testMat.setColumn( 2, normal );
   testMat.setPosition( pos );

   return testMat;
}

ShapeBaseData::Tribes2Rest::Tribes2Rest()
{
   haveIcon = false;
   icon = 0;
   dMemset(iconColour, 0, sizeof(iconColour));
   haveScale = false;
   scale = 0.0f;
   hudName = StringTable->EmptyString();
   for(U32 i = 0; i < 4; i++)
   {
      haveBlock[i] = false;
      block[i] = 0;
   }
   dMemset(bit, 0, sizeof(bit));
   visibleDistance = 0;
   haveOffset = false;
   offset.set(0.0f, 0.0f, 0.0f);
   for(U32 i = 0; i < NumSlots; i++)
   {
      haveSound[i] = false;
      sound[i] = StringTable->EmptyString();
      haveSoundLoop[i] = false;
      soundLoop[i] = StringTable->EmptyString();
   }
   dMemset(slot, 0, sizeof(slot));
}


// HH: Modified for Tribes 2 compatibility.


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

Chunker<ShapeBase::CollisionTimeout> sTimeoutChunker;
ShapeBase::CollisionTimeout* ShapeBase::sFreeTimeoutList = 0;


//----------------------------------------------------------------------------

IMPLEMENT_CO_NETOBJECT_V1(ShapeBase);

ConsoleDocClass( ShapeBase,
   "@ingroup gameObjects"
);

IMPLEMENT_CALLBACK( ShapeBase, validateCameraFov, F32, (F32 fov), (fov),
   "@brief Called on the server when the client has requested a FOV change.\n\n"

   "When the client requests that its field of view should be changed (because "
   "they want to use a sniper scope, for example) this new FOV needs to be validated "
   "by the server.  This method is called if it exists (it is optional) to validate "
   "the requested FOV, and modify it if necessary.  This could be as simple as checking "
   "that the FOV falls within a correct range, to making sure that the FOV matches the "
   "capabilities of the current weapon.\n\n"

   "Following this method, ShapeBase ensures that the given FOV still falls within "
   "the datablock's cameraMinFov and cameraMaxFov.  If that is good enough for your "
   "purposes, then you do not need to define the validateCameraFov() callback for "
   "your ShapeBase.\n\n"

   "@param fov The FOV that has been requested by the client.\n"
   "@return The FOV as validated by the server.\n\n"

   "@see ShapeBaseData\n\n");

ShapeBase::ShapeBase()
 : mDrag( 0.0f ),
   mBuoyancy( 0.0f ),
   mWaterCoverage( 0.0f ),
   mLiquidHeight( 0.0f ),
   mControllingObject( NULL ),
   mGravityMod( 1.0f ),
   mAppliedForce( Point3F::Zero ),
   mTimeoutList( NULL ),
   mDataBlock( NULL ),
   mShapeInstance( NULL ),
   mEnergy( 0.0f ),
   mRechargeRate( 0.0f ),
   mHeat( 0.0f ),               // HH: Added for Tribes 2 compatibility.
   mDamage( 0.0f ),
   mRepairRate( 0.0f ),
   mRepairReserve( 0.0f ),
   mDamageState( Enabled ),
   mDamageThread( NULL ),
   mHulkThread( NULL ),
   mLastRenderFrame( 0 ),
   mLastRenderDistance( 0.0f ),
   mCloaked( false ),
   mCloakLevel( 0.0f ),
   mDamageFlash( 0.0f ),
   mWhiteOut( 0.0f ),
   mInvincibleEffect( 0.0f ),
   mInvincibleDelta( 0.0f ),
   mInvincibleCount( 0.0f ),
   mInvincibleSpeed( 0.0f ),
   mInvincibleTime( 0.0f ),
   mInvincibleFade( 0.1f ),
   mInvincibleOn( false ),
   mIsControlled( false ),
   mConvexList( new Convex ),
   mCameraFov( 90.0f ),
   mShieldNormal( 0.0f, 0.0f, 1.0f ),
   mFadeOut( true ),
   mFading( false ),
   mFadeVal( 1.0f ),
   mFadeTime( 1.0f ),
   mFadeElapsedTime( 0.0f ),
   mFadeDelay( 0.0f ),
   mFlipFadeVal( false ),
   damageDir( 0.0f, 0.0f, 1.0f ),
   mShapeBaseMount( NULL ),
   mMass( 1.0f ),
   mOneOverMass( 1.0f ),
   mMoveMotion( false ),
   mIsAiControlled( false )
{
   mTypeMask |= ShapeBaseObjectType | LightObjectType;   

   S32 i;

   for (i = 0; i < MaxSoundThreads; i++) {
      mSoundThread[i].play = false;
      mSoundThread[i].profile = 0;
      mSoundThread[i].sound = 0;
   }

   for (i = 0; i < MaxScriptThreads; i++) {
      mScriptThread[i].sequence = -1;
      mScriptThread[i].thread = 0;
      mScriptThread[i].sound = 0;
      mScriptThread[i].state = Thread::Stop;
      mScriptThread[i].atEnd = false;
	   mScriptThread[i].timescale = 1.f;
	   mScriptThread[i].position = -1.f;
   }

   for (i = 0; i < MaxTriggerKeys; i++)
      mTrigger[i] = false;

   mWeaponCamShake = NULL;
}


ShapeBase::~ShapeBase()
{
   SAFE_DELETE( mConvexList );

   AssertFatal(mMount.link == 0,"ShapeBase::~ShapeBase: An object is still mounted");
   if( mShapeInstance && (mShapeInstance->getDebrisRefCount() == 0) )
   {
      delete mShapeInstance;
   }

   CollisionTimeout* ptr = mTimeoutList;
   while (ptr) {
      CollisionTimeout* cur = ptr;
      ptr = ptr->next;
      cur->next = sFreeTimeoutList;
      sFreeTimeoutList = cur;
   }
}

void ShapeBase::initPersistFields()
{
   addProtectedField( "skin", TypeRealString, Offset(mAppliedSkinName, ShapeBase), &_setFieldSkin, &_getFieldSkin,
      "@brief The skin applied to the shape.\n\n"

      "'Skinning' the shape effectively renames the material targets, allowing "
      "different materials to be used on different instances of the same model. "
      "Using getSkinName() and setSkinName() is equivalent to reading and "
      "writing the skin field directly.\n\n"

      "Any material targets that start with the old skin name have that part "
      "of the name replaced with the new skin name. The initial old skin name is "
      "\"base\". For example, if a new skin of \"blue\" was applied to a model "
      "that had material targets <i>base_body</i> and <i>face</i>, the new targets "
      "would be <i>blue_body</i> and <i>face</i>. Note that <i>face</i> was not "
      "renamed since it did not start with the old skin name of \"base\".\n\n"

      "To support models that do not use the default \"base\" naming convention, "
      "you can also specify the part of the name to replace in the skin field "
      "itself. For example, if a model had a material target called <i>shapemat</i>, "
      "we could apply a new skin \"shape=blue\", and the material target would be "
      "renamed to <i>bluemat</i> (note \"shape\" has been replaced with \"blue\").\n\n"

      "Multiple skin updates can also be applied at the same time by separating "
      "them with a semicolon. For example: \"base=blue;face=happy_face\".\n\n"

      "Material targets are only renamed if an existing Material maps to that "
      "name, or if there is a diffuse texture in the model folder with the same "
      "name as the new target.\n\n" );

   addField( "isAiControlled", TypeBool, Offset(mIsAiControlled, ShapeBase),
      "@brief Is this object AI controlled.\n\n"
      "If True then this object is considered AI controlled and not player controlled.\n" );

   Parent::initPersistFields();
}

bool ShapeBase::_setFieldSkin( void *object, const char *index, const char *data )
{
   ShapeBase* shape = static_cast<ShapeBase*>( object );
   if ( shape )
      shape->setSkinName( data );
   return false;
}

const char *ShapeBase::_getFieldSkin( void *object, const char *data )
{
   ShapeBase* shape = static_cast<ShapeBase*>( object );
   return shape ? shape->getSkinName() : "";
}

//----------------------------------------------------------------------------

bool ShapeBase::onAdd()
{
   if( !Parent::onAdd() )
      return false;
      
   if( !mDataBlock )
   {
      Con::errorf( "ShapeBase::onAdd - no datablock on shape %i:%s (%s)",
         getId(), getClassName(), getName() );
      return false;
   }

   // Resolve sounds that arrived in the initial update
   S32 i;
   for (i = 0; i < MaxSoundThreads; i++)
      updateAudioState(mSoundThread[i]);

   for (i = 0; i < MaxScriptThreads; i++)
   {
      Thread& st = mScriptThread[i];
      if(st.thread)
         updateThread(st);
   }   

/*
      if(mDataBlock->cloakTexName != StringTable->insert(""))
        mCloakTexture = TextureHandle(mDataBlock->cloakTexName, MeshTexture, false);
*/         

   // HH: Added for Tribes 2 compatibility.
   if (isGhost())
      mShieldImpact.load();

   return true;
}

void ShapeBase::onRemove()
{
   mConvexList->nukeList();

   Parent::onRemove();

   // Stop any running sounds on the client
   if (isGhost())
      for (S32 i = 0; i < MaxSoundThreads; i++)
         stopAudio(i);

   if ( isClientObject() )   
   {
      mCubeReflector.unregisterReflector();      

      if ( mWeaponCamShake )
      {
         if ( mWeaponCamShake->isAdded )
            gCamFXMgr.removeFX( mWeaponCamShake );

         SAFE_DELETE( mWeaponCamShake );
      }
   }
}


void ShapeBase::onSceneRemove()
{
   mConvexList->nukeList();
   Parent::onSceneRemove();
}

bool ShapeBase::onNewDataBlock( GameBaseData *dptr, bool reload )
{
   ShapeBaseData *prevDB = dynamic_cast<ShapeBaseData*>( mDataBlock );

   bool isInitialDataBlock = ( mDataBlock == 0 );

   if ( Parent::onNewDataBlock( dptr, reload ) == false )
      return false;

   mDataBlock = dynamic_cast<ShapeBaseData*>(dptr);
   if (!mDataBlock)
      return false;

   setMaskBits(DamageMask);
   mDamageThread = 0;
   mHulkThread = 0;

   // Even if loadShape succeeds, there may not actually be
   // a shape assigned to this object.
   if (bool(mDataBlock->mShape)) {
      delete mShapeInstance;
      mShapeInstance = new TSShapeInstance(mDataBlock->mShape, isClientObject());
      if (isClientObject())
         mShapeInstance->cloneMaterialList();

      mObjBox = mDataBlock->mShape->bounds;
      resetWorldBox();

      // Set the initial mesh hidden state.
      mMeshHidden.setSize( mDataBlock->mShape->objects.size() );
      mMeshHidden.clear();

      // Initialize the threads
      for (U32 i = 0; i < MaxScriptThreads; i++) {
         Thread& st = mScriptThread[i];
         if (st.sequence != -1) {
            // TG: Need to see about suppressing non-cyclic sounds
            // if the sequences were activated before the object was
            // ghosted.
            // TG: Cyclic animations need to have a random pos if
            // they were started before the object was ghosted.

            // If there was something running on the old shape, the thread
            // needs to be reset. Otherwise we assume that it's been
            // initialized either by the constructor or from the server.
            bool reset = st.thread != 0;
            st.thread = 0;
            
            // New datablock/shape may not actually HAVE this sequence.
            // In that case stop playing it.
            
            AssertFatal( prevDB != NULL, "ShapeBase::onNewDataBlock - how did you have a sequence playing without a prior datablock?" );
   
            const TSShape *prevShape = prevDB->mShape;
            const TSShape::Sequence &prevSeq = prevShape->sequences[st.sequence];
            const String &prevSeqName = prevShape->names[prevSeq.nameIndex];

            st.sequence = mDataBlock->mShape->findSequence( prevSeqName );

            if ( st.sequence != -1 )
            {
               setThreadSequence( i, st.sequence, reset );                              
            }            
         }
      }

      if (mDataBlock->damageSequence != -1) {
         mDamageThread = mShapeInstance->addThread();
         mShapeInstance->setSequence(mDamageThread,
                                     mDataBlock->damageSequence,0);
      }
      if (mDataBlock->hulkSequence != -1) {
         mHulkThread = mShapeInstance->addThread();
         mShapeInstance->setSequence(mHulkThread,
                                     mDataBlock->hulkSequence,0);
      }

      if( isGhost() )
      {
         // Reapply the current skin
         mAppliedSkinName = "";
         reSkin();
      }
   }

   //
   mEnergy = 0;
   mDamage = 0;
   mDamageState = Enabled;
   mRepairReserve = 0;
   updateMass();
   updateDamageLevel();
   updateDamageState();

   // HH: Modified for Tribes 2 compatibility.
   mCameraFov = mDataBlock->cameraDefaultFov;
   updateMass();

   if( !isInitialDataBlock && mLightPlugin )
      mLightPlugin->reset();
   
   if ( isClientObject() )
   {      
      mCubeReflector.unregisterReflector();

      if ( mDataBlock->reflectorDesc )
         mCubeReflector.registerReflector( this, mDataBlock->reflectorDesc );      
   }

   return true;
}

void ShapeBase::onDeleteNotify( SimObject *obj )
{
   if ( obj == mCurrentWaterObject )
      mCurrentWaterObject = NULL;

   Parent::onDeleteNotify( obj );      
}

void ShapeBase::onImpact(SceneObject* obj, VectorF vec)
{
   if (!isGhost())
      mDataBlock->onImpact_callback( this, obj, vec, vec.len() );
}

void ShapeBase::onImpact(VectorF vec)
{
   if (!isGhost())
      mDataBlock->onImpact_callback( this, NULL, vec, vec.len() );
}


//----------------------------------------------------------------------------

void ShapeBase::processTick(const Move* move)
{
   PROFILE_SCOPE( ShapeBase_ProcessTick );

   // Energy management
   if (mDamageState == Enabled && mDataBlock->inheritEnergyFromMount == false) {
      F32 store = mEnergy;
      mEnergy += mRechargeRate;
      if (mEnergy > mDataBlock->maxEnergy)
         mEnergy = mDataBlock->maxEnergy;
      else
         if (mEnergy < 0)
            mEnergy = 0;

      // Virtual setEnergyLevel is used here by some derived classes to
      // decide whether they really want to set the energy mask bit.
      if (mEnergy != store)
         setEnergyLevel(mEnergy);
   }

   // Repair management
   if (mDataBlock->isInvincible == false)
   {
      F32 store = mDamage;
      mDamage -= mRepairRate;
      mDamage = mClampF(mDamage, 0.f, mDataBlock->maxDamage);

      if (mRepairReserve > mDamage)
         mRepairReserve = mDamage;
      if (mRepairReserve > 0.0)
      {
         F32 rate = getMin(mDataBlock->repairRate, mRepairReserve);
         mDamage -= rate;
         mRepairReserve -= rate;
      }

      if (store != mDamage)
      {
         updateDamageLevel();
         if (isServerObject()) {
            setMaskBits(DamageMask);
            mDataBlock->onDamage_callback( this, mDamage - store );
         }
      }
   }

   if (isServerObject()) {
      // Server only...
      advanceThreads(TickSec);
      updateServerAudio();

      // update wet state
      setImageWetState(0, mWaterCoverage > 0.4); // more than 40 percent covered

      // update motion state
      mMoveMotion = false;
      if (move && (move->x || move->y || move->z))
      {
         mMoveMotion = true;
      }
      for (int i = 0; i < MaxMountedImages; i++)
      {
         setImageMotionState(i, mMoveMotion);
      }

      if(mFading)
      {
         F32 dt = TickMs / 1000.0f;
         F32 newFadeET = mFadeElapsedTime + dt;
         if(mFadeElapsedTime < mFadeDelay && newFadeET >= mFadeDelay)
            setMaskBits(CloakMask);
         mFadeElapsedTime = newFadeET;
         if(mFadeElapsedTime > mFadeTime + mFadeDelay)
         {
            mFadeVal = F32(!mFadeOut);
            mFading = false;
         }
      }
   }

   // Advance images
   if (isServerObject())
   {
      for (int i = 0; i < MaxMountedImages; i++)
      {
         if (mMountedImageList[i].dataBlock)
            updateImageState(i, TickSec);
      }
   }

   // Call script on trigger state changes
   if (move && mDataBlock && isServerObject()) 
   {
      for (S32 i = 0; i < MaxTriggerKeys; i++) 
      {
         if (move->trigger[i] != mTrigger[i]) 
         {
            mTrigger[i] = move->trigger[i];
            mDataBlock->onTrigger_callback( this, i, move->trigger[i] );
         }
      }
   }

   // Update the damage flash and the whiteout
   //
   if (mDamageFlash > 0.0)
   {
      mDamageFlash -= sDamageFlashDec;
      if (mDamageFlash <= 0.0)
         mDamageFlash = 0.0;
   }
   if (mWhiteOut > 0.0)
   {
      mWhiteOut -= sWhiteoutDec;
      if (mWhiteOut <= 0.0)
         mWhiteOut = 0.0;
   }
}

void ShapeBase::advanceTime(F32 dt)
{
   // On the client, the shape threads and images are
   // advanced at framerate.
   advanceThreads(dt);
   updateAudioPos();
   for (int i = 0; i < MaxMountedImages; i++)
      if (mMountedImageList[i].dataBlock)
      {
         updateImageState(i, dt);
         updateImageAnimation(i, dt);
      }

   // Cloaking
   if (mCloaked && mCloakLevel != 1.0) {
      if (sCloakSpeed <= 0.0f)
      {
         // Instantaneous
         mCloakLevel = 1.0;
      }
      else
      {
         // Over time determined by sCloakSpeed
         mCloakLevel += dt / sCloakSpeed;
         if (mCloakLevel >= 1.0)
            mCloakLevel = 1.0;
      }
   } else if (!mCloaked && mCloakLevel != 0.0) {
      if (sCloakSpeed <= 0.0f)
      {
         // Instantaneous
         mCloakLevel = 0.0;
      }
      else
      {
         // Over time determined by sCloakSpeed
         mCloakLevel -= dt / sCloakSpeed;
         if (mCloakLevel <= 0.0)
            mCloakLevel = 0.0;
      }
   }
   if(mInvincibleOn)
      updateInvincibleEffect(dt);

   // HH: Added for Tribes 2 compatibility.
   if (mShieldImpact.isActive())
      mShieldImpact.advance(dt);

   if(mFading)
   {
      mFadeElapsedTime += dt;
      if(mFadeElapsedTime > mFadeTime)
      {
         mFadeVal = F32(!mFadeOut);
         mFading = false;
      }
      else
      {
         mFadeVal = mFadeElapsedTime / mFadeTime;
         if(mFadeOut)
            mFadeVal = 1 - mFadeVal;
      }
   }
}

void ShapeBase::setControllingClient( GameConnection* client )
{
   if( isGhost() && gSFX3DWorld )
   {
      if( gSFX3DWorld->getListener() == this && !client && getControllingClient() && getControllingClient()->isConnectionToServer() )
      {
         // We are the current listener and are no longer a controller object on the
         // connection, so clear our listener status.
         
         gSFX3DWorld->setListener( NULL );
      }
      else if( client && client->isConnectionToServer() && !getControllingObject() )
      {
         // We're on the local client and not controlled by another object, so make
         // us the current SFX listener.
         
         gSFX3DWorld->setListener( this );
      }
   }

   Parent::setControllingClient( client );

   // Update all of the mounted images' shapes so they may
   // optimize their animation threads.
   for (U32 i=0; i<MaxMountedImages; ++i)
   {
      MountedImage& image = mMountedImageList[i];
      image.updateDoAnimateAllShapes( this );
   }
}

void ShapeBase::setControllingObject(ShapeBase* obj)
{
   if (obj) {
      setProcessTick(false);
      // Even though we don't processTick, we still need to
      // process after the controller in case anyone is mounted
      // on this object.
      processAfter(obj);
   }
   else {
      setProcessTick(true);
      clearProcessAfter();
      // Catch the case of the controlling object actually
      // mounted on this object.
      if (mControllingObject->mMount.object == this)
         mControllingObject->processAfter(this);
   }
   mControllingObject = obj;
}

ShapeBase* ShapeBase::getControlObject()
{
   return 0;
}

void ShapeBase::setControlObject(ShapeBase*)
{
}

bool ShapeBase::isFirstPerson() const
{
   // Always first person as far as the server is concerned.
   if (!isGhost())
      return true;

   if (const GameConnection* con = getControllingClient())
      return con->getControlObject() == this && con->isFirstPerson();
   return false;
}

bool ShapeBase::isValidCameraFov(F32 fov)
{
   // HH: Modified for Tribes 2 compatibility.
   if(!mDataBlock)
      return false;
   return((fov >= mDataBlock->cameraMinFov) && (fov <= mDataBlock->cameraMaxFov));
}

void ShapeBase::setCameraFov(F32 fov)
{
   // On server allow for script side checking
   if ( !isGhost() && isMethod( "validateCameraFov" ) )
   {
      fov = validateCameraFov_callback( fov );
   }

   // HH: Modified for Tribes 2 compatibility.
   if(!mDataBlock)
   {
      mCameraFov = fov;
      return;
   }

   mCameraFov = mClampF(fov, mDataBlock->cameraMinFov, mDataBlock->cameraMaxFov);
}

void ShapeBase::onCameraScopeQuery(NetConnection *cr, CameraScopeQuery * query)
{
   // update the camera query
   query->camera = this;

   if(GameConnection * con = dynamic_cast<GameConnection*>(cr))
   {
      // get the fov from the connection (in deg)
      F32 fov;
      if (con->getControlCameraFov(&fov))
      {
         query->fov = mDegToRad(fov/2);
         query->sinFov = mSin(query->fov);
         query->cosFov = mCos(query->fov);
      }
   }

   // use eye rather than camera transform (good enough and faster)
   MatrixF eyeTransform;
   getEyeTransform(&eyeTransform);
   eyeTransform.getColumn(3, &query->pos);
   eyeTransform.getColumn(1, &query->orientation);

   // Get the visible distance.
	if (getSceneManager() != NULL)
		query->visibleDistance = getSceneManager()->getVisibleDistance();

   Parent::onCameraScopeQuery( cr, query );
}


//----------------------------------------------------------------------------
F32 ShapeBase::getEnergyLevel()
{
   // HH: Modified for Tribes 2 compatibility.
   if ( mDataBlock && mDataBlock->inheritEnergyFromMount && mShapeBaseMount )
      return mShapeBaseMount->getEnergyLevel();
   return mEnergy; 
}

F32 ShapeBase::getEnergyValue()
{
   if ( mDataBlock->inheritEnergyFromMount && mShapeBaseMount )
   {
      F32 maxEnergy = mShapeBaseMount->mDataBlock->maxEnergy;
      if ( maxEnergy > 0.f )
         return (mShapeBaseMount->getEnergyLevel() / maxEnergy);
   }
   else
   {
      F32 maxEnergy = mDataBlock->maxEnergy;
      if ( maxEnergy > 0.f )
         return (mEnergy / mDataBlock->maxEnergy);   
   }

   return 0.0f;
}

void ShapeBase::setEnergyLevel(F32 energy)
{
   if (mDataBlock->inheritEnergyFromMount == false || !mShapeBaseMount) {
      if (mDamageState == Enabled) {
         mEnergy = (energy > mDataBlock->maxEnergy)?
            mDataBlock->maxEnergy: (energy < 0)? 0: energy;
      }
   } else {
      // Pass the set onto whatever we're mounted to...
      if ( mShapeBaseMount )
      {
         mShapeBaseMount->setEnergyLevel(energy);
      }
   }
}

void ShapeBase::setDamageLevel(F32 damage)
{
   if (!mDataBlock->isInvincible) {
      F32 store = mDamage;
      mDamage = mClampF(damage, 0.f, mDataBlock->maxDamage);

      if (store != mDamage) {
         updateDamageLevel();
         if (isServerObject()) {
            setMaskBits(DamageMask);
            mDataBlock->onDamage_callback( this, mDamage - store );
         }
      }
   }
}

void ShapeBase::updateContainer()
{
   PROFILE_SCOPE( ShapeBase_updateContainer );

   // Update container drag and buoyancy properties

   // HH: Modified for Tribes 2 compatibility.
   mDrag = 0.0f;
   mBuoyancy = 0.0f;
   mGravityMod = 1.0;
   mAppliedForce.set(0,0,0);

   ContainerQueryInfo info;
   info.box = getWorldBox();
   info.mass = getMass();
   info.object = this;   // HH: Added for Tribes 2 compatibility.

   mContainer->findObjects(info.box, WaterObjectType|PhysicalZoneObjectType,findRouter,&info);

   mWaterCoverage = mClampF(info.waterCoverage, 0.0f, 1.0f);
   mLiquidType    = info.liquidType;
   mLiquidHeight  = info.waterHeight;
   setCurrentWaterObject( info.waterObject );

   if (mWaterCoverage > 0.1f)
   {
      mDrag = mDataBlock->drag * 15.0f * mWaterCoverage;
      mBuoyancy = mWaterCoverage / mDataBlock->density;
   }

   mAppliedForce = info.appliedForce;
   mGravityMod = info.gravityScale;

   //Con::printf( "WaterCoverage: %f", mWaterCoverage );
   //Con::printf( "Drag: %f", mDrag );
}


//----------------------------------------------------------------------------

void ShapeBase::applyRepair(F32 amount)
{
   // Repair increases the repair reserve
   if (amount > 0 && ((mRepairReserve += amount) > mDamage))
      mRepairReserve = mDamage;
}

void ShapeBase::applyDamage(F32 amount)
{
   if (amount > 0)
      setDamageLevel(mDamage + amount);
}

F32 ShapeBase::getDamageValue()
{
   // Return a 0-1 damage value.
   return mDamage / mDataBlock->maxDamage;
}

// HH: Added for Tribes 2 compatibility.
void ShapeBase::readT2Damage(const T2Layout::Body &body)
{
   if (!body.hasDamage)
      return;

   if (mDataBlock && mDataBlock->maxDamage > 0.0f)
      setDamageLevel(body.damage * mDataBlock->maxDamage);

   if (body.hasDamageDir)
      damageDir = body.damageDir;

   const DamageState was = mDamageState;
   if (body.hasDamageState && body.damageState < NumDamageStates)
      mDamageState = DamageState(body.damageState);

   if ((was != Destroyed && mDamageState == Destroyed) || body.blowApart)
   {
      if (isProperlyAdded())
         blowUp();
   }

   updateDamageLevel();
   updateDamageState();
}

// HH: Added for Tribes 2 compatibility.
// The server sends the time and the speed, not the effect: as retail's
// unpackUpdate does (0x005efc12), the ghost runs the pulse itself.
void ShapeBase::readT2Invincible(const T2Layout::Body &body)
{
   if (body.hasInvincible)
      setupInvincibleEffect(body.invincibleTime, body.invincibleSpeed);
}

// HH: Added for Tribes 2 compatibility.
void ShapeBase::readT2Threads(const T2Layout::Body &body)
{
   const bool trace = Con::getBoolVariable("$pref::Net::traceGhostThreads",
                                           false);

   const U32 slots = getMin((U32)MaxScriptThreads, (U32)T2Layout::MaxThreads);
   for (U32 i = 0; i < slots; i++)
   {
      const T2Layout::Body::Thread &slot = body.threads[i];
      if (!slot.present)
         continue;

      Thread &st = mScriptThread[i];

      if (trace)
         Con::printf("T2 thread: %s %d slot %d sequence %d (%s) state %d "
                     "forward %d atEnd %d", getClassName(), getId(), i,
                     slot.sequence,
                     mShapeInstance && S32(slot.sequence) <
                        mShapeInstance->getShape()->sequences.size()
                        ? mShapeInstance->getShape()->getName(
                             mShapeInstance->getShape()
                                ->sequences[slot.sequence].nameIndex).c_str()
                        : "?",
                     slot.state, slot.forward, slot.atEnd);

      if (st.sequence != S32(slot.sequence))
         setThreadSequence(i, S32(slot.sequence), false);

      st.state = slot.state;
      st.atEnd = slot.atEnd;
      st.timescale = slot.forward ? 1.0f : -1.0f;
      st.position = -1.0f;

      if (mShapeInstance && st.thread)
         updateThread(st);
   }
}

// HH: Added for Tribes 2 compatibility.
// ShapeBase::unpackUpdate (0x005efb77) plays the flash only on an object
// already added, so an initial body that carries one shows nothing.
void ShapeBase::readT2Shield(const T2Layout::Body &body)
{
   if (body.hasShield && isProperlyAdded())
      playShieldEffect(body.shieldNormal, body.shieldEnergy);
}

// HH: Added for Tribes 2 compatibility.
void ShapeBase::readT2Cloak(const T2Layout::Body &body)
{
   if (body.hasCloaked)
      setCloakedState(body.cloaked);

   if (!body.hasFade)
      return;

   // The server holds a fade back until its delay is up, and then sends only
   // its length (0x005eef06), so a client starts it as it reads it.
   mFading = body.fading;
   if (mFading)
   {
      mFadeOut = body.fadeOut;
      mFadeVal = F32(mFadeOut);
      mFadeTime = body.fadeTime;
      mFadeElapsedTime = 0.0f;
      mFadeDelay = 0.0f;
   }
   else
      mFadeVal = F32(body.fadeVisible);
}

// HH: Added for Tribes 2 compatibility.
void ShapeBase::readT2Mount(const T2Layout::Body &body)
{
   if (!body.hasMount)
      return;

   const bool trace = Con::getBoolVariable("$pref::Net::traceGhostMounts",
                                           false);

   if (body.mountGhost < 0)
   {
      if (trace)
         Con::printf("T2 mount: %s %d (ghost %d) gets off %d",
                     getClassName(), getId(), getNetIndex(),
                     isMounted() ? getObjectMount()->getId() : 0);

      // Player's unmount starts a root animation, which a first body has no
      // shape for yet; retail's (0x5eb030) does nothing when not mounted.
      if (isMounted())
         unmount();
      return;
   }

   NetConnection *conn = NetConnection::getConnectionToServer();
   NetObject *ghost = conn ? conn->resolveGhost(body.mountGhost) : NULL;
   if (trace)
      Con::printf("T2 mount: %s %d (ghost %d) on ghost %d node %d = %s %d",
                  getClassName(), getId(), getNetIndex(), body.mountGhost,
                  body.mountNode, ghost ? ghost->getClassName() : "nothing",
                  ghost ? ghost->getId() : 0);

   if (!ghost)
   {
      // A server names only a mount it has ghosted already, and a recording
      // writes the mount first, so retail calls this a bad packet (0x5efcd0).
      NetConnection::setLastError("Invalid packet from server.");
      return;
   }

   // Some Tribes 2 classes are still placeholders with no shape here, and a
   // recording's first bodies keep a ghost that would not register; retail
   // has neither.
   SceneObject *mount = dynamic_cast<SceneObject *>(ghost);
   if (!mount || !mount->isProperlyAdded())
   {
      Con::warnf("T2 mount: %s %d is mounted on a %s, which cannot carry it "
                 "here.", getClassName(), getId(), ghost->getClassName());
      return;
   }

   mount->mountObject(this, S32(body.mountNode));
}

// HH: Added for Tribes 2 compatibility.
// As retail's unpackUpdate (0x5ef2a3): the profile is kept as the id the
// server sent, which updateAudioState resolves on a ghost, and a first body
// leaves that to onAdd.
void ShapeBase::readT2Sounds(const T2Layout::Body &body)
{
   const bool trace = Con::getBoolVariable("$pref::Net::traceGhostSounds",
                                           false);

   const U32 slots = getMin((U32)MaxSoundThreads, (U32)T2Layout::MaxSounds);
   for (U32 i = 0; i < slots; i++)
   {
      const T2Layout::Body::Sound &slot = body.sounds[i];
      if (!slot.present)
         continue;

      Sound &st = mSoundThread[i];
      st.play = slot.play;
      st.profile = st.play ? reinterpret_cast<SFXTrack *>(
                                uintptr_t(slot.profile))
                           : NULL;

      // A datablock comes without its name, so the file is what says which
      // profile it is.
      if (trace)
      {
         SFXProfile *profile = NULL;
         if (st.play)
            Sim::findObject(SimObjectId(slot.profile), profile);
         Con::printf("T2 sound: %s %d (ghost %d) slot %d %s %d (%s)",
                     getClassName(), getId(), getNetIndex(), i,
                     st.play ? "plays" : "stops", slot.profile,
                     profile ? profile->getSoundFileName().c_str()
                             : "nothing");
      }

      if (isProperlyAdded())
         updateAudioState(st);
   }
}

void ShapeBase::onT2BodyRead(const T2Layout::Body &body)
{
   readT2Damage(body);
   readT2Invincible(body);
   readT2Shield(body);
   readT2Cloak(body);
   readT2Mount(body);
   readT2Sounds(body);
   Parent::onT2BodyRead(body);
}

void ShapeBase::playShieldEffect(const Point3F &normal, F32 energy)
{
   mShieldNormal = normal;
   if (!isGhost())
   {
      setMaskBits(ShieldMask);
      return;
   }

   mShieldImpact.start(normal, energy,
                       F32(mDataBlock->shieldEffectLifetimeMS) * 0.001f,
                       mDataBlock->shieldEffectScale);
}

// HH: Added for Tribes 2 compatibility.
// A ghost body is the only thing that says what is in an image slot.
void ShapeBase::readT2Images(const T2Layout::Body &body)
{

   if (!mDataBlock)
      return;

   const bool trace = Con::getBoolVariable("$pref::Net::traceGhostImages",
                                           false);

      // Tribes 2 sends eight slots and fills four, which is what this engine has
      // room for.
   const U32 slots = getMin((U32)MaxMountedImages, (U32)T2Layout::MaxImages);
   for (U32 i = 0; i < slots; i++)
   {
      const T2Layout::Body::Image &slot = body.images[i];
      if (!slot.present)
         continue;

      ShapeBaseImageData *data = NULL;
      if (slot.hasDataBlock)
      {
               // Every image datablock arrives before any ghost, so one that cannot be
               // found is a decode fault.
         if (!Sim::findObject(slot.dataBlockId, data) || !data)
         {

            static Vector<U32> told;
            bool said = false;
            for (U32 j = 0; j < told.size() && !said; j++)
               said = told[j] == slot.dataBlockId;
            if (!said)
            {
               told.push_back(slot.dataBlockId);
               Con::warnf("T2 image: %s %d slot %d names datablock %d, which "
                          "is not a ShapeBaseImageData -- the slot is being "
                          "left as it was.", getClassName(), getId(), i,
                          slot.dataBlockId);
            }
            continue;
         }
      }

            // A skin arrives as a net string id or as the name; only the name can be
            // acted on.
      NetStringHandle skin = mMountedImageList[i].skinNameHandle;
      if (slot.hasSkinName && slot.skinName.isNotEmpty())
         skin = NetStringHandle(slot.skinName.c_str());

      if (trace)
      {
         Con::printf("T2 image: %s %d slot %d db %d skin %s%s loaded %d ammo "
                     "%d trigger %d wet %d target %d fire %d (had %d) added %d "
                     "firing %d",
                     getClassName(), getId(), i,
                     slot.hasDataBlock ? slot.dataBlockId : 0,
                     slot.hasSkinName ? slot.skinName.c_str() : "",
                     slot.hasSkinTag ? " (tag)" : "", slot.loaded, slot.ammo,
                     slot.triggerDown, slot.wet, slot.target, slot.fireCount,
                     mMountedImageList[i].fireCount, isProperlyAdded(),
                     slot.fireOnInitial);

         // A slot being emptied carries no datablock to say any of this of.
         if (data)
         {
            const S32 fs = data->fireState;
            Con::printf("T2 image: %s animated %d flash %d ambient %d spin "
                        "%d fireState %d seq %d vis %d", data->shapeName,
                        data->isAnimated[0], data->hasFlash[0],
                        data->ambientSequence[0], data->spinSequence[0], fs,
                        fs >= 0 ? data->state[fs].sequence[0] : -1,
                        fs >= 0 ? data->state[fs].sequenceVis[0] : -1);
         }
      }

      setImage(i, data, skin, slot.loaded, slot.ammo, slot.triggerDown,
               false, false, false, false, false, false, slot.target);


      setImageLoadedState(i, slot.loaded);
      setImageAmmoState(i, slot.ammo);
      setImageWetState(i, slot.wet);
      setImageTargetState(i, slot.target);

            // setImageTriggerState gives up on a ghost, so the trigger goes on the
            // slot.
      mMountedImageList[i].triggerDown = slot.triggerDown;

            // A client waits for this count to go up; firing is the server's to
            // decide. getImageFireState is the lookup Tribes 2 calls, and it
            // copes with an emptied slot.
      bool fire = false;
      if (isProperlyAdded())
      {
               // An added ghost takes the count even into an emptied slot, as
               // Tribes 2's does (0x5ef7ad), so a remounted weapon does not fire.
         if (slot.fireCount != mMountedImageList[i].fireCount)
         {
            mMountedImageList[i].fireCount = slot.fireCount;
            fire = true;
         }
      }
      else if (data)
      {
               // A ghost's first body fires a slot only when the server says it
               // is mid-shot (0x5ef838), whatever the count. Tribes 2 sends no
               // emptied slot in a first body (0x5eeb46) but still checks
               // (0x5ef832).
         mMountedImageList[i].fireCount = slot.fireCount;
         fire = slot.fireOnInitial;
      }

      if (fire)
      {
         setImageState(i, getImageFireState(i), true);
         // An emptied slot has no state to name.
         const char *state = getImageState(i);
         if (trace)
            Con::printf("T2 image: %s %d slot %d fired into %s", getClassName(),
                        getId(), i, state ? state : "nothing");
      }
   }
}

bool ShapeBase::unpackUpdate(BitStream *stream, bool isInitial)
{

   if (!Parent::unpackUpdate(stream, isInitial))
      return false;

   readT2Images(T2Layout::lastBody());
   readT2Threads(T2Layout::lastBody());
   return true;
}

void ShapeBase::updateDamageLevel()
{
   if (mDamageThread) {
      // mDamage is already 0-1 on the client
      if (mDamage >= mDataBlock->destroyedLevel) {
         if (getDamageState() == Destroyed)
            mShapeInstance->setPos(mDamageThread, 0);
         else
            mShapeInstance->setPos(mDamageThread, 1);
      } else {
         mShapeInstance->setPos(mDamageThread, mDamage / mDataBlock->destroyedLevel);
      }
   }      
}


//----------------------------------------------------------------------------

void ShapeBase::setDamageState(DamageState state)
{
   if (mDamageState == state)
      return;

   bool invokeCallback = false;
   const char* lastState = 0;

   if (!isGhost()) {
      if (state != getDamageState())
         setMaskBits(DamageMask);

      lastState = getDamageStateName();
      switch (state) {
         case Destroyed: {
            if (mDamageState == Enabled)
            {
               setDamageState(Disabled);

               // It is possible that the setDamageState() call above has already
               // upgraded our state to Destroyed.  If that is the case, no need
               // to continue.
               if (mDamageState == state)
                  return;
            }
            invokeCallback = true;
            break;
         }
         case Disabled:
            if (mDamageState == Enabled)
               invokeCallback = true;
            break;
         case Enabled:
            invokeCallback = true;
            break;
         default:
            AssertFatal(false, "Invalid damage state");
            break;
      }
   }

   mDamageState = state;
   if (mDamageState != Enabled) {
      mRepairReserve = 0;
      mEnergy = 0;
   }
   if (invokeCallback) {
      // Like to call the scripts after the state has been intialize.
      // This should only end up being called on the server.
      switch (state) {
         case Destroyed:
            mDataBlock->onDestroyed_callback( this, lastState );
            break;
         case Disabled:
            mDataBlock->onDisabled_callback( this, lastState );
            break;
         case Enabled:
            mDataBlock->onEnabled_callback( this, lastState );
            break;
         default:
            break;
      }
   }
   updateDamageState();
   updateDamageLevel();
}

bool ShapeBase::setDamageState(const char* state)
{
   for (S32 i = 0; i < NumDamageStates; i++)
      if (!dStricmp(state,sDamageStateName[i])) {
         setDamageState(DamageState(i));
         return true;
      }
   return false;
}

const char* ShapeBase::getDamageStateName()
{
   return sDamageStateName[mDamageState];
}

void ShapeBase::updateDamageState()
{
   if (mHulkThread) {
      F32 pos = (mDamageState == Destroyed) ? 1.0f : 0.0f;
      if (mShapeInstance->getPos(mHulkThread) != pos) {
         mShapeInstance->setPos(mHulkThread,pos);

         if (isClientObject())
            mShapeInstance->animate();
      }
   }
}


//----------------------------------------------------------------------------

void ShapeBase::blowUp()
{
   Point3F center;
   mObjBox.getCenter(&center);
   center += getPosition();
   MatrixF trans = getTransform();
   trans.setPosition( center );

   // explode
   Explosion* pExplosion = NULL;

   if( pointInWater( (Point3F &)center ) && mDataBlock->underwaterExplosion )
   {
      pExplosion = new Explosion;
      pExplosion->onNewDataBlock(mDataBlock->underwaterExplosion, false);
   }
   else
   {
      if (mDataBlock->explosion)
      {
         pExplosion = new Explosion;
         pExplosion->onNewDataBlock(mDataBlock->explosion, false);
      }
   }

   if( pExplosion )
   {
      pExplosion->setTransform(trans);
      pExplosion->setInitialState(center, damageDir);
      if (pExplosion->registerObject() == false)
      {
         Con::errorf(ConsoleLogEntry::General, "ShapeBase(%s)::explode: couldn't register explosion",
                     mDataBlock->getName() );
         delete pExplosion;
         pExplosion = NULL;
      }
   }

   TSShapeInstance *debShape = NULL;

   if( mDataBlock->debrisShape == NULL )
   {
      return;
   }
   else
   {
      debShape = new TSShapeInstance( mDataBlock->debrisShape, true);
   }


   Vector< TSPartInstance * > partList;
   TSPartInstance::breakShape( debShape, 0, partList, NULL, NULL, 0 );

   if( !mDataBlock->debris )
   {
      mDataBlock->debris = new DebrisData;
   }

   // cycle through partlist and create debris pieces
   for( U32 i=0; i<partList.size(); i++ )
   {
      //Point3F axis( 0.0, 0.0, 1.0 );
      Point3F randomDir = MathUtils::randomDir( damageDir, 0, 50 );

      Debris *debris = new Debris;
      debris->setPartInstance( partList[i] );
      debris->init( center, randomDir );
      debris->onNewDataBlock( mDataBlock->debris, false );
      debris->setTransform( trans );

      if( !debris->registerObject() )
      {
         Con::warnf( ConsoleLogEntry::General, "Could not register debris for class: %s", mDataBlock->getName() );
         delete debris;
         debris = NULL;
      }
      else
      {
         debShape->incDebrisRefCount();
      }
   }

   damageDir.set(0, 0, 1);
}

//----------------------------------------------------------------------------

void ShapeBase::onMount( SceneObject *obj, S32 node )
{   
   mConvexList->nukeList();   

   // Are we mounting to a ShapeBase object?
   mShapeBaseMount = dynamic_cast<ShapeBase*>( obj );

   Parent::onMount( obj, node );
}

void ShapeBase::onUnmount( SceneObject *obj, S32 node )
{
   Parent::onUnmount( obj, node );

   mShapeBaseMount = NULL;
}

Point3F ShapeBase::getAIRepairPoint()
{
   if (mDataBlock->mountPointNode[ShapeBaseData::AIRepairNode] < 0)
        return Point3F(0, 0, 0);
   MatrixF xf(true);
   getMountTransform( ShapeBaseData::AIRepairNode, MatrixF::Identity, &xf );
   Point3F pos(0, 0, 0);
   xf.getColumn(3,&pos);
   return pos;
}

//----------------------------------------------------------------------------

void ShapeBase::getEyeTransform(MatrixF* mat)
{
   getEyeBaseTransform(mat);
}

void ShapeBase::getEyeBaseTransform(MatrixF* mat)
{
   // HH: Modified for Tribes 2 compatibility.
   if(!mDataBlock || !mShapeInstance)
   {
      *mat = getTransform();
      return;
   }

   // Returns eye to world space transform
   S32 eyeNode = mDataBlock->eyeNode;
   if (eyeNode != -1)
      mat->mul(getTransform(), mShapeInstance->mNodeTransforms[eyeNode]);
   else
      *mat = getTransform();
}

void ShapeBase::getRenderEyeTransform(MatrixF* mat)
{
   getRenderEyeBaseTransform(mat);
}

void ShapeBase::getRenderEyeBaseTransform(MatrixF* mat)
{
   // HH: Modified for Tribes 2 compatibility.
   if(!mDataBlock || !mShapeInstance)
   {
      *mat = getRenderTransform();
      return;
   }

   // Returns eye to world space transform
   S32 eyeNode = mDataBlock->eyeNode;
   if (eyeNode != -1)
      mat->mul(getRenderTransform(), mShapeInstance->mNodeTransforms[eyeNode]);
   else
      *mat = getRenderTransform();
}

void ShapeBase::getCameraTransform(F32* pos,MatrixF* mat)
{
   // Returns camera to world space transform
   // Handles first person / third person camera position

   if (isServerObject() && mShapeInstance)
      mShapeInstance->animateNodeSubtrees(true);

   if (*pos != 0)
   {
      F32 min,max;
      Point3F offset;
      MatrixF eye,rot;
      getCameraParameters(&min,&max,&offset,&rot);
      getRenderEyeTransform(&eye);
      mat->mul(eye,rot);

      // Use the eye transform to orient the camera
      VectorF vp,vec;
      vp.x = vp.z = 0;
      vp.y = -(max - min) * *pos;
      eye.mulV(vp,&vec);
      
      VectorF minVec;
      vp.y = -min;
      eye.mulV( vp, &minVec );

      // Use the camera node's pos.
      Point3F osp,sp;
      if (mDataBlock->cameraNode != -1) {
         mShapeInstance->mNodeTransforms[mDataBlock->cameraNode].getColumn(3,&osp);

         // Scale the camera position before applying the transform
         const Point3F& scale = getScale();
         osp.convolve( scale );

         getRenderTransform().mulP(osp,&sp);
      }
      else
         getRenderTransform().getColumn(3,&sp);

      // Make sure we don't extend the camera into anything solid
      Point3F ep = sp + minVec + vec + offset;
      disableCollision();
      if (isMounted())
         getObjectMount()->disableCollision();
      RayInfo collision;
      if( mContainer && mContainer->castRay(sp, ep,
                              (0xFFFFFFFF & ~(WaterObjectType      |
                                              GameBaseObjectType   |
                                              TriggerObjectType    |
                                              DefaultObjectType)),
                              &collision) == true) {
         F32 vecLenSq = vec.lenSquared();
         F32 adj = (-mDot(vec, collision.normal) / vecLenSq) * 0.1;
         F32 newPos = getMax(0.0f, collision.t - adj);
         if (newPos == 0.0f)
            eye.getColumn(3,&ep);
         else
            ep = sp + offset + (vec * newPos);
      }
      mat->setColumn(3,ep);
      if (isMounted())
         getObjectMount()->enableCollision();
      enableCollision();
   }
   else
   {
      getRenderEyeTransform(mat);
   }

   // Apply Camera FX.
   mat->mul( gCamFXMgr.getTrans() );
}

// void ShapeBase::getCameraTransform(F32* pos,MatrixF* mat)
// {
//    // Returns camera to world space transform
//    // Handles first person / third person camera position

//    if (isServerObject() && mShapeInstance)
//       mShapeInstance->animateNodeSubtrees(true);

//    if (*pos != 0) {
//       F32 min,max;
//       Point3F offset;
//       MatrixF eye,rot;
//       getCameraParameters(&min,&max,&offset,&rot);
//       getRenderEyeTransform(&eye);
//       mat->mul(eye,rot);

//       // Use the eye transform to orient the camera
//       VectorF vp,vec;
//       vp.x = vp.z = 0;
//       vp.y = -(max - min) * *pos;
//       eye.mulV(vp,&vec);

//       // Use the camera node's pos.
//       Point3F osp,sp;
//       if (mDataBlock->cameraNode != -1) {
//          mShapeInstance->mNodeTransforms[mDataBlock->cameraNode].getColumn(3,&osp);
//          getRenderTransform().mulP(osp,&sp);
//       }
//       else
//          getRenderTransform().getColumn(3,&sp);

//       // Make sure we don't extend the camera into anything solid
//       Point3F ep = sp + vec;
//       ep += offset;
//       disableCollision();
//       if (isMounted())
//          getObjectMount()->disableCollision();
//       RayInfo collision;
//       if (mContainer->castRay(sp,ep,(0xFFFFFFFF & ~(WaterObjectType|ForceFieldObjectType|GameBaseObjectType|DefaultObjectType)),&collision)) {
//          *pos = collision.t *= 0.9;
//          if (*pos == 0)
//             eye.getColumn(3,&ep);
//          else
//             ep = sp + vec * *pos;
//       }
//       mat->setColumn(3,ep);
//       if (isMounted())
//          getObjectMount()->enableCollision();
//       enableCollision();
//    }
//    else
//    {
//       getRenderEyeTransform(mat);
//    }
// }


// void ShapeBase::getRenderCameraTransform(F32* pos,MatrixF* mat)
// {
//    // Returns camera to world space transform
//    // Handles first person / third person camera position

//    if (isServerObject() && mShapeInstance)
//       mShapeInstance->animateNodeSubtrees(true);

//    if (*pos != 0) {
//       F32 min,max;
//       Point3F offset;
//       MatrixF eye,rot;
//       getCameraParameters(&min,&max,&offset,&rot);
//       getRenderEyeTransform(&eye);
//       mat->mul(eye,rot);

//       // Use the eye transform to orient the camera
//       VectorF vp,vec;
//       vp.x = vp.z = 0;
//       vp.y = -(max - min) * *pos;
//       eye.mulV(vp,&vec);

//       // Use the camera node's pos.
//       Point3F osp,sp;
//       if (mDataBlock->cameraNode != -1) {
//          mShapeInstance->mNodeTransforms[mDataBlock->cameraNode].getColumn(3,&osp);
//          getRenderTransform().mulP(osp,&sp);
//       }
//       else
//          getRenderTransform().getColumn(3,&sp);

//       // Make sure we don't extend the camera into anything solid
//       Point3F ep = sp + vec;
//       ep += offset;
//       disableCollision();
//       if (isMounted())
//          getObjectMount()->disableCollision();
//       RayInfo collision;
//       if (mContainer->castRay(sp,ep,(0xFFFFFFFF & ~(WaterObjectType|ForceFieldObjectType|GameBaseObjectType|DefaultObjectType)),&collision)) {
//          *pos = collision.t *= 0.9;
//          if (*pos == 0)
//             eye.getColumn(3,&ep);
//          else
//             ep = sp + vec * *pos;
//       }
//       mat->setColumn(3,ep);
//       if (isMounted())
//          getObjectMount()->enableCollision();
//       enableCollision();
//    }
//    else
//    {
//       getRenderEyeTransform(mat);
//    }
// }

void ShapeBase::getCameraParameters(F32 *min,F32* max,Point3F* off,MatrixF* rot)
{
   *min = mDataBlock->cameraMinDist;
   *max = mDataBlock->cameraMaxDist;
   off->set(0,0,0);
   rot->identity();
}


//----------------------------------------------------------------------------
F32 ShapeBase::getDamageFlash() const
{
   return mDamageFlash;
}

void ShapeBase::setDamageFlash(const F32 flash)
{
   mDamageFlash = flash;
   if (mDamageFlash < 0.0)
      mDamageFlash = 0;
   else if (mDamageFlash > 1.0)
      mDamageFlash = 1.0;
}


//----------------------------------------------------------------------------
F32 ShapeBase::getWhiteOut() const
{
   return mWhiteOut;
}

void ShapeBase::setWhiteOut(const F32 flash)
{
   mWhiteOut = flash;
   if (mWhiteOut < 0.0)
      mWhiteOut = 0;
   else if (mWhiteOut > 1.5)
      mWhiteOut = 1.5;
}


//----------------------------------------------------------------------------

bool ShapeBase::onlyFirstPerson() const
{
   return mDataBlock->firstPersonOnly;
}

bool ShapeBase::useObjsEyePoint() const
{
   return mDataBlock->useEyePoint;
}


//----------------------------------------------------------------------------
F32 ShapeBase::getInvincibleEffect() const
{
   return mInvincibleEffect;
}

void ShapeBase::setupInvincibleEffect(F32 time, F32 speed)
{
   if(isClientObject())
   {
      mInvincibleCount = mInvincibleTime = time;
      mInvincibleSpeed = mInvincibleDelta = speed;
      mInvincibleEffect = 0.0f;
      mInvincibleOn = true;
      mInvincibleFade = 1.0f;
   }
   else
   {
      mInvincibleTime  = time;
      mInvincibleSpeed = speed;
      setMaskBits(InvincibleMask);
   }
}

void ShapeBase::updateInvincibleEffect(F32 dt)
{
   if(mInvincibleCount > 0.0f )
   {
      if(mInvincibleEffect >= ((0.3 * mInvincibleFade) + 0.05f) && mInvincibleDelta > 0.0f)
         mInvincibleDelta = -mInvincibleSpeed;
      else if(mInvincibleEffect <= 0.05f && mInvincibleDelta < 0.0f)
      {
         mInvincibleDelta = mInvincibleSpeed;
         mInvincibleFade = mInvincibleCount / mInvincibleTime;
      }
      mInvincibleEffect += mInvincibleDelta;
      mInvincibleCount -= dt;
   }
   else
   {
      mInvincibleEffect = 0.0f;
      mInvincibleOn = false;
   }
}

//----------------------------------------------------------------------------
void ShapeBase::setVelocity(const VectorF&)
{
}

void ShapeBase::applyImpulse(const Point3F&,const VectorF&)
{
}


//----------------------------------------------------------------------------

void ShapeBase::playAudio(U32 slot,SFXTrack* profile)
{
   AssertFatal( slot < MaxSoundThreads, "ShapeBase::playAudio() bad slot index" );
   Sound& st = mSoundThread[slot];
   if( profile && ( !st.play || st.profile != profile ) ) 
   {
      setMaskBits(SoundMaskN << slot);
      st.play = true;
      st.profile = profile;
      updateAudioState(st);
   }
}

void ShapeBase::stopAudio(U32 slot)
{
   AssertFatal( slot < MaxSoundThreads, "ShapeBase::stopAudio() bad slot index" );

   Sound& st = mSoundThread[slot];
   if ( st.play ) 
   {
      st.play = false;
      setMaskBits(SoundMaskN << slot);
      updateAudioState(st);
   }
}

void ShapeBase::updateServerAudio()
{
   // Timeout non-looping sounds
   for (int i = 0; i < MaxSoundThreads; i++) {
      Sound& st = mSoundThread[i];
      if (st.play && st.timeout && st.timeout < Sim::getCurrentTime()) {
         clearMaskBits(SoundMaskN << i);
         st.play = false;
      }
   }
}

void ShapeBase::updateAudioState(Sound& st)
{
   SFX_DELETE( st.sound );

   if ( st.play && st.profile ) 
   {
      if ( isGhost() ) 
      {
         if ( Sim::findObject( SimObjectId((uintptr_t)st.profile ), st.profile ) )
         {
            st.sound = SFX->createSource( st.profile, &getTransform() );
            if ( st.sound )
               st.sound->play();
         }
         else
            st.play = false;
      }
      else 
      {
         // Non-looping sounds timeout on the server
         st.timeout = 0;
         if ( !st.profile->getDescription()->mIsLooping )
            st.timeout = Sim::getCurrentTime() + sAudioTimeout;
      }
   }
   else
      st.play = false;
}

void ShapeBase::updateAudioPos()
{
   for (int i = 0; i < MaxSoundThreads; i++)
   {
      SFXSource* source = mSoundThread[i].sound;
      if ( source )
         source->setTransform( getTransform() );
   }
}

//----------------------------------------------------------------------------

const char *ShapeBase::getThreadSequenceName( U32 slot )
{
	Thread& st = mScriptThread[slot];
	if ( st.sequence == -1 )
	{
		// Invalid Animation.
		return "";
	}

	// Name Index
	const U32 nameIndex = getShape()->sequences[st.sequence].nameIndex;

	// Return Name.
	return getShape()->getName( nameIndex );
}

bool ShapeBase::setThreadSequence(U32 slot, S32 seq, bool reset)
{
   Thread& st = mScriptThread[slot];
   if (st.thread && st.sequence == seq && st.state == Thread::Play)
      return true;

   // Handle a -1 sequence, as this may be set when a thread has been destroyed.
   if(seq == -1)
      return true;

   if (seq < MaxSequenceIndex) {
      setMaskBits(ThreadMaskN << slot);
      st.sequence = seq;
      if (reset) {
         st.state = Thread::Play;
         st.atEnd = false;
		 st.timescale = 1.f;
		 st.position = 0.f;
      }
      if (mShapeInstance) {
         if (!st.thread)
            st.thread = mShapeInstance->addThread();
         mShapeInstance->setSequence(st.thread,seq,0);
         stopThreadSound(st);
         updateThread(st);
      }
      return true;
   }
   return false;
}

void ShapeBase::updateThread(Thread& st)
{
	switch (st.state)
	{
		case Thread::Stop:
			{
				mShapeInstance->setTimeScale( st.thread, 1.f );
				mShapeInstance->setPos( st.thread, ( st.timescale > 0.f ) ? 0.0f : 1.0f );
			} // Drop through to pause state

		case Thread::Pause:
			{
				if ( st.position != -1.f )
				{
					mShapeInstance->setTimeScale( st.thread, 1.f );
					mShapeInstance->setPos( st.thread, st.position );
				}

				mShapeInstance->setTimeScale( st.thread, 0.f );
				stopThreadSound( st );
			} break;

		case Thread::Play:
			{
				if (st.atEnd)
				{
					mShapeInstance->setTimeScale(st.thread,1);
					mShapeInstance->setPos( st.thread, ( st.timescale > 0.f ) ? 1.0f : 0.0f );
					mShapeInstance->setTimeScale(st.thread,0);
					stopThreadSound(st);
               st.state = Thread::Stop;
				}
				else
				{
					if ( st.position != -1.f )
					{
						mShapeInstance->setTimeScale( st.thread, 1.f );
						mShapeInstance->setPos( st.thread, st.position );
					}

					mShapeInstance->setTimeScale(st.thread, st.timescale );
					if (!st.sound)
					{
						startSequenceSound(st);
					}
				}
			} break;

      case Thread::Destroy:
         {
				stopThreadSound(st);
            st.atEnd = true;
            st.sequence = -1;
            if(st.thread)
            {
               mShapeInstance->destroyThread(st.thread);
               st.thread = 0;
            }
         } break;
	}
}

bool ShapeBase::stopThread(U32 slot)
{
   Thread& st = mScriptThread[slot];
   if (st.sequence != -1 && st.state != Thread::Stop) {
      setMaskBits(ThreadMaskN << slot);
      st.state = Thread::Stop;
      updateThread(st);
      return true;
   }
   return false;
}

bool ShapeBase::destroyThread(U32 slot)
{
   Thread& st = mScriptThread[slot];
   if (st.sequence != -1 && st.state != Thread::Destroy) {
      setMaskBits(ThreadMaskN << slot);
      st.state = Thread::Destroy;
      updateThread(st);
      return true;
   }
   return false;
}

bool ShapeBase::pauseThread(U32 slot)
{
   Thread& st = mScriptThread[slot];
   if (st.sequence != -1 && st.state != Thread::Pause) {
      setMaskBits(ThreadMaskN << slot);
      st.state = Thread::Pause;
      updateThread(st);
      return true;
   }
   return false;
}

bool ShapeBase::playThread(U32 slot)
{
   Thread& st = mScriptThread[slot];
   if (st.sequence != -1 && st.state != Thread::Play) {
      setMaskBits(ThreadMaskN << slot);
      st.state = Thread::Play;
      updateThread(st);
      return true;
   }
   return false;
}

bool ShapeBase::setThreadPosition( U32 slot, F32 pos )
{
	Thread& st = mScriptThread[slot];
	if (st.sequence != -1)
	{
		setMaskBits(ThreadMaskN << slot);
		st.position = pos;
		st.atEnd = false;
		updateThread(st);

		return true;
	}
	return false;
}

bool ShapeBase::setThreadDir(U32 slot,bool forward)
{
	Thread& st = mScriptThread[slot];
	if (st.sequence != -1)
	{
		if ( ( st.timescale >= 0.f ) != forward )
		{
			setMaskBits(ThreadMaskN << slot);
			st.timescale *= -1.f ;
			st.atEnd = false;
			updateThread(st);
		}
		return true;
	}
	return false;
}

bool ShapeBase::setThreadTimeScale( U32 slot, F32 timeScale )
{
	Thread& st = mScriptThread[slot];
	if (st.sequence != -1)
	{
		if (st.timescale != timeScale)
		{
			setMaskBits(ThreadMaskN << slot);
			st.timescale = timeScale;
			updateThread(st);
		}
		return true;
	}
	return false;
}

void ShapeBase::stopThreadSound(Thread& thread)
{
   if (thread.sound) {
   }
}

void ShapeBase::startSequenceSound(Thread& thread)
{
   if (!isGhost() || !thread.thread)
      return;
   stopThreadSound(thread);
}

void ShapeBase::advanceThreads(F32 dt)
{
   for (U32 i = 0; i < MaxScriptThreads; i++) {
      Thread& st = mScriptThread[i];
      if (st.thread) {
         if (!mShapeInstance->getShape()->sequences[st.sequence].isCyclic() && !st.atEnd &&
			 ( ( st.timescale > 0.f )? mShapeInstance->getPos(st.thread) >= 1.0:
              mShapeInstance->getPos(st.thread) <= 0)) {
            st.atEnd = true;
            updateThread(st);
            if (!isGhost()) {
               mDataBlock->onEndSequence_callback( this, i );
            }
         }

         // Make sure the thread is still valid after the call to onEndSequence_callback().
         // Someone could have called destroyThread() while in there.
         if(st.thread)
         {
            mShapeInstance->advanceTime(dt,st.thread);
         }
      }
   }
}

//----------------------------------------------------------------------------

/// Emit particles on the given emitter, if we're within triggerHeight above some static surface with a
/// material that has 'showDust' set to true.  The particles will have a lifetime of 'numMilliseconds'
/// and be emitted at the given offset from the contact point having a direction of 'axis'.

void ShapeBase::emitDust( ParticleEmitter* emitter, F32 triggerHeight, const Point3F& offset, U32 numMilliseconds, const Point3F& axis )
{
   if( !emitter )
      return;

   Point3F startPos = getPosition();
   Point3F endPos = startPos + Point3F( 0.0, 0.0, - triggerHeight );

   RayInfo rayInfo;
   if( getContainer()->castRay( startPos, endPos, STATIC_COLLISION_TYPEMASK, &rayInfo ) )
   {
      Material* material = ( rayInfo.material ? dynamic_cast< Material* >( rayInfo.material->getMaterial() ) : 0 );
      if( material && material->mShowDust )
      {
         ColorF colorList[ ParticleData::PDC_NUM_KEYS ];

         for( U32 x = 0; x < getMin( Material::NUM_EFFECT_COLOR_STAGES, ParticleData::PDC_NUM_KEYS ); ++ x )
            colorList[ x ] = material->mEffectColor[ x ];
         for( U32 x = Material::NUM_EFFECT_COLOR_STAGES; x < ParticleData::PDC_NUM_KEYS; ++ x )
            colorList[ x ].set( 1.0, 1.0, 1.0, 0.0 );

         emitter->setColors( colorList );

         Point3F contactPoint = rayInfo.point + offset;
         emitter->emitParticles( contactPoint, true, ( axis == Point3F::Zero ? rayInfo.normal : axis ),
                                 getVelocity(), numMilliseconds );
      }
   }
}

//----------------------------------------------------------------------------

TSShape const* ShapeBase::getShape()
{
   return mShapeInstance? mShapeInstance->getShape(): 0;
}

void ShapeBase::prepRenderImage( SceneRenderState *state )
{
   _prepRenderImage( state, true, true );
}

void ShapeBase::_prepRenderImage(   SceneRenderState *state, 
                                    bool renderSelf, 
                                    bool renderMountedImages )
{
   PROFILE_SCOPE( ShapeBase_PrepRenderImage );

   //if ( mIsCubemapUpdate )
   //   return false;

   if( ( getDamageState() == Destroyed ) && ( !mDataBlock->renderWhenDestroyed ) )
      return;

   // We don't need to render if all the meshes are forced hidden.
   if ( mMeshHidden.testAll() )   
      return;
      
   // If we're rendering shadows don't render the mounted
   // images unless the shape is also rendered.
   if ( state->isShadowPass() && !renderSelf )
      return;

   // HH: Added for Tribes 2 compatibility.
   // Tribes 2 casts no shadow at all while a cloak is on, however faint
   // (0x005ed9ac, and Player's own render at 0x005dd227).
   if ( state->isShadowPass() && mCloakLevel != 0.0f )
      return;

   // If we're currently rendering our own reflection we
   // don't want to render ourselves into it.
   if ( mCubeReflector.isRendering() )
      return;

   mLastRenderFrame = sLastRenderFrame;

   // HH: Added for Tribes 2 compatibility.
   // Split out for the shield's redraw.
   _selectRenderDetail( state );

   if (mShapeInstance)
      mShapeInstance->animate();
   
   if (  ( mShapeInstance && mShapeInstance->getCurrentDetail() < 0 ) ||
         ( !mShapeInstance && !gShowBoundingBox ) ) 
   {
      // no, don't draw anything
      return;
   }

   if( renderMountedImages )
   {
      for (U32 i = 0; i < MaxMountedImages; i++)
      {
         MountedImage& image = mMountedImageList[i];
         U32 imageShapeIndex = getImageShapeIndex(image);
         if (image.dataBlock && image.shapeInstance[imageShapeIndex])
         {
            // HH: Modified for Tribes 2 compatibility.
            if (!mIsZero( mFadeVal ))
            {
               prepBatchRender( state, i );

               // Debug rendering of the mounted shape bounds.
               if ( gShowBoundingBox )
               {
                  ObjectRenderInst *ri = state->getRenderPass()->allocInst<ObjectRenderInst>();
                  ri->renderDelegate.bind( this, &ShapeBase::_renderBoundingBox );
                  ri->objectIndex = i;
                  ri->type = RenderPassManager::RIT_Editor;
                  state->getRenderPass()->addInst( ri );
               }
            }
         }
      }
   }

   // Debug rendering of the shape bounding box.
   if ( gShowBoundingBox )
   {
      ObjectRenderInst *ri = state->getRenderPass()->allocInst<ObjectRenderInst>();
      ri->renderDelegate.bind( this, &ShapeBase::_renderBoundingBox );
      ri->objectIndex = -1;
      ri->type = RenderPassManager::RIT_Editor;
      state->getRenderPass()->addInst( ri );
   }

   if ( mShapeInstance && renderSelf )
      prepBatchRender( state, -1 );

   // HH: Added for Tribes 2 compatibility.
   // Retail's prepRenderImage (0x005ec95e) adds this without asking whether
   // the shape itself is drawn. A first-person eye is inside the bubble,
   // where every face it could see is a back face. Which of the two it
   // draws is asked each time it draws (0x00694040).
   if ( mShieldImpact.isActive() && state->isDiffusePass() )
   {
      if ( ShieldImpactT2::smUseOldShieldEffect )
         _prepShieldImpactRedraw( state );
      else
      {
         ObjectRenderInst *ri = state->getRenderPass()->allocInst<ObjectRenderInst>();
         ri->renderDelegate.bind( this, &ShapeBase::_renderShieldImpact );
         // The bin the port's other immediate-mode effects draw in. The
         // translucent object type is taken by two bins of the default
         // pass, which would add the flash in twice.
         ri->type = RenderPassManager::RIT_Foliage;
         state->getRenderPass()->addInst( ri );
      }
   }

   calcClassRenderData();
}

// HH: Added for Tribes 2 compatibility.
void ShapeBase::_selectRenderDetail( SceneRenderState *state )
{
   // We force all the shapes to use the highest detail
   // if we're the control object or mounted.
   bool forceHighestDetail = false;
   {
      GameConnection *con = GameConnection::getConnectionToServer();
      ShapeBase *co = NULL;
      if(con && ( (co = dynamic_cast<ShapeBase*>(con->getControlObject())) != NULL) )
      {
         if(co == this || co->getObjectMount() == this)
            forceHighestDetail = true;
      }
   }

   // get shape detail...we might not even need to be drawn
   Point3F cameraOffset = getWorldBox().getClosestPoint( state->getDiffuseCameraPosition() ) - state->getDiffuseCameraPosition();
   F32 dist = cameraOffset.len();
   if (dist < 0.01f)
      dist = 0.01f;

   F32 invScale = (1.0f/getMax(getMax(mObjScale.x,mObjScale.y),mObjScale.z));

   if (mShapeInstance)
   {
      if ( forceHighestDetail )         
         mShapeInstance->setCurrentDetail( 0 );
      else
         mShapeInstance->setDetailFromDistance( state, dist * invScale );
   }

   for (U32 i = 0; i < MaxMountedImages; i++)
   {
      MountedImage& image = mMountedImageList[i];
      U32 imageShapeIndex = getImageShapeIndex(image);
      if (image.dataBlock && image.shapeInstance[imageShapeIndex])
      {
         // Select detail levels on mounted items but... always 
         // draw the control object's mounted images in high detail.
         if ( forceHighestDetail )
            image.shapeInstance[imageShapeIndex]->setCurrentDetail( 0 );
         else
            image.shapeInstance[imageShapeIndex]->setDetailFromDistance( state, dist * invScale );
      }
   }
}

// HH: Added for Tribes 2 compatibility.
void ShapeBase::_renderShieldImpact( ObjectRenderInst *ri, SceneRenderState *state, BaseMatInstance *overrideMat )
{
   if ( overrideMat )
      return;

   mShieldImpact.render( this, state );
}

// HH: Added for Tribes 2 compatibility.
// Retail's hi-res effect (0x00694060) draws this shape and its images, each
// shape mounted on it and its images, then this shape's images once more.
void ShapeBase::_prepShieldImpactRedraw( SceneRenderState *state )
{
   // Near full strength it is drawn solid and lerps by reflectance alone
   // (0x0069442c).
   const F32 alpha = mShieldImpact.getAlpha();
   const bool blended = alpha < 0.99f;
   BaseMatInstance *matInst = ShieldImpactT2::getRedrawMaterial( blended, false );
   BaseMatInstance *selfIllumMatInst = ShieldImpactT2::getRedrawMaterial( blended, true );
   if ( !matInst || !selfIllumMatInst )
      return;

   const F32 fade = blended ? alpha : 1.0f;
   const F32 growth = mShieldImpact.getGrowth();

   _prepShieldRedraw( state, matInst, selfIllumMatInst, fade, growth, true );

   for ( SceneObject *obj = getMountList(); obj; obj = obj->getMountLink() )
   {
      ShapeBase *shape = dynamic_cast<ShapeBase*>( obj );
      if ( !shape )
         continue;

      // The scene may not have reached it yet this frame. Drawn at the
      // detail it is about to pick, its own draw can share the skin this
      // one leaves.
      shape->_selectRenderDetail( state );
      shape->_prepShieldRedraw( state, matInst, selfIllumMatInst, fade, growth, true );
   }

   _prepShieldRedraw( state, matInst, selfIllumMatInst, fade, growth, false );
}

namespace
{
   /// Retail draws every part at the top detail (0x006944f8). A part the
   /// scene draws at another detail this frame stays at it, as its meshes
   /// share one skin between details; one it does not draw at all takes the
   /// top one just for this.
   void renderShieldRedraw( TSShapeInstance *shapeInst, const TSRenderState &rdata )
   {
      const S32 detail = shapeInst->getCurrentDetail();
      const F32 intraDetail = shapeInst->getCurrentIntraDetail();
      if ( detail < 0 )
         shapeInst->setCurrentDetail( 0 );

      // The scene may not have animated it yet this frame, and this draw
      // marks its skin up to date.
      shapeInst->animate();
      shapeInst->render( rdata );

      if ( detail < 0 )
         shapeInst->setCurrentDetail( detail, intraDetail );
   }
}

// HH: Added for Tribes 2 compatibility.
void ShapeBase::_prepShieldRedraw( SceneRenderState *state,
                                   BaseMatInstance *matInst,
                                   BaseMatInstance *selfIllumMatInst,
                                   F32 fade, F32 growth, bool drawShape )
{
   GFXTransformSaver saver;

   TSRenderState rdata;
   rdata.setSceneState( state );
   rdata.setFadeOverride( fade );
   rdata.setOverrideMaterial( matInst, selfIllumMatInst );

   LightQuery query;
   query.init( getWorldSphere() );
   rdata.setLightQuery( &query );

   // Each grows about its own origin, over the object's own scale.
   const Point3F scale = mObjScale * growth;

   for ( U32 i = 0; i < MaxMountedImages; i++ )
   {
      MountedImage &image = mMountedImageList[i];
      if ( !image.dataBlock )
         continue;

      TSShapeInstance *shapeInst = image.shapeInstance[ getImageShapeIndex( image ) ];
      if ( !shapeInst )
         continue;

      MatrixF mat;
      getRenderImageTransform( i, &mat );
      mat.scale( scale );
      GFX->setWorldMatrix( mat );
      renderShieldRedraw( shapeInst, rdata );
   }

   if ( drawShape && mShapeInstance )
   {
      MatrixF mat = getRenderTransform();
      mat.scale( scale );
      GFX->setWorldMatrix( mat );
      renderShieldRedraw( mShapeInstance, rdata );
   }
}

// HH: Added for Tribes 2 compatibility.
// Tribes 2 draws a cloaked shape in special/cloakTexture, nudged across
// itself one step each time a cloaked shape is drawn, wrapping at 128 across
// and 127 down -- so the shimmer runs at the frame rate, not the clock.  The
// step is the draw's own texture matrix, so two cloaked shapes in a frame
// show two offsets; a material's scroll is only read when the bins draw,
// after every shape is prepared, so each draw in a frame takes a material of
// its own.  Player (0x005dcc00) and ShapeBase (0x005ed5c4) each keep their
// own count.  Alpha-always is only on below 0.99 (0x005dcd36), and above it
// the texture is laid over the shape opaque.  A cloaked shape that is also
// fading blends as any fading shape does (0x006a702d), and Player's has no
// haze at all (0x005dd034).
namespace
{
   enum CloakKind
   {
      CloakOpaque,
      CloakTranslucent,
      CloakFadedOpaque,
      CloakFadedTranslucent,
      NumCloakKinds
   };

   struct CloakDraws
   {
      Vector< SimObjectPtr<Material> > mats[NumCloakKinds];
      U32 used[NumCloakKinds];
      U32 frame;
      U32 across;
      U32 down;
   };
}

static Material *getCloakMaterial( bool player, bool translucent, bool faded,
                                   bool step )
{
   static CloakDraws sDraws[2];
   CloakDraws &draws = sDraws[player ? 1 : 0];

   if ( draws.frame != GuiTSCtrl::getFrameCount() )
   {
      draws.frame = GuiTSCtrl::getFrameCount();
      dMemset( draws.used, 0, sizeof( draws.used ) );
   }

   if ( step )
   {
      draws.across = ( draws.across + 1 ) & 0x7f;
      draws.down = ( draws.down + 1 ) % 127;
   }

   const U32 kind = faded ? ( translucent ? CloakFadedTranslucent : CloakFadedOpaque )
                          : ( translucent ? CloakTranslucent : CloakOpaque );
   Vector< SimObjectPtr<Material> > &mats = draws.mats[kind];
   U32 &used = draws.used[kind];
   if ( used == mats.size() )
      mats.increment();

   SimObjectPtr<Material> &cloak = mats[used++];
   if ( cloak.isNull() )
   {
      Material *mat = MATMGR->allocateAndRegister( String::ToString(
         "%s%s%s%d", player ? "T2PlayerCloakMaterial" : "T2CloakMaterial",
         faded ? "Faded" : "", translucent ? "" : "Opaque", used ) );
      if ( mat )
      {
         mat->mAutoGenerated = true;
         mat->mDiffuseMapFilename[0] = "textures/special/cloakTexture";
         mat->mTranslucent = translucent || faded;
         mat->mTranslucentBlendOp = ( faded && translucent ) ? Material::AddAlpha
                                                             : Material::LerpAlpha;
         mat->mTranslucentZWrite = faded && !translucent;
         mat->mT2FadeAlpha = faded;
         mat->mT2NoFog = player;
         mat->mAnimFlags[0] = Material::Scroll;
         mat->mScrollSpeed[0] = 0.0f;
      }
      cloak = mat;
   }

   if ( cloak )
      cloak->mScrollOffset[0].set( F32( draws.across ) / 127.0f,
                                   F32( draws.down ) / 126.0f );
   return cloak;
}

// HH: Added for Tribes 2 compatibility.
// ShapeBase's render thins a cloaked shape to an eighth (0x005ed6bb) and
// an image that allows it to 0.15 (0x005ed363).
F32 ShapeBase::getCloakAlpha( bool image ) const
{
   if ( image )
      return 0.15f + ( 1.0f - mCloakLevel ) * 0.85f;
   return 0.125f + ( 1.0f - mCloakLevel ) * 0.875f;
}

//----------------------------------------------------------------------------
// prepBatchRender
//----------------------------------------------------------------------------
void ShapeBase::prepBatchRender(SceneRenderState* state, S32 mountedImageIndex )
{
   // CHANGES IN HERE SHOULD BE DUPLICATED IN TSSTATIC!
   
   GFXTransformSaver saver;
   
   // Set up our TS render state. 
   TSRenderState rdata;
   rdata.setSceneState( state );
   if ( mCubeReflector.isEnabled() )
      rdata.setCubemap( mCubeReflector.getCubemap() );
   // HH: Modified for Tribes 2 compatibility.
   // A cloak is drawn, not hidden: the shape in the cloak texture, and an
   // image that allows it blended in its own.  A shape fading in or out is
   // blended with the fade alone as its alpha, the body (0x005dd03f) and the
   // images (0x005ecd94) alike.  Its shadow is cast whole and thinned only
   // as the decal is drawn (0x006538db).
   const bool fadeAlpha = mFadeVal < 1.0f && state->isDiffusePass();
   rdata.setFadeOverride( state->isShadowPass() ? 1.0f : mFadeVal );
   rdata.setFadeAlpha( fadeAlpha );

   // We might have some forward lit materials
   // so pass down a query to gather lights.
   LightQuery query;
   query.init( getWorldSphere() );
   rdata.setLightQuery( &query );

   if( mountedImageIndex != -1 )
   {
      MountedImage& image = mMountedImageList[mountedImageIndex];

      if( image.dataBlock && image.shapeInstance )
      {
         // HH: Added for Tribes 2 compatibility.
         // An image keeps its own textures and is only thinned, and only
         // blended at all below 0.99 (0x005ed363, 0x005dc8ec).
         if ( mCloakLevel != 0.0f && image.dataBlock->cloakable )
         {
            const F32 alpha = getCloakAlpha( true );
            const bool blend = alpha < 0.99f;
            rdata.setFadeOverride( fadeAlpha ? mFadeVal
                                             : ( blend ? alpha : 1.0f ) * mFadeVal );
            rdata.setBlendAlways( blend );
         }
         renderMountedImage( mountedImageIndex, rdata, state );
      }
   }
   else
   {
      MatrixF mat = getRenderTransform();
      mat.scale( mObjScale );
      GFX->setWorldMatrix( mat );

      if ( state->isDiffusePass() && mCubeReflector.isEnabled() && mCubeReflector.getOcclusionQuery() )
      {
         RenderPassManager *pass = state->getRenderPass();

         OccluderRenderInst *ri = pass->allocInst<OccluderRenderInst>();   

         ri->type = RenderPassManager::RIT_Occluder;
         ri->query = mCubeReflector.getOcclusionQuery();   
         mObjToWorld.mulP( mObjBox.getCenter(), &ri->position );
         ri->scale.set( mObjBox.getExtents() );
         ri->orientation = pass->allocUniqueXform( mObjToWorld );        
         ri->isSphere = false;
         state->getRenderPass()->addInst( ri );
      }

      // HH: Added for Tribes 2 compatibility.
      if ( mCloakLevel != 0.0f )
      {
         const bool player = ( getTypeMask() & PlayerObjectType ) != 0;
         const F32 alpha = getCloakAlpha( false );
         const bool blend = alpha < 0.99f;
         Material *cloak = getCloakMaterial( player, blend, fadeAlpha,
                                             state->isDiffusePass() );
         if ( cloak )
         {
            rdata.setMaterialOverride( cloak );
            rdata.setFadeOverride( fadeAlpha ? mFadeVal
                                             : ( blend ? alpha : 1.0f ) * mFadeVal );
         }

         // Player's render turns decals off around a cloaked body
         // (0x005dcd0f to 0x005dd12f); ShapeBase's never does.
         rdata.setNoDecals( player );
      }

      mShapeInstance->animate();
      mShapeInstance->render( rdata );
   }
}

void ShapeBase::renderMountedImage( U32 imageSlot, TSRenderState &rstate, SceneRenderState *state )
{
   GFX->pushWorldMatrix();

   MatrixF mat;
   getRenderImageTransform(imageSlot, &mat, rstate.getSceneState()->isShadowPass());
   GFX->setWorldMatrix( mat );

   MountedImage& image = mMountedImageList[imageSlot];
   U32 imageShapeIndex = getImageShapeIndex(image);
   image.shapeInstance[imageShapeIndex]->animate();
   image.shapeInstance[imageShapeIndex]->render( rstate );

   GFX->popWorldMatrix();
}

void ShapeBase::_renderBoundingBox( ObjectRenderInst *ri, SceneRenderState *state, BaseMatInstance *overrideMat )
{
   // If we got an override mat then this some
   // special rendering pass... skip out of it.
   if ( overrideMat )
      return;

   GFXStateBlockDesc desc;
   desc.setZReadWrite( true, false );
   desc.setBlend( true );
   desc.fillMode = GFXFillWireframe;

   GFXDrawUtil *drawer = GFX->getDrawUtil();

   if ( ri->objectIndex != -1 )
   {
      MountedImage &image = mMountedImageList[ ri->objectIndex ];

      if ( image.shapeInstance )
      {
         MatrixF mat;
         getRenderImageTransform( ri->objectIndex, &mat );         

         const Box3F &objBox = image.shapeInstance[getImageShapeIndex(image)]->getShape()->bounds;

         drawer->drawCube( desc, objBox, ColorI( 255, 255, 255 ), &mat );
      }
   }
   else   
      drawer->drawCube( desc, mObjBox, ColorI( 255, 255, 255 ), &mRenderObjToWorld );
}

bool ShapeBase::castRay(const Point3F &start, const Point3F &end, RayInfo* info)
{
   if (mShapeInstance) 
   {
      RayInfo shortest;
      shortest.t = 1e8;

      info->object = NULL;
      for (U32 i = 0; i < mDataBlock->LOSDetails.size(); i++)
      {
         mShapeInstance->animate(mDataBlock->LOSDetails[i]);
         if (mShapeInstance->castRay(start, end, info, mDataBlock->LOSDetails[i]))
         {
            info->object = this;
            if (info->t < shortest.t)
               shortest = *info;
         }
      }

      if (info->object == this) 
      {
         // Copy out the shortest time...
         *info = shortest;
         return true;
      }
   }

   return false;
}

bool ShapeBase::castRayRendered(const Point3F &start, const Point3F &end, RayInfo* info)
{
   if (mShapeInstance) 
   {
      RayInfo localInfo;
      mShapeInstance->animate();
      bool res = mShapeInstance->castRayRendered(start, end, &localInfo, mShapeInstance->getCurrentDetail());
      if (res)
      {
         *info = localInfo;
         info->object = this;
         return true;
      }
   }

   return false;
}


//----------------------------------------------------------------------------

bool ShapeBase::buildPolyList(PolyListContext context, AbstractPolyList* polyList, const Box3F &, const SphereF &)
{
   if ( !mShapeInstance )
      return false;

   polyList->setTransform(&mObjToWorld, mObjScale);
   polyList->setObject(this);

   if ( context == PLC_Selection )
   {
      mShapeInstance->animate();
      mShapeInstance->buildPolyList(polyList,mShapeInstance->getCurrentDetail());
      return true;
   }
   else if ( context == PLC_Export )
   {
      // Try to call on the client so we can export materials
      ShapeBase* exportObj = this;
      if ( isServerObject() && getClientObject() )
         exportObj = dynamic_cast<ShapeBase*>(getClientObject());

      S32 dl = 0;
      exportObj->mShapeInstance->animate();
      exportObj->mShapeInstance->buildPolyList(polyList, dl);
      return true;
   }
   else
   {
      bool ret = false;
      for (U32 i = 0; i < mDataBlock->collisionDetails.size(); i++)
      {
         mShapeInstance->buildPolyList(polyList,mDataBlock->collisionDetails[i]);
         ret = true;
      }

      return ret;
   }
}

void ShapeBase::buildConvex(const Box3F& box, Convex* convex)
{
   if (mShapeInstance == NULL)
      return;

   // These should really come out of a pool
   mConvexList->collectGarbage();

   Box3F realBox = box;
   mWorldToObj.mul(realBox);
   realBox.minExtents.convolveInverse(mObjScale);
   realBox.maxExtents.convolveInverse(mObjScale);

   if (realBox.isOverlapped(getObjBox()) == false)
      return;

   for (U32 i = 0; i < mDataBlock->collisionDetails.size(); i++)
   {
         Box3F newbox = mDataBlock->collisionBounds[i];
         newbox.minExtents.convolve(mObjScale);
         newbox.maxExtents.convolve(mObjScale);
         mObjToWorld.mul(newbox);
         if (box.isOverlapped(newbox) == false)
            continue;

         // See if this hull exists in the working set already...
         Convex* cc = 0;
         CollisionWorkingList& wl = convex->getWorkingList();
         for (CollisionWorkingList* itr = wl.wLink.mNext; itr != &wl; itr = itr->wLink.mNext) {
            if (itr->mConvex->getType() == ShapeBaseConvexType &&
                (static_cast<ShapeBaseConvex*>(itr->mConvex)->pShapeBase == this &&
                 static_cast<ShapeBaseConvex*>(itr->mConvex)->hullId     == i)) {
               cc = itr->mConvex;
               break;
            }
         }
         if (cc)
            continue;

         // Create a new convex.
         ShapeBaseConvex* cp = new ShapeBaseConvex;
         mConvexList->registerObject(cp);
         convex->addToWorkingList(cp);
         cp->mObject    = this;
         cp->pShapeBase = this;
         cp->hullId     = i;
         cp->box        = mDataBlock->collisionBounds[i];
         cp->transform = 0;
         cp->findNodeTransform();
   }
}


//----------------------------------------------------------------------------

void ShapeBase::queueCollision( SceneObject *obj, const VectorF &vec)
{
   // Add object to list of collisions.
   SimTime time = Sim::getCurrentTime();
   S32 num = obj->getId();

   CollisionTimeout** adr = &mTimeoutList;
   CollisionTimeout* ptr = mTimeoutList;
   while (ptr) {
      if (ptr->objectNumber == num) {
         if (ptr->expireTime < time) {
            ptr->expireTime = time + CollisionTimeoutValue;
            ptr->object = obj;
            ptr->vector = vec;
         }
         return;
      }
      // Recover expired entries
      if (ptr->expireTime < time) {
         CollisionTimeout* cur = ptr;
         *adr = ptr->next;
         ptr = ptr->next;
         cur->next = sFreeTimeoutList;
         sFreeTimeoutList = cur;
      }
      else {
         adr = &ptr->next;
         ptr = ptr->next;
      }
   }

   // New entry for the object
   if (sFreeTimeoutList != NULL)
   {
      ptr = sFreeTimeoutList;
      sFreeTimeoutList = ptr->next;
      ptr->next = NULL;
   }
   else
   {
      ptr = sTimeoutChunker.alloc();
   }

   ptr->object = obj;
   ptr->objectNumber = obj->getId();
   ptr->vector = vec;
   ptr->expireTime = time + CollisionTimeoutValue;
   ptr->next = mTimeoutList;

   mTimeoutList = ptr;
}

void ShapeBase::notifyCollision()
{
   // Notify all the objects that were just stamped during the queueing
   // process.
   SimTime expireTime = Sim::getCurrentTime() + CollisionTimeoutValue;
   for (CollisionTimeout* ptr = mTimeoutList; ptr; ptr = ptr->next)
   {
      if (ptr->expireTime == expireTime && ptr->object)
      {
         SimObjectPtr<SceneObject> safePtr(ptr->object);
         SimObjectPtr<ShapeBase> safeThis(this);
         onCollision(ptr->object,ptr->vector);
         ptr->object = 0;

         if(!bool(safeThis))
            return;

         if(bool(safePtr))
            safePtr->onCollision(this,ptr->vector);

         if(!bool(safeThis))
            return;
      }
   }
}

void ShapeBase::onCollision( SceneObject *object, const VectorF &vec )
{
   if (!isGhost())
      mDataBlock->onCollision_callback( this, object, vec, vec.len() );
}

//--------------------------------------------------------------------------
bool ShapeBase::pointInWater( Point3F &point )
{
   if ( mCurrentWaterObject == NULL )
      return false;

   return mCurrentWaterObject->isUnderwater( point );   
}

//----------------------------------------------------------------------------

void ShapeBase::writePacketData(GameConnection *connection, BitStream *stream)
{
   Parent::writePacketData(connection, stream);

   stream->write(getEnergyLevel());
   stream->write(mRechargeRate);
}

void ShapeBase::readPacketData(GameConnection *connection, BitStream *stream)
{
   Parent::readPacketData(connection, stream);

   F32 energy;
   stream->read(&energy);
   setEnergyLevel(energy);

   stream->read(&mRechargeRate);
}

F32 ShapeBase::getUpdatePriority(CameraScopeQuery *camInfo, U32 updateMask, S32 updateSkips)
{
   // If it's the scope object, must be high priority
   if (camInfo->camera == this) {
      // Most priorities are between 0 and 1, so this
      // should be something larger.
      return 10.0f;
   }
   if( camInfo->camera )
   {
      ShapeBase* camera = dynamic_cast< ShapeBase* >( camInfo->camera );
      // see if the camera is mounted to this...
      // if it is, this should have a high priority
      if( camera && camera->getObjectMount() == this)
         return 10.0f;
   }
   return Parent::getUpdatePriority(camInfo, updateMask, updateSkips);
}




//--------------------------------------------------------------------------

void ShapeBase::forceUncloak(const char * reason)
{
   AssertFatal(isServerObject(), "ShapeBase::forceUncloak: server only call");
   if(!mCloaked)
      return;

   mDataBlock->onForceUncloak_callback( this, reason ? reason : "" );
}

void ShapeBase::setCloakedState(bool cloaked)
{
   if (cloaked == mCloaked)
      return;

   if (isServerObject())
      setMaskBits(CloakMask);

   // Have to do this for the client, if we are ghosted over in the initial
   //  packet as cloaked, we set the state immediately to the extreme
   if (isProperlyAdded() == false) {
      mCloaked = cloaked;
      if (mCloaked)
         mCloakLevel = 1.0;
      else
         mCloakLevel = 0.0;
   } else {
      mCloaked = cloaked;
   }
}


//--------------------------------------------------------------------------

void ShapeBase::setHidden( bool hidden )
{
   if( hidden != isHidden() )
   {
      Parent::setHidden( hidden );

      if( hidden )
         setProcessTick( false );
      else
         setProcessTick( true );
   }
}

//--------------------------------------------------------------------------

void ShapeBaseConvex::findNodeTransform()
{
   S32 dl = pShapeBase->mDataBlock->collisionDetails[hullId];

   TSShapeInstance* si = pShapeBase->getShapeInstance();
   TSShape* shape = si->getShape();

   const TSShape::Detail* detail = &shape->details[dl];
   const S32 subs = detail->subShapeNum;
   const S32 start = shape->subShapeFirstObject[subs];
   const S32 end = start + shape->subShapeNumObjects[subs];

   // Find the first object that contains a mesh for this
   // detail level. There should only be one mesh per
   // collision detail level.
   for (S32 i = start; i < end; i++) 
   {
      const TSShape::Object* obj = &shape->objects[i];
      if (obj->numMeshes && detail->objectDetailNum < obj->numMeshes) 
      {
         nodeTransform = &si->mNodeTransforms[obj->nodeIndex];
         return;
      }
   }
   return;
}

const MatrixF& ShapeBaseConvex::getTransform() const
{
   // If the transform isn't specified, it's assumed to be the
   // origin of the shape.
   const MatrixF& omat = (transform != 0)? *transform: mObject->getTransform();

   // Multiply on the mesh shape offset
   // tg: Returning this static here is not really a good idea, but
   // all this Convex code needs to be re-organized.
   if (nodeTransform) {
      static MatrixF mat;
      mat.mul(omat,*nodeTransform);
      return mat;
   }
   return omat;
}

Box3F ShapeBaseConvex::getBoundingBox() const
{
   const MatrixF& omat = (transform != 0)? *transform: mObject->getTransform();
   return getBoundingBox(omat, mObject->getScale());
}

Box3F ShapeBaseConvex::getBoundingBox(const MatrixF& mat, const Point3F& scale) const
{
   Box3F newBox = box;
   newBox.minExtents.convolve(scale);
   newBox.maxExtents.convolve(scale);
   mat.mul(newBox);
   return newBox;
}

Point3F ShapeBaseConvex::support(const VectorF& v) const
{
   TSShape::ConvexHullAccelerator* pAccel =
      pShapeBase->mShapeInstance->getShape()->getAccelerator(pShapeBase->mDataBlock->collisionDetails[hullId]);
   AssertFatal(pAccel != NULL, "Error, no accel!");

   F32 currMaxDP = mDot(pAccel->vertexList[0], v);
   U32 index = 0;
   for (U32 i = 1; i < pAccel->numVerts; i++) {
      F32 dp = mDot(pAccel->vertexList[i], v);
      if (dp > currMaxDP) {
         currMaxDP = dp;
         index = i;
      }
   }

   return pAccel->vertexList[index];
}


void ShapeBaseConvex::getFeatures(const MatrixF& mat, const VectorF& n, ConvexFeature* cf)
{
   cf->material = 0;
   cf->object = mObject;

   TSShape::ConvexHullAccelerator* pAccel =
      pShapeBase->mShapeInstance->getShape()->getAccelerator(pShapeBase->mDataBlock->collisionDetails[hullId]);
   AssertFatal(pAccel != NULL, "Error, no accel!");

   F32 currMaxDP = mDot(pAccel->vertexList[0], n);
   U32 index = 0;
   U32 i;
   for (i = 1; i < pAccel->numVerts; i++) {
      F32 dp = mDot(pAccel->vertexList[i], n);
      if (dp > currMaxDP) {
         currMaxDP = dp;
         index = i;
      }
   }

   const U8* emitString = pAccel->emitStrings[index];
   U32 currPos = 0;
   U32 numVerts = emitString[currPos++];
   for (i = 0; i < numVerts; i++) {
      cf->mVertexList.increment();
      U32 index = emitString[currPos++];
      mat.mulP(pAccel->vertexList[index], &cf->mVertexList.last());
   }

   U32 numEdges = emitString[currPos++];
   for (i = 0; i < numEdges; i++) {
      U32 ev0 = emitString[currPos++];
      U32 ev1 = emitString[currPos++];
      cf->mEdgeList.increment();
      cf->mEdgeList.last().vertex[0] = ev0;
      cf->mEdgeList.last().vertex[1] = ev1;
   }

   U32 numFaces = emitString[currPos++];
   for (i = 0; i < numFaces; i++) {
      cf->mFaceList.increment();
      U32 plane = emitString[currPos++];
      mat.mulV(pAccel->normalList[plane], &cf->mFaceList.last().normal);
      for (U32 j = 0; j < 3; j++)
         cf->mFaceList.last().vertex[j] = emitString[currPos++];
   }
}


void ShapeBaseConvex::getPolyList(AbstractPolyList* list)
{
   list->setTransform(&pShapeBase->getTransform(), pShapeBase->getScale());
   list->setObject(pShapeBase);

   pShapeBase->mShapeInstance->animate(pShapeBase->mDataBlock->collisionDetails[hullId]);
   pShapeBase->mShapeInstance->buildPolyList(list,pShapeBase->mDataBlock->collisionDetails[hullId]);
}


//--------------------------------------------------------------------------

bool ShapeBase::isInvincible()
{
   if( mDataBlock )
   {
      return mDataBlock->isInvincible;
   }
   return false;
}

void ShapeBase::startFade( F32 fadeTime, F32 fadeDelay, bool fadeOut )
{
   setMaskBits(CloakMask);
   mFadeElapsedTime = 0;
   mFading = true;
   if(fadeDelay < 0)
      fadeDelay = 0;
   if(fadeTime < 0)
      fadeTime = 0;
   mFadeTime = fadeTime;
   mFadeDelay = fadeDelay;
   mFadeOut = fadeOut;
   mFadeVal = F32(mFadeOut);
}

//--------------------------------------------------------------------------

void ShapeBase::setShapeName(const char* name)
{
   if (!isGhost()) {
      if (name[0] != '\0') {
         // Use tags for better network performance
         // Should be a tag, but we'll convert to one if it isn't.
         if (name[0] == StringTagPrefixByte)
            mShapeNameHandle = NetStringHandle(U32(dAtoi(name + 1)));
         else
            mShapeNameHandle = NetStringHandle(name);
      }
      else {
         mShapeNameHandle = NetStringHandle();
      }
      setMaskBits(NameMask);
   }
}

void ShapeBase::setSkinName(const char* name)
{
   if (!isGhost()) {
      if (name[0] != '\0') {
         // Use tags for better network performance
         // Should be a tag, but we'll convert to one if it isn't.
         if (name[0] == StringTagPrefixByte)
            mSkinNameHandle = NetStringHandle(U32(dAtoi(name + 1)));
         else
            mSkinNameHandle = NetStringHandle(name);
      }
      else
         mSkinNameHandle = NetStringHandle();
      setMaskBits(SkinMask);
   }
}

//----------------------------------------------------------------------------

void ShapeBase::reSkin()
{
   if ( isGhost() && mShapeInstance && mSkinNameHandle.isValidString() )
   {
      Vector<String> skins;
      String(mSkinNameHandle.getString()).split( ";", skins );

      for (int i = 0; i < skins.size(); i++)
      {
         String oldSkin( mAppliedSkinName.c_str() );
         String newSkin( skins[i] );

         // Check if the skin handle contains an explicit "old" base string. This
         // allows all models to support skinning, even if they don't follow the 
         // "base_xxx" material naming convention.
         S32 split = newSkin.find( '=' );    // "old=new" format skin?
         if ( split != String::NPos )
         {
            oldSkin = newSkin.substr( 0, split );
            newSkin = newSkin.erase( 0, split+1 );
         }

         mShapeInstance->reSkin( newSkin, oldSkin );
         mAppliedSkinName = newSkin;
      }
   }
}

void ShapeBase::setCurrentWaterObject( WaterObject *obj )
{
   if ( obj )
      deleteNotify( obj );
   if ( mCurrentWaterObject )
      clearNotify( mCurrentWaterObject );

   mCurrentWaterObject = obj;
}

//--------------------------------------------------------------------------
//----------------------------------------------------------------------------
DefineEngineMethod( ShapeBase, setHidden, void, ( bool show ),,
   "@brief Add or remove this object from the scene.\n\n"
   "When removed from the scene, the object will not be processed or rendered.\n"
   "@param show False to hide the object, true to re-show it\n\n" )
{
   object->setHidden( show );
}

DefineEngineMethod( ShapeBase, isHidden, bool, (),,
   "Check if the object is hidden.\n"
   "@return true if the object is hidden, false if visible.\n\n" )
{
   return object->isHidden();
}

//----------------------------------------------------------------------------
DefineEngineMethod( ShapeBase, playAudio, bool, ( S32 slot, SFXTrack* track ),,
   "@brief Attach a sound to this shape and start playing it.\n\n"

   "@param slot Audio slot index for the sound (valid range is 0 - 3)\n" // 3 = ShapeBase::MaxSoundThreads-1
   "@param track SFXTrack to play\n"
   "@return true if the sound was attached successfully, false if failed\n\n"
   
   "@see stopAudio()\n")
{
   if (track && slot >= 0 && slot < ShapeBase::MaxSoundThreads) {
      object->playAudio(slot,track);
      return true;
   }
   return false;
}

DefineEngineMethod( ShapeBase, stopAudio, bool, ( S32 slot ),,
   "@brief Stop a sound started with playAudio.\n\n"

   "@param slot audio slot index (started with playAudio)\n"
   "@return true if the sound was stopped successfully, false if failed\n\n"
   
   "@see playAudio()\n")
{
   if (slot >= 0 && slot < ShapeBase::MaxSoundThreads) {
      object->stopAudio(slot);
      return true;
   }
   return false;
}


//----------------------------------------------------------------------------
DefineEngineMethod( ShapeBase, playThread, bool, ( S32 slot, const char* name ), ( "" ),
   "@brief Start a new animation thread, or restart one that has been paused or "
   "stopped.\n\n"

   "@param slot thread slot to play. Valid range is 0 - 3)\n"  // 3 = ShapeBase::MaxScriptThreads-1
   "@param name name of the animation sequence to play in this slot. If not "
   "specified, the paused or stopped thread in this slot will be resumed.\n"
   "@return true if successful, false if failed\n\n"

   "@tsexample\n"
   "%obj.playThread( 0, \"ambient\" );      // Play the ambient sequence in slot 0\n"
   "%obj.setThreadTimeScale( 0, 0.5 );    // Play at half-speed\n"
   "%obj.pauseThread( 0 );                // Pause the sequence\n"
   "%obj.playThread( 0 );                 // Resume playback\n"
   "%obj.playThread( 0, \"spin\" );         // Replace the sequence in slot 0\n"
   "@endtsexample\n"
   
   "@see pauseThread()\n"
   "@see stopThread()\n"
   "@see setThreadDir()\n"
   "@see setThreadTimeScale()\n"
   "@see destroyThread()\n")
{
   if (slot >= 0 && slot < ShapeBase::MaxScriptThreads) {
      if (!dStrEqual(name, "")) {
         if (object->getShape()) {
            S32 seq = object->getShape()->findSequence(name);
            if (seq != -1 && object->setThreadSequence(slot,seq))
               return true;
         }
      }
      else
         if (object->playThread(slot))
            return true;
   }
   return false;
}

DefineEngineMethod( ShapeBase, setThreadDir, bool, ( S32 slot, bool fwd ),,
   "@brief Set the playback direction of an animation thread.\n\n"

   "@param slot thread slot to modify\n"
   "@param fwd true to play the animation forwards, false to play backwards\n"
   "@return true if successful, false if failed\n\n"
   
   "@see playThread()\n" )
{
   if (slot >= 0 && slot < ShapeBase::MaxScriptThreads) {
      if (object->setThreadDir(slot,fwd))
         return true;
   }
   return false;
}

DefineEngineMethod( ShapeBase, setThreadTimeScale, bool, ( S32 slot, F32 scale ),,
   "@brief Set the playback time scale of an animation thread.\n\n"

   "@param slot thread slot to modify\n"
   "@param scale new thread time scale (1=normal speed, 0.5=half speed etc)\n"
   "@return true if successful, false if failed\n\n"
   
   "@see playThread\n" )
{
   if (slot >= 0 && slot < ShapeBase::MaxScriptThreads) {
      if (object->setThreadTimeScale(slot,scale))
         return true;
   }
   return false;
}

DefineEngineMethod( ShapeBase, setThreadPosition, bool, ( S32 slot, F32 pos ),,
   "@brief Set the position within an animation thread.\n\n"

   "@param slot thread slot to modify\n"
   "@param pos position within thread\n"
   "@return true if successful, false if failed\n\n"
   
   "@see playThread\n" )
{
   if (slot >= 0 && slot < ShapeBase::MaxScriptThreads) {
      if (object->setThreadPosition(slot,pos))
         return true;
   }
   return false;
}

DefineEngineMethod( ShapeBase, stopThread, bool, ( S32 slot ),,
   "@brief Stop an animation thread.\n\n"

   "If restarted using playThread, the animation "
   "will start from the beginning again.\n"
   "@param slot thread slot to stop\n"
   "@return true if successful, false if failed\n\n"
   
   "@see playThread\n" )
{
   if (slot >= 0 && slot < ShapeBase::MaxScriptThreads) {
      if (object->stopThread(slot))
         return true;
   }
   return false;
}

DefineEngineMethod( ShapeBase, destroyThread, bool, ( S32 slot ),,
   "@brief Destroy an animation thread, which prevents it from playing.\n\n"

   "@param slot thread slot to destroy\n"
   "@return true if successful, false if failed\n\n"
   
   "@see playThread\n" )
{
   if (slot >= 0 && slot < ShapeBase::MaxScriptThreads) {
      if (object->destroyThread(slot))
         return true;
   }
   return false;
}

DefineEngineMethod( ShapeBase, pauseThread, bool, ( S32 slot ),,
   "@brief Pause an animation thread.\n\n"
   
   "If restarted using playThread, the animation "
   "will resume from the paused position.\n"
   "@param slot thread slot to stop\n"
   "@return true if successful, false if failed\n\n"
   
   "@see playThread\n" )
{
   if (slot >= 0 && slot < ShapeBase::MaxScriptThreads) {
      if (object->pauseThread(slot))
         return true;
   }
   return false;
}

//----------------------------------------------------------------------------
DefineEngineMethod( ShapeBase, mountImage, bool,
   ( ShapeBaseImageData* image, S32 slot, bool loaded, const char* skinTag ), ( true, "" ),
   "@brief Mount a new Image.\n\n"

   "@param image the Image to mount\n"
   "@param slot Image slot to mount into (valid range is 0 - 3)\n"
   "@param loaded initial loaded state for the Image\n"
   "@param skinTag tagged string to reskin the mounted Image\n"
   "@return true if successful, false if failed\n\n"

   "@tsexample\n"
   "%player.mountImage( PistolImage, 1 );\n"
   "%player.mountImage( CrossbowImage, 0, false );\n"
   "%player.mountImage( RocketLauncherImage, 0, true, 'blue' );\n"
   "@endtsexample\n"
   
   "@see unmountImage()\n"
   "@see getMountedImage()\n"
   "@see getPendingImage()\n"
   "@see isImageMounted()\n")
{
   if (image && slot >= 0 && slot < ShapeBase::MaxMountedImages) {

      NetStringHandle team;
      if (skinTag[0] == StringTagPrefixByte)
         team = NetStringHandle(U32(dAtoi(skinTag+1)));

      return object->mountImage( image, slot, loaded, team );
   }
   return false;
}

DefineEngineMethod( ShapeBase, unmountImage, bool, ( S32 slot ),,
   "@brief Unmount the mounted Image in the specified slot.\n\n"

   "@param slot Image slot to unmount\n"
   "@return true if successful, false if failed\n\n"
   
   "@see mountImage()\n")
{
   if (slot >= 0 && slot < ShapeBase::MaxMountedImages)
      return object->unmountImage(slot);
   return false;
}

DefineEngineMethod( ShapeBase, getMountedImage, S32, ( S32 slot ),,
   "@brief Get the Image mounted in the specified slot.\n\n"

   "@param slot Image slot to query\n"
   "@return ID of the ShapeBaseImageData datablock mounted in the slot, or 0 "
   "if no Image is mounted there.\n\n" )
{
   if (slot >= 0 && slot < ShapeBase::MaxMountedImages)
      if (ShapeBaseImageData* data = object->getMountedImage(slot))
         return data->getId();
   return 0;
}

DefineEngineMethod( ShapeBase, getPendingImage, S32, ( S32 slot ),,
   "@brief Get the Image that will be mounted next in the specified slot.\n\n"

   "Calling mountImage when an Image is already mounted does one of two things: "
   "<ol><li>Mount the new Image immediately, the old Image is discarded and "
   "whatever state it was in is ignored.</li>"
   "<li>If the current Image state does not allow Image changes, the new "
   "Image is marked as pending, and will not be mounted until the current "
   "state completes. eg. if the user changes weapons, you may wish to ensure "
   "that the current weapon firing state plays to completion first.</li></ol>\n"
   "This command retrieves the ID of the pending Image (2nd case above).\n"
   
   "@param slot Image slot to query\n"
   "@return ID of the pending ShapeBaseImageData datablock, or 0 if none.\n\n" )
{
   if (slot >= 0 && slot < ShapeBase::MaxMountedImages)
      if (ShapeBaseImageData* data = object->getPendingImage(slot))
         return data->getId();
   return 0;
}

DefineEngineMethod( ShapeBase, isImageFiring, bool, ( S32 slot ),,
   "@brief Check if the current Image state is firing.\n\n"

   "@param slot Image slot to query\n"
   "@return true if the current Image state in this slot has the 'stateFire' flag set.\n" )
{
   if (slot >= 0 && slot < ShapeBase::MaxMountedImages)
      return object->isImageFiring(slot);
   return false;
}

DefineEngineMethod( ShapeBase, isImageMounted, bool, ( ShapeBaseImageData* image ),,
   "@brief Check if the given datablock is mounted to any slot on this object.\n\n"

   "@param image ShapeBaseImageData datablock to query\n"
   "@return true if the Image is mounted to any slot, false otherwise.\n\n" )
{
   return (image && object->isImageMounted(image));
}

DefineEngineMethod( ShapeBase, getMountSlot, S32, ( ShapeBaseImageData* image ),,
   "@brief Get the first slot the given datablock is mounted to on this object.\n\n"

   "@param image ShapeBaseImageData datablock to query\n"
   "@return index of the first slot the Image is mounted in, or -1 if the Image "
   "is not mounted in any slot on this object.\n\n" )

{
   return image ? object->getMountSlot(image) : -1;
}

DefineEngineMethod( ShapeBase, getImageSkinTag, S32, ( S32 slot ),,
   "@brief Get the skin tag ID for the Image mounted in the specified slot.\n\n"

   "@param slot Image slot to query\n"
   "@return the skinTag value passed to mountImage when the image was "
   "mounted\n\n" )
{
   if (slot >= 0 && slot < ShapeBase::MaxMountedImages)
      return object->getImageSkinTag(slot).getIndex();
   return -1;
}

DefineEngineMethod( ShapeBase, getImageState, const char*, ( S32 slot ),,
   "@brief Get the name of the current state of the Image in the specified slot.\n\n"

   "@param slot Image slot to query\n"
   "@return name of the current Image state, or \"Error\" if slot is invalid\n\n" )
{
   if (slot >= 0 && slot < ShapeBase::MaxMountedImages)
      return object->getImageState(slot);
   return "Error";
}

DefineEngineMethod( ShapeBase, hasImageState, bool, ( S32 slot, const char* state ),,
   "@brief Check if the given state exists on the mounted Image.\n\n"

   "@param slot Image slot to query\n"
   "@param state Image state to check for\n"
   "@return true if the Image has the requested state defined.\n\n" )
{
   if (slot >= 0 && slot < ShapeBase::MaxMountedImages)
      return object->hasImageState(slot, state);
   return false;
}

DefineEngineMethod( ShapeBase, getImageTrigger, bool, ( S32 slot ),,
   "@brief Get the trigger state of the Image mounted in the specified slot.\n\n"

   "@param slot Image slot to query\n"
   "@return the Image's current trigger state\n\n" )
{
   if (slot >= 0 && slot < ShapeBase::MaxMountedImages)
      return object->getImageTriggerState(slot);
   return false;
}

DefineEngineMethod( ShapeBase, setImageTrigger, bool, ( S32 slot, bool state ),,
   "@brief Set the trigger state of the Image mounted in the specified slot.\n\n"

   "@param slot Image slot to modify\n"
   "@param state new trigger state for the Image\n"
   "@return the Image's new trigger state\n\n" )
{
   if (slot >= 0 && slot < ShapeBase::MaxMountedImages) {
      object->setImageTriggerState(slot,state);
      return object->getImageTriggerState(slot);
   }
   return false;
}

DefineEngineMethod( ShapeBase, getImageGenericTrigger, bool, ( S32 slot, S32 trigger ),,
   "@brief Get the generic trigger state of the Image mounted in the specified slot.\n\n"

   "@param slot Image slot to query\n"
   "@param trigger Generic trigger number\n"
   "@return the Image's current generic trigger state\n\n" )
{
   if (slot >= 0 && slot < ShapeBase::MaxMountedImages && trigger >= 0 && trigger < ShapeBaseImageData::MaxGenericTriggers)
      return object->getImageGenericTriggerState(slot, trigger);
   return false;
}

DefineEngineMethod( ShapeBase, setImageGenericTrigger, S32, ( S32 slot, S32 trigger, bool state ),,
   "@brief Set the generic trigger state of the Image mounted in the specified slot.\n\n"

   "@param slot Image slot to modify\n"
   "@param trigger Generic trigger number\n"
   "@param state new generic trigger state for the Image\n"
   "@return the Image's new generic trigger state or -1 if there was a problem.\n\n" )
{
   if (slot >= 0 && slot < ShapeBase::MaxMountedImages && trigger >= 0 && trigger < ShapeBaseImageData::MaxGenericTriggers) {
      object->setImageGenericTriggerState(slot,trigger,state);
      return object->getImageGenericTriggerState(slot,trigger);
   }
   return -1;
}

DefineEngineMethod( ShapeBase, getImageAltTrigger, bool, ( S32 slot ),,
   "@brief Get the alt trigger state of the Image mounted in the specified slot.\n\n"

   "@param slot Image slot to query\n"
   "@return the Image's current alt trigger state\n\n" )
{
   if (slot >= 0 && slot < ShapeBase::MaxMountedImages)
      return object->getImageAltTriggerState(slot);
   return false;
}

DefineEngineMethod( ShapeBase, setImageAltTrigger, bool, ( S32 slot, bool state ),,
   "@brief Set the alt trigger state of the Image mounted in the specified slot.\n\n"

   "@param slot Image slot to modify\n"
   "@param state new alt trigger state for the Image\n"
   "@return the Image's new alt trigger state\n\n" )
{
   if (slot >= 0 && slot < ShapeBase::MaxMountedImages) {
      object->setImageAltTriggerState(slot,state);
      return object->getImageAltTriggerState(slot);
   }
   return false;
}

DefineEngineMethod( ShapeBase, getImageAmmo, bool, ( S32 slot ),,
   "@brief Get the ammo state of the Image mounted in the specified slot.\n\n"

   "@param slot Image slot to query\n"
   "@return the Image's current ammo state\n\n" )
{
   if (slot >= 0 && slot < ShapeBase::MaxMountedImages)
      return object->getImageAmmoState(slot);
   return false;
}

DefineEngineMethod( ShapeBase, setImageAmmo, bool, ( S32 slot, bool state ),,
   "@brief Set the ammo state of the Image mounted in the specified slot.\n\n"

   "@param slot Image slot to modify\n"
   "@param state new ammo state for the Image\n"
   "@return the Image's new ammo state\n\n" )
{
   if (slot >= 0 && slot < ShapeBase::MaxMountedImages) {
      object->setImageAmmoState(slot,state);
      return state;
   }
   return false;
}

DefineEngineMethod( ShapeBase, getImageLoaded, bool, ( S32 slot ),,
   "@brief Get the loaded state of the Image mounted in the specified slot.\n\n"

   "@param slot Image slot to query\n"
   "@return the Image's current loaded state\n\n" )
{
   if (slot >= 0 && slot < ShapeBase::MaxMountedImages)
      return object->getImageLoadedState(slot);
   return false;
}

DefineEngineMethod( ShapeBase, setImageLoaded, bool, ( S32 slot, bool state ),,
   "@brief Set the loaded state of the Image mounted in the specified slot.\n\n"

   "@param slot Image slot to modify\n"
   "@param state new loaded state for the Image\n"
   "@return the Image's new loaded state\n\n" )
{
   if (slot >= 0 && slot < ShapeBase::MaxMountedImages) {
      object->setImageLoadedState(slot, state);
      return state;
   }
   return false;
}

DefineEngineMethod( ShapeBase, getImageTarget, bool, ( S32 slot ),,
   "@brief Get the target state of the Image mounted in the specified slot.\n\n"

   "@param slot Image slot to query\n"
   "@return the Image's current target state\n\n" )
{
   if (slot >= 0 && slot < ShapeBase::MaxMountedImages)
      return object->getImageTargetState(slot);
   return false;
}

DefineEngineMethod( ShapeBase, setImageTarget, bool, ( S32 slot, bool state ),,
   "@brief Set the target state of the Image mounted in the specified slot.\n\n"

   "@param slot Image slot to modify\n"
   "@param state new target state for the Image\n"
   "@return the Image's new target state\n\n" )
{
   if (slot >= 0 && slot < ShapeBase::MaxMountedImages) {
      object->setImageTargetState(slot,state);
      return state;
   }
   return false;
}

DefineEngineMethod( ShapeBase, getImageScriptAnimPrefix, const char*, ( S32 slot ),,
   "@brief Get the script animation prefix of the Image mounted in the specified slot.\n\n"

   "@param slot Image slot to query\n"
   "@return the Image's current script animation prefix\n\n" )
{
   if (slot >= 0 && slot < ShapeBase::MaxMountedImages)
      return object->getImageScriptAnimPrefix(slot).getString();
   return "";
}

DefineEngineMethod( ShapeBase, setImageScriptAnimPrefix, void, ( S32 slot, const char* prefix ),,
   "@brief Set the script animation prefix for the Image mounted in the specified slot.\n\n"
   "This is used to further modify the prefix used when deciding which animation sequence to "
   "play while this image is mounted.\n"

   "@param slot Image slot to modify\n"
   "@param prefix The prefix applied to the image\n" )
{
   if (slot >= 0 && slot < ShapeBase::MaxMountedImages) {

      NetStringHandle prefixHandle;
      if (prefix[0] == StringTagPrefixByte)
         prefixHandle = NetStringHandle(U32(dAtoi(prefix+1)));

      object->setImageScriptAnimPrefix(slot, prefixHandle);
   }
}

DefineEngineMethod( ShapeBase, getMuzzleVector, VectorF, ( S32 slot ),,
   "@brief Get the muzzle vector of the Image mounted in the specified slot.\n\n"

   "If the Image shape contains a node called 'muzzlePoint', then the muzzle "
   "vector is the forward direction vector of that node's transform in world "
   "space. If no such node is specified, the slot's mount node is used "
   "instead.\n\n"

   "If the correctMuzzleVector flag (correctMuzzleVectorTP in 3rd person) "
   "is set in the Image, the muzzle vector is computed to point at whatever "
   "object is right in front of the object's 'eye' node.\n"

   "@param slot Image slot to query\n"
   "@return the muzzle vector, or \"0 1 0\" if the slot is invalid\n\n" )
{
   VectorF vec(0, 1, 0);
   if (slot >= 0 && slot < ShapeBase::MaxMountedImages)
      object->getMuzzleVector(slot, &vec);

   return vec;
}

DefineEngineMethod( ShapeBase, getMuzzlePoint, Point3F, ( S32 slot ),,
   "@brief Get the muzzle position of the Image mounted in the specified slot.\n\n"

   "If the Image shape contains a node called 'muzzlePoint', then the muzzle "
   "position is the position of that node in world space. If no such node "
   "is specified, the slot's mount node is used instead.\n"

   "@param slot Image slot to query\n"
   "@return the muzzle position, or \"0 0 0\" if the slot is invalid\n\n" )
{
   Point3F pos(0, 0, 0);
   if (slot >= 0 && slot < ShapeBase::MaxMountedImages)
      object->getMuzzlePoint(slot, &pos);

   return pos;
}

DefineEngineMethod( ShapeBase, getSlotTransform, TransformF, ( S32 slot ),,
   "@brief Get the world transform of the specified mount slot.\n\n"

   "@param slot Image slot to query\n"
   "@return the mount transform\n\n" )
{
   MatrixF xf(true);
   if (slot >= 0 && slot < ShapeBase::MaxMountedImages)
      object->getMountTransform( slot, MatrixF::Identity, &xf );

   return xf;
}

//----------------------------------------------------------------------------

DefineEngineMethod( ShapeBase, getAIRepairPoint, Point3F, (),,
   "@brief Get the position at which the AI should stand to repair things.\n\n"

   "If the shape defines a node called \"AIRepairNode\", this method will "
   "return the current world position of that node, otherwise \"0 0 0\".\n"
   "@return the AI repair position\n\n" )
{
   return object->getAIRepairPoint();
}

DefineEngineMethod( ShapeBase, getVelocity, VectorF, (),,
   "@brief Get the object's current velocity.\n\n"

   "@return the current velocity\n\n" )
{
   return object->getVelocity();
}

DefineEngineMethod( ShapeBase, setVelocity, bool, ( Point3F vel ),,
   "@brief Set the object's velocity.\n\n"

   "@param vel new velocity for the object\n"
   "@return true\n\n" )
{
   object->setVelocity( vel );
   return true;
}

DefineEngineMethod( ShapeBase, applyImpulse, bool, ( Point3F pos, Point3F vec ),,
   "@brief Apply an impulse to the object.\n\n"

   "@param pos world position of the impulse\n"
   "@param vec impulse momentum (velocity * mass)\n"
   "@return true\n\n" )
{
   object->applyImpulse( pos, vec );
   return true;
}

DefineEngineMethod( ShapeBase, getEyeVector, VectorF, (),,
   "@brief Get the forward direction of the 'eye' for this object.\n\n"

   "If the object model has a node called 'eye', this method will return that "
   "node's current forward direction vector, otherwise it will return the "
   "object's current forward direction vector.\n"

   "@return the eye vector for this object\n"

   "@see getEyePoint\n"
   "@see getEyeTransform\n" )
{
   MatrixF mat;
   object->getEyeTransform(&mat);
   return mat.getForwardVector();
}

DefineEngineMethod( ShapeBase, getEyePoint, Point3F, (),,
   "@brief Get the position of the 'eye' for this object.\n\n"

   "If the object model has a node called 'eye', this method will return that "
   "node's current world position, otherwise it will return the object's current "
   "world position.\n"

   "@return the eye position for this object\n"

   "@see getEyeVector\n"
   "@see getEyeTransform\n" )
{
   MatrixF mat;
   object->getEyeTransform(&mat);
   return mat.getPosition();
}

DefineEngineMethod( ShapeBase, getEyeTransform, TransformF, (),,
   "@brief Get the 'eye' transform for this object.\n\n"

   "If the object model has a node called 'eye', this method will return that "
   "node's current transform, otherwise it will return the object's current "
   "transform.\n"

   "@return the eye transform for this object\n"

   "@see getEyeVector\n"
   "@see getEyePoint\n" )
{
   MatrixF mat;
   object->getEyeTransform(&mat);
   return mat;
}

DefineEngineMethod( ShapeBase, getLookAtPoint, const char*, ( F32 distance, S32 typeMask ), ( 2000, 0xFFFFFFFF ),
   "@brief Get the world position this object is looking at.\n\n"

   "Casts a ray from the eye and returns information about what the ray hits.\n"

   "@param distance maximum distance of the raycast\n"
   "@param typeMask typeMask of objects to include for raycast collision testing\n"
   "@return look-at information as \"Object HitX HitY HitZ [Material]\" or empty string for no hit\n\n"

   "@tsexample\n"
   "%lookat = %obj.getLookAtPoint();\n"
   "echo( \"Looking at: \" @ getWords( %lookat, 1, 3 ) );\n"
   "@endtsexample\n" )
{
   MatrixF mat;
   object->getEyeTransform( &mat );
   
   // Get eye vector.
   
   VectorF eyeVector;
   mat.getColumn( 1, &eyeVector );
   
   // Get eye position.
   
   VectorF eyePos;
   mat.getColumn( 3, &eyePos );
   
   // Make sure the eye vector covers the distance.
   
   eyeVector *= distance;
   
   // Do a container search.
   
   VectorF start = eyePos;
   VectorF end = eyePos + eyeVector;
   
   RayInfo ri;
   if( !gServerContainer.castRay( start, end, typeMask, &ri ) || !ri.object )
      return ""; // No hit.
   
   // Gather hit info.
      
   enum { BUFFER_SIZE = 256 };
   char* buffer = Con::getReturnBuffer( BUFFER_SIZE );
   if( ri.material )
      dSprintf( buffer, BUFFER_SIZE, "%u %f %f %f %u",
         ri.object->getId(),
         ri.point.x,
         ri.point.y,
         ri.point.z,
         ri.material->getMaterial()->getId() );
   else
      dSprintf( buffer, BUFFER_SIZE, "%u %f %f %f",
         ri.object->getId(),
         ri.point.x,
         ri.point.y,
         ri.point.z );

   return buffer;
}

DefineEngineMethod( ShapeBase, setEnergyLevel, void, ( F32 level ),,
   "@brief Set this object's current energy level.\n\n"

   "@param level new energy level\n"
   
   "@see getEnergyLevel()\n"
   "@see getEnergyPercent()\n")
{
   object->setEnergyLevel( level );
}

DefineEngineMethod( ShapeBase, getEnergyLevel, F32, (),,
   "@brief Get the object's current energy level.\n\n"

   "@return energy level\n"
   
   "@see setEnergyLevel()\n")
{
   return object->getEnergyLevel();
}

DefineEngineMethod( ShapeBase, getEnergyPercent, F32, (),,
   "@brief Get the object's current energy level as a percentage of maxEnergy.\n\n"
   "@return energyLevel / datablock.maxEnergy\n"

   "@see setEnergyLevel()\n")
{
   return object->getEnergyValue();
}

// HH: Added for Tribes 2 compatibility.
// Tribes 2's, at 0x005f2170: a server's call, which a ghost ignores.
DefineEngineMethod( ShapeBase, playShieldEffect, void, ( Point3F normal ),,
   "@brief Flash the shield round this object on every client.\n\n"

   "@param normal Where the hit came from, which need not be unit length\n")
{
   if ( object->isGhost() )
      return;

   normal.normalize();
   object->playShieldEffect( normal, 1.0f );
}

// HH: Added for Tribes 2 compatibility.
// Both are Tribes 2's, at 0x005f1f60 and 0x005f1e50.
DefineEngineMethod( ShapeBase, getHeat, F32, (),,
   "@brief Get the object's heat signature, 0 to 1.\n\n"

   "@see setHeat()\n")
{
   return object->getHeat();
}

DefineEngineMethod( ShapeBase, setHeat, void, ( F32 heat ),,
   "@brief Set the object's heat signature, clamped to 0 through 1.\n\n"

   "@param heat new heat signature\n"

   "@see getHeat()\n")
{
   object->setHeat( heat );
}

DefineEngineMethod( ShapeBase, setDamageLevel, void, ( F32 level ),,
   "@brief Set the object's current damage level.\n\n"

   "@param level new damage level\n"
   
   "@see getDamageLevel()\n"
   "@see getDamagePercent()\n")
{
   object->setDamageLevel( level );
}

DefineEngineMethod( ShapeBase, getDamageLevel, F32, (),,
   "@brief Get the object's current damage level.\n\n"

   "@return damage level\n"
   
   "@see setDamageLevel()\n")
{
   return object->getDamageLevel();
}

DefineEngineMethod( ShapeBase, getDamagePercent, F32, (),,
   "@brief Get the object's current damage level as a percentage of maxDamage.\n\n"

   "@return damageLevel / datablock.maxDamage\n"
   
   "@see setDamageLevel()\n")
{
   return object->getDamageValue();
}

DefineEngineMethod( ShapeBase, setDamageState, bool, ( const char* state ),,
   "@brief Set the object's damage state.\n\n"

   "@param state should be one of \"Enabled\", \"Disabled\", \"Destroyed\"\n"
   "@return true if successful, false if failed\n"
   
   "@see getDamageState()\n")
{
   return object->setDamageState( state );
}

DefineEngineMethod( ShapeBase, getDamageState, const char*, (),,
   "@brief Get the object's damage state.\n\n"

   "@return the damage state; one of \"Enabled\", \"Disabled\", \"Destroyed\"\n"
   
   "@see setDamageState()\n")
{
   return object->getDamageStateName();
}

DefineEngineMethod( ShapeBase, isDestroyed, bool, (),,
   "@brief Check if the object is in the Destroyed damage state.\n\n"

   "@return true if damage state is \"Destroyed\", false if not\n" 
   
   "@see isDisabled()\n"
   "@see isEnabled()\n")
{
   return object->isDestroyed();
}

DefineEngineMethod( ShapeBase, isDisabled, bool, (),,
   "@brief Check if the object is in the Disabled or Destroyed damage state.\n\n"

   "@return true if damage state is not \"Enabled\", false if it is\n"
   
   "@see isDestroyed()\n"
   "@see isEnabled()\n")
{
   return object->getDamageState() != ShapeBase::Enabled;
}

DefineEngineMethod( ShapeBase, isEnabled, bool, (),,
   "@brief Check if the object is in the Enabled damage state.\n\n"

   "@return true if damage state is \"Enabled\", false if not\n"
   
   "@see isDestroyed()\n"
   "@see isDisabled()\n")
{
   return object->getDamageState() == ShapeBase::Enabled;
}

DefineEngineMethod( ShapeBase, applyDamage, void, ( F32 amount ),,
   "@brief Increment the current damage level by the specified amount.\n\n"

   "@param amount value to add to current damage level\n" )
{
   object->applyDamage( amount );
}

DefineEngineMethod( ShapeBase, applyRepair, void, ( F32 amount ),,
   "@brief Repair damage by the specified amount.\n\n"

   "Note that the damage level is only reduced by repairRate per tick, so it may "
   "take several ticks for the total repair to complete.\n"

   "@param amount total repair value (subtracted from damage level over time)\n" )
{
   object->applyRepair( amount );
}

DefineEngineMethod( ShapeBase, setRepairRate, void, ( F32 rate ),,
   "@brief Set amount to repair damage by each tick.\n\n"

   "Note that this value is separate to the repairRate field in ShapeBaseData. "
   "This value will be subtracted from the damage level each tick, whereas the "
   "ShapeBaseData field limits how much of the applyRepair value is subtracted "
   "each tick. Both repair types can be active at the same time.\n"
   
   "@param rate value to subtract from damage level each tick (must be > 0)\n"
   
   "@see getRepairRate()\n")
{
   if(rate < 0)
      rate = 0;
   object->setRepairRate( rate );
}

DefineEngineMethod( ShapeBase, getRepairRate, F32, (),,
   "@brief Get the per-tick repair amount.\n\n"

   "@return the current value to be subtracted from damage level each tick\n"

   "@see setRepairRate\n" )
{
   return object->getRepairRate();
}

DefineEngineMethod( ShapeBase, setRechargeRate, void, ( F32 rate ),,
   "@brief Set the recharge rate.\n\n"

   "The recharge rate is added to the object's current energy level each tick, "
   "up to the maxEnergy level set in the ShapeBaseData datablock.\n"
   
   "@param rate the recharge rate (per tick)\n"
   
   "@see getRechargeRate()\n")
{
   object->setRechargeRate( rate );
}

DefineEngineMethod( ShapeBase, getRechargeRate, F32, (),,
   "@brief Get the current recharge rate.\n\n"

   "@return the recharge rate (per tick)\n"
   
   "@see setRechargeRate()\n")
{
   return object->getRechargeRate();
}

DefineEngineMethod( ShapeBase, getControllingClient, S32, (),,
   "@brief Get the client (if any) that controls this object.\n\n"

   "The controlling client is the one that will send moves to us to act on.\n"

   "@return the ID of the controlling GameConnection, or 0 if this object is not "
   "controlled by any client.\n"
   
   "@see GameConnection\n")
{
   if (GameConnection* con = object->getControllingClient())
      return con->getId();
   return 0;
}

DefineEngineMethod( ShapeBase, getControllingObject, S32, (),,
   "@brief Get the object (if any) that controls this object.\n\n"

   "@return the ID of the controlling ShapeBase object, or 0 if this object is "
   "not controlled by another object.\n" )
{
   if (ShapeBase* con = object->getControllingObject())
      return con->getId();
   return 0;
}

DefineEngineMethod( ShapeBase, canCloak, bool, (),,
   "@brief Check if this object can cloak.\n\n"
   "@return true\n"
   
   "@note Not implemented as it always returns true.")
{
   return true;
}

DefineEngineMethod( ShapeBase, setCloaked, void, ( bool cloak ),,
   "@brief Set the cloaked state of this object.\n\n"

   "When an object is cloaked it is not rendered.\n"

   "@param cloak true to cloak the object, false to uncloak\n"
   
   "@see isCloaked()\n")
{
   if (object->isServerObject())
      object->setCloakedState( cloak );
}

DefineEngineMethod( ShapeBase, isCloaked, bool, (),,
   "@brief Check if this object is cloaked.\n\n"

   "@return true if cloaked, false if not\n" 
   
   "@see setCloaked()\n")
{
   return object->getCloakedState();
}

DefineEngineMethod( ShapeBase, setDamageFlash, void, ( F32 level ),,
   "@brief Set the damage flash level.\n\n"

   "Damage flash may be used as a postfx effect to flash the screen when the "
   "client is damaged.\n"

   "@note Relies on the flash postFx.\n"

   "@param level flash level (0-1)\n"
   
   "@see getDamageFlash()\n")
{
   if (object->isServerObject())
      object->setDamageFlash( level );
}

DefineEngineMethod( ShapeBase, getDamageFlash, F32, (),,
   "@brief Get the damage flash level.\n\n"

   "@return flash level\n"

   "@see setDamageFlash\n" )
{
   return object->getDamageFlash();
}

DefineEngineMethod( ShapeBase, setWhiteOut, void, ( F32 level ),,
   "@brief Set the white-out level.\n\n"

   "White-out may be used as a postfx effect to brighten the screen in response "
   "to a game event.\n"

   "@note Relies on the flash postFx.\n"

   "@param level flash level (0-1)\n"
   
   "@see getWhiteOut()\n")
{
   if (object->isServerObject())
      object->setWhiteOut( level );
}

DefineEngineMethod( ShapeBase, getWhiteOut, F32, (),,
   "@brief Get the white-out level.\n\n"

   "@return white-out level\n"

   "@see setWhiteOut\n" )
{
   return object->getWhiteOut();
}

DefineEngineMethod( ShapeBase, getDefaultCameraFov, F32, (),,
   "@brief Returns the default vertical field of view in degrees for this object if used as a camera.\n\n"

   "@return Default FOV\n" )
{
   if (object->isServerObject())
      return object->getDefaultCameraFov();
   return 0.0;
}

DefineEngineMethod( ShapeBase, getCameraFov, F32, (),,
   "@brief Returns the vertical field of view in degrees for this object if used as a camera.\n\n"

   "@return current FOV as defined in ShapeBaseData::cameraDefaultFov\n" )
{
   if (object->isServerObject())
      return object->getCameraFov();
   return 0.0;
}

DefineEngineMethod( ShapeBase, setCameraFov, void, ( F32 fov ),,
   "@brief Set the vertical field of view in degrees for this object if used as a camera.\n\n"

   "@param fov new FOV value\n" )
{
   if (object->isServerObject())
      object->setCameraFov( fov );
}

DefineEngineMethod( ShapeBase, setInvincibleMode, void, ( F32 time, F32 speed ),,
   "@brief Setup the invincible effect.\n\n"

   "This effect is used for HUD feedback to the user that they are invincible.\n"
   "@note Currently not implemented\n"

   "@param time duration in seconds for the invincible effect\n"
   "@param speed speed at which the invincible effect progresses\n" )
{
   object->setupInvincibleEffect( time, speed );
}

DefineEngineMethod( ShapeBase, startFade, void, ( S32 time, S32 delay, bool fadeOut ),,
   "@brief Fade the object in or out without removing it from the scene.\n\n"

   "A faded out object is still in the scene and can still be collided with, "
   "so if you want to disable collisions for this shape after it fades out "
   "use setHidden to temporarily remove this shape from the scene.\n"
  
   "@note Items have the ability to light their surroundings. When an Item with "
   "an active light is fading out, the light it emits is correspondingly "
   "reduced until it goes out. Likewise, when the item fades in, the light is "
   "turned-up till it reaches it's normal brightntess.\n"

   "@param time duration of the fade effect in ms\n"
   "@param delay delay in ms before the fade effect begins\n"
   "@param fadeOut true to fade-out to invisible, false to fade-in to full visibility\n" )
{
   object->startFade( (F32)time / (F32)1000.0, delay / 1000.0, fadeOut );
}

DefineEngineMethod( ShapeBase, setDamageVector, void, ( Point3F vec ),,
   "@brief Set the damage direction vector.\n\n"

   "Currently this is only used to initialise the explosion if this object "
   "is blown up.\n"

   "@param vec damage direction vector\n\n"

   "@tsexample\n"
   "%obj.setDamageVector( \"0 0 1\" );\n"
   "@endtsexample\n" )
{
   vec.normalize();
   object->setDamageDir( vec );
}

DefineEngineMethod( ShapeBase, setShapeName, void, ( const char* name ),,
   "@brief Set the name of this shape.\n\n"

   "@note This is the name of the shape object that is sent to the client, "
   "not the DTS or DAE model filename.\n"

   "@param name new name for the shape\n\n"
   
   "@see getShapeName()\n")
{
   object->setShapeName( name );
}

DefineEngineMethod( ShapeBase, getShapeName, const char*, (),,
   "@brief Get the name of the shape.\n\n"

   "@note This is the name of the shape object that is sent to the client, "
   "not the DTS or DAE model filename.\n"

   "@return the name of the shape\n\n" 
   
   "@see setShapeName()\n")
{
   return object->getShapeName();
}

DefineEngineMethod( ShapeBase, setSkinName, void, ( const char* name ),,
   "@brief Apply a new skin to this shape.\n\n"

   "'Skinning' the shape effectively renames the material targets, allowing "
   "different materials to be used on different instances of the same model.\n\n"

   "@param name name of the skin to apply\n\n"

   "@see skin\n"
   "@see getSkinName()\n")
{
   object->setSkinName( name );
}

DefineEngineMethod( ShapeBase, getSkinName, const char*, (),,
   "@brief Get the name of the skin applied to this shape.\n\n"

   "@return the name of the skin\n\n" 

   "@see skin\n"
   "@see setSkinName()\n")
{
   return object->getSkinName();
}

//----------------------------------------------------------------------------
void ShapeBase::consoleInit()
{
   Con::addVariable("SB::DFDec", TypeF32, &sDamageFlashDec, "Speed to reduce the damage flash effect per tick.\n\n"
      "@see ShapeBase::setDamageFlash()\n"
      "@see ShapeBase::getDamageFlash()\n"
      "@note Relies on the flash postFx.\n"
	   "@ingroup gameObjects\n");
   Con::addVariable("SB::WODec", TypeF32, &sWhiteoutDec, "Speed to reduce the whiteout effect per tick.\n\n"
      "@see ShapeBase::setWhiteOut()\n"
      "@see ShapeBase::getWhiteOut"
      "@note Relies on the flash postFx.\n"
	   "@ingroup gameObjects\n");
   Con::addVariable("SB::FullCorrectionDistance", TypeF32, &sFullCorrectionDistance, 
      "@brief Distance at which a weapon's muzzle vector is fully corrected to match where the player is looking.\n\n"
      "When a weapon image has correctMuzzleVector set and the Player is in 1st person, the muzzle vector from the "
      "weapon is modified to match where the player is looking.  Beyond the FullCorrectionDistance the muzzle vector "
      "is always corrected.  Between FullCorrectionDistance and the player, the weapon's muzzle vector is adjusted so that "
      "the closer the aim point is to the player, the closer the muzzle vector is to the true (non-corrected) one.\n"
	   "@ingroup gameObjects\n");
   Con::addVariable("SB::CloakSpeed", TypeF32, &sCloakSpeed, 
      "@brief Time to cloak, in seconds.\n\n"
	   "@ingroup gameObjects\n");
}

void ShapeBase::_updateHiddenMeshes()
{
   if ( !mShapeInstance )
      return;

   // This may happen at some point in the future... lets
   // detect it so that it can be fixed at that time.
   AssertFatal( mMeshHidden.getSize() == mShapeInstance->mMeshObjects.size(),
      "ShapeBase::_updateMeshVisibility() - Mesh visibility size mismatch!" );

   for ( U32 i = 0; i < mMeshHidden.getSize(); i++ )
      setMeshHidden( i, mMeshHidden.test( i ) );
}

void ShapeBase::setMeshHidden( const char *meshName, bool forceHidden )
{
   setMeshHidden( mDataBlock->mShape->findObject( meshName ), forceHidden );
}

void ShapeBase::setMeshHidden( S32 meshIndex, bool forceHidden )
{
   if ( meshIndex == -1 || meshIndex >= mMeshHidden.getSize() )
      return;

   if ( forceHidden )
      mMeshHidden.set( meshIndex );   
   else
      mMeshHidden.clear( meshIndex );   

   if ( mShapeInstance )
      mShapeInstance->setMeshForceHidden( meshIndex, forceHidden );

   setMaskBits( MeshHiddenMask );
}

void ShapeBase::setAllMeshesHidden( bool forceHidden )
{
   if ( forceHidden )
      mMeshHidden.set();
   else
      mMeshHidden.clear();

   if ( mShapeInstance )
   {
      for ( U32 i = 0; i < mMeshHidden.getSize(); i++ )
         mShapeInstance->setMeshForceHidden( i, forceHidden );
   }

   setMaskBits( MeshHiddenMask );
}

DefineEngineMethod( ShapeBase, setAllMeshesHidden, void, ( bool hide ),,
   "@brief Set the hidden state on all the shape meshes.\n\n"

   "This allows you to hide all meshes in the shape, for example, and then only "
   "enable a few.\n"

   "@param hide new hidden state for all meshes\n\n" )
{
   object->setAllMeshesHidden( hide );
}

DefineEngineMethod( ShapeBase, setMeshHidden, void, ( const char* name, bool hide ),,
   "@brief Set the hidden state on the named shape mesh.\n\n"

   "@param name name of the mesh to hide/show\n"
   "@param hide new hidden state for the mesh\n\n" )
{
   object->setMeshHidden( name, hide );
}

// Some development-handy functions
#ifndef TORQUE_SHIPPING

void ShapeBase::dumpMeshVisibility()
{
   if ( !mShapeInstance )
      return;

   const Vector<TSShapeInstance::MeshObjectInstance> &meshes = mShapeInstance->mMeshObjects;

   for ( U32 i = 0; i < meshes.size(); i++)
   {
      const TSShapeInstance::MeshObjectInstance &mesh = meshes[i];

      const String &meshName = mDataBlock->mShape->getMeshName( i );

      Con::printf( "%d - %s - forceHidden = %s, visibility = %f", 
         i,
         meshName.c_str(),
         mesh.forceHidden ? "true" : "false",
         mesh.visible );
   }
}

DefineEngineMethod( ShapeBase, dumpMeshVisibility, void, (),,
   "@brief Print a list of visible and hidden meshes in the shape to the console "
   "for debugging purposes.\n\n"
   "@note Only in a SHIPPING build.\n")
{
   object->dumpMeshVisibility();
}

#endif // #ifndef TORQUE_SHIPPING

//------------------------------------------------------------------------
//These functions are duplicated in tsStatic, shapeBase, and interiorInstance.
//They each function a little differently; but achieve the same purpose of gathering
//target names/counts without polluting simObject.

DefineEngineMethod( ShapeBase, getTargetName, const char*, ( S32 index ),,
   "@brief Get the name of the indexed shape material.\n\n"

   "@param index index of the material to get (valid range is 0 - getTargetCount()-1).\n"
   "@return the name of the indexed material.\n\n"
   
   "@see getTargetCount()\n")
{
	ShapeBase *obj = dynamic_cast< ShapeBase* > ( object );
	if(obj)
	{
		// Try to use the client object (so we get the reskinned targets in the Material Editor)
		if ((ShapeBase*)obj->getClientObject())
			obj = (ShapeBase*)obj->getClientObject();

		return obj->getShapeInstance()->getTargetName(index);
	}

	return "";
}

DefineEngineMethod( ShapeBase, getTargetCount, S32, (),,
   "@brief Get the number of materials in the shape.\n\n"

   "@return the number of materials in the shape.\n\n" 
   
   "@see getTargetName()\n")
{
	ShapeBase *obj = dynamic_cast< ShapeBase* > ( object );
	if(obj)
	{
		// Try to use the client object (so we get the reskinned targets in the Material Editor)
		if ((ShapeBase*)obj->getClientObject())
			obj = (ShapeBase*)obj->getClientObject();

		return obj->getShapeInstance()->getTargetCount();
	}

	return -1;
}

DefineEngineMethod( ShapeBase, changeMaterial, void, ( const char* mapTo, Material* oldMat, Material* newMat ),,
   "@brief Change one of the materials on the shape.\n\n"

   "This method changes materials per mapTo with others. The material that "
   "is being replaced is mapped to unmapped_mat as a part of this transition.\n"

   "@note Warning, right now this only sort of works. It doesn't do a live "
   "update like it should.\n"

   "@param mapTo the name of the material target to remap (from getTargetName)\n"
   "@param oldMat the old Material that was mapped \n"
   "@param newMat the new Material to map\n\n"

   "@tsexample\n"
   "// remap the first material in the shape\n"
   "%mapTo = %obj.getTargetName( 0 );\n"
   "%obj.changeMaterial( %mapTo, 0, MyMaterial );\n"
   "@endtsexample\n" )
{
   // if no valid new material, theres no reason for doing this
   if( !newMat )
   {
      Con::errorf("ShapeBase::changeMaterial failed: New material does not exist!");
      return;
   }

   // initilize server/client versions
   ShapeBase *serverObj = object;
   ShapeBase *clientObj = dynamic_cast< ShapeBase* > ( object->getClientObject() );

   // Check the mapTo name exists for this shape
   S32 matIndex = serverObj->getShape()->materialList->getMaterialNameList().find_next(String(mapTo));
   if (matIndex < 0)
   {
      Con::errorf("ShapeBase::changeMaterial failed: Invalid mapTo name '%s'", mapTo);
      return;
   }

   // Lets remap the old material off, so as to let room for our current material room to claim its spot
   if( oldMat )
      oldMat->mMapTo = String("unmapped_mat");

   newMat->mMapTo = mapTo;

   // Map the material in the in the matmgr
   MATMGR->mapMaterial( mapTo, newMat->mMapTo );

   // Replace instances with the new material being traded in. For ShapeBase
   // class we have to update the server/client objects separately so both
   // represent our changes
   delete serverObj->getShape()->materialList->mMatInstList[matIndex];
   serverObj->getShape()->materialList->mMatInstList[matIndex] = newMat->createMatInstance();
   if (clientObj)
   {
      delete clientObj->getShape()->materialList->mMatInstList[matIndex];
      clientObj->getShape()->materialList->mMatInstList[matIndex] = newMat->createMatInstance();
   }

   // Finish up preparing the material instances for rendering
   const GFXVertexFormat *flags = getGFXVertexFormat<GFXVertexPNTTB>();
   FeatureSet features = MATMGR->getDefaultFeatures();

   serverObj->getShape()->materialList->getMaterialInst(matIndex)->init( features, flags );
   if (clientObj)
      clientObj->getShapeInstance()->mMaterialList->getMaterialInst(matIndex)->init( features, flags );
}

DefineEngineMethod( ShapeBase, getModelFile, const char *, (),,
   "@brief Get the model filename used by this shape.\n\n"

   "@return the shape filename\n\n" )
{
	GameBaseData * datablock = object->getDataBlock();
	if( !datablock )
		return String::EmptyString;

	const char *fieldName = StringTable->insert( String("shapeFile") );
   return datablock->getDataField( fieldName, NULL );
}
