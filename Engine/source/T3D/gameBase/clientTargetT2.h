// HH: Added for Tribes 2 compatibility.
#ifndef _CLIENTTARGETT2_H_
#define _CLIENTTARGETT2_H_

#ifndef _SIMOBJECT_H_
#include "console/simObject.h"
#endif
#ifndef _MPOINT3_H_
#include "math/mPoint3.h"
#endif

/// Something the commander map hands its scripts to give an order about: a
/// server target, or a place.  Made into a waypoint or a task it becomes a
/// target of this client's own, which the map and its list draw beside the
/// server's until the scripts delete it.
class ClientTarget : public SimObject
{
   typedef SimObject Parent;

public:
   ClientTarget();

   /// The server's target, or -1 for a place.  A waypoint or task answers
   /// its own id instead, which is never -1.
   S32 getTargetId() const;
   S32 getServerTarget() const { return mServerTarget; }

   /// One of the $CMD_ types, or the type of the server's target; 0 for none.
   U32 getType() const { return mType; }
   const Point3F &getPosition() const { return mPosition; }
   const char *getText() const { return mText; }
   bool isTyped() const { return mLocalId != -1; }

   void setServerTarget(S32 target) { mServerTarget = target; }
   void setType(U32 type) { mType = type; }
   void setPosition(const Point3F &pos) { mPosition = pos; }

   /// Become one of this client's own targets.
   bool makeTyped(U32 type, const char *text);

   /// Tell the server what the next order is about.
   void sendToServer();

   void onRemove();

   /// This client's own targets, in the order they were made.
   static const Vector<ClientTarget *> &typed() { return smTyped; }

   /// The tags behind $CMD_LOCATIONTYPEID and the rest, set once the net
   /// string table can hold them.
   enum Kind { Location, WayPoint, PotentialTask, AssignedTask, NumKinds };
   static U32 typeId(Kind kind);
   static void initTypes();

   DECLARE_CONOBJECT(ClientTarget);

private:
   S32 mServerTarget;
   S32 mLocalId;
   U32 mType;
   Point3F mPosition;
   StringTableEntry mText;

   static Vector<ClientTarget *> smTyped;
   static S32 smNextLocalId;
};

#endif
