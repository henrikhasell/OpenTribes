#include "platform/platform.h"
#include "T3D/gameBase/navigationGraphT2.h"

#include "console/console.h"
#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "core/stream/fileStream.h"
#include "core/strings/stringFunctions.h"
#include "math/mRandom.h"
#include "T3D/gameBase/t2Layout.h"

// HH: Added for Tribes 2 compatibility.

IMPLEMENT_CONOBJECT(NavigationGraph);

NavigationGraph *NavigationGraph::smActive = NULL;

namespace
{
   // The spawn file: a header of seven words, the node count last, then one
   // x y z float triple per node.
   const U32 SpawnHeaderBytes     = 28;
   const U32 SpawnHeaderCountWord = 6;
   const U32 SpawnNodeBytes       = 12;
}

NavigationGraph::NavigationGraph()
{
}

void NavigationGraph::initPersistFields()
{
   addField("GraphFile", TypeRealString, Offset(mGraphFile, NavigationGraph),
      "The nav file the mission names, such as TestingArea.nav. Its spawn "
      "graph is the same name with a .spn extension, under terrains/.");

   Parent::initPersistFields();
}

bool NavigationGraph::onAdd()
{
   if(!Parent::onAdd())
      return false;

   smActive = this;
   loadSpawnGraph();
   return true;
}

void NavigationGraph::onRemove()
{
   if(smActive == this)
      smActive = NULL;

   Parent::onRemove();
}

bool NavigationGraph::loadSpawnGraph()
{
   mSpawnNodes.clear();

   if(mGraphFile.isEmpty())
      return false;

   // The spawn graph shares the nav file's name, with a .spn extension.
   char base[256];
   dStrncpy(base, mGraphFile.c_str(), sizeof(base) - 1);
   base[sizeof(base) - 1] = 0;
   if(char *dot = dStrrchr(base, '.'))
      *dot = 0;

   const String path = T2Layout::resourcePath(avar("%s.spn", base), "terrains/");

   FileStream fs;
   if(!fs.open(path, Torque::FS::File::Read))
   {
      Con::warnf("NavigationGraph: no spawn graph at %s", path.c_str());
      return false;
   }

   S32 header[SpawnHeaderBytes / 4];
   if(!fs.read(SpawnHeaderBytes, header))
   {
      Con::warnf("NavigationGraph: %s has no header", path.c_str());
      return false;
   }

   const S32 count = header[SpawnHeaderCountWord];
   if(count <= 0 || fs.getStreamSize() < SpawnHeaderBytes + U32(count) * SpawnNodeBytes)
   {
      Con::warnf("NavigationGraph: %s holds no spawn nodes", path.c_str());
      return false;
   }

   mSpawnNodes.reserve(count);
   for(S32 i = 0; i < count; i++)
   {
      F32 xyz[3];
      if(!fs.read(SpawnNodeBytes, xyz))
      {
         Con::warnf("NavigationGraph: %s ends at node %d", path.c_str(), i);
         mSpawnNodes.clear();
         return false;
      }
      mSpawnNodes.push_back(Point3F(xyz[0], xyz[1], xyz[2]));
   }

   return true;
}

S32 NavigationGraph::randNode(const Point3F &center, F32 radius, bool indoor, bool outdoor) const
{
   // Every node is outdoor, so a request for indoor nodes alone has none.
   TORQUE_UNUSED(indoor);
   if(!outdoor)
      return -1;

   Vector<S32> inSphere(__FILE__, __LINE__);
   for(U32 i = 0; i < mSpawnNodes.size(); i++)
      if((mSpawnNodes[i] - center).len() <= radius)
         inSphere.push_back(S32(i));

   if(inSphere.empty())
      return -1;

   return inSphere[gRandGen.randI(0, S32(inSphere.size()) - 1)];
}

Point3F NavigationGraph::nodeLocation(S32 index) const
{
   if(index < 0 || U32(index) >= mSpawnNodes.size())
   {
      Con::warnf("NavigationGraph: no spawn node %d", index);
      return Point3F::Zero;
   }

   return mSpawnNodes[index];
}

DefineEngineFunction(navGraphExists, bool, (),,
   "@brief Whether the running mission has a navigation graph with spawn nodes.\n\n"
   "@ingroup GameObjects")
{
   const NavigationGraph *graph = NavigationGraph::getActive();
   return graph && graph->hasSpawnNodes();
}

DefineEngineMethod(NavigationGraph, randNode, S32, (Point3F center, F32 radius, bool indoor, bool outdoor),,
   "@brief Pick a random spawn node within a sphere.\n\n"
   "@param center The centre of the sphere.\n"
   "@param radius The radius of the sphere.\n"
   "@param indoor Whether indoor nodes may be picked.\n"
   "@param outdoor Whether outdoor nodes may be picked.\n"
   "@return The index of the node, or -1 when none lies in the sphere.\n"
   "@ingroup GameObjects")
{
   return object->randNode(center, radius, indoor, outdoor);
}

DefineEngineMethod(NavigationGraph, randNodeLoc, Point3F, (S32 index),,
   "@brief The location of a spawn node.\n\n"
   "@param index An index returned by randNode.\n"
   "@ingroup GameObjects")
{
   return object->nodeLocation(index);
}
