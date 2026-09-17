// Loads the GL entry points through SDL, replacing gfx/gl/ggl/x11/glxBind.cpp.

#include "platform/platform.h"
#include "gfx/gl/tGL/tGL.h"
#include "console/console.h"

#include <SDL3/SDL.h>

namespace GL
{

void loadGLProcs()
{
   if( !gladLoadGLLoader( reinterpret_cast< GLADloadproc >( SDL_GL_GetProcAddress ) ) )
      Con::errorf( "GL: glad could not load the OpenGL entry points" );
}

} // namespace GL
