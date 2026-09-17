#ifndef _SCREENSHOTGL_H_
#define _SCREENSHOTGL_H_

#include "gfx/screenshot.h"

//**************************************************************************
// OpenGL implementation of screenshot
//**************************************************************************
class ScreenShotGL : public ScreenShot
{
protected:

   GBitmap* _captureBackBuffer();

};


#endif  // _SCREENSHOTGL_H_
