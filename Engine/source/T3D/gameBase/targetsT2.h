// HH: Added for Tribes 2 compatibility.
#ifndef _TARGETST2_H_
#define _TARGETST2_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _COLOR_H_
#include "core/color.h"
#endif

class BitStream;
class ShapeBase;

namespace T2Targets
{
   /// The sensor group of a target, or 0 for none -- which is what
   /// GameBase::getSensorGroup (0x005e3490) answers for an object with no
   /// target.
   U32 sensorGroup(S32 target);

   /// $pref::usePrefSkins, which lets a player be seen in the skin they chose
   /// rather than in their team's.  Each client answers it for itself.
   extern bool smUsePrefSkins;

   /// $pref::use512PlayerSkins, which takes the larger artwork of a player's
   /// own skin where the game ships one.  Retail asks it only about a
   /// preferred skin, never about a team's.
   extern bool smUse512PlayerSkins;

   /// Put the skin a target carries on the shape that wears the target, which
   /// is what makes a flag its team's and a player their own or their team's.
   /// Does nothing for a shape with no target.
   void applySkin(ShapeBase *shape);

   /// Which of the hud images of its datablock a target is drawn with, one
   /// bit each; bit 8 lists it on the commander map.
   U32 renderMask(S32 target);

   /// What the HUD calls a target: its name and type, "Storm Flag", leaving
   /// out either if it is empty or starts with an underscore. False for a
   /// target that was never allocated.
   bool getTargetName(S32 target, char *buffer, U32 size);

   /// Whether the client's sensor group senses a target. The server sends
   /// only the changes, a word of 32 targets at a time.
   bool isSensed(S32 target);
   void toggleSensed(U32 word, U32 bits);

   /// The sensor group the server put this client in.
   U32 clientSensorGroup();

   /// The colour the server gave a target group as a viewer group sees it.
   ColorI groupColor(U32 viewer, U32 target);

   /// Every target and colour as a recording's start block holds them.
   void readDemoState(BitStream *stream);

   /// Forget every target, as when the connection that sent them goes.
   void reset();
}

#endif
