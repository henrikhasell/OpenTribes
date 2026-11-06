// HH: Added for Tribes 2 compatibility.
#include "platform/platform.h"
#include "gui/commander/commanderIconData.h"

#include "console/consoleTypes.h"
#include "core/stream/fileStream.h"
#include "core/strings/stringUnit.h"
#include "gui/core/guiTypes.h"
#include "T3D/gameBase/t2Layout.h"

IMPLEMENT_CO_DATABLOCK_V1(CommanderIconData);

ConsoleDocClass( CommanderIconData,
   "@brief What the commander map draws an object with.\n\n@internal" );

static const char *sStateNames[CommanderIconData::NumStates] =
{
   "baseImage", "activeImage", "inactiveImage", "selectImage", "hilightImage",
};

CommanderIconData::CommanderIconData()
{
   for (U32 i = 0; i < NumStates; i++)
      mImageNames[i] = StringTable->EmptyString();
   mLoaded = false;
}

void CommanderIconData::initPersistFields()
{
   for (U32 i = 0; i < NumStates; i++)
      addField(sStateNames[i], TypeString,
               Offset(mImageNames, CommanderIconData) + i * sizeof(StringTableEntry),
               "type image overlay modulate [looping|flipflop|oneshot] [ms]");

   Parent::initPersistFields();
}

/// The five images are sent in the order the class declares them, base first.
bool CommanderIconData::unpackData(BitStream *stream, U32 classId)
{
   if (!Parent::unpackData(stream, classId))
      return false;

   const Vector<String> &strings = T2Layout::lastStrings();
   for (U32 i = 0; i < NumStates; i++)
      mImageNames[i] = StringTable->insert(i < strings.size() ? strings[i].c_str() : "");

   mLoaded = false;
   return true;
}

bool CommanderIconData::preload(bool server, String &errorStr)
{
   if (!Parent::preload(server, errorStr))
      return false;

   mLoaded = false;
   return true;
}

static bool isTrue(const char *word)
{
   return !dStricmp(word, "true") || dAtoi(word) != 0;
}

static void loadFrame(Vector<GFXTexHandle> &frames, const String &name)
{
   GFXTexHandle tex(name, &GFXDefaultGUIProfile, "CommanderIconData frame");
   if (tex)
      frames.push_back(tex);
}

/// A static image is one texture under commander/icons; an animation is the
/// texture each line of its .dml names, in order.
void CommanderIconData::loadImages()
{
   mLoaded = true;

   for (U32 i = 0; i < NumStates; i++)
   {
      Image &image = mImages[i];
      image = Image();

      const char *spec = mImageNames[i];
      if (!spec || !spec[0] || StringUnit::getUnitCount(spec, " \t") < 4)
         continue;

      const String type = StringUnit::getUnit(spec, 0, " \t");
      const String name = StringUnit::getUnit(spec, 1, " \t");
      image.overlay  = isTrue(StringUnit::getUnit(spec, 2, " \t"));
      image.modulate = isTrue(StringUnit::getUnit(spec, 3, " \t"));

      if (type.equal("static", String::NoCase))
      {
         loadFrame(image.frames, String("commander/icons/") + name);
      }
      else if (type.equal("animation", String::NoCase))
      {
         const String play = StringUnit::getUnit(spec, 4, " \t");
         if (play.equal("flipflop", String::NoCase))
            image.play = FlipFlop;
         else if (play.equal("oneshot", String::NoCase))
            image.play = OneShot;

         const U32 ms = dAtoi(StringUnit::getUnit(spec, 5, " \t"));
         image.frameTime = ms ? ms : 100;

         FileStream file;
         const String dml = String("textures/commander/icons/") + name + ".dml";
         if (file.open(dml, Torque::FS::File::Read))
         {
            char line[256];
            while (file.getStatus() == Stream::Ok)
            {
               file.readLine((U8 *)line, sizeof(line));
               String frame = String(line).trim();
               if (frame.isNotEmpty())
                  loadFrame(image.frames, frame);
            }
         }
      }

      if (!image.valid())
         Con::warnf("CommanderIconImage::construct: failed to construct image '%s'", spec);
   }
}

const CommanderIconData::Image &CommanderIconData::getImage(State state)
{
   if (!mLoaded)
      loadImages();
   return mImages[state];
}

GFXTextureObject *CommanderIconData::Image::frame(U32 time) const
{
   const U32 count = frames.size();
   if (!count)
      return NULL;

   U32 at = time / frameTime;
   switch (play)
   {
      case OneShot:
         at = getMin(at, count - 1);
         break;
      case FlipFlop:
         if (count > 1)
         {
            at %= 2 * (count - 1);
            if (at >= count)
               at = 2 * (count - 1) - at;
         }
         else
            at = 0;
         break;
      default:
         at %= count;
         break;
   }

   return frames[at];
}
