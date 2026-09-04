#include "platform/platform.h"
#include "platform/platformInput.h"

#include <SDL3/SDL.h>

U8 Input::smModifierKeys = 0;
InputEvent Input::smInputEvent;

static bool sInputActive = false;

void Input::activate()
{
   sInputActive = true;
}

void Input::deactivate()
{
   sInputActive = false;
}

bool Input::isActive()
{
   return sInputActive;
}

U16 Input::getAscii(U16 keyCode, KEY_STATE keyState)
{
   // Translation is driven by SDL_EVENT_TEXT_INPUT in the window manager rather
   // than by mapping keycodes here.
   TORQUE_UNUSED(keyCode);
   TORQUE_UNUSED(keyState);
   return 0;
}

U16 Input::getKeyCode(U16 asciiCode)
{
   TORQUE_UNUSED(asciiCode);
   return 0;
}
