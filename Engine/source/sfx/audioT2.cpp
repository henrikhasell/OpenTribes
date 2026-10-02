// HH: Added for Tribes 2 compatibility.

#include "platform/platform.h"
#include "sfx/audioT2.h"

#include "console/consoleTypes.h"
#include "console/engineAPI.h"


IMPLEMENT_CO_DATABLOCK_V1( AudioDescription );
IMPLEMENT_CO_DATABLOCK_V1( AudioProfile );

ConsoleDocClass( AudioDescription,
   "@brief Tribes 2's name for an SFXDescription.\n\n"
   "Tribes 2 declares its sound descriptions as AudioDescription, with the "
   "same volume, isLooping and is3D that SFXDescription has.\n\n"
   "@ingroup SFX" );

ConsoleDocClass( AudioProfile,
   "@brief Tribes 2's name for an SFXProfile.\n\n"
   "Tribes 2 declares its sounds as AudioProfile, with the same filename, "
   "description and preload that SFXProfile has.  Without this class none of "
   "them were built, so a shell button had no sound to play.\n\n"
   "@ingroup SFX" );
