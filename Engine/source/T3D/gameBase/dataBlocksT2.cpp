// HH: Added for Tribes 2 compatibility.
#include "platform/platform.h"
#include "core/stream/bitStream.h"
#include "core/stringTable.h"
#include "console/consoleTypes.h"
#include "console/simDatablock.h"

/// The datablocks Tribes 2 has and Torque does not.
///
/// A datablock's contents ride inside one event with no length in front of
/// them, so a client that reads one wrongly reads the rest of the stream
/// wrongly too.  Each class here takes exactly the bits Tribes 2 wrote.

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
