//-----------------------------------------------------------------------------
// SDL3 platform layer -- entry point.
//
// Phase 2 of the SDL3 migration. Previously this was split three ways: the body
// lived in Engine/source/main/main.cpp as TorqueMain(), and each platform layer
// carried its own main() that called it -- platformWin32/winWindow.cpp,
// platformX86UNIX/x86UNIXMain.cpp and the macOS equivalent. main/main.cpp also
// doubled as a shared-library shim that dlopened the game module and called
// torque_winmain/torque_macmain, which only ever worked on Windows and macOS.
//
// With those layers deleted there is one entry point for every target, and the
// TORQUE_SHARED split is gone.
//
// Phase 3 fills in the rest of platform/sdl: window lifecycle, the event pump
// feeding platform/event.h, input, timers, threads, TLS, volumes and dlopen.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "app/mainLoop.h"

// Ensures the console function definitions are not stripped by the linker.
extern bool LinkConsoleFunctions;

int main(int argc, const char** argv)
{
   LinkConsoleFunctions = true;

   StandardMainLoop::init();

   if (!StandardMainLoop::handleCommandLine(argc, argv))
   {
      Platform::AlertOK("Error", "Failed to initialize game, shutting down.");
      return 1;
   }

   while (StandardMainLoop::doMainLoop())
   {
      // doMainLoop() returns false once the engine has been asked to quit.
   }

   StandardMainLoop::shutdown();

   if (StandardMainLoop::requiresRestart())
      Platform::restartInstance();

   return 0;
}
