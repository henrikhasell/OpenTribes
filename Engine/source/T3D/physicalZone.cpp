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

#include "T3D/physicalZone.h"
#include "core/stream/bitStream.h"
#include "collision/boxConvex.h"
#include "collision/clippedPolyList.h"
#include "console/consoleTypes.h"
#include "math/mathIO.h"
#include "scene/sceneRenderState.h"
#include "T3D/trigger.h"
#include "gfx/gfxTransformSaver.h"
#include "renderInstance/renderPassManager.h"
#include "gfx/gfxDrawUtil.h"
#include "console/engineAPI.h"
#include "T3D/gameBase/t2Fields.h"
#include "T3D/gameBase/t2Layout.h"

IMPLEMENT_CO_NETOBJECT_V1(PhysicalZone);

ConsoleDocClass( PhysicalZone,
   "@brief Physical Zones are areas that modify the player's gravity and/or velocity and/or applied force.\n\n"

   "The datablock properties determine how the physics, velocity and applied forces affect a player who enters this zone.\n"

   "@tsexample\n"
      "new PhysicalZone(Team1JumpPad) {\n"
         "velocityMod = \"1\";"
         "gravityMod = \"0\";\n"
         "appliedForce = \"0 0 20000\";\n"
         "polyhedron = \"0.0000000 0.0000000 0.0000000 1.0000000 0.0000000 0.0000000 0.0000000 -1.0000000 0.0000000 0.0000000 0.0000000 1.0000000\";\n"
         "position = \"273.559 -166.371 249.856\";\n"
         "rotation = \"0 0 1 13.0216\";\n"
         "scale = \"8 4.95 28.31\";\n"
         "isRenderEnabled = \"true\";\n"
         "canSaveDynamicFields = \"1\";\n"
         "enabled = \"1\";\n"
      "};\n"
   "@endtsexample\n\n"
   "@ingroup enviroMisc\n"
);

bool PhysicalZone::smRenderPZones = false;

DefineEngineMethod(PhysicalZone, activate, void, (),, "Activate the physical zone's effects.\n"
													"@tsexample\n"
														"// Activate effects for a specific physical zone.\n"
														"%thisPhysicalZone.activate();\n"
													"@endtsexample\n"
													"@ingroup Datablocks\n"
				  )
{
   if (object->isClientObject())
      return;

   object->activate();
}

DefineEngineMethod(PhysicalZone, deactivate, void, (),, "Deactivate the physical zone's effects.\n"
													"@tsexample\n"
														"// Deactivate effects for a specific physical zone.\n"
														"%thisPhysicalZone.deactivate();\n"
													"@endtsexample\n"
													"@ingroup Datablocks\n"
				  )
{
   if (object->isClientObject())
      return;

   object->deactivate();
}


//--------------------------------------------------------------------------
//--------------------------------------
//
PhysicalZone::PhysicalZone()
{
   mNetFlags.set(Ghostable | ScopeAlways);

   mTypeMask |= PhysicalZoneObjectType;

   mVelocityMod = 1.0f;
   mGravityMod  = 1.0f;
   mAppliedForce.set(0, 0, 0);

   mConvexList = new Convex;
   mActive = true;
}

PhysicalZone::~PhysicalZone()
{
   delete mConvexList;
   mConvexList = NULL;
}

//--------------------------------------------------------------------------
void PhysicalZone::consoleInit()
{
   Con::addVariable( "$PhysicalZone::renderZones", TypeBool, &smRenderPZones, "If true, a box will render around the location of all PhysicalZones.\n"
	   "@ingroup EnviroMisc\n");
}

void PhysicalZone::initPersistFields()
{
   addGroup("Misc");
   addField("velocityMod",  TypeF32,               Offset(mVelocityMod,  PhysicalZone), "Multiply velocity of objects entering zone by this value every tick.");
   addField("gravityMod",   TypeF32,               Offset(mGravityMod,   PhysicalZone), "Gravity in PhysicalZone. Multiplies against standard gravity.");
   addField("appliedForce", TypePoint3F,           Offset(mAppliedForce, PhysicalZone), "Three-element floating point value representing forces in three axes to apply to objects entering PhysicalZone.");
   addField("polyhedron",   TypeTriggerPolyhedron, Offset(mPolyhedron,   PhysicalZone),
      "The polyhedron type is really a quadrilateral and consists of a corner"
      "point followed by three vectors representing the edges extending from the corner." );
   endGroup("Misc");

   Parent::initPersistFields();
}

//--------------------------------------------------------------------------
bool PhysicalZone::onAdd()
{
   if(!Parent::onAdd())
      return false;

   if (mVelocityMod < -40.0f || mVelocityMod > 40.0f) {
      Con::errorf("PhysicalZone: velocity mod out of range.  [-40, 40]");
      mVelocityMod = mVelocityMod < -40.0f ? -40.0f : 40.0f;
   }
   if (mGravityMod < -40.0f || mGravityMod > 40.0f) {
      Con::errorf("PhysicalZone: GravityMod out of range.  [-40, 40]");
      mGravityMod = mGravityMod < -40.0f ? -40.0f : 40.0f;
   }
   static const char* coordString[] = { "x", "y", "z" };
   F32* p = mAppliedForce;
   for (U32 i = 0; i < 3; i++) {
      if (p[i] < -40000.0f || p[i] > 40000.0f) {
         Con::errorf("PhysicalZone: applied force: %s out of range.  [-40000, 40000]", coordString[i]);
         p[i] = p[i] < -40000.0f ? -40000.0f : 40000.0f;
      }
   }

   Polyhedron temp = mPolyhedron;
   setPolyhedron(temp);

   addToScene();

   return true;
}


void PhysicalZone::onRemove()
{
   mConvexList->nukeList();

   removeFromScene();
   Parent::onRemove();
}

void PhysicalZone::inspectPostApply()
{
   setPolyhedron(mPolyhedron);
   Parent::inspectPostApply();
}

//------------------------------------------------------------------------------
void PhysicalZone::setTransform(const MatrixF & mat)
{
   Parent::setTransform(mat);

      MatrixF base(true);
      base.scale(Point3F(1.0/mObjScale.x,
                         1.0/mObjScale.y,
                         1.0/mObjScale.z));
      base.mul(mWorldToObj);
      mClippedList.setBaseTransform(base);

   if (isServerObject())
      setMaskBits(InitialUpdateMask);
}


void PhysicalZone::prepRenderImage( SceneRenderState *state )
{
   // only render if selected or render flag is set
   if ( !smRenderPZones && !isSelected() )
      return;

   ObjectRenderInst *ri = state->getRenderPass()->allocInst<ObjectRenderInst>();
   ri->renderDelegate.bind( this, &PhysicalZone::renderObject );
   ri->type = RenderPassManager::RIT_Editor;
   ri->defaultKey = 0;
   ri->defaultKey2 = 0;
   state->getRenderPass()->addInst( ri );
}


void PhysicalZone::renderObject( ObjectRenderInst *ri,
                                 SceneRenderState *state,
                                 BaseMatInstance *overrideMat )
{
   if (overrideMat)
      return;

   GFXStateBlockDesc desc;
   desc.setZReadWrite( true, false );
   desc.setBlend( true );
   desc.setCullMode( GFXCullNone );

   GFXTransformSaver saver;

   MatrixF mat = getRenderTransform();
   mat.scale( getScale() );

   GFX->multWorld( mat );

   GFXDrawUtil *drawer = GFX->getDrawUtil();
   drawer->drawPolyhedron( desc, mPolyhedron, ColorI( 0, 255, 0, 45 ) );

   desc.setFillModeWireframe();
   drawer->drawPolyhedron( desc, mPolyhedron, ColorF::BLACK );
}

//--------------------------------------------------------------------------
U32 PhysicalZone::packUpdate(NetConnection* con, U32 mask, BitStream* stream)
{
   U32 i;
   U32 retMask = Parent::packUpdate(con, mask, stream);

   if (stream->writeFlag((mask & InitialUpdateMask) != 0)) {
      // Note that we don't really care about efficiency here, since this is an
      //  edit-only ghost...
      mathWrite(*stream, mObjToWorld);
      mathWrite(*stream, mObjScale);

      // Write the polyhedron
      stream->write(mPolyhedron.pointList.size());
      for (i = 0; i < mPolyhedron.pointList.size(); i++)
         mathWrite(*stream, mPolyhedron.pointList[i]);

      stream->write(mPolyhedron.planeList.size());
      for (i = 0; i < mPolyhedron.planeList.size(); i++)
         mathWrite(*stream, mPolyhedron.planeList[i]);

      stream->write(mPolyhedron.edgeList.size());
      for (i = 0; i < mPolyhedron.edgeList.size(); i++) {
         const Polyhedron::Edge& rEdge = mPolyhedron.edgeList[i];

         stream->write(rEdge.face[0]);
         stream->write(rEdge.face[1]);
         stream->write(rEdge.vertex[0]);
         stream->write(rEdge.vertex[1]);
      }

      stream->write(mVelocityMod);
      stream->write(mGravityMod);
      mathWrite(*stream, mAppliedForce);
      stream->writeFlag(mActive);
   } else {
      stream->writeFlag(mActive);
   }

   return retMask;
}

void PhysicalZone::unpackUpdate(NetConnection* con, BitStream* stream)
{
   Parent::unpackUpdate(con, stream);

   if (stream->readFlag()) {
      U32 i, size;
      MatrixF temp;
      Point3F tempScale;
      Polyhedron tempPH;

      // Transform
      mathRead(*stream, &temp);
      mathRead(*stream, &tempScale);

      // Read the polyhedron
      stream->read(&size);
      tempPH.pointList.setSize(size);
      for (i = 0; i < tempPH.pointList.size(); i++)
         mathRead(*stream, &tempPH.pointList[i]);

      stream->read(&size);
      tempPH.planeList.setSize(size);
      for (i = 0; i < tempPH.planeList.size(); i++)
         mathRead(*stream, &tempPH.planeList[i]);

      stream->read(&size);
      tempPH.edgeList.setSize(size);
      for (i = 0; i < tempPH.edgeList.size(); i++) {
         Polyhedron::Edge& rEdge = tempPH.edgeList[i];

         stream->read(&rEdge.face[0]);
         stream->read(&rEdge.face[1]);
         stream->read(&rEdge.vertex[0]);
         stream->read(&rEdge.vertex[1]);
      }

      stream->read(&mVelocityMod);
      stream->read(&mGravityMod);
      mathRead(*stream, &mAppliedForce);

      setPolyhedron(tempPH);
      setScale(tempScale);
      setTransform(temp);
      mActive = stream->readFlag();
   } else {
      mActive = stream->readFlag();
   }
}


//--------------------------------------------------------------------------
/// HH: Added for Tribes 2 compatibility.
///
/// The nodes of this class's layout that name a field no shape can, resolved
/// once per class rather than per packet: the lookup is a string search and a
/// ghost body arrives as often as packets do.
///
/// A Tribes 2 zone sends its volume as three lists of thirty-two bit words one
/// after another -- points, then planes, then winged edges -- and the counts
/// that separate them are read by the cursor to bound the loops, so they are
/// not fields a class can see.  What tells a point from a plane from an edge is
/// which node of the layout the cursor stands on: each list's first word has
/// one of its own, and an iteration returns to it.  The flags are here for the
/// same reason, a flag never being a field.
///
/// Tools/t2/handlayout.py names them where it writes the layout, so these are
/// the generator's numbers rather than this file's -- which is the difference
/// between a regeneration that moves one being a diff to follow and being a
/// silent misread.
struct PhysicalZoneT2Nodes
{
   U32 point, plane, edge, velocityMod, active, initial;
};

static const PhysicalZoneT2Nodes *t2NodesFor( const char *className )
{
   static const char *sName = NULL;
   static PhysicalZoneT2Nodes sNodes;

   if ( !sName || dStrcmp( sName, className ) )
   {
      sNodes.point       = T2Layout::ghostNode( className, "point" );
      sNodes.plane       = T2Layout::ghostNode( className, "plane" );
      sNodes.edge        = T2Layout::ghostNode( className, "edge" );
      sNodes.velocityMod = T2Layout::ghostNode( className, "velocityMod" );
      sNodes.active      = T2Layout::ghostNode( className, "active" );
      sNodes.initial     = T2Layout::ghostNode( className, "initial" );
      sName = className;
   }

   return &sNodes;
}

/// HH: Added for Tribes 2 compatibility.
///
/// Where you are pushed, slowed or made light.  The whole of a Tribes 2
/// physical zone: the transform and scale that place it, the polyhedron that
/// is its volume, the two modifiers and the force it applies, and whether it
/// is switched on.
///
/// Read in the order the server writes it, which is the order
/// PhysicalZone::unpackUpdate below reads it in -- the two functions share an
/// ancestor -- so this hands the fields to the same three calls.  The
/// polyhedron goes in only when the body ran whole, the test being that the
/// cursor reached the modifiers behind it: five points of eight and no planes
/// would be a volume the server never described, where no polyhedron at all is
/// merely a zone that enforces nothing.  The transform is applied either way,
/// since it arrives in front of the lists and cannot be made wrong by them.
void PhysicalZone::readT2Fields(T2Reader &reader)
{
   Parent::readT2Fields(reader);

   T2Fields f(reader);

   const PhysicalZoneT2Nodes *n = t2NodesFor(getClassName());

   bool initial = false;
   if(f.takeFlag(n->initial, initial) && initial)
      _readT2Volume(f);

   // Last rather than first, which is the one thing about reading a flag that
   // is not obvious: a flag is there to be read once the cursor has passed it,
   // and this one sits behind every field in the body.  Taken at the top it
   // would arrive only on an update body, where there is no field at all and
   // the cursor resolved both flags running to the end looking for one.
   f.takeFlag(n->active, mActive);
}

/// HH: Added for Tribes 2 compatibility.
///
/// The initial body: the place, the volume, and what the zone does to what is
/// inside it.  Split out so that the flag behind all of it has one place to be
/// taken from however far down this gets.
void PhysicalZone::_readT2Volume(T2Fields &f)
{
   const PhysicalZoneT2Nodes *n = t2NodesFor(getClassName());

   MatrixF mat;
   F32 *m = mat;
   for(U32 i = 0; i < 16; i++)
      if(!f.takeRawFloat(m[i]))
         return;

   Point3F scale;
   if(!f.takeRawPoint(scale))
      return;

   Polyhedron poly;

   while(f.atNode(n->point))
   {
      Point3F point;
      if(!f.takeRawPoint(point))
         break;
      poly.pointList.push_back(point);
   }

   while(f.atNode(n->plane))
   {
      PlaneF plane;
      if(!f.takeRawFloat(plane.x) || !f.takeRawFloat(plane.y) ||
         !f.takeRawFloat(plane.z) || !f.takeRawFloat(plane.d))
         break;
      poly.planeList.push_back(plane);
   }

   while(f.atNode(n->edge))
   {
      Polyhedron::Edge edge;
      if(!f.takeInt(32, edge.face[0])   || !f.takeInt(32, edge.face[1]) ||
         !f.takeInt(32, edge.vertex[0]) || !f.takeInt(32, edge.vertex[1]))
         break;
      poly.edgeList.push_back(edge);
   }

   // The lists ran to their end, so what they hold is the volume the server
   // described rather than part of it.
   const bool whole = f.atNode(n->velocityMod);

   if(whole && f.takeRawFloat(mVelocityMod) && f.takeRawFloat(mGravityMod))
   {
      Point3F force;
      if(f.takeRawPoint(force))
         mAppliedForce = force;
   }

   if(whole)
      setPolyhedron(poly);
   setScale(scale);
   setTransform(mat);
}

void PhysicalZone::setPolyhedron(const Polyhedron& rPolyhedron)
{
   mPolyhedron = rPolyhedron;

   if (mPolyhedron.pointList.size() != 0) {
      mObjBox.minExtents.set(1e10, 1e10, 1e10);
      mObjBox.maxExtents.set(-1e10, -1e10, -1e10);
      for (U32 i = 0; i < mPolyhedron.pointList.size(); i++) {
         mObjBox.minExtents.setMin(mPolyhedron.pointList[i]);
         mObjBox.maxExtents.setMax(mPolyhedron.pointList[i]);
      }
   } else {
      mObjBox.minExtents.set(-0.5, -0.5, -0.5);
      mObjBox.maxExtents.set( 0.5,  0.5,  0.5);
   }

   MatrixF xform = getTransform();
   setTransform(xform);

   mClippedList.clear();
   mClippedList.mPlaneList = mPolyhedron.planeList;

   MatrixF base(true);
   base.scale(Point3F(1.0/mObjScale.x,
                      1.0/mObjScale.y,
                      1.0/mObjScale.z));
   base.mul(mWorldToObj);

   mClippedList.setBaseTransform(base);
}


//--------------------------------------------------------------------------
void PhysicalZone::buildConvex(const Box3F& box, Convex* convex)
{
   // These should really come out of a pool
   mConvexList->collectGarbage();

   Box3F realBox = box;
   mWorldToObj.mul(realBox);
   realBox.minExtents.convolveInverse(mObjScale);
   realBox.maxExtents.convolveInverse(mObjScale);

   if (realBox.isOverlapped(getObjBox()) == false)
      return;

   // Just return a box convex for the entire shape...
   Convex* cc = 0;
   CollisionWorkingList& wl = convex->getWorkingList();
   for (CollisionWorkingList* itr = wl.wLink.mNext; itr != &wl; itr = itr->wLink.mNext) {
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

bool PhysicalZone::testObject(SceneObject* enter)
{
   // TODO: This doesn't look like it's testing against the polyhedron at
   // all.  And whats the point of building a convex if no collision methods
   // are implemented?

   if (mPolyhedron.pointList.size() == 0)
      return false;

   mClippedList.clear();

   SphereF sphere;
   sphere.center = (mWorldBox.minExtents + mWorldBox.maxExtents) * 0.5;
   VectorF bv = mWorldBox.maxExtents - sphere.center;
   sphere.radius = bv.len();

   enter->buildPolyList(PLC_Collision, &mClippedList, mWorldBox, sphere);
   return mClippedList.isEmpty() == false;
}

bool PhysicalZone::testBox( const Box3F &box ) const
{
   return mWorldBox.isOverlapped( box );
}

void PhysicalZone::activate()
{
   AssertFatal(isServerObject(), "Client objects not allowed in ForceFieldInstance::open()");

   if (mActive != true)
      setMaskBits(ActiveMask);
   mActive = true;
}

void PhysicalZone::deactivate()
{
   AssertFatal(isServerObject(), "Client objects not allowed in ForceFieldInstance::close()");

   if (mActive != false)
      setMaskBits(ActiveMask);
   mActive = false;
}

