// HH: Added for Tribes 2 compatibility.
#ifndef _T2SHAPEPATH_H_
#define _T2SHAPEPATH_H_

// shapePathT2 moved to t2ArtPath.h, which is the one place that knows which
// directory each kind of Tribes 2 art name belongs to.  The shape name turned
// out to be the first of three instances of one fault rather than a case of
// its own; see the comment there.  This header stays for the callers that ask
// for it by name.
#ifndef _T2ARTPATH_H_
#include "T3D/gameBase/t2ArtPath.h"
#endif

#endif
