// HH: Added for Tribes 2 compatibility.
#include "platform/platform.h"
#include "core/stream/bitStream.h"
#include "core/stringTable.h"
#include "console/consoleTypes.h"
#include "console/console.h"
#include "console/simDatablock.h"
#include "T3D/gameBase/dataBlocksT2.h"
#include "T3D/shapeBase.h"
#include "T3D/gameBase/gameBase.h"

/// The datablocks Tribes 2 has and Torque does not.
///
/// A datablock's contents ride inside one event with no length in front of
/// them, so a client that reads one wrongly reads the rest of the stream
/// wrongly too.  Each class here takes exactly the bits Tribes 2 wrote.

//----------------------------------------------------------------------------

IMPLEMENT_CONOBJECT(DataBlockT2);

ConsoleDocClass( DataBlockT2,
   "@brief A Tribes 2 datablock the port reads but has no class for.\n\n@internal" );

DataBlockT2::DataBlockT2()
{
}

const U16 *DataBlockT2::getReadProgram() const
{
   static const U16 sNothing[] = { OpEnd };
   return sNothing;
}

/// Print each field a read program takes, to find where one has gone wrong.
static bool sTraceFields = false;

/// Walk a read program, taking each field off the stream and keeping it.
static const U16 *readT2(BitStream *stream, const U16 *p,
                         Vector<U32> &value, Vector<StringTableEntry> &string)
{
   for(;;)
   {
      // A program that has gone wrong must stop at the packet's end rather
      // than read on into whatever follows it.
      if(stream->isFull())
         return p;

      switch(*p++)
      {
         case DataBlockT2::OpEnd:
            return p;

         case DataBlockT2::OpFlag:
            value.push_back(stream->readFlag() ? 1 : 0);
            if(sTraceFields)
               Con::printf("   @%d flag %d", stream->getCurPos(), value.last());
            break;

         case DataBlockT2::OpInt:
         case DataBlockT2::OpFloat:
         {
            U32 bits = *p++;
            U32 v = stream->readInt(bits);
            value.push_back(v);
            if(sTraceFields)
               Con::printf("   @%d int%d %u", stream->getCurPos(), bits, v);
            break;
         }

         case DataBlockT2::OpRaw:
         {
            U32 bytes = *p++;
            U32 word = 0;
            for(U32 i = 0; i < bytes; i++)
            {
               U32 byte = stream->readInt(8);
               value.push_back(byte);
               word |= byte << (8 * i);
            }
            if(sTraceFields)
               Con::printf("   @%d raw%d %u (%g)", stream->getCurPos(), bytes,
                           word, *(F32 *)&word);
            break;
         }

         case DataBlockT2::OpString:
         {
            char buffer[256];
            stream->readString(buffer);
            string.push_back(StringTable->insert(buffer));
            if(sTraceFields)
               Con::printf("   @%d str '%s'", stream->getCurPos(), buffer);
            break;
         }

         case DataBlockT2::OpBlock:
            value.push_back(stream->readInt(DataBlockObjectIdBitSize));
            break;

         case DataBlockT2::OpIf:
         {
            U32 words = *p++;
            bool set = stream->readFlag();
            value.push_back(set ? 1 : 0);
            if(sTraceFields)
               Con::printf("   @%d if %d", stream->getCurPos(), set ? 1 : 0);
            if(set)
               readT2(stream, p, value, string);
            p += words;
            break;
         }

         case DataBlockT2::OpIfKept:
         {
            U32 words = *p++;
            bool set = !value.empty() && value.last() != 0;
            if(set)
               readT2(stream, p, value, string);
            p += words;
            break;
         }

         case DataBlockT2::OpLoop:
         {
            U32 times = *p++;
            U32 words = *p++;
            for(U32 i = 0; i < times; i++)
               readT2(stream, p, value, string);
            p += words;
            break;
         }

         case DataBlockT2::OpTimes:
         {
            U32 bits  = *p++;
            U32 bias  = *p++;
            U32 words = *p++;
            U32 times = stream->readInt(bits) + bias;
            value.push_back(times);
            if(sTraceFields)
               Con::printf("   @%d times %u", stream->getCurPos(), times);
            for(U32 i = 0; i < times && !stream->isFull(); i++)
               readT2(stream, p, value, string);
            p += words;
            break;
         }
      }
   }
}

/// Walk the same program, putting back what was read.
static const U16 *writeT2(BitStream *stream, const U16 *p,
                          const Vector<U32> &value, U32 &at,
                          const Vector<StringTableEntry> &string, U32 &strAt)
{
   for(;;)
   {
      switch(*p++)
      {
         case DataBlockT2::OpEnd:
            return p;

         case DataBlockT2::OpFlag:
            stream->writeFlag(at < (U32)value.size() ? value[at++] != 0 : false);
            break;

         case DataBlockT2::OpInt:
         case DataBlockT2::OpFloat:
         {
            U32 bits = *p++;
            stream->writeInt(at < (U32)value.size() ? value[at++] : 0, bits);
            break;
         }

         case DataBlockT2::OpRaw:
         {
            U32 bytes = *p++;
            for(U32 i = 0; i < bytes; i++)
               stream->writeInt(at < (U32)value.size() ? value[at++] : 0, 8);
            break;
         }

         case DataBlockT2::OpString:
            stream->writeString(strAt < (U32)string.size() ? string[strAt++]
                                                           : StringTable->EmptyString());
            break;

         case DataBlockT2::OpBlock:
            stream->writeInt(at < (U32)value.size() ? value[at++] : 0,
                             DataBlockObjectIdBitSize);
            break;

         case DataBlockT2::OpIf:
         {
            U32 words = *p++;
            bool set = at < (U32)value.size() ? value[at++] != 0 : false;
            stream->writeFlag(set);
            if(set)
               writeT2(stream, p, value, at, string, strAt);
            p += words;
            break;
         }

         case DataBlockT2::OpIfKept:
         {
            U32 words = *p++;
            bool set = at > 0 && at <= (U32)value.size() && value[at - 1] != 0;
            if(set)
               writeT2(stream, p, value, at, string, strAt);
            p += words;
            break;
         }

         case DataBlockT2::OpLoop:
         {
            U32 times = *p++;
            U32 words = *p++;
            for(U32 i = 0; i < times; i++)
               writeT2(stream, p, value, at, string, strAt);
            p += words;
            break;
         }

         case DataBlockT2::OpTimes:
         {
            U32 bits  = *p++;
            U32 bias  = *p++;
            U32 words = *p++;
            U32 times = at < (U32)value.size() ? value[at++] : bias;
            stream->writeInt(times - bias, bits);
            for(U32 i = 0; i < times; i++)
               writeT2(stream, p, value, at, string, strAt);
            p += words;
            break;
         }
      }
   }
}

void DataBlockT2::runProgram(BitStream *stream, const U16 *program,
                             Vector<U32> &value, Vector<StringTableEntry> &string)
{
   sTraceFields = Con::getBoolVariable("$T2::traceFields");
   readT2(stream, program, value, string);
}

void DataBlockT2::unpackData(BitStream *stream)
{
   sTraceFields = Con::getBoolVariable("$T2::traceFields");
   Parent::unpackData(stream);
   mValue.clear();
   mString.clear();
   readT2(stream, getReadProgram(), mValue, mString);
}

void DataBlockT2::packData(BitStream *stream)
{
   Parent::packData(stream);
   U32 at = 0, strAt = 0;
   writeT2(stream, getReadProgram(), mValue, at, mString, strAt);
}

//----------------------------------------------------------------------------


class CommanderIconData : public SimDataBlock
{
   typedef SimDataBlock Parent;

   enum { NumIcons = 5 };

   StringTableEntry mIcon[NumIcons];

public:
   CommanderIconData()
   {
      for(U32 i = 0; i < NumIcons; i++)
         mIcon[i] = StringTable->EmptyString();
   }

   static void initPersistFields()
   {
      Parent::initPersistFields();
   }

   void packData(BitStream *stream)
   {
      Parent::packData(stream);
      for(U32 i = 0; i < NumIcons; i++)
         stream->writeString(mIcon[i]);
   }

   void unpackData(BitStream *stream)
   {
      Parent::unpackData(stream);
      for(U32 i = 0; i < NumIcons; i++)
      {
         char buffer[256];
         stream->readString(buffer);
         mIcon[i] = StringTable->insert(buffer);
      }
   }

   DECLARE_CONOBJECT(CommanderIconData);
};

IMPLEMENT_CO_DATABLOCK_V1(CommanderIconData);

ConsoleDocClass( CommanderIconData,
   "@brief The icons the commander map draws an object with.\n\n@internal" );

// --- generated read programs ---

// HH: Added for Tribes 2 compatibility.
// Short names for the program words, which are read out of the retail binary
// by scratchpad/genprog.py -- one entry per field, in the wire's order.
static const U16 OpEnd    = DataBlockT2::OpEnd;
static const U16 OpFlag   = DataBlockT2::OpFlag;
static const U16 OpInt    = DataBlockT2::OpInt;
static const U16 OpFloat  = DataBlockT2::OpFloat;
static const U16 OpRaw    = DataBlockT2::OpRaw;
static const U16 OpString = DataBlockT2::OpString;
static const U16 OpBlock  = DataBlockT2::OpBlock;
static const U16 OpIf     = DataBlockT2::OpIf;
static const U16 OpLoop   = DataBlockT2::OpLoop;
static const U16 OpTimes  = DataBlockT2::OpTimes;
static const U16 OpIfKept = DataBlockT2::OpIfKept;

static const U16 sProgTSShapeConstructorT2[] = { OpString, OpTimes, 7, 0, 2, OpString, OpEnd, OpEnd };
IMPLEMENT_DATABLOCK_T2( TSShapeConstructorT2, sProgTSShapeConstructorT2 );

static const U16 sProgCannedChatItem[] = { OpEnd };
IMPLEMENT_DATABLOCK_T2( CannedChatItem, sProgCannedChatItem );

static const U16 sProgSensorData[] = { OpEnd };
IMPLEMENT_DATABLOCK_T2( SensorData, sProgSensorData );

static const U16 sProgAudioDescription[] = { OpFloat, 6, OpIf, 7, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpIf, 17, OpRaw, 4, OpRaw, 4, OpInt, 9, OpInt, 9, OpFloat, 6, OpInt, 9, OpInt, 8, OpRaw, 4, OpEnd, OpInt, 3, OpEnd };
IMPLEMENT_DATABLOCK_T2( AudioDescription, sProgAudioDescription );

static const U16 sProgAudioEnvironment[] = { OpIf, 3, OpInt, 5, OpEnd, OpInt, 14, OpInt, 15, OpInt, 14, OpInt, 8, OpInt, 8, OpInt, 8, OpInt, 9, OpInt, 7, OpInt, 14, OpInt, 9, OpInt, 10, OpInt, 8, OpInt, 10, OpInt, 6, OpInt, 8, OpEnd };
IMPLEMENT_DATABLOCK_T2( AudioEnvironment, sProgAudioEnvironment );

static const U16 sProgAudioProfile[] = { OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpString, OpEnd };
IMPLEMENT_DATABLOCK_T2( AudioProfile, sProgAudioProfile );

static const U16 sProgAudioSampleEnvironment[] = { OpInt, 14, OpInt, 14, OpInt, 14, OpInt, 14, OpInt, 9, OpInt, 8, OpInt, 9, OpInt, 8, OpInt, 9, OpInt, 9, OpInt, 9, OpInt, 14, OpInt, 3, OpEnd };
IMPLEMENT_DATABLOCK_T2( AudioSampleEnvironment, sProgAudioSampleEnvironment );

static const U16 sProgBombProjectileData[] = { OpString, OpRaw, 4, OpRaw, 4, OpFlag, OpIf, 7, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 9, OpFloat, 8, OpFloat, 7, OpFloat, 7, OpFloat, 7, OpEnd, OpIf, 7, OpFloat, 7, OpFloat, 7, OpFloat, 7, OpEnd, OpRaw, 1, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpLoop, 2, 2, OpString, OpEnd, OpEnd };
IMPLEMENT_DATABLOCK_T2( BombProjectileData, sProgBombProjectileData );

static const U16 sProgELFProjectileData[] = { OpString, OpRaw, 4, OpRaw, 4, OpFlag, OpIf, 7, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 9, OpFloat, 8, OpFloat, 7, OpFloat, 7, OpFloat, 7, OpEnd, OpIf, 7, OpFloat, 7, OpFloat, 7, OpFloat, 7, OpEnd, OpRaw, 1, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpLoop, 3, 2, OpString, OpEnd, OpLoop, 1, 5, OpIf, 2, OpBlock, OpEnd, OpEnd, OpEnd };
IMPLEMENT_DATABLOCK_T2( ELFProjectileData, sProgELFProjectileData );

static const U16 sProgEffectProfile[] = { OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 1, OpString, OpEnd };
IMPLEMENT_DATABLOCK_T2( EffectProfile, sProgEffectProfile );

static const U16 sProgEnergyProjectileData[] = { OpString, OpRaw, 4, OpRaw, 4, OpFlag, OpIf, 7, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 9, OpFloat, 8, OpFloat, 7, OpFloat, 7, OpFloat, 7, OpEnd, OpIf, 7, OpFloat, 7, OpFloat, 7, OpFloat, 7, OpEnd, OpRaw, 1, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpLoop, 2, 2, OpString, OpEnd, OpEnd };
IMPLEMENT_DATABLOCK_T2( EnergyProjectileData, sProgEnergyProjectileData );

static const U16 sProgFireballAtmosphereData[] = { OpIf, 2, OpBlock, OpEnd, OpEnd };
IMPLEMENT_DATABLOCK_T2( FireballAtmosphereData, sProgFireballAtmosphereData );

static const U16 sProgFlareProjectileData[] = { OpString, OpRaw, 4, OpRaw, 4, OpFlag, OpIf, 7, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 9, OpFloat, 8, OpFloat, 7, OpFloat, 7, OpFloat, 7, OpEnd, OpIf, 7, OpFloat, 7, OpFloat, 7, OpFloat, 7, OpEnd, OpRaw, 1, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 1, OpLoop, 2, 2, OpString, OpEnd, OpEnd };
IMPLEMENT_DATABLOCK_T2( FlareProjectileData, sProgFlareProjectileData );

static const U16 sProgForceFieldBareData[] = { OpRaw, 4, OpRaw, 4, OpRaw, 4, OpFlag, OpFlag, OpRaw, 3, OpRaw, 3, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpLoop, 5, 2, OpString, OpEnd, OpEnd };
IMPLEMENT_DATABLOCK_T2( ForceFieldBareData, sProgForceFieldBareData );

static const U16 sProgGrenadeProjectileData[] = { OpString, OpRaw, 4, OpRaw, 4, OpFlag, OpIf, 7, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 9, OpFloat, 8, OpFloat, 7, OpFloat, 7, OpFloat, 7, OpEnd, OpIf, 7, OpFloat, 7, OpFloat, 7, OpFloat, 7, OpEnd, OpRaw, 1, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd };
IMPLEMENT_DATABLOCK_T2( GrenadeProjectileData, sProgGrenadeProjectileData );

static const U16 sProgJetEffectData[] = { OpRaw, 3, OpRaw, 3, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpLoop, 1, 5, OpIf, 2, OpString, OpEnd, OpEnd, OpEnd };
IMPLEMENT_DATABLOCK_T2( JetEffectData, sProgJetEffectData );

static const U16 sProgLinearFlareProjectileData[] = { OpString, OpRaw, 4, OpRaw, 4, OpFlag, OpIf, 7, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 9, OpFloat, 8, OpFloat, 7, OpFloat, 7, OpFloat, 7, OpEnd, OpIf, 7, OpFloat, 7, OpFloat, 7, OpFloat, 7, OpEnd, OpRaw, 1, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpFlag, OpInt, 7, OpInt, 7, OpRaw, 4, OpRaw, 4, OpFlag, OpRaw, 4, OpRaw, 3, OpString, OpString, OpLoop, 3, 3, OpRaw, 4, OpEnd, OpEnd };
IMPLEMENT_DATABLOCK_T2( LinearFlareProjectileData, sProgLinearFlareProjectileData );

static const U16 sProgLinearProjectileData[] = { OpString, OpRaw, 4, OpRaw, 4, OpFlag, OpIf, 7, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 9, OpFloat, 8, OpFloat, 7, OpFloat, 7, OpFloat, 7, OpEnd, OpIf, 7, OpFloat, 7, OpFloat, 7, OpFloat, 7, OpEnd, OpRaw, 1, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpFlag, OpInt, 7, OpInt, 7, OpRaw, 4, OpRaw, 4, OpFlag, OpEnd };
IMPLEMENT_DATABLOCK_T2( LinearProjectileData, sProgLinearProjectileData );

static const U16 sProgParticleEmissionDummyData[] = { OpRaw, 4, OpEnd };
IMPLEMENT_DATABLOCK_T2( ParticleEmissionDummyData, sProgParticleEmissionDummyData );

static const U16 sProgRepairProjectileData[] = { OpString, OpRaw, 4, OpRaw, 4, OpFlag, OpIf, 7, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 9, OpFloat, 8, OpFloat, 7, OpFloat, 7, OpFloat, 7, OpEnd, OpIf, 7, OpFloat, 7, OpFloat, 7, OpFloat, 7, OpEnd, OpRaw, 1, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpLoop, 2, 2, OpString, OpEnd, OpEnd };
IMPLEMENT_DATABLOCK_T2( RepairProjectileData, sProgRepairProjectileData );

static const U16 sProgRunningLightData[] = { OpRaw, 4, OpRaw, 3, OpRaw, 4, OpRaw, 4, OpString, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpLoop, 1, 5, OpIf, 2, OpString, OpEnd, OpEnd, OpEnd };
IMPLEMENT_DATABLOCK_T2( RunningLightData, sProgRunningLightData );

static const U16 sProgSeekerProjectileData[] = { OpString, OpRaw, 4, OpRaw, 4, OpFlag, OpIf, 7, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 9, OpFloat, 8, OpFloat, 7, OpFloat, 7, OpFloat, 7, OpEnd, OpIf, 7, OpFloat, 7, OpFloat, 7, OpFloat, 7, OpEnd, OpRaw, 1, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 1, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpString, OpString, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpEnd };
IMPLEMENT_DATABLOCK_T2( SeekerProjectileData, sProgSeekerProjectileData );

static const U16 sProgShockLanceProjectileData[] = { OpString, OpRaw, 4, OpRaw, 4, OpFlag, OpIf, 7, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 9, OpFloat, 8, OpFloat, 7, OpFloat, 7, OpFloat, 7, OpEnd, OpIf, 7, OpFloat, 7, OpFloat, 7, OpFloat, 7, OpEnd, OpRaw, 1, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpIf, 2, OpBlock, OpEnd, OpLoop, 2, 9, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpLoop, 4, 2, OpString, OpEnd, OpLoop, 1, 5, OpIf, 2, OpBlock, OpEnd, OpEnd, OpEnd };
IMPLEMENT_DATABLOCK_T2( ShockLanceProjectileData, sProgShockLanceProjectileData );

static const U16 sProgShockwaveData[] = { OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 1, OpRaw, 1, OpRaw, 1, OpRaw, 1, OpRaw, 1, OpLoop, 3, 5, OpIf, 2, OpBlock, OpEnd, OpEnd, OpLoop, 4, 3, OpRaw, 3, OpEnd, OpLoop, 4, 3, OpRaw, 4, OpEnd, OpLoop, 2, 2, OpString, OpEnd, OpEnd };
IMPLEMENT_DATABLOCK_T2( ShockwaveData, sProgShockwaveData );

static const U16 sProgSniperProjectileData[] = { OpString, OpRaw, 4, OpRaw, 4, OpFlag, OpIf, 7, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 9, OpFloat, 8, OpFloat, 7, OpFloat, 7, OpFloat, 7, OpEnd, OpIf, 7, OpFloat, 7, OpFloat, 7, OpFloat, 7, OpEnd, OpRaw, 1, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 3, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 3, OpRaw, 4, OpString, OpEnd };
IMPLEMENT_DATABLOCK_T2( SniperProjectileData, sProgSniperProjectileData );

static const U16 sProgStationFXPersonalData[] = { OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpString, OpString, OpLoop, 2, 2, OpString, OpEnd, OpEnd };
IMPLEMENT_DATABLOCK_T2( StationFXPersonalData, sProgStationFXPersonalData );

static const U16 sProgStationFXVehicleData[] = { OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 3, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpString, OpLoop, 4, 3, OpString, OpString, OpEnd, OpLoop, 2, 2, OpString, OpEnd, OpEnd };
IMPLEMENT_DATABLOCK_T2( StationFXVehicleData, sProgStationFXVehicleData );

static const U16 sProgTargetProjectileData[] = { OpString, OpRaw, 4, OpRaw, 4, OpFlag, OpIf, 7, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 9, OpFloat, 8, OpFloat, 7, OpFloat, 7, OpFloat, 7, OpEnd, OpIf, 7, OpFloat, 7, OpFloat, 7, OpFloat, 7, OpEnd, OpRaw, 1, OpRaw, 4, OpRaw, 4, OpRaw, 3, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpLoop, 4, 2, OpString, OpEnd, OpEnd };
IMPLEMENT_DATABLOCK_T2( TargetProjectileData, sProgTargetProjectileData );

static const U16 sProgTracerProjectileData[] = { OpString, OpRaw, 4, OpRaw, 4, OpFlag, OpIf, 7, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 9, OpFloat, 8, OpFloat, 7, OpFloat, 7, OpFloat, 7, OpEnd, OpIf, 7, OpFloat, 7, OpFloat, 7, OpFloat, 7, OpEnd, OpRaw, 1, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpFlag, OpInt, 7, OpInt, 7, OpRaw, 4, OpRaw, 4, OpFlag, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 1, OpRaw, 3, OpRaw, 4, OpRaw, 4, OpRaw, 1, OpLoop, 2, 2, OpString, OpEnd, OpEnd };
IMPLEMENT_DATABLOCK_T2( TracerProjectileData, sProgTracerProjectileData );

static const U16 sProgTurretData[] = { OpIf, 3, OpRaw, 4, OpEnd, OpString, OpIf, 3, OpRaw, 4, OpEnd, OpIf, 3, OpRaw, 4, OpEnd, OpIf, 3, OpRaw, 4, OpEnd, OpIf, 3, OpRaw, 4, OpEnd, OpIf, 3, OpRaw, 4, OpEnd, OpIf, 3, OpRaw, 4, OpEnd, OpIf, 3, OpRaw, 4, OpEnd, OpIf, 3, OpRaw, 4, OpEnd, OpIf, 3, OpRaw, 4, OpEnd, OpString, OpIf, 11, OpInt, 10, OpRaw, 1, OpRaw, 1, OpRaw, 1, OpRaw, 1, OpEnd, OpIf, 3, OpRaw, 4, OpEnd, OpString, OpString, OpFlag, OpFlag, OpFlag, OpFlag, OpFlag, OpFlag, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpFlag, OpFlag, OpFlag, OpRaw, 4, OpIf, 7, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpLoop, 8, 14, OpIf, 11, OpString, OpIf, 2, OpString, OpEnd, OpFlag, OpFlag, OpFlag, OpFlag, OpFlag, OpEnd, OpEnd, OpFlag, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpFlag, OpInt, 2, OpRaw, 4, OpRaw, 4, OpEnd };
IMPLEMENT_DATABLOCK_T2( TurretData, sProgTurretData );

static const U16 sProgTurretImageData[] = { OpIf, 3, OpRaw, 4, OpEnd, OpString, OpRaw, 4, OpIf, 1, OpEnd, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpFlag, OpRaw, 4, OpFlag, OpRaw, 4, OpFlag, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 12, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpFlag, OpRaw, 4, OpEnd, OpFlag, OpInt, 2, OpRaw, 4, OpRaw, 4, OpFloat, 7, OpFloat, 7, OpFloat, 7, OpFloat, 7, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpIf, 2, OpBlock, OpEnd, OpFlag, OpIf, 70, OpString, OpInt, 5, OpInt, 5, OpInt, 5, OpInt, 5, OpInt, 5, OpInt, 5, OpInt, 5, OpInt, 5, OpInt, 5, OpInt, 5, OpInt, 5, OpIf, 3, OpRaw, 4, OpEnd, OpFlag, OpFlag, OpFlag, OpFlag, OpFlag, OpFlag, OpIf, 3, OpRaw, 4, OpEnd, OpInt, 3, OpInt, 3, OpInt, 3, OpIf, 3, OpInt, 16, OpEnd, OpIf, 3, OpInt, 16, OpEnd, OpFlag, OpFlag, OpIf, 6, OpBlock, OpRaw, 4, OpRaw, 4, OpEnd, OpIf, 2, OpBlock, OpEnd, OpEnd, OpInt, 8, OpInt, 8, OpInt, 11, OpInt, 11, OpFlag, OpRaw, 4, OpFlag, OpEnd };
IMPLEMENT_DATABLOCK_T2( TurretImageData, sProgTurretImageData );

// --- end generated read programs ---

//----------------------------------------------------------------------------

// HH: Added for Tribes 2 compatibility.
// The classes the port has whose fields Tribes 2 writes differently.  The
// hand-ported part of the chain runs as it stands, then the rest of the
// block is taken off by its field widths, read out of the retail binary.












static const U16 sDeltaCameraData[] = { OpEnd };
static const U16 sDeltaDebrisData[] = { OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 1, OpRaw, 1, OpRaw, 1, OpRaw, 1, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 1, OpRaw, 1, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 1, OpString, OpString, OpLoop, 2, 5, OpIf, 2, OpBlock, OpEnd, OpEnd, OpIf, 2, OpBlock, OpEnd, OpEnd };
static const U16 sDeltaDecalData[] = { OpRaw, 4, OpRaw, 4, OpString, OpEnd };
static const U16 sDeltaExplosionData[] = { OpString, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpInt, 14, OpRaw, 4, OpFlag, OpIf, 7, OpInt, 16, OpInt, 16, OpInt, 16, OpEnd, OpInt, 14, OpInt, 8, OpInt, 8, OpInt, 9, OpInt, 9, OpInt, 10, OpInt, 10, OpInt, 14, OpInt, 14, OpInt, 16, OpInt, 16, OpInt, 16, OpInt, 16, OpRaw, 4, OpFlag, OpFlag, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpIf, 2, OpBlock, OpEnd, OpLoop, 1, 5, OpIf, 2, OpBlock, OpEnd, OpEnd, OpLoop, 4, 5, OpIf, 2, OpBlock, OpEnd, OpEnd, OpLoop, 5, 5, OpIf, 2, OpBlock, OpEnd, OpEnd, OpInt, 3, OpFloat, 8, OpInt, 14, OpInt, 14, OpInt, 14, OpEnd };
static const U16 sDeltaFlyingVehicleData[] = { OpRaw, 4, OpRaw, 4, OpLoop, 2, 5, OpIf, 2, OpBlock, OpEnd, OpEnd, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpLoop, 5, 5, OpIf, 2, OpBlock, OpEnd, OpEnd, OpIf, 2, OpBlock, OpEnd, OpLoop, 3, 5, OpIf, 2, OpBlock, OpEnd, OpEnd, OpLoop, 2, 5, OpIf, 2, OpBlock, OpEnd, OpEnd, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpLoop, 2, 3, OpRaw, 4, OpEnd, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpLoop, 2, 5, OpIf, 2, OpBlock, OpEnd, OpEnd, OpLoop, 4, 5, OpIf, 2, OpBlock, OpEnd, OpEnd, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd };
static const U16 sDeltaHoverVehicleData[] = { OpRaw, 4, OpRaw, 4, OpLoop, 2, 5, OpIf, 2, OpBlock, OpEnd, OpEnd, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpLoop, 5, 5, OpIf, 2, OpBlock, OpEnd, OpEnd, OpIf, 2, OpBlock, OpEnd, OpLoop, 3, 5, OpIf, 2, OpBlock, OpEnd, OpEnd, OpLoop, 2, 5, OpIf, 2, OpBlock, OpEnd, OpEnd, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpLoop, 2, 3, OpRaw, 4, OpEnd, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpLoop, 3, 5, OpIf, 2, OpBlock, OpEnd, OpEnd, OpLoop, 3, 5, OpIf, 2, OpBlock, OpEnd, OpEnd, OpIf, 2, OpBlock, OpEnd, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd };
static const U16 sDeltaItemData[] = { OpFloat, 10, OpFloat, 10, OpFlag, OpIf, 3, OpFloat, 10, OpEnd, OpIf, 3, OpRaw, 4, OpEnd, OpIf, 16, OpInt, 2, OpFloat, 7, OpFloat, 7, OpFloat, 7, OpFloat, 7, OpRaw, 4, OpRaw, 4, OpFlag, OpEnd, OpEnd };
static const U16 sDeltaLightningData[] = { OpLoop, 8, 5, OpIf, 2, OpBlock, OpEnd, OpEnd, OpLoop, 8, 2, OpString, OpEnd, OpIf, 2, OpBlock, OpEnd, OpEnd };
static const U16 sDeltaParticleData[] = { OpFloat, 10, OpIf, 3, OpRaw, 4, OpEnd, OpInt, 12, OpFloat, 9, OpIf, 3, OpRaw, 4, OpEnd, OpInt, 10, OpInt, 10, OpIf, 3, OpRaw, 4, OpEnd, OpIf, 5, OpInt, 11, OpInt, 11, OpEnd, OpFlag, OpTimes, 2, 1, 13, OpFloat, 7, OpFloat, 7, OpFloat, 7, OpFloat, 7, OpFloat, 14, OpFloat, 8, OpEnd, OpTimes, 6, 0, 2, OpString, OpEnd, OpEnd };
static const U16 sDeltaParticleEmitterData[] = { OpInt, 10, OpInt, 10, OpInt, 16, OpInt, 14, OpIf, 3, OpInt, 16, OpEnd, OpInt, 8, OpInt, 8, OpIf, 3, OpInt, 9, OpEnd, OpIf, 3, OpInt, 9, OpEnd, OpFlag, OpFlag, OpFlag, OpInt, 10, OpInt, 10, OpFlag, OpFlag, OpRaw, 4, OpIf, 2, OpBlock, OpEnd, OpEnd };
static const U16 sDeltaPlayerData[] = { OpFlag, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpInt, 7, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpIf, 2, OpBlock, OpEnd, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpIf, 2, OpBlock, OpEnd, OpRaw, 4, OpRaw, 4, OpIf, 2, OpBlock, OpEnd, OpRaw, 4, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd };
static const U16 sDeltaPrecipitationData[] = { OpIf, 2, OpBlock, OpEnd, OpRaw, 4, OpRaw, 4, OpString, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd };
static const U16 sDeltaProjectileData[] = { OpString, OpRaw, 4, OpRaw, 4, OpFlag, OpIf, 7, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 9, OpFloat, 8, OpFloat, 7, OpFloat, 7, OpFloat, 7, OpEnd, OpIf, 7, OpFloat, 7, OpFloat, 7, OpFloat, 7, OpEnd, OpRaw, 1, OpRaw, 4, OpEnd };
static const U16 sDeltaShapeBaseImageData[] = { OpIf, 3, OpRaw, 4, OpEnd, OpString, OpRaw, 4, OpIf, 1, OpEnd, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpFlag, OpRaw, 4, OpFlag, OpRaw, 4, OpFlag, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpIf, 12, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpFlag, OpRaw, 4, OpEnd, OpFlag, OpInt, 2, OpRaw, 4, OpRaw, 4, OpFloat, 7, OpFloat, 7, OpFloat, 7, OpFloat, 7, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpIf, 2, OpBlock, OpEnd, OpFlag, OpIf, 70, OpString, OpInt, 5, OpInt, 5, OpInt, 5, OpInt, 5, OpInt, 5, OpInt, 5, OpInt, 5, OpInt, 5, OpInt, 5, OpInt, 5, OpInt, 5, OpIf, 3, OpRaw, 4, OpEnd, OpFlag, OpFlag, OpFlag, OpFlag, OpFlag, OpFlag, OpIf, 3, OpRaw, 4, OpEnd, OpInt, 3, OpInt, 3, OpInt, 3, OpIf, 3, OpInt, 16, OpEnd, OpIf, 3, OpInt, 16, OpEnd, OpFlag, OpFlag, OpIf, 6, OpBlock, OpRaw, 4, OpRaw, 4, OpEnd, OpIf, 2, OpBlock, OpEnd, OpEnd, OpEnd };
static const U16 sDeltaSplashData[] = { OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpIf, 2, OpBlock, OpEnd, OpLoop, 3, 5, OpIf, 2, OpBlock, OpEnd, OpEnd, OpLoop, 4, 3, OpRaw, 3, OpEnd, OpLoop, 4, 3, OpRaw, 4, OpEnd, OpLoop, 2, 2, OpString, OpEnd, OpEnd };
static const U16 sDeltaStaticShapeData[] = { OpFlag, OpRaw, 4, OpEnd };
static const U16 sDeltaTriggerData[] = { OpRaw, 4, OpEnd };
static const U16 sDeltaWheeledVehicleData[] = { OpRaw, 4, OpRaw, 4, OpLoop, 2, 5, OpIf, 2, OpBlock, OpEnd, OpEnd, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpLoop, 5, 5, OpIf, 2, OpBlock, OpEnd, OpEnd, OpIf, 2, OpBlock, OpEnd, OpLoop, 3, 5, OpIf, 2, OpBlock, OpEnd, OpEnd, OpLoop, 2, 5, OpIf, 2, OpBlock, OpEnd, OpEnd, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpLoop, 2, 3, OpRaw, 4, OpEnd, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpIf, 2, OpBlock, OpEnd, OpIf, 2, OpBlock, OpEnd, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpRaw, 4, OpEnd };

enum DeltaBase { BaseSimDataBlock, BaseGameBase, BaseShapeBase };

static const struct DeltaEntry
{
   const char *name;
   U32         base;
   const U16  *program;
} sDeltaTable[] =
{
   { "CameraData", BaseShapeBase, sDeltaCameraData },
   { "DebrisData", BaseGameBase, sDeltaDebrisData },
   { "DecalData", BaseSimDataBlock, sDeltaDecalData },
   { "ExplosionData", BaseGameBase, sDeltaExplosionData },
   { "FlyingVehicleData", BaseShapeBase, sDeltaFlyingVehicleData },
   { "HoverVehicleData", BaseShapeBase, sDeltaHoverVehicleData },
   { "ItemData", BaseShapeBase, sDeltaItemData },
   { "LightningData", BaseGameBase, sDeltaLightningData },
   { "ParticleData", BaseSimDataBlock, sDeltaParticleData },
   { "ParticleEmitterData", BaseGameBase, sDeltaParticleEmitterData },
   { "PlayerData", BaseShapeBase, sDeltaPlayerData },
   { "PrecipitationData", BaseGameBase, sDeltaPrecipitationData },
   { "ProjectileData", BaseGameBase, sDeltaProjectileData },
   { "ShapeBaseImageData", BaseGameBase, sDeltaShapeBaseImageData },
   { "SplashData", BaseGameBase, sDeltaSplashData },
   { "StaticShapeData", BaseShapeBase, sDeltaStaticShapeData },
   { "TriggerData", BaseGameBase, sDeltaTriggerData },
   { "WheeledVehicleData", BaseShapeBase, sDeltaWheeledVehicleData },
};


bool DataBlockT2::unpackDelta( SimDataBlock *obj, BitStream *stream )
{
   const char *name = obj->getClassName();
   for( U32 i = 0; i < sizeof( sDeltaTable ) / sizeof( sDeltaTable[ 0 ] ); i++ )
   {
      if( dStrcmp( name, sDeltaTable[ i ].name ) )
         continue;

      switch( sDeltaTable[ i ].base )
      {
         case BaseShapeBase:
            static_cast< ShapeBaseData * >( obj )->ShapeBaseData::unpackData( stream );
            break;
         case BaseGameBase:
            static_cast< GameBaseData * >( obj )->GameBaseData::unpackData( stream );
            break;
         default:
            obj->SimDataBlock::unpackData( stream );
            break;
      }

      Vector< U32 >              value;
      Vector< StringTableEntry > string;
      readT2( stream, sDeltaTable[ i ].program, value, string );
      return true;
   }

   return false;
}
