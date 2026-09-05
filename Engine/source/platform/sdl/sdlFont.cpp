#include "platform/platform.h"
#include "platform/platformFont.h"
#include "core/strings/stringFunctions.h"
#include "core/strings/unicode.h"
#include "console/console.h"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

class PlatformFontSDL : public PlatformFont
{
public:
   PlatformFontSDL();
   ~PlatformFontSDL();

   bool create(const char* name, U32 size, U32 charset);

   bool isValidChar(const UTF16 ch) const;
   bool isValidChar(const UTF8* str) const;

   U32 getFontHeight() const   { return mHeight; }
   U32 getFontBaseLine() const { return mBaseline; }

   PlatformFont::CharInfo& getCharInfo(const UTF16 ch) const;
   PlatformFont::CharInfo& getCharInfo(const UTF8* str) const;

private:
   TTF_Font* mFont;
   U32       mHeight;
   U32       mBaseline;
};

static bool ensureTTF()
{
   static bool initialized = false;
   if (!initialized)
   {
      if (!TTF_Init())
      {
         Con::errorf("TTF_Init failed: %s", SDL_GetError());
         return false;
      }
      initialized = true;
   }
   return true;
}

// The engine's font names come from Windows -- "Arial", "Arial Bold". Map the
// style out of the name and onto the font shipped beside the executable.
static String resolveFontFile(const char* name)
{
   String lower = String::ToLower(name ? name : "");

   const bool bold   = lower.find("bold") != String::NPos;
   const bool italic = lower.find("italic") != String::NPos ||
                       lower.find("oblique") != String::NPos;

   const char* file = "DejaVuSans.ttf";
   if (bold && italic) file = "DejaVuSans-BoldOblique.ttf";
   else if (bold)      file = "DejaVuSans-Bold.ttf";
   else if (italic)    file = "DejaVuSans-Oblique.ttf";

   char path[1024];
   dSprintf(path, sizeof(path), "%sfonts/%s", Platform::getExecutablePath(), file);
   return String(path);
}

PlatformFontSDL::PlatformFontSDL()
   : mFont(NULL), mHeight(0), mBaseline(0)
{
}

PlatformFontSDL::~PlatformFontSDL()
{
   if (mFont)
      TTF_CloseFont(mFont);
}

bool PlatformFontSDL::create(const char* name, U32 size, U32 charset)
{
   TORQUE_UNUSED(charset);

   if (!ensureTTF())
      return false;

   const String path = resolveFontFile(name);
   mFont = TTF_OpenFont(path.c_str(), F32(size));

   if (!mFont)
   {
      Con::errorf("PlatformFontSDL::create - could not open %s: %s", path.c_str(), SDL_GetError());
      return false;
   }

   mHeight   = U32(TTF_GetFontHeight(mFont));
   mBaseline = U32(TTF_GetFontAscent(mFont));
   return true;
}

bool PlatformFontSDL::isValidChar(const UTF16 ch) const
{
   if (!mFont || ch == 0)
      return false;

   // Control characters are handled by the caller, not rendered.
   if (ch < 0x20)
      return false;

   return TTF_FontHasGlyph(mFont, ch);
}

bool PlatformFontSDL::isValidChar(const UTF8* str) const
{
   return isValidChar(oneUTF32toUTF16(oneUTF8toUTF32(str)));
}

PlatformFont::CharInfo& PlatformFontSDL::getCharInfo(const UTF16 ch) const
{
   static PlatformFont::CharInfo info;
   dMemset(&info, 0, sizeof(info));
   info.bitmapIndex = -1;

   if (!mFont)
      return info;

   int minx = 0, maxx = 0, miny = 0, maxy = 0, advance = 0;
   if (!TTF_GetGlyphMetrics(mFont, ch, &minx, &maxx, &miny, &maxy, &advance))
      return info;

   // TTF_RenderGlyph_Blended returns the full glyph cell: the surface spans the
   // whole advance width and the entire font height, with the ink already
   // positioned inside it. So the bitmap is drawn straight at the pen position,
   // and its top edge sits on the ascent line. Taking xOrigin/yOrigin from the
   // per-glyph minx/maxy instead offsets every character differently, which
   // staggers the text on both axes.
   info.xIncrement = advance;
   info.xOrigin    = 0;
   info.yOrigin    = S32(mBaseline);

   SDL_Color white = { 255, 255, 255, 255 };
   SDL_Surface* glyph = TTF_RenderGlyph_Blended(mFont, ch, white);
   if (!glyph)
      return info;

   SDL_Surface* rgba = SDL_ConvertSurface(glyph, SDL_PIXELFORMAT_RGBA32);
   SDL_DestroySurface(glyph);
   if (!rgba)
      return info;

   info.bitmapIndex = 0;
   info.width       = U32(rgba->w);
   info.height      = U32(rgba->h);
   info.bitmapData  = new U8[info.width * info.height];

   // The engine wants an 8-bit coverage map; keep the alpha channel.
   for (U32 y = 0; y < info.height; ++y)
   {
      const U8* src = static_cast<const U8*>(rgba->pixels) + y * rgba->pitch;
      for (U32 x = 0; x < info.width; ++x)
         info.bitmapData[y * info.width + x] = src[x * 4 + 3];
   }

   SDL_DestroySurface(rgba);
   return info;
}

PlatformFont::CharInfo& PlatformFontSDL::getCharInfo(const UTF8* str) const
{
   return getCharInfo(oneUTF32toUTF16(oneUTF8toUTF32(str)));
}

PlatformFont* createPlatformFont(const char* name, U32 size, U32 charset)
{
   PlatformFontSDL* font = new PlatformFontSDL();

   if (!font->create(name, size, charset))
   {
      delete font;
      return NULL;
   }

   return font;
}
