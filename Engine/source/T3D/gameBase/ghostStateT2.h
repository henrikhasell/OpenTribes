// HH: Added for Tribes 2 compatibility.
#ifndef _GHOSTSTATET2_H_
#define _GHOSTSTATET2_H_

class NetObject;
class BitStream;

/// What the port keeps out of a Tribes 2 ghost body.
///
/// Every body is walked from its recovered layout (see t2Layout.h), which is
/// what makes the rest of the packet readable.  A class named here is also
/// handed the values the walk took, so the object arrives with its fields set
/// and can be drawn instead of standing empty in the scene.
///
/// Fields are picked out by the node that read them, so the graph stays the
/// only place a width is written down.  A node whose meaning is not known is
/// left alone rather than guessed at.
namespace GhostStateT2
{
   /// Read `obj`'s body.  False only when the walk failed, which leaves the
   /// caller to unpack the body itself.
   bool readGhost(NetObject *obj, BitStream *stream, bool isInitial);
}

#endif
