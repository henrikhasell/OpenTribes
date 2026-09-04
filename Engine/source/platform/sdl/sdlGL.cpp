// Binds the GGL entry points through SDL, replacing gfx/gl/ggl/x11/glxBind.cpp.
// The pointers are declared extern by ggl.h; this is their definition.

#include "platform/platform.h"
#include "gfx/gl/ggl/ggl.h"
#include "console/console.h"
#include "core/strings/stringFunctions.h"

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

static bool hasExtension(const char* extensions, const char* name)
{
   if (!extensions || !name)
      return false;

   const dsize_t nameLen = dStrlen(name);
   for (const char* walk = extensions; (walk = dStrstr(walk, name)) != NULL; walk += nameLen)
   {
      const char after = walk[nameLen];
      if ((walk == extensions || walk[-1] == ' ') && (after == ' ' || after == '\0'))
         return true;
   }

   return false;
}

// The context argument is ignored: SDL keeps one GL context current and the
// pointers below are shared. It stays in the signature because gfxGLDevice.cpp
// declares loadGLExtensions(void*) for the platform backends.
void gglPerformExtensionBinds(void* context)
{
   TORQUE_UNUSED(context);

   static GLExtensionPtrs  sPtrs;
   static GLExtensionFlags sFlags;

   if (sPtrs.bound)
      return;

   _GGLptr  = &sPtrs;
   _GGLflag = &sFlags;

   const char* extensions = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));

   #define GL_GROUP_BEGIN(name) sFlags.has_##name = hasExtension(extensions, #name);
   #define GL_FUNCTION(fn_name, fn_type, fn_args) \
      bindEntry(*reinterpret_cast<void**>(&sPtrs._##fn_name), #fn_name);
   #define GL_GROUP_END()
   #include "gfx/gl/ggl/generated/glefn.h"
   #undef GL_GROUP_BEGIN
   #undef GL_FUNCTION
   #undef GL_GROUP_END

   sPtrs.bound  = true;
   sFlags.bound = true;
}

} // namespace GL
