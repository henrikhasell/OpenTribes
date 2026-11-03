// HH: Added for Tribes 2 compatibility.
#ifndef _GAMMARAMPT2_H_
#define _GAMMARAMPT2_H_

/// The display gamma ramp retail's videoSetGammaCorrection (0x00402450)
/// loads through SetDeviceGammaRamp. SDL3 has no gamma ramps, and a ramp
/// would not reach a screenshot anyway, so the frame is put through the
/// same curve by a last full-screen pass instead.
namespace GammaRampT2
{
   /// Run the finished frame through the ramp, if one has been set.
   void apply();
}

#endif
