#include "platform/platform.h"
#include "core/stream/bitStream.h"
#include "core/stringTable.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "console/simDatablock.h"
#include "T3D/gameBase/dataBlocksT2.h"
#include "T3D/gameBase/t2Layout.h"
#include "core/resourceManager.h"
#include "ts/tsShapeConstruct.h"

IMPLEMENT_CONOBJECT(DataBlockT2);

ConsoleDocClass( DataBlockT2,
   "@brief A Tribes 2 datablock the port reads by its layout.\n\n@internal" );

DataBlockT2::DataBlockT2()
{
   shapeName = StringTable->EmptyString();
}

bool DataBlockT2::preload(bool server, String &errorStr)
{
   if(!Parent::preload(server, errorStr))
      return false;

   if(namesGameBaseShape() && shapeName && shapeName[0])
   {
      mShape = ResourceManager::get().load(T2Layout::resourcePath(shapeName, "shapes/"));
   }

   return true;
}

static String sequenceEntry(const String &field)
{
   const String::SizeType split = field.find(' ', 0, String::Right);
   if(split == String::NPos)
      return T2Layout::resourcePath(field.c_str(), "shapes/");

   return T2Layout::resourcePath(field.substr(0, split).c_str(), "shapes/") +
          field.substr(split);
}

class TSShapeConstructorT2 : public DataBlockT2
{
   typedef DataBlockT2 Parent;

   Vector<String> mFields;

public:
   DECLARE_CONOBJECT(TSShapeConstructorT2);

   bool unpackData(BitStream *stream, U32 classId)
   {
      if(!Parent::unpackData(stream, classId))
         return false;

      mFields = T2Layout::lastStrings();
      return true;
   }

   bool preload(bool server, String &errorStr)
   {
      if(!Parent::preload(server, errorStr))
         return false;

      if(mFields.empty())
         return true;

      const String shape = T2Layout::resourcePath(mFields[0].c_str(),
                                                  "shapes/");
      if(TSShapeConstructor::findShapeConstructor(shape))
         return true;

      TSShapeConstructor *ctor = new TSShapeConstructor(shape);
      for(U32 i = 1; i < mFields.size(); i++)
      {
         const String entry = sequenceEntry(mFields[i]);
         ctor->setDataField(StringTable->insert("sequence"), NULL,
                            entry.c_str());
      }

      if(!ctor->registerObject())
         delete ctor;

      return true;
   }
};

IMPLEMENT_CO_DATABLOCK_V1(TSShapeConstructorT2);

ConsoleDocClass( TSShapeConstructorT2,
   "@brief The Tribes 2 datablock that names a shape's animations.\n\n"
   "@internal" );

IMPLEMENT_DATABLOCK_T2( CannedChatItem );
IMPLEMENT_DATABLOCK_T2( SensorData );
IMPLEMENT_DATABLOCK_T2( AudioEnvironment );
IMPLEMENT_DATABLOCK_T2( AudioSampleEnvironment );
IMPLEMENT_DATABLOCK_T2( BombProjectileData );
static const StubT2::Field sT2EffectProfileFields[] =
{
   { "minDistance", T2Layout::T2DbEffectMinDistance, StubT2::Whole    },
   { "maxDistance", T2Layout::T2DbEffectMaxDistance, StubT2::Whole    },
   { "0.125",       T2Layout::T2DbEffectThird,       StubT2::Constant },
   { "effectname",  T2Layout::T2DbEffectName,        StubT2::Text     },
};

IMPLEMENT_DATABLOCK_T2_WRITES( EffectProfile );
static const StubT2::Field sT2FireballAtmosphereDataFields[] =
{
   { "fireball", T2Layout::T2DbFireballDebris, StubT2::DataBlock },
};

IMPLEMENT_DATABLOCK_T2_WRITES( FireballAtmosphereData );
IMPLEMENT_DATABLOCK_T2( FlareProjectileData );
// HH: The jet flame, settled against a recorded retail server: two colours,
// then seven whole floats, then the texture behind a flag. Body 1 of demo004
// is HumanArmorJetEffect, whose 0.05, 0.75, -15 and 2.0 pin four of the seven
// outright, and body 2 is the medium armour, whose yOffset of 0.4 pins the
// last. activateTime and width are 0.2 in every jet effect the game has, so
// nothing on the wire can tell those two apart; the order is the script's.
static const StubT2::Field sT2JetEffectDataFields[] =
{
   { "coolColor",      T2Layout::T2DbJetCoolColor,      StubT2::Colour },
   { "hotColor",       T2Layout::T2DbJetHotColor,       StubT2::Colour },
   { "activateTime",   T2Layout::T2DbJetActivateTime,   StubT2::Whole  },
   { "deactivateTime", T2Layout::T2DbJetDeactivateTime, StubT2::Whole  },
   { "length",         T2Layout::T2DbJetLength,         StubT2::Whole  },
   { "width",          T2Layout::T2DbJetWidth,          StubT2::Whole  },
   { "speed",          T2Layout::T2DbJetSpeed,          StubT2::Whole  },
   { "stretch",        T2Layout::T2DbJetStretch,        StubT2::Whole  },
   { "yOffset",        T2Layout::T2DbJetYOffset,        StubT2::Whole  },
   { "texture",        T2Layout::T2DbJetTexture,        StubT2::Text   },
};

IMPLEMENT_DATABLOCK_T2_WRITES( JetEffectData );
static const StubT2::Field sT2ParticleEmissionDummyDataFields[] =
{
   { "timeMultiple", T2Layout::T2DbEmissionDummyTimeMultiple, StubT2::Whole },
};

IMPLEMENT_DATABLOCK_T2_WRITES( ParticleEmissionDummyData );
// A light names its nodes one way or the other by its type, and a script sets
// only the set it means, so both namings are listed against the same slots.
static const StubT2::Field sT2RunningLightDataFields[] =
{
   { "radius",        T2Layout::T2DbLightRadius,    StubT2::Whole   },
   { "pointSize",     T2Layout::T2DbLightRadius,    StubT2::Whole   },
   { "color",         T2Layout::T2DbLightColor,     StubT2::Colour  },
   { "pointColor",    T2Layout::T2DbLightColor,     StubT2::Colour  },
   { "type",          T2Layout::T2DbLightType,      StubT2::Word    },
   { "length",        T2Layout::T2DbLightLength,    StubT2::Whole   },
   { "nodeName",      T2Layout::T2DbLightNodeName,  StubT2::Text    },
   { "pointNodeName", T2Layout::T2DbLightNodeName,  StubT2::Text    },
   { "direction",     T2Layout::T2DbLightDirection, StubT2::Vector3 },
   { "offset",        T2Layout::T2DbLightOffset,    StubT2::Vector3 },
   { "texture",       T2Layout::T2DbLightTexture,   StubT2::Text    },
};

IMPLEMENT_DATABLOCK_T2_WRITES( RunningLightData );
IMPLEMENT_DATABLOCK_T2( ShockwaveData );
