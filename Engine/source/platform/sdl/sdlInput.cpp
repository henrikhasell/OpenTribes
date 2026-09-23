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
   static_assert( KEY_Z - KEY_A == 25, "KEY_A..KEY_Z must stay contiguous" );
   static_assert( KEY_9 - KEY_0 ==  9, "KEY_0..KEY_9 must stay contiguous" );

   if ( asciiCode >= 'a' && asciiCode <= 'z' )
      return KEY_A + ( asciiCode - 'a' );

   if ( asciiCode >= 'A' && asciiCode <= 'Z' )
      return KEY_A + ( asciiCode - 'A' );

   if ( asciiCode >= '0' && asciiCode <= '9' )
      return KEY_0 + ( asciiCode - '0' );

   switch ( asciiCode )
   {
      case ' ':   return KEY_SPACE;
      case '`':   return KEY_TILDE;
      case '-':   return KEY_MINUS;
      case '=':   return KEY_EQUALS;
      case '[':   return KEY_LBRACKET;
      case ']':   return KEY_RBRACKET;
      case '\\':  return KEY_BACKSLASH;
      case ';':   return KEY_SEMICOLON;
      case '\'':  return KEY_APOSTROPHE;
      case ',':   return KEY_COMMA;
      case '.':   return KEY_PERIOD;
      case '/':   return KEY_SLASH;
      case '\t':  return KEY_TAB;
      case '\r':  return KEY_RETURN;
   }

   return 0;
}
