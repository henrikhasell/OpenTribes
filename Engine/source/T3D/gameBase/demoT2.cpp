// HH: Added for Tribes 2 compatibility.
#include "platform/platform.h"
#include "T3D/gameBase/demoT2.h"

#include "console/console.h"
#include "console/engineAPI.h"
#include "core/stream/bitStream.h"
#include "core/stream/fileStream.h"
#include "sim/netConnection.h"
#include "T3D/gameBase/gameConnection.h"

namespace DemoT2
{

static const char *sSignature = "Tribes2 Recording";

bool readHeader(Stream *stream, U32 *version, U32 *lengthMs, U32 *startSize)
{
   char signature[256];
   stream->readString(signature);
   if(dStrcmp(signature, sSignature))
      return false;

   stream->read(version);
   stream->read(lengthMs);
   stream->read(startSize);
   return stream->getStatus() == Stream::Ok;
}

// The target manager's state (0x00670660).  Tribes 2's targeting has no
// Torque counterpart, so the bits are consumed and nothing is kept: a word of
// its own, a sensor-group by target-type grid, then 512 target slots.
void readTargetManagerState(BitStream *stream)
{
   U32 word;
   stream->read(&word);

   for(U32 i = 0; i < 32; i++)
      for(U32 j = 0; j < 32; j++)
         if(stream->readFlag())
            stream->read(&word);

   for(U32 i = 0; i < 512; i++)
   {
      if(!stream->readFlag())
         continue;

      if(stream->readFlag())
         stream->read(&word);
      if(stream->readFlag())
         stream->read(&word);

      char name[256];
      for(U32 j = 0; j < 5; j++)
         if(stream->readFlag())
            stream->readString(name);

      stream->readInt(5);                       // target type
      stream->readInt(9);                       // owner
      // The first 32 slots are the sensor groups' own and skip the read.
      if(i >= 32 && stream->readFlag())
         stream->readInt(11);                   // datablock id
      stream->readInt(7);                       // scale
   }
}

// The read at 0x006021b0, run over the client and server process lists.
void readProcessListState(BitStream *stream)
{
   bool flag;
   U32 word;
   stream->read(&flag);
   for(U32 i = 0; i < 4; i++)
      stream->read(&word);
}

} // namespace DemoT2

//-----------------------------------------------------------------------------
// The console surface `scripts/recordings.cs` and the shell's RECORDINGS tab
// call.  These are global functions in retail, not NetConnection methods.

static GameConnection *demoConnection()
{
   return dynamic_cast<GameConnection *>(NetConnection::getConnectionToServer());
}

DefineEngineFunction( getDemoVersion, S32, (),,
   "@brief The recording version this engine writes and plays.\n\n"
   "@ingroup Networking" )
{
   return DemoT2::Version;
}

DefineEngineFunction( getDemoVersionLength, const char *, (const char *fileName),,
   "@brief A recording's version and length in ms, or -1 if it cannot be read.\n\n"
   "@ingroup Networking" )
{
   char *ret = Con::getReturnBuffer(64);
   dStrcpy(ret, "-1");

   FileStream *fs = FileStream::createAndOpen(fileName, Torque::FS::File::Read);
   if(!fs)
      return ret;

   U32 version, lengthMs, startSize;
   if(DemoT2::readHeader(fs, &version, &lengthMs, &startSize))
      dSprintf(ret, 64, "%d\t%d", version, lengthMs);

   delete fs;
   return ret;
}

DefineEngineFunction( playDemo, bool, (const char *fileName),,
   "@brief Play a Tribes 2 recording.\n\n"
   "@ingroup Networking" )
{
   GameConnection *conn = new GameConnection();

   conn->setGhostFrom(false);
   conn->setGhostTo(true);
   conn->setSendingEvents(false);
   conn->setTranslatesStrings(true);
   conn->setIsConnectionToServer();
   NetConnection::setConnectionToServer(conn);

   // The name is retail's (0x005c322d), and the scripts reach the playing
   // connection by it.
   if(!conn->registerObject("ServerConnection"))
   {
      delete conn;
      return false;
   }

   if(!conn->replayDemoRecord(fileName))
   {
      conn->deleteObject();
      return false;
   }
   return true;
}

DefineEngineFunction( stopDemoPlayback, void, (),,
   "@brief Stop playback and fire demoPlaybackComplete.\n\n"
   "@ingroup Networking" )
{
   GameConnection *conn = demoConnection();
   if(conn && conn->isPlayingBack())
      conn->stopDemoPlayback();
}

DefineEngineFunction( isPlayingDemo, bool, (),,
   "@brief Whether a recording is playing.\n\n"
   "@ingroup Networking" )
{
   GameConnection *conn = demoConnection();
   return conn && conn->isPlayingBack();
}

DefineEngineFunction( getDemoCurrentTime, S32, (),,
   "@brief How far playback has got, in ms, or -1 when nothing is playing.\n\n"
   "@ingroup Networking" )
{
   GameConnection *conn = demoConnection();
   if(!conn || !conn->isPlayingBack())
      return -1;
   return conn->getDemoCurrentTime();
}

DefineEngineFunction( isRecordingDemo, bool, (),,
   "@brief Whether a recording is being written.\n\n"
   "@ingroup Networking" )
{
   GameConnection *conn = demoConnection();
   return conn && conn->isRecording();
}

DefineEngineFunction( startRecord, void, (const char *fileName),,
   "@brief Begin recording. Not implemented yet.\n\n"
   "@ingroup Networking" )
{
   TORQUE_UNUSED(fileName);
}

DefineEngineFunction( stopRecord, void, (),,
   "@brief End recording. Not implemented yet.\n\n"
   "@ingroup Networking" )
{
}

DefineEngineFunction( isDemo, bool, (),,
   "@brief Whether this is the shareware build, which retail answers no.\n\n"
   "@ingroup Platform" )
{
   return false;
}
