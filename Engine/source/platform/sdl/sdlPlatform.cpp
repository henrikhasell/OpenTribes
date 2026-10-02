#include "platform/platform.h"
#include "platform/platformVolume.h"
#include "core/util/str.h"
#include "core/strings/stringFunctions.h"
#include "core/stringTable.h"
#include "console/console.h"

#include <SDL3/SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

Platform::SystemInfo_struct Platform::SystemInfo;

static U32 sVirtualMilliseconds = 0;
static bool sQuitRequested = false;

//-----------------------------------------------------------------------------
// Lifecycle

void Platform::init()
{
   // HH: Added to prevent Wayland scaling the display.
   SDL_SetHint(SDL_HINT_VIDEO_WAYLAND_SCALE_TO_DISPLAY, "1");

   if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
      Con::errorf("SDL_Init failed: %s", SDL_GetError());

   // GFXGLDevice's constructor binds the core GL entry points, which happens
   // well before any window or context exists. SDL_GL_GetProcAddress can only
   // resolve them once SDL has loaded the driver, so do that up front and let
   // SDL pick the library name for the platform.
   if (!SDL_GL_LoadLibrary(NULL))
      Con::errorf("SDL_GL_LoadLibrary failed: %s", SDL_GetError());

   Processor::init();
}

void Platform::shutdown()
{
   SDL_Quit();
}

void Platform::restartInstance()
{
   const char* executable = Platform::getExecutableName();
   if (!executable)
      return;

   char* argv[] = { const_cast<char*>(executable), NULL };
   execv(executable, argv);
}

void Platform::postQuitMessage(const U32 in_quitVal)
{
   TORQUE_UNUSED(in_quitVal);
   sQuitRequested = true;

   SDL_Event quit;
   quit.type = SDL_EVENT_QUIT;
   SDL_PushEvent(&quit);
}

void Platform::forceShutdown(S32 returnValue)
{
   exit(returnValue);
}

void Platform::debugBreak()
{
   SDL_TriggerBreakpoint();
}

void Platform::outputDebugString(const char* string, ...)
{
   char buffer[2048];
   va_list args;
   va_start(args, string);
   dVsprintf(buffer, sizeof(buffer), string, args);
   va_end(args);

   fprintf(stderr, "%s\n", buffer);
}

void Platform::sleep(U32 ms)
{
   SDL_Delay(ms);
}

bool Platform::excludeOtherInstances(const char* string)
{
   // Single-instance enforcement used a named mutex on Windows; nothing
   // equivalent is wired up yet.
   TORQUE_UNUSED(string);
   return true;
}

//-----------------------------------------------------------------------------
// Time

U32 Platform::getRealMilliseconds()
{
   return U32(SDL_GetTicks());
}

U32 Platform::getVirtualMilliseconds()
{
   return sVirtualMilliseconds;
}

void Platform::advanceTime(U32 delta)
{
   sVirtualMilliseconds += delta;
}

U32 Platform::getTime()
{
   return U32(::time(NULL));
}

void Platform::getLocalTime(LocalTime& lt)
{
   time_t now = ::time(NULL);
   struct tm parts;
   localtime_r(&now, &parts);

   lt.sec      = parts.tm_sec;
   lt.min      = parts.tm_min;
   lt.hour     = parts.tm_hour;
   lt.month    = parts.tm_mon;
   lt.monthday = parts.tm_mday;
   lt.weekday  = parts.tm_wday;
   lt.year     = parts.tm_year;
   lt.yearday  = parts.tm_yday;
   lt.isdst    = parts.tm_isdst > 0;
}

String Platform::localTimeToString(const LocalTime& lt)
{
   struct tm parts;
   dMemset(&parts, 0, sizeof(parts));
   parts.tm_sec   = lt.sec;
   parts.tm_min   = lt.min;
   parts.tm_hour  = lt.hour;
   parts.tm_mon   = lt.month;
   parts.tm_mday  = lt.monthday;
   parts.tm_wday  = lt.weekday;
   parts.tm_year  = lt.year;
   parts.tm_yday  = lt.yearday;
   parts.tm_isdst = lt.isdst;

   char buffer[128];
   strftime(buffer, sizeof(buffer), "%c", &parts);
   return String(buffer);
}

void Platform::fileToLocalTime(const FileTime& ft, LocalTime* lt)
{
   if (!lt)
      return;

   time_t stamp = time_t(ft);
   struct tm parts;
   localtime_r(&stamp, &parts);

   lt->sec      = parts.tm_sec;
   lt->min      = parts.tm_min;
   lt->hour     = parts.tm_hour;
   lt->month    = parts.tm_mon;
   lt->monthday = parts.tm_mday;
   lt->weekday  = parts.tm_wday;
   lt->year     = parts.tm_year;
   lt->yearday  = parts.tm_yday;
   lt->isdst    = parts.tm_isdst > 0;
}

S32 Platform::compareFileTimes(const FileTime& a, const FileTime& b)
{
   if (a > b) return 1;
   if (a < b) return -1;
   return 0;
}

//-----------------------------------------------------------------------------
// Math control -- x87 control-word manipulation, meaningless on SSE.

U32 Platform::getMathControlState()
{
   return 0;
}

void Platform::setMathControlState(U32 state)
{
   TORQUE_UNUSED(state);
}

void Platform::setMathControlStateKnown()
{
}

float Platform::getRandom()
{
   return F32(rand()) / F32(RAND_MAX);
}

//-----------------------------------------------------------------------------
// Dialogs and shell integration

void Platform::AlertOK(const char* windowTitle, const char* message)
{
   SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, windowTitle, message, NULL);
}

bool Platform::AlertOKCancel(const char* windowTitle, const char* message)
{
   const SDL_MessageBoxButtonData buttons[] = {
      { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "OK" },
      { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "Cancel" },
   };
   const SDL_MessageBoxData data = {
      SDL_MESSAGEBOX_INFORMATION, NULL, windowTitle, message, 2, buttons, NULL
   };

   int result = 0;
   if (!SDL_ShowMessageBox(&data, &result))
      return false;
   return result == 1;
}

bool Platform::AlertRetry(const char* windowTitle, const char* message)
{
   const SDL_MessageBoxButtonData buttons[] = {
      { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Retry" },
      { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "Cancel" },
   };
   const SDL_MessageBoxData data = {
      SDL_MESSAGEBOX_WARNING, NULL, windowTitle, message, 2, buttons, NULL
   };

   int result = 0;
   if (!SDL_ShowMessageBox(&data, &result))
      return false;
   return result == 1;
}

S32 Platform::messageBox(const UTF8* title, const UTF8* message, MBButtons buttons, MBIcons icon)
{
   TORQUE_UNUSED(icon);

   switch (buttons)
   {
      case MBOkCancel:
         return AlertOKCancel(title, message) ? MROk : MRCancel;
      case MBRetryCancel:
         return AlertRetry(title, message) ? MRRetry : MRCancel;
      default:
         AlertOK(title, message);
         return MROk;
   }
}

bool Platform::openWebBrowser(const char* webAddress)
{
   return SDL_OpenURL(webAddress);
}

const char* Platform::getClipboard()
{
   char* text = SDL_GetClipboardText();
   if (!text)
      return "";

   const char* result = StringTable->insert(text);
   SDL_free(text);
   return result;
}

bool Platform::setClipboard(const char* text)
{
   return SDL_SetClipboardText(text);
}

bool Platform::displaySplashWindow()
{
   return false;
}

void Processor::init()
{
   Platform::SystemInfo.processor.type      = CPU_X86Compatible;
   Platform::SystemInfo.processor.name      = StringTable->insert(SDL_GetPlatform());
   Platform::SystemInfo.processor.mhz       = 0;
   Platform::SystemInfo.processor.numLogicalProcessors  = SDL_GetNumLogicalCPUCores();
   Platform::SystemInfo.processor.numPhysicalProcessors = SDL_GetNumLogicalCPUCores();
   Platform::SystemInfo.processor.numAvailableCores     = SDL_GetNumLogicalCPUCores();
   Platform::SystemInfo.processor.properties = CPU_PROP_LE | CPU_PROP_FPU | CPU_PROP_64bit;

   if (SDL_HasMMX())   Platform::SystemInfo.processor.properties |= CPU_PROP_MMX;
   if (SDL_HasSSE())   Platform::SystemInfo.processor.properties |= CPU_PROP_SSE;
   if (SDL_HasSSE2())  Platform::SystemInfo.processor.properties |= CPU_PROP_SSE2;
   if (SDL_HasSSE3())  Platform::SystemInfo.processor.properties |= CPU_PROP_SSE3;
   if (SDL_HasSSE41()) Platform::SystemInfo.processor.properties |= CPU_PROP_SSE4_1;
   if (SDL_HasSSE42()) Platform::SystemInfo.processor.properties |= CPU_PROP_SSE4_2;

   if (Platform::SystemInfo.processor.numLogicalProcessors > 1)
      Platform::SystemInfo.processor.properties |= CPU_PROP_MP;
}
