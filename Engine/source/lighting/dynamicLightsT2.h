// HH: Added for Tribes 2 compatibility.
#ifndef _DYNAMICLIGHTST2_H_
#define _DYNAMICLIGHTST2_H_

#ifndef _MSPHERE_H_
#include "math/mSphere.h"
#endif

class LightInfo;
class SceneRenderState;

/// The dynamic lights -- a projectile's, an explosion's -- retail lets fall
/// on an interior (0x005228c0) or a terrain (0x005a8409): those whose
/// radius covers more than the clip pixels on screen at their distance from
/// the camera, dimmed linearly to nothing between the fade and the clip
/// pixels.
namespace DynamicLightsT2
{
   /// Write up to maxLights of them that reach bounds into outLights,
   /// dimmed where they are fading, and return how many there are.
   U32 gather( const SceneRenderState *state,
               const SphereF &bounds,
               F32 clipPix,
               F32 fadePix,
               LightInfo **outLights,
               U32 maxLights );
}

#endif
