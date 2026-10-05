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

#include "T3D/missionMarker.h"
#include "T3D/gameBase/t2MemberDump.h"   // HH: Added for Tribes 2 compatibility.
#include "T3D/gameBase/t2Fields.h"   // HH: Added for Tribes 2 compatibility.
#include "T3D/gameBase/t2Layout.h"
#include "console/consoleTypes.h"
#include "core/color.h"
#include "console/engineAPI.h"

extern bool gEditingMission;
IMPLEMENT_CO_DATABLOCK_V1(MissionMarkerData);

ConsoleDocClass( MissionMarkerData,
   "@brief A very basic class containing information used by MissionMarker objects for rendering\n\n"

   "MissionMarkerData, is an extremely barebones class derived from ShapeBaseData. It is solely used by "
   "MissionMarker classes (such as SpawnSphere), so that you can see the object while editing a level.\n\n"

   "@tsexample\n"
   "datablock MissionMarkerData(SpawnSphereMarker)\n"
   "{\n"
   "   category = \"Misc\";\n"
   "   shapeFile = \"core/art/shapes/octahedron.dts\";\n"
   "};\n"
   "@endtsexample\n\n"

   "@see MissionMarker\n\n"
   "@see SpawnSphere\n\n"
   "@see WayPoint\n\n"
   "@ingroup enviroMisc\n"
);

//------------------------------------------------------------------------------
// Class: MissionMarker
//------------------------------------------------------------------------------
IMPLEMENT_CO_NETOBJECT_V1(MissionMarker);

ConsoleDocClass( MissionMarker,
   "@brief This is a base class for all \"marker\" related objets. It is a 3D representation of a point in the level.\n\n"

   "The main use of a MissionMarker is to represent a point in 3D space with a mesh and basic ShapeBase information. "
   "If you simply need to mark a spot in your level, with no overhead from additional fields, this is a useful object.\n\n"

   "@tsexample\n"
   "new MissionMarker()\n"
   "{\n"
   "   dataBlock = \"WayPointMarker\";\n"
   "   position = \"295.699 -171.817 280.124\";\n"
   "   rotation = \"0 0 -1 13.8204\";\n"
   "   scale = \"1 1 1\";\n"
   "   isRenderEnabled = \"true\";\n"
   "   canSaveDynamicFields = \"1\";\n"
   "   enabled = \"1\";\n"
   "};\n"
   "@endtsexample\n\n"

   "@note MissionMarkers will not add themselves to the scene except when in the editor.\n\n"

   "@see MissionMarkerData\n\n"
   "@see SpawnSphere\n\n"
   "@see WayPoint\n\n"
   "@ingroup enviroMisc\n"
);

MissionMarker::MissionMarker()
{
   mTypeMask |= StaticObjectType;
   mDataBlock = 0;
   mAddedToScene = false;
   mNetFlags.set(Ghostable | ScopeAlways);
}

bool MissionMarker::onAdd()
{
   if(!Parent::onAdd() || !mDataBlock)
      return(false);

   if(gEditingMission)
   {
      addToScene();
      mAddedToScene = true;
   }

   return(true);
}

void MissionMarker::onRemove()
{
   if( mAddedToScene )
   {
      removeFromScene();
      mAddedToScene = false;
   }

   Parent::onRemove();
}

void MissionMarker::inspectPostApply()
{
   Parent::inspectPostApply();
   setMaskBits(PositionMask);
}

void MissionMarker::onEditorEnable()
{
   if(!mAddedToScene)
   {
      addToScene();
      mAddedToScene = true;
   }
}

void MissionMarker::onEditorDisable()
{
   if(mAddedToScene)
   {
      removeFromScene();
      mAddedToScene = false;
   }
}

bool MissionMarker::onNewDataBlock( GameBaseData *dptr, bool reload )
{
   mDataBlock = dynamic_cast<MissionMarkerData*>( dptr );
   if ( !mDataBlock || !Parent::onNewDataBlock( dptr, reload ) )
      return(false);
   scriptOnNewDataBlock();
   return(true);
}

void MissionMarker::setTransform(const MatrixF& mat)
{
   Parent::setTransform(mat);
   setMaskBits(PositionMask);
}

U32 MissionMarker::packUpdate(NetConnection * con, U32 mask, BitStream * stream)
{
   U32 retMask = Parent::packUpdate(con, mask, stream);
   if(stream->writeFlag(mask & PositionMask))
   {
      stream->writeAffineTransform(mObjToWorld);
      mathWrite(*stream, mObjScale);
   }

   return(retMask);
}

// HH: Added for Tribes 2 compatibility.
//
// MissionMarker's own section is the transform and the scale that the
// mission_marker() layer in Tools/t2/handlayout.py ends with, and the reading
// of it now lives on ShapeBase: StaticShape's description is this layer plus a
// power flag, so the two read the same bits in the same place and reading them
// twice is how they come to disagree.  WayPoint, SpawnSphere and Turret are
// built from the same layer.
//
// Unlike ShapeBase's own layout, replacing this one was measurable rather than
// a judgement: MissionMarker agrees with t2-demo-parser to the bit over nine
// sessions, and still does.
void MissionMarker::readT2Fields( T2Reader& reader )
{
   // ShapeBase's body: the datablock, the damage, the sounds, the animation
   // threads, the image slots, the shield group and the mount.
   Parent::readT2Fields( reader );

   T2Fields f( reader );
   readT2Position( f );
}

// MissionMarkerData adds no fields of its own: its layout in
// t2DataBlockTables.h is byte-identical to ShapeBaseData's, so what reads one
// reads the other and ShapeBaseData::readT2Fields is inherited unchanged.

void MissionMarker::unpackUpdate(NetConnection * con, BitStream * stream)
{
   Parent::unpackUpdate(con, stream);
   if(stream->readFlag())
   {
      MatrixF mat;
      stream->readAffineTransform(&mat);
      Parent::setTransform(mat);

      Point3F scale;
      mathRead(*stream, &scale);
      setScale(scale);
   }
}

void MissionMarker::initPersistFields() {
   Parent::initPersistFields();
}

//------------------------------------------------------------------------------
// Class: WayPoint
//------------------------------------------------------------------------------
IMPLEMENT_CO_NETOBJECT_V1(WayPoint);

ConsoleDocClass( WayPoint,
   "@brief Special type of marker, distinguished by a name and team ID number\n\n"

   "The original Torque engines were built from a multi-player game called Tribes. "
   "The Tribes series featured various team based game modes, such as capture the flag. "
   "The WayPoint class survived the conversion from game (Tribes) to game engine (Torque).\n\n"

   "Essentially, this is a MissionMarker with the addition of two variables: markerName and team. "
   "Whenever a WayPoint is created, it is added to a unique global list called WayPointSet. "
   "You can iterate through this set, seeking out specific markers determined by their markerName and team ID. "
   "This avoids the overhead of constantly calling commandToClient and commandToServer to determine "
   "a WayPoint object's name, unique ID, etc.\n\n"

   "@note The <i>markerName<i> field was previously called <i>name</i>, but was changed "
   "because this conflicted with the SimObject name field. Existing scripts that relied "
   "on the WayPoint <i>name</i> field will need to be updated.\n\n"

   "@tsexample\n"
   "new WayPoint()\n"
   "{\n"
   "   team = \"1\";\n"
   "   dataBlock = \"WayPointMarker\";\n"
   "   position = \"-0.0224786 1.53471 2.93219\";\n"
   "   rotation = \"1 0 0 0\";\n"
   "   scale = \"1 1 1\";\n"
   "   canSave = \"1\";\n"
   "   canSaveDynamicFields = \"1\";\n"
   "};\n"
   "@endtsexample\n\n"

   "@see MissionMarker\n\n"
   "@see MissionMarkerData\n\n"
   "@ingroup enviroMisc\n"
);

WayPointTeam::WayPointTeam()
{
   mTeamId = 0;
   mWayPoint = 0;
}

WayPoint::WayPoint()
{
   mName = StringTable->insert("");
}

void WayPoint::setHidden(bool hidden)
{
   // Skip ShapeBase::setHidden (only ever added to scene if in the editor)
   ShapeBase::Parent::setHidden( hidden );
   if(isServerObject())
      setMaskBits(UpdateHiddenMask);
}

bool WayPoint::onAdd()
{
   if(!Parent::onAdd())
      return(false);

   //
   if(isClientObject())
      Sim::getWayPointSet()->addObject(this);
   else
   {
      mTeam.mWayPoint = this;
      setMaskBits(UpdateNameMask|UpdateTeamMask);
   }

   return(true);
}

void WayPoint::inspectPostApply()
{
   Parent::inspectPostApply();
   if(!mName || !mName[0])
      mName = StringTable->insert("");
   setMaskBits(UpdateNameMask|UpdateTeamMask);
}

U32 WayPoint::packUpdate(NetConnection * con, U32 mask, BitStream * stream)
{
   U32 retMask = Parent::packUpdate(con, mask, stream);
   if(stream->writeFlag(mask & UpdateNameMask))
      stream->writeString(mName);
   if(stream->writeFlag(mask & UpdateTeamMask))
      stream->write(mTeam.mTeamId);
   if(stream->writeFlag(mask & UpdateHiddenMask))
      stream->writeFlag(isHidden());
   return(retMask);
}

void WayPoint::unpackUpdate(NetConnection * con, BitStream * stream)
{
   Parent::unpackUpdate(con, stream);
   if(stream->readFlag())
      mName = stream->readSTString(true);
   if(stream->readFlag())
      stream->read(&mTeam.mTeamId);
   if(stream->readFlag())
      setHidden(stream->readFlag());
}

// HH: Added for Tribes 2 compatibility.
//
// The three masks WayPoint adds to MissionMarker's, which are the reading
// above field for field: this engine's WayPoint::unpackUpdate is already the
// retail one, and BananaEngine's transcription and t2-demo-parser agree with
// both on the same string, the same thirty-two bit team and the same hidden
// flag in the same order.
//
// Its layout was the largest the walk produced -- 1,052 nodes where this
// description takes 204 -- and almost none of it was WayPoint's: ShapeBase's
// body and MissionMarker's position, inlined in place.  Built from
// mission_marker() now, so everything above here is read by the class that
// owns it and this adds only what is genuinely new.
void WayPoint::readT2Fields( T2Reader& reader )
{
   // ShapeBase's body, then MissionMarker's transform and scale.
   Parent::readT2Fields( reader );

   // Resolved once per class rather than per packet: the lookup is a string
   // search.  Only the hidden flag needs a node -- a flag is never a field the
   // cursor stands on -- while the name is the only string left in the body
   // and the team the only remaining word, so those two go by shape and
   // survive a regeneration that moved them.
   static U32 sHiddenNode = U32( T2Reader::NoNode );
   static bool sResolved = false;
   if ( !sResolved )
   {
      sHiddenNode = T2Layout::ghostNode( "WayPoint", "hidden" );
      sResolved = true;
   }

   T2Fields f( reader );

   char name[256];
   if ( f.takeString( name, sizeof( name ) ) )
      // HH: Interned case-sensitively, which the default is not.
      //
      // StringTable::insert's caseSens defaults to false, and that is not a
      // lowercasing -- it is a case-insensitive *lookup* that returns whatever
      // spelling got there first.  A waypoint named "Base" therefore came back
      // as "base", because this mission's interiors intern a skinBase of
      // "base" and they arrive first.  So the name a marker ends up with
      // depended on an unrelated string in an earlier packet, and the HUD drew
      // the loser.
      //
      // Found by the ghost value check and findable by nothing else here: the
      // body is the right length, the string is the right length, and the only
      // difference is a letter.  Every other reader that interns a string off
      // the wire has the same hazard waiting on a collision -- shapeName,
      // interiorFile, materialList, the flare textures -- and they are left
      // alone because for a file name resolved through the ResManager hash the
      // case-insensitive form may be load-bearing.  This one is a name shown
      // to a player.
      mName = StringTable->insert( name, true );

   f.takeInt( 32, mTeam.mTeamId );

   // After the fields, which is the rule: a flag is recorded only once the
   // cursor has gone past it, and the cursor moves only when a field is taken.
   bool hidden = false;
   if ( f.takeFlag( sHiddenNode, hidden ) )
      setHidden( hidden );
}

// HH: What the reader above filed.
void WayPoint::dumpT2Members( T2Dump& d ) const
{
   Parent::dumpT2Members( d );

   d.put( "name", mName );
   d.put( "teamId", mTeam.mTeamId );
   d.put( "hidden", isHidden() );
}

//-----------------------------------------------------------------------------
// TypeWayPointTeam
//-----------------------------------------------------------------------------

IMPLEMENT_STRUCT( WayPointTeam, WayPointTeam,,
   "" )
END_IMPLEMENT_STRUCT;

//FIXME: this should work but does not; need to check the stripping down to base types within TYPE
//ConsoleType( WayPointTeam, TypeWayPointTeam, WayPointTeam* )
ConsoleType( WayPointTeam, TypeWayPointTeam, WayPointTeam )

ConsoleGetType( TypeWayPointTeam )
{
   char * buf = Con::getReturnBuffer(32);
   dSprintf(buf, 32, "%d", ((WayPointTeam*)dptr)->mTeamId);
   return(buf);
}

ConsoleSetType( TypeWayPointTeam )
{
   WayPointTeam * pTeam = (WayPointTeam*)dptr;
   pTeam->mTeamId = dAtoi(argv[0]);

   if(pTeam->mWayPoint && pTeam->mWayPoint->isServerObject())
      pTeam->mWayPoint->setMaskBits(WayPoint::UpdateTeamMask);
}

void WayPoint::initPersistFields()
{
   addGroup("Misc");	
   addField("markerName", TypeCaseString, Offset(mName, WayPoint), "Unique name representing this waypoint");
   addField("team", TypeWayPointTeam, Offset(mTeam, WayPoint), "Unique numerical ID assigned to this waypoint, or set of waypoints");
   endGroup("Misc");
   
   Parent::initPersistFields();
}

//------------------------------------------------------------------------------
// Class: SpawnSphere
//------------------------------------------------------------------------------
IMPLEMENT_CO_NETOBJECT_V1(SpawnSphere);

ConsoleDocClass( SpawnSphere,
   "@brief This class is used for creating any type of game object, assigning it a class, datablock, and other "
   "properties when it is spawned.\n\n"

   "Torque 3D uses a simple spawn system, which can be easily modified to spawn any kind of object (of any class). "
   "Each new level already contains at least one SpawnSphere, which is represented by a green octahedron in stock Torque 3D. "
   "The spawnClass field determines the object type, such as Player, AIPlayer, etc. The spawnDataBlock field applies the "
   "pre-defined datablock to each spawned object instance. The really powerful feature of this class is provided by "
   "the spawnScript field which allows you to define a simple script (multiple lines) that will be executed once the "
   "object has been spawned.\n\n"

   "@tsexample\n"
   "// Define an SpawnSphere that essentially performs the following each time an object is spawned\n"
   "//$SpawnObject = new Player()\n"
   "//{\n"
   "//   dataBlock = \"DefaultPlayerData\";\n"
   "//   name = \"Bob\";\n"
   "//   lifeTotal = 3;\n"
   "//};\n"
   "//echo(\"Spawned a Player: \" @ $SpawnObject);\n\n"
   "new SpawnSphere(DefaultSpawnSphere)\n"
   "{\n"
   "   spawnClass = \"Player\";\n"
   "   spawnDatablock = \"DefaultPlayerData\";\n"
   "   spawnScript = \"echo(\\\"Spawned a Player: \\\" @ $SpawnObject);\"; // embedded quotes must be escaped with \\ \n"
   "   spawnProperties = \"name = \\\"Bob\\\";lifeTotal = 3;\"; // embedded quotes must be escaped with \\ \n"
   "   autoSpawn = \"1\";\n"
   "   dataBlock = \"SpawnSphereMarker\";\n"
   "   position = \"-0.77266 -19.882 17.8153\";\n"
   "   rotation = \"1 0 0 0\";\n"
   "   scale = \"1 1 1\";\n"
   "   canSave = \"1\";\n"
   "   canSaveDynamicFields = \"1\";\n"
   "};\n\n"
   "// Because autoSpawn is set to true in the above example, the following lines\n"
   "// of code will execute AFTER the Player object has been spawned.\n"
   "echo(\"Object Spawned\");\n"
   "echo(\"Hello World\");\n\n"
   "@endtsexample\n\n"

   "@see MissionMarker\n\n"
   "@see MissionMarkerData\n\n"
   "@ingroup gameObjects\n"
   "@ingroup enviroMisc\n"
);

SpawnSphere::SpawnSphere()
{
   mAutoSpawn = false;

   mRadius = 100.f;
   mSphereWeight = 100.f;
   mIndoorWeight = 100.f;
   mOutdoorWeight = 100.f;
}

IMPLEMENT_CALLBACK( SpawnSphere, onAdd, void, ( U32 objectId ), ( objectId ),
   "Called when the SpawnSphere is being created.\n"
   "@param objectId The unique SimObjectId generated when SpawnSphere is created (%%this in script)\n" );

bool SpawnSphere::onAdd()
{
   if(!Parent::onAdd())
      return(false);

   if(!isClientObject())
      setMaskBits(UpdateSphereMask);

   if (!isGhost())
   {
	   onAdd_callback( getId());

      if (mAutoSpawn)
         spawnObject();
   }

   return true;
}

SimObject* SpawnSphere::spawnObject(String additionalProps)
{
   SimObject* spawnObject = Sim::spawnObject(mSpawnClass, mSpawnDataBlock, mSpawnName,
                                             mSpawnProperties + " " + additionalProps, mSpawnScript);

   // If we have a spawnObject add it to the MissionCleanup group
   if (spawnObject)
   {
      SimObject* cleanup = Sim::findObject("MissionCleanup");

      if (cleanup)
      {
         SimGroup* missionCleanup = dynamic_cast<SimGroup*>(cleanup);

         missionCleanup->addObject(spawnObject);
      }
   }

   return spawnObject;
}

void SpawnSphere::inspectPostApply()
{
   Parent::inspectPostApply();
   setMaskBits(UpdateSphereMask);
}

U32 SpawnSphere::packUpdate(NetConnection * con, U32 mask, BitStream * stream)
{
   U32 retMask = Parent::packUpdate(con, mask, stream);

   //
   if(stream->writeFlag(mask & UpdateSphereMask))
   {
      stream->writeFlag(mAutoSpawn);

      stream->write(mSpawnClass);
      stream->write(mSpawnDataBlock);
      stream->write(mSpawnName);
      stream->write(mSpawnProperties);
      stream->write(mSpawnScript);
   }
   return(retMask);
}

void SpawnSphere::unpackUpdate(NetConnection * con, BitStream * stream)
{
   Parent::unpackUpdate(con, stream);
   if(stream->readFlag())
   {
      mAutoSpawn = stream->readFlag();

      stream->read(&mSpawnClass);
      stream->read(&mSpawnDataBlock);
      stream->read(&mSpawnName);
      stream->read(&mSpawnProperties);
      stream->read(&mSpawnScript);
   }
}

// HH: Added for Tribes 2 compatibility.
//
// Not the reading above, which is the one place in this file where the two
// engines parted company: Torque 3D's spawn sphere sends an auto-spawn flag
// and five strings naming what to build, where Tribes 2 sends the geometry of
// the sphere itself -- a radius and the three weights that choose between
// spawning on the sphere, indoors or outdoors.  WayPoint beside it did not
// diverge; this one did, so the fields come from t2-demo-parser and
// BananaEngine's transcription of the retail reader, which agree, rather than
// from the function above.
//
// The four land in the members this engine still carries for them and marks
// deprecated, which is what makes them worth filing: they are script-visible
// persist fields, and a Tribes 2 mission's spawn geometry is read from script
// rather than from the engine's own spawnObject().  Where the sphere stands
// comes from MissionMarker's transform, read before this.
//
// SpawnSphere::unpackUpdate at 0x66af50.  Its layout was the last in the
// protocol still on a raw walk: 1,051 nodes of which one mask was its own.
void SpawnSphere::readT2Fields( T2Reader& reader )
{
   // ShapeBase's body, then MissionMarker's transform and scale.
   Parent::readT2Fields( reader );

   T2Fields f( reader );

   // By shape, with no node needed: these four are the only fields left in the
   // body by the time the parents are done, they are unconditional within
   // their mask, and a body without that mask leaves the cursor at the end so
   // the first take misses and the rest with it.  Shape also survives the
   // tables being regenerated, which a node index does not.
   if ( !f.takeRawFloat( mRadius ) )
      return;

   f.takeRawFloat( mSphereWeight );
   f.takeRawFloat( mIndoorWeight );
   f.takeRawFloat( mOutdoorWeight );
}

// HH: What the reader above filed.
void SpawnSphere::dumpT2Members( T2Dump& d ) const
{
   Parent::dumpT2Members( d );

   d.put( "radius", mRadius );
   d.put( "sphereWeight", mSphereWeight );
   d.put( "indoorWeight", mIndoorWeight );
   d.put( "outdoorWeight", mOutdoorWeight );
}

void SpawnSphere::processTick( const Move *move )
{
   if ( isServerObject() && isMounted() )
   {
      MatrixF mat( true );
      mMount.object->getRenderMountTransform( 0.f, mMount.node, mMount.xfm, &mat );
      setTransform( mat );
   }
}

void SpawnSphere::advanceTime( F32 timeDelta )
{
   if ( isMounted() )
   {
      MatrixF mat( true );
      mMount.object->getRenderMountTransform( 0.f, mMount.node, mMount.xfm, &mat );
      setTransform( mat );
   }
}

void SpawnSphere::initPersistFields()
{
   addGroup( "Spawn" );
   addField( "spawnClass", TypeRealString, Offset(mSpawnClass, SpawnSphere),
      "Object class to create (eg. Player, AIPlayer, Debris etc)" );
   addField( "spawnDatablock", TypeRealString, Offset(mSpawnDataBlock, SpawnSphere),
      "Predefined datablock assigned to the object when created" );
   addField( "spawnProperties", TypeRealString, Offset(mSpawnProperties, SpawnSphere),
      "String containing semicolon (;) delimited properties to set when the object is created." );
   addField( "spawnScript", TypeCommand, Offset(mSpawnScript, SpawnSphere),
      "Command to execute immediately after spawning an object. New object id is stored in $SpawnObject.  Max 255 characters." );
   addField( "autoSpawn", TypeBool, Offset(mAutoSpawn, SpawnSphere),
      "Flag to spawn object as soon as SpawnSphere is created, true to enable or false to disable." );
   endGroup( "Spawn" );

   addGroup( "Dimensions" );
   addField( "radius", TypeF32, Offset(mRadius, SpawnSphere), "Deprecated" );
   endGroup( "Dimensions" );

   addGroup( "Weight" );
   addField( "sphereWeight", TypeF32, Offset(mSphereWeight, SpawnSphere), "Deprecated" );
   addField( "indoorWeight", TypeF32, Offset(mIndoorWeight, SpawnSphere), "Deprecated" );
   addField( "outdoorWeight", TypeF32, Offset(mOutdoorWeight, SpawnSphere), "Deprecated" );
   endGroup( "Weight" );

   Parent::initPersistFields();
}

ConsoleDocFragment _SpawnSpherespawnObject1(
   "@brief Dynamically create a new game object with a specified class, datablock, and optional properties.\n\n"
   "This is called on the actual SpawnSphere, not to be confused with the Sim::spawnObject() "
   "global function\n\n"
   "@param additionalProps Optional set of semiconlon delimited parameters applied to the spawn object during creation.\n\n"
   "@tsexample\n"
   "// Use the SpawnSphere::spawnObject function to create a game object\n"
   "// No additional properties assigned\n"
   "%player = DefaultSpawnSphere.spawnObject();\n\n"
   "@endtsexample\n\n",
   "SpawnSphere",
   "bool spawnObject(string additionalProps);"
);

ConsoleMethod(SpawnSphere, spawnObject, S32, 2, 3,
   "([string additionalProps]) Spawns the object based on the SpawnSphere's "
   "class, datablock, properties, and script settings. Allows you to pass in "
   "extra properties."
   "@hide" )
{
   String additionalProps;

   if (argc == 3)
      additionalProps = String(argv[2]);

   SimObject* obj = object->spawnObject(additionalProps);

   if (obj)
      return obj->getId();

   return -1;
}


//------------------------------------------------------------------------------
// Class: CameraBookmark
//------------------------------------------------------------------------------
IMPLEMENT_CO_NETOBJECT_V1(CameraBookmark);

ConsoleDocClass( CameraBookmark,
   "@brief Special type of mission marker which allows a level editor's camera to jump to specific points.\n\n"
   "For Torque 3D editors only, not for actual game development\n\n"
   "@internal"
);

CameraBookmark::CameraBookmark()
{
   mName = StringTable->insert("");
}

bool CameraBookmark::onAdd()
{
   if(!Parent::onAdd())
      return(false);

   //
   if(!isClientObject())
   {
      setMaskBits(UpdateNameMask);
   }

   if( isServerObject() && isMethod("onAdd") )
      Con::executef( this, "onAdd" );

   return(true);
}

void CameraBookmark::onRemove()
{
   if( isServerObject() && isMethod("onRemove") )
      Con::executef( this, "onRemove" );

   Parent::onRemove();
}

void CameraBookmark::onGroupAdd()
{
   if( isServerObject() && isMethod("onGroupAdd") )
      Con::executef( this, "onGroupAdd" );
}

void CameraBookmark::onGroupRemove()
{
   if( isServerObject() && isMethod("onGroupRemove") )
      Con::executef( this, "onGroupRemove" );
}

void CameraBookmark::inspectPostApply()
{
   Parent::inspectPostApply();
   if(!mName || !mName[0])
      mName = StringTable->insert("");
   setMaskBits(UpdateNameMask);

   if( isMethod("onInspectPostApply") )
      Con::executef( this, "onInspectPostApply" );
}

U32 CameraBookmark::packUpdate(NetConnection * con, U32 mask, BitStream * stream)
{
   U32 retMask = Parent::packUpdate(con, mask, stream);
   if(stream->writeFlag(mask & UpdateNameMask))
      stream->writeString(mName);
   return(retMask);
}

void CameraBookmark::unpackUpdate(NetConnection * con, BitStream * stream)
{
   Parent::unpackUpdate(con, stream);
   if(stream->readFlag())
      mName = stream->readSTString(true);
}

void CameraBookmark::initPersistFields()
{
   //addGroup("Misc");	
   //addField("name", TypeCaseString, Offset(mName, CameraBookmark));
   //endGroup("Misc");

   Parent::initPersistFields();

   removeField("nameTag"); // From GameBase
}
