// HH: Added for Tribes 2 compatibility.
#ifndef _GRAVITYT2_H_
#define _GRAVITYT2_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif

class NetConnection;

extern F32 gWorldGravity;

const F32 WorldGravityScale = 0.4905f;

void postWorldGravity(NetConnection *conn);

#endif
