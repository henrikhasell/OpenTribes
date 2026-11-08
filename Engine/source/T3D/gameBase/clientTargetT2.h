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
/// target of this client's own, which the map, its list and the hud draw
/// beside the server's until the scripts delete it.
class ClientTarget : public SimObject
{
   typedef SimObject Parent;

public:
   /// The tags behind $CMD_LOCATIONTYPEID and the rest, set once the net
   /// string table can hold them.
   enum Kind { Location, WayPoint, PotentialTask, AssignedTask, NumKinds };

   /// How long a task stays offered before it dies ([0x7b8d2c]).  The
   /// scripts keep the object alive for their list; what goes is the marker.
   enum { PotentialTaskLife = 2000 };

   ClientTarget();

   /// The server's target, or -1 for a place.  A waypoint or task answers
   /// its own id instead, which is never -1.
   S32 getTargetId() const;
   S32 getServerTarget() const { return mServerTarget; }

   /// One of the $CMD_ types, or the type of the server's target; 0 for none.
   U32 getType() const { return mType; }
   /// Which of this client's own kinds it was made, Location until it is.
   Kind getKind() const { return mKind; }
   const Point3F &getPosition() const { return mPosition; }
   const char *getText() const { return mText; }
   bool isTyped() const { return mLocalId != -1; }

   void setServerTarget(S32 target) { mServerTarget = target; }
   void setType(U32 type) { mType = type; }
   void setPosition(const Point3F &pos) { mPosition = pos; }
   void setText(const char *text) { mText = StringTable->insert(text ? text : ""); }

   /// Become one of this client's own targets.
   bool makeTyped(Kind kind, const char *text);

   /// Tell the server what the next order is about.
   void sendToServer();

   /// Take a task off the map and tell the scripts, which keep the ones
   /// their list still holds (0x00671660).
   void die();

   void onRemove();

   /// Expire the tasks whose time is up and follow whatever the rest are
   /// about, once a client process (0x00671450).
   static void update();

   /// This client's own targets, in the order they were made.
   static const Vector<ClientTarget *> &typed() { return smTyped; }

   static U32 typeId(Kind kind);
   static void initTypes();

   DECLARE_CONOBJECT(ClientTarget);

private:
   S32 mServerTarget;
   S32 mLocalId;
   U32 mType;
   Kind mKind;
   /// When a task goes, or 0 for a target that stays.
   U32 mDeathTime;
   Point3F mPosition;
   StringTableEntry mText;

   void forget();

   static Vector<ClientTarget *> smTyped;
   static S32 smNextLocalId;
};

#endif
