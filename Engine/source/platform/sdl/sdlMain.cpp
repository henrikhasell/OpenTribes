// Single entry point, replacing TorqueMain() in main/main.cpp and the per-platform
// main()s that called it.

#include "platform/platform.h"
#include "app/mainLoop.h"

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
      ;

   StandardMainLoop::shutdown();

   if (StandardMainLoop::requiresRestart())
      Platform::restartInstance();

   return 0;
}
