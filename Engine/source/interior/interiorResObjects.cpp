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

#include "interior/interiorResObjects.h"
#include "console/console.h"
#include "core/stream/stream.h"
#include "math/mathIO.h"

//--------------------------------------------------------------------------
//--------------------------------------
//

bool InteriorDict::read(Stream &stream)
{
   U32 sz;
   stream.read(&sz);

   // HH: A count the file cannot be holding is not a count.
   //
   // An entry is two length-prefixed strings, so it is at least two bytes, and
   // a stream with fewer than 2*sz left has not got sz of them.  Without this
   // the first thing a lost cursor does here is setSize() on a number in the
   // billions, and the process dies inside the allocator -- no return value,
   // no file name, nothing to tell the sweep which .dif it was reading.  An
   // unreadable interior should cost a building.
   const U32 remaining = stream.getStreamSize() - stream.getPosition();
   if( sz > remaining / 2 )
   {
      Con::errorf( "InteriorDict::read: %u entries with %u bytes left", sz, remaining );
      return false;
   }

   setSize(sz);
   for(U32 i = 0; i < sz; i++)
   {
      InteriorDictEntry e;
      stream.readString(e.name);
      stream.readString(e.value);
      (*this)[i] = e;
   }

   return true;
}

void InteriorDict::write(Stream &stream) const
{
   U32 sz = size();
   stream.write(sz);
   for(U32 i = 0; i < sz; i++)
   {
      stream.writeString((*this)[i].name);
      stream.writeString((*this)[i].value);
   }
}

// HH: Modified for Tribes 2 compatibility.
//
// A version 0 trigger carries a name and a polyhedron and nothing else.  The
// datablock string and the dictionary that stand between them here are later
// additions, and reading them off a Tribes 2 file takes the length byte of
// the point count as a string length and the points themselves as a count --
// which is how xbunk2.dif came to die inside the allocator in
// InteriorDict::read rather than fail.
//
// Read out of the file rather than assumed.  xbunk2 is the only interior of
// the 325 with a trigger at all, and both of its decode exactly on this
// reading, each landing on the first byte of the next record:
//
//     2 triggers
//     "elevator2_t2"  8 points  6 planes  12 edges  offset (-9, 19.5, 0)
//     "elevator2_t1"  8 points  6 planes  12 edges  offset (-8, 19.5, -6.75)
//
// It is the elevator bunker, and those are its two elevator shafts.
bool InteriorResTrigger::read(Stream& stream)
{
   U32 i, size;
   stream.readString(mName);
   mDataBlock = "";

   // Read the polyhedron
   stream.read(&size);
   mPolyhedron.pointList.setSize(size);
   for (i = 0; i < mPolyhedron.pointList.size(); i++)
      mathRead(stream, &mPolyhedron.pointList[i]);

   stream.read(&size);
   mPolyhedron.planeList.setSize(size);
   for (i = 0; i < mPolyhedron.planeList.size(); i++)
      mathRead(stream, &mPolyhedron.planeList[i]);

   stream.read(&size);
   mPolyhedron.edgeList.setSize(size);
   for (i = 0; i < mPolyhedron.edgeList.size(); i++) {
      Polyhedron::Edge& rEdge = mPolyhedron.edgeList[i];

      stream.read(&rEdge.face[0]);
      stream.read(&rEdge.face[1]);
      stream.read(&rEdge.vertex[0]);
      stream.read(&rEdge.vertex[1]);
   }

   // And the offset
   mathRead(stream, &mOffset);

   return (stream.getStatus() == Stream::Ok);
}

bool InteriorResTrigger::write(Stream& stream) const
{
   U32 i;

   stream.writeString(mName);
   stream.writeString(mDataBlock);
   mDictionary.write(stream);

   // Write the polyhedron
   stream.write(mPolyhedron.pointList.size());
   for (i = 0; i < mPolyhedron.pointList.size(); i++)
      mathWrite(stream, mPolyhedron.pointList[i]);

   stream.write(mPolyhedron.planeList.size());
   for (i = 0; i < mPolyhedron.planeList.size(); i++)
      mathWrite(stream, mPolyhedron.planeList[i]);

   stream.write(mPolyhedron.edgeList.size());
   for (i = 0; i < mPolyhedron.edgeList.size(); i++) {
      const Polyhedron::Edge& rEdge = mPolyhedron.edgeList[i];

      stream.write(rEdge.face[0]);
      stream.write(rEdge.face[1]);
      stream.write(rEdge.vertex[0]);
      stream.write(rEdge.vertex[1]);
   }

   // And the offset
   mathWrite(stream, mOffset);

   return (stream.getStatus() == Stream::Ok);
}

InteriorPathFollower::InteriorPathFollower()
{
   VECTOR_SET_ASSOCIATION( mTriggerIds );
   VECTOR_SET_ASSOCIATION( mWayPoints );

   mName      = "";
   mPathIndex = 0;
   mOffset.set(0, 0, 0);
}

InteriorPathFollower::~InteriorPathFollower()
{

}

// HH: Modified for Tribes 2 compatibility.
//
// The same shape of difference as the trigger above, and read the same way.
// A version 0 path follower is a name, its waypoints and a total, and a
// waypoint is a position, a rotation and the milliseconds to the next one --
// no datablock, no resource index, no offset, no dictionary, no trigger ids,
// and no smoothing type.  xbunk2.dif's three decode cleanly and consecutively
// on it, which is the whole of the evidence and also all there is: it is the
// only interior in the game with a path follower.
//
//     "epath"  2 waypoints  (-8, 19.5, -11.5) -> (-8, 19.5, -3.5)  1000 ms
//     "bn1"    2 waypoints  (16.5, 1.25, 10.75) -> (16.5, 3.5, 10.75)  500 ms
//     "bs1"    2 waypoints  (16.5, -1.25, 10.75) -> (16.5, -3.5, 10.75)  500 ms
//
// An elevator that rises eight units in a second and two doors that slide two
// and a half in half of one.  The last waypoint's msToNext is zero in all
// three, which is what a last waypoint should hold.
//
// One byte is left that this cannot name.  It follows the total, it is zero
// in all three records, and taking it is what puts the cursor on the next
// record's name length.  It is read as a byte rather than guessed at as a
// field: a wrong guess about what it is costs nothing, a wrong guess about
// how wide it is loses the file.
bool InteriorPathFollower::read(Stream& stream)
{
   mName = stream.readSTString();
   mDataBlock = "";
   mInteriorResIndex = 0;
   mOffset.set(0, 0, 0);

   U32 numWayPoints;
   stream.read(&numWayPoints);
   mWayPoints.setSize(numWayPoints);
   for(U32 i = 0; i < numWayPoints; i++)
   {
      mathRead(stream, &mWayPoints[i].pos);
      mathRead(stream, &mWayPoints[i].rot);
      stream.read(&mWayPoints[i].msToNext);
      mWayPoints[i].smoothingType = 0;
   }
   stream.read(&mTotalMS);

   U8 unnamed;
   stream.read(&unnamed);

   return (stream.getStatus() == Stream::Ok);
}

bool InteriorPathFollower::write(Stream& stream) const
{
   stream.writeString(mName);
   stream.writeString(mDataBlock);
   stream.write(mInteriorResIndex);
   mathWrite(stream, mOffset);
   mDictionary.write(stream);

   stream.write(mTriggerIds.size());
   for (U32 i = 0; i < mTriggerIds.size(); i++)
      stream.write(mTriggerIds[i]);

   stream.write(U32(mWayPoints.size()));
   for (U32 i = 0; i < mWayPoints.size(); i++) {
      mathWrite(stream, mWayPoints[i].pos);
      mathWrite(stream, mWayPoints[i].rot);
      stream.write(mWayPoints[i].msToNext);
      stream.write(mWayPoints[i].smoothingType);
   }
   stream.write(mTotalMS);

   return (stream.getStatus() == Stream::Ok);
}

AISpecialNode::AISpecialNode()
{
   mName = "";
   mPos.set(0, 0, 0);
}

AISpecialNode::~AISpecialNode()
{
}

bool AISpecialNode::read(Stream& stream)
{
   mName = stream.readSTString();
   mathRead(stream, &mPos);

   return (stream.getStatus() == Stream::Ok);
}

bool AISpecialNode::write(Stream& stream) const
{
   stream.writeString(mName);
   mathWrite(stream, mPos);

   return (stream.getStatus() == Stream::Ok);
}

ItrGameEntity::ItrGameEntity()
{
   mDataBlock = "";
   mGameClass = "";
   mPos.set(0, 0, 0);
}

ItrGameEntity::~ItrGameEntity()
{
}

bool ItrGameEntity::read(Stream& stream)
{
   mDataBlock = stream.readSTString();
   mGameClass = stream.readSTString();
   mathRead(stream, &mPos);
   if( !mDictionary.read(stream) )
      return false;

   return (stream.getStatus() == Stream::Ok);
}

bool ItrGameEntity::write(Stream& stream) const
{
   stream.writeString(mDataBlock);
   stream.writeString(mGameClass);
   mathWrite(stream, mPos);
   mDictionary.write(stream);

   return (stream.getStatus() == Stream::Ok);
}
