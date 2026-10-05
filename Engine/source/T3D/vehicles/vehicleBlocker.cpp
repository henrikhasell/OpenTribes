//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "T3D/vehicles/vehicleBlocker.h"
#include "T3D/gameBase/t2MemberDump.h"   // HH: Added for Tribes 2 compatibility.

#include "core/stream/bitStream.h"
#include "scene/sceneRenderState.h"
#include "scene/sceneManager.h"
#include "math/mathIO.h"
#include "console/consoleTypes.h"
#include "T3D/gameBase/t2Fields.h"

IMPLEMENT_CO_NETOBJECT_V1(VehicleBlocker);

ConsoleDocClass( VehicleBlocker,
   "@brief Legacy class from Tribes, originally used for blocking Vehicle objects.\n\n"

   "@note This is no longer useful and should be deprecated soon.\n\n"

   "@internal\n"
);

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
VehicleBlocker::VehicleBlocker()
{
   mNetFlags.set(Ghostable | ScopeAlways);

   mTypeMask = VehicleBlockerObjectType;

   mConvexList = new Convex;
}

VehicleBlocker::~VehicleBlocker()
{
   delete mConvexList;
   mConvexList = NULL;
}

//--------------------------------------------------------------------------
void VehicleBlocker::initPersistFields()
{
   addField("dimensions", TypePoint3F, Offset(mDimensions, VehicleBlocker));
   Parent::initPersistFields();
}

//--------------------------------------------------------------------------
bool VehicleBlocker::onAdd()
{
   if(!Parent::onAdd())
      return false;

   mObjBox.minExtents.set(-mDimensions.x, -mDimensions.y, 0);
   mObjBox.maxExtents.set( mDimensions.x,  mDimensions.y, mDimensions.z);
   resetWorldBox();
   setRenderTransform(mObjToWorld);

   addToScene();

   return true;
}


void VehicleBlocker::onRemove()
{
   mConvexList->nukeList();
   removeFromScene();

   Parent::onRemove();
}


U32 VehicleBlocker::packUpdate(NetConnection *con, U32 mask, BitStream *stream)
{
   U32 retMask = Parent::packUpdate(con, mask, stream);

   mathWrite(*stream, getTransform());
   mathWrite(*stream, getScale());
   mathWrite(*stream, mDimensions);

   return retMask;
}


void VehicleBlocker::unpackUpdate(NetConnection *con, BitStream *stream)
{
   Parent::unpackUpdate(con, stream);

   MatrixF mat;
   Point3F scale;
   Box3F objBox;
   mathRead(*stream, &mat);
   mathRead(*stream, &scale);
   mathRead(*stream, &mDimensions);
   mObjBox.minExtents.set(-mDimensions.x, -mDimensions.y, 0);
   mObjBox.maxExtents.set( mDimensions.x,  mDimensions.y, mDimensions.z);
   setScale(scale);
   setTransform(mat);
}


/// HH: Added for Tribes 2 compatibility.
///
/// Twenty-two words, which is what unpackUpdate below already reads: the
/// transform, the scale and the half-extents of the box a vehicle may not
/// drive into.  The box is the whole object -- nothing is rendered for one --
/// so the dimensions reaching mObjBox is the difference between a blocker that
/// blocks and an empty place in the container.
///
/// The second reader names the last six words two corners of a bounding box
/// rather than a scale and a set of dimensions.  Both readings are three words
/// and three words, so no recorded session can separate them, but this one is
/// what the Torque tree this class came from writes, and what a session shows
/// agrees with it: the first triple arrives as (1, 1, 1), which is an identity
/// scale rather than a plausible lower corner.
void VehicleBlocker::readT2Fields(T2Reader &reader)
{
   Parent::readT2Fields(reader);

   T2Fields f(reader);

   MatrixF mat;
   F32 *m = mat;
   for(U32 i = 0; i < 16; i++)
      if(!f.takeRawFloat(m[i]))
         return;

   Point3F scale;
   Point3F dimensions;
   if(!f.takeRawPoint(scale) || !f.takeRawPoint(dimensions))
      return;

   mDimensions = dimensions;
   mObjBox.minExtents.set(-mDimensions.x, -mDimensions.y, 0);
   mObjBox.maxExtents.set( mDimensions.x,  mDimensions.y, mDimensions.z);
   setScale(scale);
   setTransform(mat);
}

// HH: What the reader above filed.
//
// Read back out of mObjBox rather than out of mDimensions, because the box is
// what the rest of the engine collides against: a reader that filed the right
// dimensions into the wrong half of the box would still have mDimensions
// right, and that is the fault worth catching.
//
// Only one of the second reader's two names is here, and the other is a
// finding rather than an omission.  It calls them boundsMin and boundsMax and
// reports (1,1,1) and (2,2,4) for this mission's blockers -- but the body
// sends a scale and then the dimensions, in that order, and (1,1,1) is the
// scale.  So its boundsMin is the scale under another name; this tree files
// it with setScale() and SceneObject dumps it as "scale", where it compares
// and agrees.  Dumping mObjBox.minExtents as boundsMin would put (-2,-2,0)
// against (1,1,1) and report a disagreement that is the name's and not the
// reader's.
void VehicleBlocker::dumpT2Members(T2Dump &d) const
{
   Parent::dumpT2Members(d);

   d.put("boundsMax", mObjBox.maxExtents);
}


void VehicleBlocker::buildConvex(const Box3F& box, Convex* convex)
{
   // These should really come out of a pool
   mConvexList->collectGarbage();

   if (box.isOverlapped(getWorldBox()) == false)
      return;

   // Just return a box convex for the entire shape...
   Convex* cc = 0;
   CollisionWorkingList& wl = convex->getWorkingList();
   for (CollisionWorkingList* itr = wl.wLink.mNext; itr != &wl; itr = itr->wLink.mNext) 
   {
      if (itr->mConvex->getType() == BoxConvexType &&
          itr->mConvex->getObject() == this) {
         cc = itr->mConvex;
         break;
      }
   }
   if (cc)
      return;

   // Create a new convex.
   BoxConvex* cp = new BoxConvex;
   mConvexList->registerObject(cp);
   convex->addToWorkingList(cp);
   cp->init(this);

   mObjBox.getCenter(&cp->mCenter);
   cp->mSize.x = mObjBox.len_x() / 2.0f;
   cp->mSize.y = mObjBox.len_y() / 2.0f;
   cp->mSize.z = mObjBox.len_z() / 2.0f;
}

