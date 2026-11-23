#ifndef _NAVIGATIONGRAPHT2_H_
#define _NAVIGATIONGRAPHT2_H_

#ifndef _SIMOBJECT_H_
#include "console/simObject.h"
#endif
#ifndef _MPOINT3_H_
#include "math/mPoint3.h"
#endif
#ifndef _TVECTOR_H_
#include "core/util/tVector.h"
#endif

/// HH: Added for Tribes 2 compatibility. The NavigationGraph a mission
/// declares as NavGraph.
///
/// This engine has no AI, so the graph is read for the one part the scripts
/// use: the spawn graph. DefaultGame::pickTeamSpawn asks navGraphExists()
/// whether there is one, then picks a random node inside a spawn sphere
/// with randNode and reads its place with randNodeLoc. The spawn graph is
/// the nav file's name with a .spn extension, read from terrains/.
///
/// The spawn file has a 28 byte header whose last word, at byte 24, counts
/// the nodes. The nodes follow as x y z float triples, 12 bytes each. Nothing
/// after them is read: the mission holds the spawn spheres themselves. The
/// file keeps no indoor flag, so every node is outdoor.
class NavigationGraph : public SimObject
{
   typedef SimObject Parent;

public:
   NavigationGraph();

   /// @name SimObject Inheritance
   /// @{
   bool onAdd();
   void onRemove();

   static void initPersistFields();
   DECLARE_CONOBJECT(NavigationGraph);
   /// @}

   /// The graph of the mission that is running, or NULL when there is none.
   static NavigationGraph *getActive() { return smActive; }

   /// Whether the spawn file loaded any nodes.
   bool hasSpawnNodes() const { return !mSpawnNodes.empty(); }

   /// A random node index within radius of center, or -1 when there is none.
   /// The distance is taken in three dimensions, as the spawn sphere is one.
   S32 randNode(const Point3F &center, F32 radius, bool indoor, bool outdoor) const;

   /// The location of a node. A bad index gives the origin.
   Point3F nodeLocation(S32 index) const;

private:
   bool loadSpawnGraph();

   String mGraphFile;
   Vector<Point3F> mSpawnNodes;

   static NavigationGraph *smActive;
};

#endif // _NAVIGATIONGRAPHT2_H_
