// Binds the GGL entry points through SDL, replacing gfx/gl/ggl/x11/glxBind.cpp.
// The pointers are declared extern by ggl.h; this is their definition.

#include "platform/platform.h"
#include "gfx/gl/ggl/ggl.h"
#include "console/console.h"

#include <SDL3/SDL.h>

#define GL_GROUP_BEGIN(name)
#define GL_FUNCTION(name, type, args) type (XGL_DLL *name) args = NULL;
#define GL_GROUP_END()
#include "gfx/gl/ggl/generated/glcfn.h"
#undef GL_GROUP_BEGIN
#undef GL_FUNCTION
#undef GL_GROUP_END

namespace GL
{

static bool bindEntry(void*& fnAddress, const char* name)
{
   fnAddress = reinterpret_cast<void*>(SDL_GL_GetProcAddress(name));
   return fnAddress != NULL;
}

void gglPerformBinds()
{
   #define GL_GROUP_BEGIN(name)
   #define GL_FUNCTION(fn_name, fn_type, fn_args) \
      if (!bindEntry(*reinterpret_cast<void**>(&fn_name), #fn_name)) \
         Con::warnf("GL: could not bind %s", #fn_name);
   #define GL_GROUP_END()
   #include "gfx/gl/ggl/generated/glcfn.h"
   #undef GL_GROUP_BEGIN
   #undef GL_FUNCTION
   #undef GL_GROUP_END
}

void gglPerformExtensionBinds(void* context)
{
   GLExtensionPtrs* ptrs = static_cast<GLExtensionPtrs*>(context);
   if (!ptrs || ptrs->bound)
      return;

   #define GL_GROUP_BEGIN(name)
   #define GL_FUNCTION(fn_name, fn_type, fn_args) \
      bindEntry(*reinterpret_cast<void**>(&ptrs->_##fn_name), #fn_name);
   #define GL_GROUP_END()
   #include "gfx/gl/ggl/generated/glefn.h"
   #undef GL_GROUP_BEGIN
   #undef GL_FUNCTION
   #undef GL_GROUP_END

   ptrs->bound = true;
}

} // namespace GL
