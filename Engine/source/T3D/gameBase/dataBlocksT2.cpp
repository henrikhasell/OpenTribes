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

IMPLEMENT_DATABLOCK_T2( CommanderIconData );
IMPLEMENT_DATABLOCK_T2( CannedChatItem );
IMPLEMENT_DATABLOCK_T2( SensorData );
IMPLEMENT_DATABLOCK_T2( AudioEnvironment );
IMPLEMENT_DATABLOCK_T2( AudioSampleEnvironment );
IMPLEMENT_DATABLOCK_T2( BombProjectileData );
IMPLEMENT_DATABLOCK_T2( ELFProjectileData );
IMPLEMENT_DATABLOCK_T2( EffectProfile );
IMPLEMENT_DATABLOCK_T2( FireballAtmosphereData );
IMPLEMENT_DATABLOCK_T2( FlareProjectileData );
IMPLEMENT_DATABLOCK_T2( JetEffectData );
IMPLEMENT_DATABLOCK_T2( ParticleEmissionDummyData );
IMPLEMENT_DATABLOCK_T2( RepairProjectileData );
IMPLEMENT_DATABLOCK_T2( RunningLightData );
IMPLEMENT_DATABLOCK_T2( SeekerProjectileData );
IMPLEMENT_DATABLOCK_T2( ShockLanceProjectileData );
IMPLEMENT_DATABLOCK_T2( ShockwaveData );
IMPLEMENT_DATABLOCK_T2( StationFXPersonalData );
