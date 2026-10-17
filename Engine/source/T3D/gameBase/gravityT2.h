// HH: Added for Tribes 2 compatibility.
#ifndef _GRAVITYT2_H_
#define _GRAVITYT2_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif

class NetConnection;

/// The world's gravity, which is retail's one global at 0x007a1a20.  It
/// starts at -20, setGravity and GravityEvent write it, and every player,
/// item, vehicle, projectile, piece of debris and particle reads it.
extern F32 gWorldGravity;

/// What a projectile, a piece of debris and a particle take of the world's
/// gravity before their datablock's own share of it (0x00636670, 0x006860d2,
/// 0x0062592a).  It is 9.81 / 20, so at the default they fall at the 9.81
/// Torque hard-codes.
const F32 WorldGravityScale = 0.4905f;

/// Tell one client what the world's gravity is, which retail does for every
/// connection as it is set up (0x005faf30).
void postWorldGravity(NetConnection *conn);

#endif
