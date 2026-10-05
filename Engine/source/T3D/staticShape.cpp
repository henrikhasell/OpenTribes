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
#include "app/game.h"
#include "math/mMath.h"
#include "console/simBase.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "T3D/gameBase/moveManager.h"
#include "ts/tsShapeInstance.h"
#include "T3D/staticShape.h"
// HH: Added for Tribes 2 compatibility.
#include "T3D/gameBase/t2Fields.h"
#include "T3D/gameBase/t2Affine.h"
#include "T3D/gameBase/t2Layout.h"
#include "math/mathIO.h"
#include "sim/netConnection.h"
#include "scene/sceneObjectLightingPlugin.h"

extern void wireCube(F32 size,Point3F pos);

static const U32 sgAllowedDynamicTypes = 0xffffff;

//----------------------------------------------------------------------------

IMPLEMENT_CO_DATABLOCK_V1(StaticShapeData);

ConsoleDocClass( StaticShapeData,
	"@brief The most basic ShapeBaseData derrived shape datablock available in Torque 3D.\n\n"

	"When it comes to placing 3D objects in the scene, you effectively have two options:\n\n"
	"1. TSStatic objects\n\n"
	"2. ShapeBase derived objects\n\n"

	"Since ShapeBase and ShapeBaseData are not meant to be instantiated in script, you "
	"will use one of its child classes instead. Several game related objects are derived "
	"from ShapeBase: Player, Vehicle, Item, and so on.\n\n"

	"When you need a 3D object with datablock capabilities, you will use an object derived "
	"from ShapeBase. When you need an object with extremely low overhead, and with no other "
	"purpose than to be a 3D object in the scene, you will use TSStatic.\n\n"

	"The most basic child of ShapeBase is StaticShape. It does not introduce any of the "
	"additional functionality you see in Player, Item, Vehicle or the other game play "
	"heavy classes. At its core, it is comparable to a TSStatic, but with a datbalock.  Having "
   "a datablock provides a location for common variables as well as having access to "
   "various ShapeBaseData, GameBaseData and SimDataBlock callbacks.\n\n"

	"@tsexample\n"
	   "// Create a StaticShape using a datablock\n"
	   "datablock StaticShapeData(BasicShapeData)\n" 
	   "{\n"
	   "	shapeFile = \"art/shapes/items/kit/healthkit.dts\";\n"
	   "	testVar = \"Simple string, not a stock variable\";\n"
	   "};\n\n"
	   "new StaticShape()\n"
	   "{\n"
	   "	dataBlock = \"BasicShapeData\";\n"
	   "	position = \"0.0 0.0 0.0\";\n"
	   "	rotation = \"1 0 0 0\";\n"
	   "	scale = \"1 1 1\";\n"
	   "};\n"
	"@endtsexample\n\n"

	"@see StaticShape\n"
   "@see ShapeBaseData\n"
	"@see TSStatic\n\n"

	"@ingroup gameObjects\n"
	"@ingroup Datablocks");

StaticShapeData::StaticShapeData()
{
   dynamicTypeField     = 0;

   shadowEnable = true;

   noIndividualDamage = false;
}

void StaticShapeData::initPersistFields()
{
   addField("noIndividualDamage",   TypeBool, Offset(noIndividualDamage,   StaticShapeData), "Deprecated\n\n @internal");
   addField("dynamicType",          TypeS32,  Offset(dynamicTypeField,     StaticShapeData), 
      "@brief An integer value which, if speficied, is added to the value retured by getType().\n\n"
      "This allows you to extend the type mask for a StaticShape that uses this datablock.  Type masks "
      "are used for container queries, etc.");

   Parent::initPersistFields();
}

void StaticShapeData::packData(BitStream* stream)
{
   Parent::packData(stream);
   stream->writeFlag(noIndividualDamage);
   stream->write(dynamicTypeField);
}

void StaticShapeData::unpackData(BitStream* stream)
{
   Parent::unpackData(stream);
   noIndividualDamage = stream->readFlag();
   stream->read(&dynamicTypeField);
}


//----------------------------------------------------------------------------

IMPLEMENT_CO_NETOBJECT_V1(StaticShape);

ConsoleDocClass( StaticShape,
	"@brief The most basic 3D shape with a datablock available in Torque 3D.\n\n"

	"When it comes to placing 3D objects in the scene, you technically have two options:\n\n"
	"1. TSStatic objects\n\n"
	"2. ShapeBase derived objects\n\n"

	"Since ShapeBase and ShapeBaseData are not meant to be instantiated in script, you "
	"will use one of its child classes instead. Several game related objects are derived "
	"from ShapeBase: Player, Vehicle, Item, and so on.\n\n"

	"When you need a 3D object with datablock capabilities, you will use an object derived "
	"from ShapeBase. When you need an object with extremely low overhead, and with no other "
	"purpose than to be a 3D object in the scene, you will use TSStatic.\n\n"

	"The most basic child of ShapeBase is StaticShape. It does not introduce any of the "
	"additional functionality you see in Player, Item, Vehicle or the other game play "
	"heavy classes. At its core, it is comparable to a TSStatic, but with a datbalock.  Having "
   "a datablock provides a location for common variables as well as having access to "
   "various ShapeBaseData, GameBaseData and SimDataBlock callbacks.\n\n"

	"@tsexample\n"
	   "// Create a StaticShape using a datablock\n"
	   "datablock StaticShapeData(BasicShapeData)\n" 
	   "{\n"
	   "	shapeFile = \"art/shapes/items/kit/healthkit.dts\";\n"
	   "	testVar = \"Simple string, not a stock variable\";\n"
	   "};\n\n"
	   "new StaticShape()\n"
	   "{\n"
	   "	dataBlock = \"BasicShapeData\";\n"
	   "	position = \"0.0 0.0 0.0\";\n"
	   "	rotation = \"1 0 0 0\";\n"
	   "	scale = \"1 1 1\";\n"
	   "};\n"
	"@endtsexample\n\n"

	"@see StaticShapeData\n"
	"@see ShapeBase\n"
	"@see TSStatic\n\n"

	"@ingroup gameObjects\n");

StaticShape::StaticShape()
{
   mTypeMask |= StaticShapeObjectType | StaticObjectType;
   mDataBlock = 0;
}

StaticShape::~StaticShape()
{
}


//----------------------------------------------------------------------------

void StaticShape::initPersistFields()
{
   Parent::initPersistFields();
}

bool StaticShape::onAdd()
{
   if(!Parent::onAdd() || !mDataBlock)
      return false;

   // We need to modify our type mask based on what our datablock says...
   mTypeMask |= (mDataBlock->dynamicTypeField & sgAllowedDynamicTypes);

   addToScene();

   if (isServerObject())
      scriptOnAdd();
   return true;
}

bool StaticShape::onNewDataBlock(GameBaseData* dptr, bool reload)
{
   mDataBlock = dynamic_cast<StaticShapeData*>(dptr);
   if (!mDataBlock || !Parent::onNewDataBlock(dptr, reload))
      return false;

   scriptOnNewDataBlock();
   return true;
}

void StaticShape::onRemove()
{
   scriptOnRemove();
   removeFromScene();
   Parent::onRemove();
}


//----------------------------------------------------------------------------

void StaticShape::processTick(const Move* move)
{
   Parent::processTick(move);

   // Image Triggers
   if (move && mDamageState == Enabled) {
      setImageTriggerState(0,move->trigger[0]);
      setImageTriggerState(1,move->trigger[1]);
   }

   if (isMounted()) {
      MatrixF mat;
      mMount.object->getMountTransform( mMount.node, mMount.xfm, &mat );
      Parent::setTransform(mat);
      Parent::setRenderTransform(mat);
   }
}

void StaticShape::interpolateTick(F32 delta)
{
   if (isMounted()) {
      MatrixF mat;
      mMount.object->getRenderMountTransform( delta, mMount.node, mMount.xfm, &mat );
      Parent::setRenderTransform(mat);
   }
}

void StaticShape::setTransform(const MatrixF& mat)
{
   Parent::setTransform(mat);
   setMaskBits(PositionMask);
}

void StaticShape::onUnmount(ShapeBase*,S32)
{
   // Make sure the client get's the final server pos.
   setMaskBits(PositionMask);
}


//----------------------------------------------------------------------------

U32 StaticShape::packUpdate(NetConnection *connection, U32 mask, BitStream *bstream)
{
   U32 retMask = Parent::packUpdate(connection,mask,bstream);
   if (bstream->writeFlag(mask & PositionMask | ExtendedInfoMask))
   {

      // Write the transform (do _not_ use writeAffineTransform.  Since this is a static
      //  object, the transform must be RIGHT THE *&)*$&^ ON or it will goof up the
      //  synchronization between the client and the server.
      mathWrite(*bstream,mObjToWorld);
      mathWrite(*bstream, mObjScale);
   }

   // powered?
   bstream->writeFlag(mPowered);

   if (mLightPlugin) 
   {
      retMask |= mLightPlugin->packUpdate(this, ExtendedInfoMask, connection, mask, bstream);
   }

   return retMask;
}


//----------------------------------------------------------------------------
// HH: Added for Tribes 2 compatibility.

/// StaticShapeData's two fields of its own sit at the tail of its layout, past
/// the eight HUD image slots ShapeBaseData leaves alone.
///
/// Numbers rather than names, because t2DataBlockTables.h is shipped and no
/// generator writes it -- there are no names to ask for and nothing that can
/// move these underneath the reader.  They hold for TurretData too: its layout
/// is this one with more appended, so the shared prefix numbers identically,
/// which is checked here by taking the shape as well as the node.  A plain
/// ShapeBaseData ends at node 39, so a class that inherited this reader by
/// mistake would take nothing rather than something wrong.
///
///   [39] flag    noIndividualDamage
///   [40] raw 32  dynamicTypeField
enum
{
   StaticShapeDataNoIndividualDamage = 39,
   StaticShapeDataDynamicType        = 40,
};

void StaticShapeData::readT2Fields( T2Reader &reader )
{
   // ShapeBaseData: the shape name above all, which is what preload() builds
   // the structure from, and the mass, the sensor and the explosion.
   Parent::readT2Fields( reader );

   T2Fields f( reader );

   // Step over the HUD image slots in between.  Skipping is how the cursor
   // reaches the tail at all: it moves only when something takes a field.
   while ( !f.atNode( StaticShapeDataDynamicType ) && f.skip() )
      ;

   f.takeInt( 32, dynamicTypeField );

   // The flag after the field, which is the order the cursor requires: it
   // records a flag as it reads it, and it only reads ahead when a field is
   // taken.
   f.takeFlag( StaticShapeDataNoIndividualDamage, noIndividualDamage );
}

/// Which node of this class's layout holds the power flag.  Resolved once per
/// class rather than per packet: the lookup is a string search and a map's
/// worth of bases, generators and stations arrive as often as packets do.
static U32 t2PoweredNode( const char *className )
{
   enum { MaxClasses = 8 };
   static const char *sName[MaxClasses] = { 0 };
   static U32 sNode[MaxClasses];
   static U32 sCount = 0;

   for ( U32 i = 0; i < sCount; i++ )
      if ( sName[i] == className || !dStrcmp( sName[i], className ) )
         return sNode[i];

   const U32 node = T2Layout::ghostNode( className, "powered" );
   if ( sCount < MaxClasses )
   {
      sName[sCount] = className;
      sNode[sCount] = node;
      sCount++;
   }
   return node;
}

// HH: Added for Tribes 2 compatibility.
//
// ShapeBase's body and then this class's own section: where it stands and
// whether it has power.  The whole of what StaticShape, Turret, BeaconObject,
// AIObjective and ScopeAlwaysShape read before their own fields, and for three
// of those five the whole body.
//
// One function for the five, and it lives here rather than on ShapeBase
// because the power flag has to land in StaticShape::mPowered -- the member
// isPowered(), setPowered() and packUpdate() all use.  It used to be two
// functions: this one and ShapeBase::_readT2StaticShapeSection, which read the
// same nine words without the quaternion's sign and put the power state in a
// ShapeBase member nothing read.  Turret, BeaconObject, AIObjective and
// ScopeAlwaysShape went through that one, so a turret rotated the mirrored way
// and its power decoded into nothing.  Neither a session nor the second reader
// could see either fault: both readings consumed the same bits.
void StaticShape::readT2Fields( T2Reader &reader )
{
   // Qualified rather than virtual: a subclass's override is what called this,
   // and going back through the vtable would call it again.
   ShapeBase::readT2Fields( reader );

   T2Fields f( reader );

   // The transform and the scale, by the reader MissionMarker shares -- this
   // layout is that description plus the one flag below.  This is what puts a
   // base, a generator or a station where the mission says it stands.
   readT2Position( f );

   // And the power state last, because it sits behind the position: asked for
   // with ShapeBase's own flags it would be read before the transform had
   // moved the cursor past it, and would take its default on every body that
   // carries a position.  Asked here whether the position arrived or not --
   // when it did not there was nothing to stand on, the cursor ran to the end,
   // and that is past this flag either way.
   takeT2Flag( f, t2PoweredNode( getClassName() ), mPowered );
}


void StaticShape::unpackUpdate(NetConnection *connection, BitStream *bstream)
{
   Parent::unpackUpdate(connection,bstream);
   if (bstream->readFlag())
   {
      MatrixF mat;
      mathRead(*bstream,&mat);
      Parent::setTransform(mat);
      Parent::setRenderTransform(mat);

      VectorF scale;
      mathRead(*bstream, &scale);
      setScale(scale);
   }

   // powered?
   mPowered = bstream->readFlag();

   if (mLightPlugin)
   {
      mLightPlugin->unpackUpdate(this, connection, bstream);
   }
}


//----------------------------------------------------------------------------
// This appears to be legacy T2 stuff
// Marked internal, as this is flagged to be deleted
// [8/1/2010 mperry]
ConsoleMethod( StaticShape, setPoweredState, void, 3, 3, "(bool isPowered)"
			  "@internal")
{
   if(!object->isServerObject())
      return;
   object->setPowered(dAtob(argv[2]));
}

ConsoleMethod( StaticShape, getPoweredState, bool, 2, 2, "@internal")
{
   if(!object->isServerObject())
      return(false);
   return(object->isPowered());
}
