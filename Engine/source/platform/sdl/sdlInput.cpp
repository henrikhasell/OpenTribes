#include "platform/platform.h"
#include "platform/platformInput.h"
#include "platform/sdl/sdlWindowMgr.h"

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

// The scancode that reports engine key keyCode, or SDL_SCANCODE_UNKNOWN.
static SDL_Scancode sdlScancodeForKey( U16 keyCode )
{
   for ( S32 i = 0; i < SDL_SCANCODE_COUNT; i++ )
   {
      if ( sdlToTorqueKey( SDL_Scancode( i ) ) == keyCode )
         return SDL_Scancode( i );
   }

   return SDL_SCANCODE_UNKNOWN;
}

// Names follow the active layout: a key is named for the character it types
// there, so the key left of Z is "backslash" on a UK layout and "lessthan" on
// a German one. Only printable ASCII is returned; anything else is named by
// the engine's own table.
U16 Input::getAscii( U16 keyCode, KEY_STATE keyState )
{
   if ( !SDL_WasInit( SDL_INIT_VIDEO ) )
      return 0;

   SDL_Scancode code = sdlScancodeForKey( keyCode );
   if ( code == SDL_SCANCODE_UNKNOWN )
      return 0;

   SDL_Keymod mods = ( keyState == STATE_UPPER ) ? SDL_KMOD_SHIFT : SDL_KMOD_NONE;
   SDL_Keycode key = SDL_GetKeyFromScancode( code, mods, false );
   if ( key < 0x21 || key > 0x7e )
      return 0;

   return U16( key );
}

U16 Input::getKeyCode( U16 asciiCode )
{
   static_assert( KEY_Z - KEY_A == 25, "KEY_A..KEY_Z must stay contiguous" );
   static_assert( KEY_9 - KEY_0 ==  9, "KEY_0..KEY_9 must stay contiguous" );

   // A character is the key that types it unshifted on the active layout. The
   // table below is the US layout, which is wrong for, say, '\' on a UK one.
   if ( SDL_WasInit( SDL_INIT_VIDEO ) )
   {
      U16 lower = asciiCode;
      if ( asciiCode >= 'A' && asciiCode <= 'Z' )
         lower = U16( asciiCode - 'A' + 'a' );

      SDL_Keymod mods = SDL_KMOD_NONE;
      SDL_Scancode code = SDL_GetScancodeFromKey( SDL_Keycode( lower ), &mods );
      if ( code != SDL_SCANCODE_UNKNOWN && mods == SDL_KMOD_NONE )
      {
         U16 keyCode = sdlToTorqueKey( code );
         if ( keyCode != 0 )
            return keyCode;
      }
   }

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
