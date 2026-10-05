// HH: Added for Tribes 2 compatibility.
#ifndef _DATABLOCKSMISCT2_H_
#define _DATABLOCKSMISCT2_H_

#ifndef _SIMDATABLOCK_H_
#include "console/simDatablock.h"
#endif
#ifndef _GAMEBASE_H_
#include "T3D/gameBase/gameBase.h"
#endif
#ifndef _COLOR_H_
#include "core/color.h"
#endif
#ifndef _MPOINT3_H_
#include "math/mPoint3.h"
#endif
#ifndef _TVECTOR_H_
#include "core/util/tVector.h"
#endif

class T2Reader;

/// The Tribes 2 datablocks this port has no class of its own for.
///
/// They are together because each is a handful of fields with no ghost behind
/// it and nothing in Torque 3D to map onto -- a commander map icon, a chat
/// menu entry, the trim lights and jet flames on a vehicle.  Splitting six of
/// those across six files would say they were six subjects.
///
/// TSShapeConstructorT2 is the exception and does something: it is how a
/// Tribes 2 client is told which .dsq animations belong to which shape, and
/// this tree's own TSShapeConstructor takes exactly that list.


/// The five images a commander map icon is drawn from.
///
/// Each is a small drawing command rather than a bare bitmap name --
/// "static com_icon_generator false true" -- which is why they are kept as
/// sent rather than resolved to a path here.  This port has no commander map
/// yet; what it has is the icon every object on one would be drawn with.
class CommanderIconData : public SimDataBlock
{
   typedef SimDataBlock Parent;

public:

   StringTableEntry mBaseImage;
   StringTableEntry mActiveImage;
   StringTableEntry mInactiveImage;
   StringTableEntry mSelectImage;
   StringTableEntry mHilightImage;

   CommanderIconData();

   void readT2Fields( T2Reader &reader );

   DECLARE_CONOBJECT( CommanderIconData );
};


/// One line of the chat menu.
///
/// Nothing is decoded, and that is a layout disagreement rather than an empty
/// body: t2-demo-parser reads a string here from the disassembly, and this
/// tree's layout has one node, an OpEnd.  A one-node layout is what a walk
/// that could not enter unpackData leaves behind, which is how AudioEmitter's
/// came out at six nodes.  No recorded session carries a CannedChatItem, so
/// the gate cannot settle it either way -- and the two possible mistakes are
/// not equally bad: reading nothing where a string was sent loses the rest of
/// the packet, while the member below merely stays empty until the table is
/// fixed upstream in Tools/t2.
class CannedChatItem : public SimDataBlock
{
   typedef SimDataBlock Parent;

public:

   StringTableEntry mChatText;

   CannedChatItem();

   void readT2Fields( T2Reader &reader );

   DECLARE_CONOBJECT( CannedChatItem );
};


/// What a sensor can pick up.
///
/// Its body really is empty, which is worth saying because this class decides
/// what a player sees on the commander map and so looks like it ought to carry
/// something.  It does not: Tribes 2 keeps a sensor's radius, what it detects
/// and whether it can be jammed on the server, and sends the client only what
/// it needs to draw -- the sensor group and its colour, which arrive as
/// SetSensorGroupEvent and SensorGroupColorEvent, and the per-object target id
/// on GameBase.  Both readers agree to the bit over sixteen of these in nine
/// sessions, so this is settled rather than unfinished.
class SensorData : public SimDataBlock
{
   typedef SimDataBlock Parent;

public:

   void readT2Fields( T2Reader &reader );

   DECLARE_CONOBJECT( SensorData );
};


/// A light on a vehicle's hull.
class RunningLightData : public GameBaseData
{
   typedef GameBaseData Parent;

public:

   F32 mRadius;
   ColorF mColor;
   F32 mType;        ///< A float on the wire, though it counts kinds.
   F32 mLength;
   StringTableEntry mNodeName;   ///< The shape node the light rides on.
   Point3F mDirection;
   Point3F mOffset;
   StringTableEntry mTexture;    ///< Absent when the server sent none.

   RunningLightData();

   void readT2Fields( T2Reader &reader );

   DECLARE_CONOBJECT( RunningLightData );
};


/// The flame out of a jetting armour or vehicle.
class JetEffectData : public GameBaseData
{
   typedef GameBaseData Parent;

public:

   ColorF mCoolColor;
   ColorF mHotColor;
   F32 mActivateTime;
   F32 mDeactivateTime;
   F32 mLength;
   F32 mWidth;
   F32 mSpeed;
   F32 mStretch;
   F32 mYOffset;
   StringTableEntry mTexture;    ///< Absent when the server sent none.

   JetEffectData();

   void readT2Fields( T2Reader &reader );

   DECLARE_CONOBJECT( JetEffectData );
};


/// Which .dsq animations belong to a shape.
///
/// A Tribes 2 client is sent a shape name and up to 127 "<file>.dsq <name>"
/// pairs, which is how light_male.dts comes to have a run, a jet and eleven
/// deaths.  This tree's own TSShapeConstructor takes that list in that exact
/// form through its legacy `sequence` field and applies it when the shape
/// loads, so this class's job is to build one rather than to keep a copy: the
/// animations arrive by the engine's own path and nothing downstream has to
/// know they came off a wire.
///
/// The suffix in the name is not cosmetic.  This port keeps a
/// TSShapeConstructor of its own which is no datablock, so the class table
/// names Tribes 2's with a T2 while the recovered layout stays keyed on the
/// unsuffixed name.  The engine indexes layouts by class id and never notices;
/// readDataBlock asserts one layout per class id, 54 for 54, so the two names
/// must not be reconciled by adding a row.  The name-keyed lookups carry an
/// alias instead, in Tools/t2/replay.py.
class TSShapeConstructorT2 : public SimDataBlock
{
   typedef SimDataBlock Parent;

public:

   StringTableEntry mShape;
   Vector<StringTableEntry> mSequences;

   TSShapeConstructorT2();

   void readT2Fields( T2Reader &reader );
   bool onAdd();

   DECLARE_CONOBJECT( TSShapeConstructorT2 );
};

#endif
