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

#include "interior/interior.h"
#include "core/bitVector.h"
#include "core/stream/stream.h"
#include "math/mathIO.h"
#include "gfx/bitmap/gBitmap.h"
#include "interior/interiorSubObject.h"
#include "console/console.h"
#include "core/frameAllocator.h"
#include "materials/materialList.h"

int QSORT_CALLBACK cmpU32(const void* p1, const void* p2)
{
   return S32(*((U32*)p1)) - S32(*((U32*)p2));
}

//------------------------------------------------------------------------------
//-------------------------------------- PERSISTENCE IMPLEMENTATION
//
U32 Interior::smFileVersion = 14;

bool Interior::read(Stream& stream)
{
   AssertFatal(stream.hasCapability(Stream::StreamRead), "Interior::read: non-read capable stream passed");
   AssertFatal(stream.getStatus() == Stream::Ok, "Interior::read: Error, stream in inconsistent state");

   S32 i;

   // Version this stream.  We only load stream of the current version
   U32 fileVersion;
   stream.read(&fileVersion);

   // We need to store the version in case there is any post processing that
   // needs to take place that is dependent on the file version
   mFileVersion = fileVersion;
   
   if (fileVersion > smFileVersion)
   {
      Con::errorf(ConsoleLogEntry::General, "Interior::read: incompatible file version found.");
      return false;
   }

   // Geometry factors...
   stream.read(&mDetailLevel);

   stream.read(&mMinPixels);
   mathRead(stream, &mBoundingBox);
   mathRead(stream, &mBoundingSphere);
   stream.read(&mHasAlarmState);
   stream.read(&mNumLightStateEntries);

   // Now read in our data vectors.
   S32 vectorSize;

   // mPlanes
   readPlaneVector(stream);

   // mPoints
   stream.read(&vectorSize);
   mPoints.setSize(vectorSize);
   for (i = 0; i < mPoints.size(); i++)
      mathRead(stream, &mPoints[i].point);

   // mPointVisibility
   stream.read(&vectorSize);
   mPointVisibility.setSize(vectorSize);
   stream.read(vectorSize, mPointVisibility.address());

   // mTexGenEQs
   stream.read(&vectorSize);
   mTexGenEQs.setSize(vectorSize);
   for(i = 0; i < mTexGenEQs.size(); i++)
   {
      mathRead(stream, &mTexGenEQs[i].planeX);
      mathRead(stream, &mTexGenEQs[i].planeY);
   }

   // mBSPNodes;
   stream.read(&vectorSize);
   mBSPNodes.setSize(vectorSize);
   for(i = 0; i < mBSPNodes.size(); i++)
   {
      stream.read(&mBSPNodes[i].planeIndex);

      if (fileVersion >= 14)
      {
         stream.read(&mBSPNodes[i].frontIndex);
         stream.read(&mBSPNodes[i].backIndex);
      }
      else
      {
         U16 frontIndex, backIndex;
         stream.read(&frontIndex);
         stream.read(&backIndex);

         mBSPNodes[i].frontIndex = U32(frontIndex);
         mBSPNodes[i].backIndex = U32(backIndex);
      }
   }

   // mBSPSolidLeaves
   stream.read(&vectorSize);
   mBSPSolidLeaves.setSize(vectorSize);
   for(i = 0; i < mBSPSolidLeaves.size(); i++)
   {
      stream.read(&mBSPSolidLeaves[i].surfaceIndex);
      stream.read(&mBSPSolidLeaves[i].surfaceCount);
   }

   // MaterialList
   if(mMaterialList != NULL)
      delete mMaterialList;
   mMaterialList = new MaterialList;
   mMaterialList->read(stream);


   // mWindings
   stream.read(&vectorSize);
   mWindings.setSize(vectorSize);
   for(i = 0; i < mWindings.size(); i++)
   {
      stream.read(&mWindings[i]);
   }

   // mWindingIndices
   stream.read(&vectorSize);
   mWindingIndices.setSize(vectorSize);
   for(i = 0; i < mWindingIndices.size(); i++)
   {
      stream.read(&mWindingIndices[i].windingStart);
      stream.read(&mWindingIndices[i].windingCount);
   }

   // mEdges
   if (fileVersion >= 12)
   {
      stream.read(&vectorSize);

      mEdges.setSize(vectorSize);
      for (i = 0; i < mEdges.size(); i++)
      {
         stream.read(&mEdges[i].vertexes[0]);
         stream.read(&mEdges[i].vertexes[1]);
         stream.read(&mEdges[i].faces[0]);
         stream.read(&mEdges[i].faces[1]);
      }
   }

   // mZones
   stream.read(&vectorSize);
   mZones.setSize(vectorSize);
   for(i = 0; i < mZones.size(); i++)
   {
      stream.read(&mZones[i].portalStart);
      stream.read(&mZones[i].portalCount);
      stream.read(&mZones[i].surfaceStart);
      stream.read(&mZones[i].surfaceCount);

      if (fileVersion >= 12)
      {
         stream.read(&mZones[i].staticMeshStart);
         stream.read(&mZones[i].staticMeshCount);
      }
      else
      {
         mZones[i].staticMeshStart = 0;
         mZones[i].staticMeshCount = 0;
      }

      stream.read(&mZones[i].flags);
      mZones[i].zoneId = 0;
   }

   // Zone surfaces
   stream.read(&vectorSize);
   mZoneSurfaces.setSize(vectorSize);
   for(i = 0; i < mZoneSurfaces.size(); i++)
      stream.read(&mZoneSurfaces[i]);

   // Zone static meshes
   if (fileVersion >= 12)
   {
      stream.read(&vectorSize);

      mZoneStaticMeshes.setSize(vectorSize);
      for (i = 0; i < mZoneStaticMeshes.size(); i++)
         stream.read(&mZoneStaticMeshes[i]);
   }

   //  mZonePortalList;
   stream.read(&vectorSize);
   mZonePortalList.setSize(vectorSize);
   for(i = 0; i < mZonePortalList.size(); i++)
      stream.read(&mZonePortalList[i]);

   // mPortals
   stream.read(&vectorSize);
   mPortals.setSize(vectorSize);
   for(i = 0; i < mPortals.size(); i++)
   {
      stream.read(&mPortals[i].planeIndex);
      stream.read(&mPortals[i].triFanCount);
      stream.read(&mPortals[i].triFanStart);
      stream.read(&mPortals[i].zoneFront);
      stream.read(&mPortals[i].zoneBack);
   }

   // mSurfaces
   stream.read(&vectorSize);
   mSurfaces.setSize(vectorSize);
   mLMTexGenEQs.setSize(vectorSize);

   // HH: A version 0 interior is read one way rather than guessed at.
   //
   // Every .dif this port will ever open is a Tribes 2 one -- 902 interiors in
   // 325 files across the three volumes the game ships -- and all of them are
   // InteriorResource version 44 holding Interior version 0.  That reading is
   // the one tgeInterior selects: no trailing byte behind a surface's map sizes
   // and one lightmap per lightmap rather than two.  It was checked against the
   // engine that wrote the files, and against the files: a reading of the whole
   // resource taken from that engine consumes all 902 of them to the last byte,
   // where adding the byte loses between a third and a half of each volume.
   //
   // What was here instead read the surfaces as a TGEA version 0 first and only
   // fell back to this when readSurface returned false.  That is not a test of
   // which variant the file is -- it is a test of whether the stream has already
   // been lost, and the wrong variant can pass it: the surfaces come out
   // misaligned but inside their bounds, and nothing fails until some vector
   // length further down the body reads absurd.  So the fallback was reached by
   // luck, and a file it was not reached for was read as rubbish in silence.
   //
   // A TGEA version 0 interior is no longer readable here.  Nothing in this tree
   // has one: there is not a single .dif in the repository, and the three
   // volumes hold only Tribes 2's.
   const bool tgeInterior = (fileVersion == 0);

   for(i = 0; i < mSurfaces.size(); i++)
      if (readSurface(stream, mSurfaces[i], mLMTexGenEQs[i], tgeInterior) == false)
         return false;

   // Edges
   if (fileVersion == 5)
   {
      stream.read(&vectorSize);
      mEdges.setSize(vectorSize);
      for (i = 0; i < mEdges.size(); i++)
      {
         stream.read(&mEdges[i].vertexes[0]);
         stream.read(&mEdges[i].vertexes[1]);
         U32 normals[2];
         stream.read(&normals[0]);
         stream.read(&normals[1]);

         if (fileVersion > 2) // version 3 is where surface id's get added
         {
            stream.read(&mEdges[i].faces[0]);
            stream.read(&mEdges[i].faces[1]);
         }
      }
   }

   // mNormals
   if (fileVersion == 5)
   {
      stream.read(&vectorSize);
      Vector<Point3F> normals;
      normals.setSize(vectorSize);
      for(i = 0; i < normals.size(); i++)
         mathRead(stream, &normals[i]);

      // mNormalIndices
      stream.read(&vectorSize);
      Vector<U16> normalIndices;
      normalIndices.setSize(vectorSize);
      for (i = 0; i < normalIndices.size(); i++)
         stream.read(&normalIndices[i]);
   }
   
   // NormalLMapIndices
   stream.read(&vectorSize);
   mNormalLMapIndices.setSize(vectorSize);
   for (U32 i = 0; i < mNormalLMapIndices.size(); i++)
   {
      if (fileVersion >= 13)
         stream.read(&mNormalLMapIndices[i]);
      else
      {
         U8 index = 0;
         stream.read(&index);

         mNormalLMapIndices[i] = (U32)index;
      }
   }

   // AlarmLMapIndices
   stream.read(&vectorSize);
   mAlarmLMapIndices.setSize(vectorSize);
   for (U32 i = 0; i < mAlarmLMapIndices.size(); i++)
   {
      if (fileVersion >= 13)
         stream.read(&mAlarmLMapIndices[i]);
      else
      {
         U8 index = 0;
         stream.read(&index);

         mAlarmLMapIndices[i] = (U32)index;
      }
   }

   // mNullSurfaces
   stream.read(&vectorSize);
   mNullSurfaces.setSize(vectorSize);
   for(i = 0; i < mNullSurfaces.size(); i++)
   {
      stream.read(&mNullSurfaces[i].windingStart);
      stream.read(&mNullSurfaces[i].planeIndex);
      stream.read(&mNullSurfaces[i].surfaceFlags);

      if (fileVersion >= 13)
         stream.read(&mNullSurfaces[i].windingCount);
      else
      {
         U8 count;
         stream.read(&count);
         mNullSurfaces[i].windingCount = (U32)count;
      }
   }

   // mLightmaps
   stream.read(&vectorSize);
   mLightmaps.setSize(vectorSize);
   mLightmapKeep.setSize(vectorSize);
   GBitmap dummyBmp;
   for(i = 0; i < mLightmaps.size(); i++)
   {
      mLightmaps[i] = new GBitmap;
      mLightmaps[i]->readBitmap("png",stream);

      if (!tgeInterior && (fileVersion == 0 || fileVersion == 5 || fileVersion >= 12))
      {
         // The "light normal maps" or "light direction maps" were
         // removed from Torque 3D... this just reads and throws 
         // them away.
         dummyBmp.readBitmap("png",stream);
      }

      stream.read(&mLightmapKeep[i]);
   }

   // mSolidLeafSurfaces
   stream.read(&vectorSize);
   mSolidLeafSurfaces.setSize(vectorSize);
   for(i = 0; i < mSolidLeafSurfaces.size(); i++)
   {
      stream.read(&mSolidLeafSurfaces[i]);
   }

   // mAnimatedLights
   mNumTriggerableLights = 0;
   stream.read(&vectorSize);
   mAnimatedLights.setSize(vectorSize);
   for(i = 0; i < mAnimatedLights.size(); i++)
   {
      stream.read(&mAnimatedLights[i].nameIndex);
      stream.read(&mAnimatedLights[i].stateIndex);
      stream.read(&mAnimatedLights[i].stateCount);
      stream.read(&mAnimatedLights[i].flags);
      stream.read(&mAnimatedLights[i].duration);

      if((mAnimatedLights[i].flags & AnimationAmbient) == 0)
         mNumTriggerableLights++;
   }

   // mLightStates
   stream.read(&vectorSize);
   mLightStates.setSize(vectorSize);
   for(i = 0; i < mLightStates.size(); i++)
   {
      stream.read(&mLightStates[i].red);
      stream.read(&mLightStates[i].green);
      stream.read(&mLightStates[i].blue);
      stream.read(&mLightStates[i].activeTime);
      stream.read(&mLightStates[i].dataIndex);
      stream.read(&mLightStates[i].dataCount);
   }

   // mStateData
   stream.read(&vectorSize);
   mStateData.setSize(vectorSize);
   for(i = 0; i < mStateData.size(); i++)
   {
      stream.read(&mStateData[i].surfaceIndex);
      stream.read(&mStateData[i].mapIndex);
      stream.read(&mStateData[i].lightStateIndex);
   }

   // mStateDataBuffer
   stream.read(&vectorSize);
   mStateDataBuffer.setSize(vectorSize);
   U32 flags;
   stream.read(&flags);
   stream.read(mStateDataBuffer.size(), mStateDataBuffer.address());

   // mNameBuffer
   stream.read(&vectorSize);
   mNameBuffer.setSize(vectorSize);
   stream.read(mNameBuffer.size(), mNameBuffer.address());

   // mSubObjects
   stream.read(&vectorSize);
   mSubObjects.setSize(vectorSize);
   for (i = 0; i < mSubObjects.size(); i++)
   {
      InteriorSubObject* iso = InteriorSubObject::readISO(stream);
      AssertFatal(iso != NULL, "Error, bad sub object in stream!");
      mSubObjects[i] = iso;
   }

   // Convex hulls
   stream.read(&vectorSize);
   mConvexHulls.setSize(vectorSize);
   for(i = 0; i < mConvexHulls.size(); i++)
   {
      stream.read(&mConvexHulls[i].hullStart);
      stream.read(&mConvexHulls[i].hullCount);
      stream.read(&mConvexHulls[i].minX);
      stream.read(&mConvexHulls[i].maxX);
      stream.read(&mConvexHulls[i].minY);
      stream.read(&mConvexHulls[i].maxY);
      stream.read(&mConvexHulls[i].minZ);
      stream.read(&mConvexHulls[i].maxZ);
      stream.read(&mConvexHulls[i].surfaceStart);
      stream.read(&mConvexHulls[i].surfaceCount);
      stream.read(&mConvexHulls[i].planeStart);
      stream.read(&mConvexHulls[i].polyListPlaneStart);
      stream.read(&mConvexHulls[i].polyListPointStart);
      stream.read(&mConvexHulls[i].polyListStringStart);

      if (fileVersion >= 12)
         stream.read(&mConvexHulls[i].staticMesh);
      else
         mConvexHulls[i].staticMesh = false;
   }

   // Convex hull emit strings
   stream.read(&vectorSize);
   mConvexHullEmitStrings.setSize(vectorSize);
   stream.read(mConvexHullEmitStrings.size(), mConvexHullEmitStrings.address());

   // Hull indices
   stream.read(&vectorSize);
   mHullIndices.setSize(vectorSize);
   for(i = 0; i < mHullIndices.size(); i++)
      stream.read(&mHullIndices[i]);

   // Hull plane indices
   stream.read(&vectorSize);
   mHullPlaneIndices.setSize(vectorSize);
   for(i = 0; i < mHullPlaneIndices.size(); i++)
      stream.read(&mHullPlaneIndices[i]);

   // Hull emit string indices
   stream.read(&vectorSize);
   mHullEmitStringIndices.setSize(vectorSize);
   for(i = 0; i < mHullEmitStringIndices.size(); i++)
      stream.read(&mHullEmitStringIndices[i]);

   // Hull surface indices
   stream.read(&vectorSize);
   mHullSurfaceIndices.setSize(vectorSize);
   for(i = 0; i < mHullSurfaceIndices.size(); i++)
      stream.read(&mHullSurfaceIndices[i]);

   // PolyList planes
   stream.read(&vectorSize);
   mPolyListPlanes.setSize(vectorSize);
   for(i = 0; i < mPolyListPlanes.size(); i++)
      stream.read(&mPolyListPlanes[i]);

   // PolyList points
   stream.read(&vectorSize);
   mPolyListPoints.setSize(vectorSize);
   for(i = 0; i < mPolyListPoints.size(); i++)
      stream.read(&mPolyListPoints[i]);

   // PolyList strings
   stream.read(&vectorSize);
   mPolyListStrings.setSize(vectorSize);
   for(i = 0; i < mPolyListStrings.size(); i++)
      stream.read(&mPolyListStrings[i]);

   // Coord bins
   for(i = 0; i < NumCoordBins * NumCoordBins; i++)
   {
      stream.read(&mCoordBins[i].binStart);
      stream.read(&mCoordBins[i].binCount);
   }

   // Coord bin indices
   stream.read(&vectorSize);
   mCoordBinIndices.setSize(vectorSize);
   for(i = 0; i < mCoordBinIndices.size(); i++)
      stream.read(&mCoordBinIndices[i]);

   // Coord bin mode
   stream.read(&mCoordBinMode);

   // Ambient colors
   stream.read(&mBaseAmbient);
   stream.read(&mAlarmAmbient);

   if (fileVersion >= 10)
   {
      // Static meshes
      stream.read(&vectorSize);

      mStaticMeshes.setSize(vectorSize);
      for (i = 0; i < mStaticMeshes.size(); i++)
      {
         mStaticMeshes[i] = new InteriorSimpleMesh;
         mStaticMeshes[i]->read(stream);
      }
   }

   if (fileVersion >= 11)
   {
      // Normals
      stream.read(&vectorSize);

      mNormals.setSize(vectorSize);
      for (i = 0; i < mNormals.size(); i++)
         mathRead(stream, &mNormals[i]);

      // TexMatrices
      stream.read(&vectorSize);

      mTexMatrices.setSize(vectorSize);
      for (i = 0; i < mTexMatrices.size(); i++)
      {
         stream.read(&mTexMatrices[i].T);
         stream.read(&mTexMatrices[i].N);
         stream.read(&mTexMatrices[i].B);
      }

      // TexMatIndices
      stream.read(&vectorSize);

      mTexMatIndices.setSize(vectorSize);
      for (i = 0; i < mTexMatIndices.size(); i++)
         stream.read(&mTexMatIndices[i]);
   }

   // For future expandability
   U32 dummy;
   if (fileVersion < 10)
   {
      stream.read(&dummy); if (dummy != 0) return false;
   }
   if (fileVersion < 11)
   {
      stream.read(&dummy); if (dummy != 0) return false;
      stream.read(&dummy); if (dummy != 0) return false;
   }

   //
   // Support for interior light map border sizes.
   //
   U32 extendedlightmapdata;
   stream.read(&extendedlightmapdata);
   if(extendedlightmapdata == 1)
   {
      stream.read(&mLightMapBorderSize);

	  //future expansion under current block (avoid using too
	  //many of the above expansion slots by allowing nested
	  //blocks)...
	  stream.read(&dummy); if (dummy != 0) return false;
   }

   // Setup the zone planes
   setupZonePlanes();
   truncateZoneTree();

   buildSurfaceZones();

   if (stream.getStatus() != Stream::Ok)
      return false;

   // HH: Added for Tribes 2 compatibility -- see checkIndexRanges.
   // After the zone setup above, because truncateZoneTree can remove zones and
   // a bound taken before it would be a bound on a vector this file no longer
   // has.
   return checkIndexRanges();
}

// HH: Added for Tribes 2 compatibility.
//
// readSurface already refuses a surface whose winding, plane, texture or
// texgen index points outside the vector it addresses, live in Release rather
// than as an assertion.  This is the same question asked of the structures it
// does not cover: zones, portals and the lightmap index per surface.
//
// Every bound here is transcribed from an expression this tree evaluates, and
// the file and line of that expression is named beside it.  A bound derived
// from the field's name instead is a second reading of the format, and a check
// that guesses at the format cannot tell a malformed file from a correct one
// it has misunderstood -- which is how a shape check came to refuse every
// human player in the game.
bool Interior::checkIndexRanges() const
{
   // do/while so that it is one statement: without it the macro's own `if`
   // swallows an `else` written after a use, which is not a hypothetical --
   // it silently took the wrong branch of the tagged surface-index check
   // below and reported 152 interiors as malformed.
   #define T2_WANT( cond, fmt, ... )                                           \
      do {                                                                     \
         if( !(cond) )                                                         \
         {                                                                     \
            Con::errorf( ConsoleLogEntry::General,                             \
                         "Error: interior index out of range: " fmt, ##__VA_ARGS__ ); \
            return false;                                                      \
         }                                                                     \
      } while( false )

   const U32 numZones = mZones.size();

   // interior.cpp traverseZone: mPortals[mZonePortalList[i]] for i in
   // [portalStart, portalStart+portalCount), and interior.cpp:1051
   // mZoneSurfaces[zone.surfaceStart + j] for j < surfaceCount.
   //
   // Not checked: planeStart/planeCount and staticMeshStart/staticMeshCount.
   // They look exactly like the two above and nothing in this tree indexes
   // mZonePlanes or mZoneStaticMeshes by them -- the vectors are read, written
   // and never addressed.  A bound on a field nothing uses can only cost a
   // file it was wrong about.
   for( U32 i = 0; i < numZones; i++ )
   {
      const Zone &z = mZones[i];
      T2_WANT( U32(z.portalStart) + z.portalCount <= mZonePortalList.size(),
               "zone %u takes portal entries %u..%u of %u", i, (U32)z.portalStart,
               U32(z.portalStart) + z.portalCount - 1, (U32)mZonePortalList.size() );
      T2_WANT( z.surfaceStart + z.surfaceCount <= mZoneSurfaces.size(),
               "zone %u takes surface entries %u..%u of %u", i, z.surfaceStart,
               z.surfaceStart + z.surfaceCount - 1, (U32)mZoneSurfaces.size() );
   }

   // The entries are themselves indices, which is the half a per-zone range
   // check does not reach.
   for( U32 i = 0; i < mZonePortalList.size(); i++ )
      T2_WANT( mZonePortalList[i] < mPortals.size(),
               "zone portal entry %u names portal %u of %u", i,
               (U32)mZonePortalList[i], (U32)mPortals.size() );

   for( U32 i = 0; i < mZoneSurfaces.size(); i++ )
      T2_WANT( mZoneSurfaces[i] < mSurfaces.size(),
               "zone surface entry %u names surface %u of %u", i,
               (U32)mZoneSurfaces[i], (U32)mSurfaces.size() );

   // Every portal's fans, because interiorDebug.cpp:361 walks all of them and
   // draws each fan's windings without going through a zone.
   for( U32 i = 0; i < mPortals.size(); i++ )
   {
      const Portal &p = mPortals[i];

      T2_WANT( p.triFanStart + p.triFanCount <= mWindingIndices.size(),
               "portal %u takes fans %u..%u of %u", i, p.triFanStart,
               p.triFanStart + p.triFanCount - 1, (U32)mWindingIndices.size() );

      for( U32 j = 0; j < p.triFanCount; j++ )
      {
         const TriFan &fan = mWindingIndices[p.triFanStart + j];
         T2_WANT( fan.windingStart + fan.windingCount <= mWindings.size(),
                  "portal %u fan %u takes windings %u..%u of %u", i,
                  p.triFanStart + j, fan.windingStart,
                  fan.windingStart + fan.windingCount - 1, (U32)mWindings.size() );
      }
   }

   // And the fields that are read only of a portal some zone names.
   //
   // This distinction is the whole of what the first version of this got
   // wrong.  Checking every portal's zoneFront and zoneBack refused nineteen
   // of the game's interiors, every one of them for a portal whose two zones
   // are both 0xFFFF -- U16(-1), a portal attached to nothing.  No zone lists
   // it, so traverseZone never reaches it and the sentinel is never read as an
   // index.  The bound is not "a portal names a zone that exists"; it is "a
   // portal *this interior can reach* names a zone that exists", and the
   // difference is nineteen buildings.
   for( U32 i = 0; i < mZonePortalList.size(); i++ )
   {
      const Portal &p = mPortals[mZonePortalList[i]];

      // interior.cpp:821, through getPlane, which masks the flip bit before
      // indexing -- so the bound is on the masked value and not on the word.
      T2_WANT( U32(p.planeIndex & ~0x8000) < mPlanes.size(),
               "portal %u names plane %u of %u", (U32)mZonePortalList[i],
               U32(p.planeIndex & ~0x8000), (U32)mPlanes.size() );

      // interior.cpp:825 picks whichever of these is not the current zone and
      // traverseZone recurses into it, opening with mZones[currZone].
      T2_WANT( p.zoneFront < numZones && p.zoneBack < numZones,
               "portal %u joins zones %u and %u of %u", (U32)mZonePortalList[i],
               (U32)p.zoneFront, (U32)p.zoneBack, numZones );

      // projectClipAndBoundFan copies a fan's points into Point3F[128] with
      // the file's own count and no guard of any kind -- not an assertion,
      // nothing.  Only the traversal reaches it, which is why this is here and
      // not in the loop over all portals above.  The clipping afterwards can
      // still grow a fan that started inside the bound, which is a property of
      // the algorithm rather than of the file and is not something a reader
      // can answer.
      for( U32 j = 0; j < p.triFanCount; j++ )
         T2_WANT( mWindingIndices[p.triFanStart + j].windingCount <= 128,
                  "portal %u fan %u has %u windings, and the fan buffer holds 128",
                  (U32)mZonePortalList[i], p.triFanStart + j,
                  mWindingIndices[p.triFanStart + j].windingCount );
   }

   // interior.cpp:2064 mNormalLMapIndices[surfaceIndex], with surfaceIndex an
   // index into mSurfaces.
   T2_WANT( mNormalLMapIndices.size() >= mSurfaces.size(),
            "%u lightmap indices for %u surfaces",
            (U32)mNormalLMapIndices.size(), (U32)mSurfaces.size() );

   // And the value, which becomes RenderNode::lightMapIndex -- a U8, compared
   // against U8(-1) for "none" and otherwise handed to
   // InteriorLMManager::getHandle, where it indexes the handle vector behind
   // an assertion.  Checked after the truncation the assignment performs,
   // because that is the number that reaches the subscript.
   //
   // Not checked: mAlarmLMapIndices.  It is read and written and nothing in
   // this tree indexes anything with it.
   for( U32 i = 0; i < mNormalLMapIndices.size(); i++ )
   {
      const U32 index = U8( mNormalLMapIndices[i] );
      T2_WANT( index == U8(-1) || index < mLightmaps.size(),
               "surface %u names lightmap %u of %u", i, index,
               (U32)mLightmaps.size() );
   }

   // The convex hulls, which are the collision side of the same question and
   // the last part of this reader that had not been asked it.  Both sets: the
   // ordinary hulls a player walks into, and the vehicle ones, which are the
   // same structures over a parallel set of vectors and which nothing had ever
   // read until this port read them.
   if( !checkHullRanges( "hull", mConvexHulls, mHullIndices,
                         mHullEmitStringIndices, mHullSurfaceIndices,
                         mHullPlaneIndices, mConvexHullEmitStrings,
                         mPolyListPlanes, mPolyListPoints, mPolyListStrings,
                         mPoints.size(), mPlanes.size() ) )
      return false;

   // interior.cpp:1149 and interiorCollision.cpp:813: an entry with the top
   // bit set is an index into mNullSurfaces once masked, and one without is an
   // index into mSurfaces as it stands.  One field, two vectors, chosen by a
   // bit -- and 152 of the 325 interiors in this game carry at least one
   // tagged entry, so a bound that reads the whole word as a surface index
   // refuses half the buildings in the game.
   for( U32 i = 0; i < mHullSurfaceIndices.size(); i++ )
   {
      const U32 entry = mHullSurfaceIndices[i];
      if( isNullSurfaceIndex( entry ) )
         T2_WANT( getNullSurfaceIndex( entry ) < mNullSurfaces.size(),
                  "hull surface entry %u names null surface %u of %u", i,
                  getNullSurfaceIndex( entry ), (U32)mNullSurfaces.size() );
      else
         T2_WANT( entry < mSurfaces.size(),
                  "hull surface entry %u names surface %u of %u", i, entry,
                  (U32)mSurfaces.size() );
   }

   if( !checkHullRanges( "vehicle hull", mVehicleConvexHulls, mVehicleHullIndices,
                         mVehicleHullEmitStringIndices, mVehicleHullSurfaceIndices,
                         mVehicleHullPlaneIndices, mVehicleConvexHullEmitStrings,
                         mVehiclePolyListPlanes, mVehiclePolyListPoints,
                         mVehiclePolyListStrings,
                         mVehiclePoints.size(), mVehiclePlanes.size() ) )
      return false;

   // interior.cpp:1495 takes every vehicle entry through
   // getVehicleNullSurfaceIndex with no tag test, so there is only one vector
   // to land in and the mask is the two-bit one.
   for( U32 i = 0; i < mVehicleHullSurfaceIndices.size(); i++ )
      T2_WANT( getVehicleNullSurfaceIndex( mVehicleHullSurfaceIndices[i] )
                  < mVehicleNullSurfaces.size(),
               "vehicle hull surface entry %u names null surface %u of %u", i,
               getVehicleNullSurfaceIndex( mVehicleHullSurfaceIndices[i] ),
               (U32)mVehicleNullSurfaces.size() );

   #undef T2_WANT

   return true;
}

// HH: Added for Tribes 2 compatibility.
//
// One function for both hull sets, because interiorCollision.cpp holds the
// two walks side by side over parallel vectors -- writing it twice is how the
// vehicle half would come to be checked differently from the half anyone
// looks at.
//
// The two string formats below are mirrored from the walks that read them at
// runtime, and that is the dangerous part of this: a mirror is a second
// reading of the format, which is exactly what the shape pass was burnt by.
// Mirrored statement by statement from InteriorConvex::getFeatures and
// InteriorConvex::getPolyList rather than from a description of them, and the
// counts and widths are theirs -- the two-byte big-endian point count in the
// poly list, the one-byte everything else.
bool Interior::checkHullRanges( const char *what,
                                const Vector<ConvexHull> &hulls,
                                const Vector<U32> &hullIndices,
                                const Vector<U32> &emitStringIndices,
                                const Vector<U32> &surfaceIndices,
                                const Vector<U16> &planeIndices,
                                const Vector<U8> &emitStrings,
                                const Vector<U16> &polyListPlanes,
                                const Vector<U32> &polyListPoints,
                                const Vector<U8> &polyListStrings,
                                U32 numPoints, U32 numPlanes ) const
{
   // do/while so that it is one statement: without it the macro's own `if`
   // swallows an `else` written after a use, which is not a hypothetical --
   // it silently took the wrong branch of the tagged surface-index check
   // below and reported 152 interiors as malformed.
   #define T2_WANT( cond, fmt, ... )                                           \
      do {                                                                     \
         if( !(cond) )                                                         \
         {                                                                     \
            Con::errorf( ConsoleLogEntry::General,                             \
                         "Error: interior index out of range: " fmt, ##__VA_ARGS__ ); \
            return false;                                                      \
         }                                                                     \
      } while( false )

   // Every entry of these two is an index in its own right, wherever a hull
   // reaches it.
   for( U32 i = 0; i < hullIndices.size(); i++ )
      T2_WANT( hullIndices[i] < numPoints, "%s point entry %u names point %u of %u",
               what, i, hullIndices[i], numPoints );

   // The surface entries are deliberately not checked here.  They are the one
   // thing the two hull sets do not share: an ordinary entry is a tagged
   // union -- top bit set means an index into mNullSurfaces and clear means
   // mSurfaces -- while a vehicle entry always goes through
   // getVehicleNullSurfaceIndex, which masks two bits, with no tag test at
   // all.  Checked at each caller in its own convention, because writing one
   // rule for both is how one of them would come to be checked as the other.

   for( U32 i = 0; i < planeIndices.size(); i++ )
      T2_WANT( U32(planeIndices[i] & ~0x8000) < numPlanes,
               "%s plane entry %u names plane %u of %u", what, i,
               U32(planeIndices[i] & ~0x8000), numPlanes );

   for( U32 i = 0; i < polyListPlanes.size(); i++ )
      T2_WANT( U32(polyListPlanes[i] & ~0x8000) < numPlanes,
               "%s poly-list plane %u names plane %u of %u", what, i,
               U32(polyListPlanes[i] & ~0x8000), numPlanes );

   for( U32 i = 0; i < polyListPoints.size(); i++ )
      T2_WANT( polyListPoints[i] < numPoints,
               "%s poly-list point %u names point %u of %u", what, i,
               polyListPoints[i], numPoints );

   for( U32 h = 0; h < hulls.size(); h++ )
   {
      const ConvexHull &hull = hulls[h];

      // interiorCollision.cpp:1373 walks hullStart..+hullCount, and :1387
      // takes the emit string of the support vertex at the same offset -- so
      // the same range has to fit in both vectors.
      T2_WANT( hull.hullStart + hull.hullCount <= hullIndices.size(),
               "%s %u takes point entries %u..%u of %u", what, h, hull.hullStart,
               hull.hullStart + hull.hullCount - 1, (U32)hullIndices.size() );
      T2_WANT( hull.hullStart + hull.hullCount <= emitStringIndices.size(),
               "%s %u takes emit-string entries %u..%u of %u", what, h,
               hull.hullStart, hull.hullStart + hull.hullCount - 1,
               (U32)emitStringIndices.size() );

      // interior.cpp:1149 and interiorCollision.cpp:813.
      T2_WANT( hull.surfaceStart + hull.surfaceCount <= surfaceIndices.size(),
               "%s %u takes surface entries %u..%u of %u", what, h,
               hull.surfaceStart, hull.surfaceStart + hull.surfaceCount - 1,
               (U32)surfaceIndices.size() );

      // The emit string of every vertex of this hull, walked the way
      // InteriorConvex::getFeatures walks it.
      for( U32 v = 0; v < hull.hullCount; v++ )
      {
         const U32 at = emitStringIndices[hull.hullStart + v];
         T2_WANT( at < emitStrings.size(),
                  "%s %u vertex %u names emit string at %u of %u", what, h, v,
                  at, (U32)emitStrings.size() );

         U32 pos = at;

         // Reading a count is itself a read, so the room for it is checked
         // before it is taken rather than after.
         #define T2_TAKE( n, why )                                             \
            T2_WANT( pos + (n) <= emitStrings.size(),                          \
                     "%s %u vertex %u runs off its emit string at %u of %u (%s)", \
                     what, h, v, pos, (U32)emitStrings.size(), why );           \
            pos += (n);

         T2_TAKE( 1, "point count" )
         const U32 points = emitStrings[pos - 1];
         for( U32 i = 0; i < points; i++ )
         {
            T2_TAKE( 1, "point index" )
            // interiorCollision.cpp:1401 adds this byte to hullStart, so the
            // bound is on the sum and not on the byte.
            T2_WANT( hull.hullStart + emitStrings[pos - 1] < hullIndices.size(),
                     "%s %u vertex %u point %u names entry %u of %u", what, h, v,
                     i, hull.hullStart + emitStrings[pos - 1],
                     (U32)hullIndices.size() );
         }

         T2_TAKE( 1, "edge count" )
         const U32 edges = emitStrings[pos - 1];
         for( U32 i = 0; i < edges; i++ )
         {
            T2_TAKE( 2, "edge" )
            // Both index pRemaps, which holds one entry per point above.
            T2_WANT( emitStrings[pos - 2] < points && emitStrings[pos - 1] < points,
                     "%s %u vertex %u edge %u names points %u and %u of %u", what,
                     h, v, i, (U32)emitStrings[pos - 2], (U32)emitStrings[pos - 1],
                     points );
         }

         T2_TAKE( 1, "poly count" )
         const U32 polys = emitStrings[pos - 1];
         for( U32 i = 0; i < polys; i++ )
         {
            T2_TAKE( 2, "poly header" )
            const U32 vertexCount = emitStrings[pos - 2];
            const U32 planeIndex = emitStrings[pos - 1];

            T2_WANT( hull.planeStart + planeIndex < planeIndices.size(),
                     "%s %u vertex %u poly %u names plane entry %u of %u", what,
                     h, v, i, hull.planeStart + planeIndex,
                     (U32)planeIndices.size() );

            // Two vertices, then one more per triangle of the fan.  A fan of
            // fewer than three is not a polygon and the loop below would run
            // backwards on it.
            T2_WANT( vertexCount >= 3, "%s %u vertex %u poly %u has %u vertices",
                     what, h, v, i, vertexCount );
            T2_TAKE( vertexCount, "poly vertices" )
            for( U32 j = 0; j < vertexCount; j++ )
               T2_WANT( emitStrings[pos - vertexCount + j] < points,
                        "%s %u vertex %u poly %u names point %u of %u", what, h,
                        v, i, (U32)emitStrings[pos - vertexCount + j], points );
         }

         #undef T2_TAKE
      }

      // And the poly list, walked the way InteriorConvex::getPolyList walks it.
      if( polyListStrings.empty() )
         continue;

      T2_WANT( hull.polyListStringStart < polyListStrings.size(),
               "%s %u names a poly-list string at %u of %u", what, h,
               hull.polyListStringStart, (U32)polyListStrings.size() );

      U32 pos = hull.polyListStringStart;

      #define T2_TAKE( n, why )                                                \
         T2_WANT( pos + (n) <= polyListStrings.size(),                         \
                  "%s %u runs off its poly-list string at %u of %u (%s)",       \
                  what, h, pos, (U32)polyListStrings.size(), why );             \
         pos += (n);

      T2_TAKE( 1, "plane count" )
      const U32 planes = polyListStrings[pos - 1];

      // getPolyList returns on a hull with no collision surfaces before it
      // reads anything else, so neither does this.
      if( planes == 0 )
         continue;

      T2_WANT( hull.polyListPlaneStart + planes <= polyListPlanes.size(),
               "%s %u takes poly-list planes %u..%u of %u", what, h,
               hull.polyListPlaneStart, hull.polyListPlaneStart + planes - 1,
               (U32)polyListPlanes.size() );

      T2_TAKE( planes, "plane string" )

      T2_TAKE( 2, "point count" )
      const U32 points = ( U32( polyListStrings[pos - 2] ) << 8 )
                       | polyListStrings[pos - 1];

      T2_WANT( hull.polyListPointStart + points <= polyListPoints.size(),
               "%s %u takes poly-list points %u..%u of %u", what, h,
               hull.polyListPointStart, hull.polyListPointStart + points - 1,
               (U32)polyListPoints.size() );

      T2_TAKE( points, "point string" )

      T2_TAKE( 1, "surface count" )
      const U32 surfaces = polyListStrings[pos - 1];

      for( U32 i = 0; i < surfaces; i++ )
      {
         T2_TAKE( 3, "surface header" )
         const U32 surfPoints = polyListStrings[pos - 3];
         const U32 surfPlane = polyListStrings[pos - 1];

         T2_WANT( surfPlane < planes, "%s %u surface %u names plane %u of %u",
                  what, h, i, surfPlane, planes );

         T2_TAKE( surfPoints * 2, "surface points" )
         for( U32 j = 0; j < surfPoints; j++ )
         {
            const U32 base = pos - surfPoints * 2 + j * 2;
            const U32 index = ( U32( polyListStrings[base] ) << 8 )
                            | polyListStrings[base + 1];
            T2_WANT( index < points, "%s %u surface %u names point %u of %u",
                     what, h, i, index, points );
         }
      }

      #undef T2_TAKE
   }

   #undef T2_WANT

   return true;
}

bool Interior::write(Stream& stream) const
{
   AssertFatal(stream.hasCapability(Stream::StreamWrite), "Interior::write: non-write capable stream passed");
   AssertFatal(stream.getStatus() == Stream::Ok, "Interior::write: Error, stream in inconsistent state");

   U32 i;

   // Version this stream
   stream.write(smFileVersion);

   stream.write(mDetailLevel);
   stream.write(mMinPixels);
   mathWrite(stream, mBoundingBox);
   mathWrite(stream, mBoundingSphere);

   stream.write(mHasAlarmState);
   stream.write(mNumLightStateEntries);

   // Now write out our data vectors.  Remember, for cross-platform capability, no
   //  structure writing is allowed...

   // mPlanes
   writePlaneVector(stream);

   // mPoints
   stream.write(mPoints.size());
   for (i = 0; i < mPoints.size(); i++)
      mathWrite(stream, mPoints[i].point);

   // mPointVisibility
   stream.write(mPointVisibility.size());
   stream.write(mPointVisibility.size(), mPointVisibility.address());

   // mTexGenEQs
   stream.write(mTexGenEQs.size());
   for (i = 0; i < mTexGenEQs.size(); i++)
   {
      mathWrite(stream, mTexGenEQs[i].planeX);
      mathWrite(stream, mTexGenEQs[i].planeY);
   }

   // mBSPNodes;
   stream.write(mBSPNodes.size());
   for (i = 0; i < mBSPNodes.size(); i++)
   {
      stream.write(mBSPNodes[i].planeIndex);

      if (smFileVersion < 14)
      {
         stream.write(U16(mBSPNodes[i].frontIndex));
         stream.write(U16(mBSPNodes[i].backIndex));
      }
      else
      {
         stream.write(mBSPNodes[i].frontIndex);
         stream.write(mBSPNodes[i].backIndex);
      }
   }

   // mBSPSolidLeaves
   stream.write(mBSPSolidLeaves.size());
   for (i = 0; i < mBSPSolidLeaves.size(); i++)
   {
      stream.write(mBSPSolidLeaves[i].surfaceIndex);
      stream.write(mBSPSolidLeaves[i].surfaceCount);
   }

   // MaterialList
   mMaterialList->write(stream);

   // mWindings
   stream.write(mWindings.size());
   for (i = 0; i < mWindings.size(); i++)
   {
      stream.write(mWindings[i]);
   }

   // mWindingIndices
   stream.write(mWindingIndices.size());
   for (i = 0; i < mWindingIndices.size(); i++)
   {
      stream.write(mWindingIndices[i].windingStart);
      stream.write(mWindingIndices[i].windingCount);
   }

   // mEdges
   if (smFileVersion >= 12)
   {
      stream.write(mEdges.size());
      for (i = 0; i < mEdges.size(); i++)
      {
         stream.write(mEdges[i].vertexes[0]);
         stream.write(mEdges[i].vertexes[1]);
         stream.write(mEdges[i].faces[0]);
         stream.write(mEdges[i].faces[1]);
      }
   }

   // mZones
   stream.write(mZones.size());
   for (i = 0; i < mZones.size(); i++)
   {
      stream.write(mZones[i].portalStart);
      stream.write(mZones[i].portalCount);
      stream.write(mZones[i].surfaceStart);
      stream.write(mZones[i].surfaceCount);

      if (smFileVersion >= 12)
      {
         stream.write(mZones[i].staticMeshStart);
         stream.write(mZones[i].staticMeshCount);
      }

      stream.write(mZones[i].flags);
   }

   // Zone surfaces
   stream.write(mZoneSurfaces.size());
   for (i = 0; i < mZoneSurfaces.size(); i++)
      stream.write(mZoneSurfaces[i]);

   // Zone static meshes
   if (smFileVersion >= 12)
   {
      stream.write(mZoneStaticMeshes.size());
      for (i = 0; i < mZoneStaticMeshes.size(); i++)
         stream.write(mZoneStaticMeshes[i]);
   }

   //  mZonePortalList;
   stream.write(mZonePortalList.size());
   for (i = 0; i < mZonePortalList.size(); i++)
      stream.write(mZonePortalList[i]);

   // mPortals
   stream.write(mPortals.size());
   for (i = 0; i < mPortals.size(); i++)
   {
      stream.write(mPortals[i].planeIndex);
      stream.write(mPortals[i].triFanCount);
      stream.write(mPortals[i].triFanStart);
      stream.write(mPortals[i].zoneFront);
      stream.write(mPortals[i].zoneBack);
   }

   // mSurfaces
   stream.write(mSurfaces.size());
   for (i = 0; i < mSurfaces.size(); i++)
   {
      stream.write(mSurfaces[i].windingStart);
      
      if (smFileVersion >= 13)
         stream.write(mSurfaces[i].windingCount);
      else
      {
         U8 count = (U8)mSurfaces[i].windingCount;
         stream.write(count);
      }

      stream.write(mSurfaces[i].planeIndex);
      stream.write(mSurfaces[i].textureIndex);
      stream.write(mSurfaces[i].texGenIndex);
      stream.write(mSurfaces[i].surfaceFlags);
      stream.write(mSurfaces[i].fanMask);
      writeLMapTexGen(stream, mLMTexGenEQs[i].planeX, mLMTexGenEQs[i].planeY);

      stream.write(mSurfaces[i].lightCount);
      stream.write(mSurfaces[i].lightStateInfoStart);

      if (smFileVersion >= 13)
      {
         stream.write(mSurfaces[i].mapOffsetX);
         stream.write(mSurfaces[i].mapOffsetY);
         stream.write(mSurfaces[i].mapSizeX);
         stream.write(mSurfaces[i].mapSizeY);
      }
      else
      {
         U8 offX, offY, sizeX, sizeY;
         offX = (U8)mSurfaces[i].mapOffsetX;
         offY = (U8)mSurfaces[i].mapOffsetY;
         sizeX = (U8)mSurfaces[i].mapSizeX;
         sizeY = (U8)mSurfaces[i].mapSizeY;

         stream.write(offX);
         stream.write(offY);
         stream.write(sizeX);
         stream.write(sizeY);
      }

      if (smFileVersion == 0 || smFileVersion >= 12)
         stream.write(mSurfaces[i].unused);
   }
   // NormalLMapIndices
   stream.write(mNormalLMapIndices.size());
   for (U32 i = 0; i < mNormalLMapIndices.size(); i++)
   {
      if (smFileVersion >= 13)
         stream.write(mNormalLMapIndices[i]);
      else
      {
         U8 index = (U8)mNormalLMapIndices[i];
         stream.write(index);
      }
   }

   // AlarmLMapIndices
   stream.write(mAlarmLMapIndices.size());
   for (U32 i = 0; i < mAlarmLMapIndices.size(); i++)
   {
      if (smFileVersion >= 13)
         stream.write(mAlarmLMapIndices[i]);
      else
      {
         U8 index = (U8)mAlarmLMapIndices[i];
         stream.write(index);
      }
   }


   // mNullSurfaces
   stream.write(mNullSurfaces.size());
   for(i = 0; i < mNullSurfaces.size(); i++)
   {
      stream.write(mNullSurfaces[i].windingStart);
      stream.write(mNullSurfaces[i].planeIndex);
      stream.write(mNullSurfaces[i].surfaceFlags);

      if (smFileVersion >= 13)
         stream.write(mNullSurfaces[i].windingCount);
      else
      {
         U8 count = (U8)mNullSurfaces[i].windingCount;
         stream.write(count);
      }
   }

   // mLightmaps
   stream.write(mLightmaps.size());
   for(i = 0; i < mLightmaps.size(); i++)
   {
      mLightmaps[i]->writeBitmap("png",stream);
      
      if (smFileVersion == 0 || smFileVersion >= 12)
      {
         // The "light normal maps" or "light direction maps" were
         // removed from Torque 3D... this just writes a dummy 2x2
         // texture so that the read/write functions don't change.
         GBitmap dummyBmp( 2, 2 );
         dummyBmp.writeBitmap("png",stream);
      }

      stream.write(mLightmapKeep[i]);
   }

   // mSolidLeafSurfaces
   stream.write(mSolidLeafSurfaces.size());
   for(i = 0; i < mSolidLeafSurfaces.size(); i++)
   {
      stream.write(mSolidLeafSurfaces[i]);
   }


   // Animated lights
   stream.write(mAnimatedLights.size());
   for(i = 0; i < mAnimatedLights.size(); i++)
   {
      stream.write(mAnimatedLights[i].nameIndex);
      stream.write(mAnimatedLights[i].stateIndex);
      stream.write(mAnimatedLights[i].stateCount);
      stream.write(mAnimatedLights[i].flags);
      stream.write(mAnimatedLights[i].duration);
   }

   stream.write(mLightStates.size());
   for(i = 0; i < mLightStates.size(); i++)
   {
      stream.write(mLightStates[i].red);
      stream.write(mLightStates[i].green);
      stream.write(mLightStates[i].blue);
      stream.write(mLightStates[i].activeTime);
      stream.write(mLightStates[i].dataIndex);
      stream.write(mLightStates[i].dataCount);
   }

   // mStateData
   stream.write(mStateData.size());
   for(i = 0; i < mStateData.size(); i++)
   {
      stream.write(mStateData[i].surfaceIndex);
      stream.write(mStateData[i].mapIndex);
      stream.write(mStateData[i].lightStateIndex);
   }

   // mStateDataBuffer: Note: superfluous 0 is for flags in future versions.
   //                    that may add compression.  This way, we can maintain
   //                    compatability with previous versions.
   stream.write(mStateDataBuffer.size());
   stream.write(U32(0));
   stream.write(mStateDataBuffer.size(), mStateDataBuffer.address());

   // mNameBuffer
   stream.write(mNameBuffer.size());
   stream.write(mNameBuffer.size(), mNameBuffer.address());

   // mSubObjects
   stream.write(mSubObjects.size());
   for (i = 0; i < mSubObjects.size(); i++)
   {
      bool writeSuccess = mSubObjects[i]->writeISO(stream);
      AssertFatal(writeSuccess == true, "Error writing sub object to stream!");
   }

   // Convex hulls
   stream.write(mConvexHulls.size());
   for(i = 0; i < mConvexHulls.size(); i++)
   {
      stream.write(mConvexHulls[i].hullStart);
      stream.write(mConvexHulls[i].hullCount);
      stream.write(mConvexHulls[i].minX);
      stream.write(mConvexHulls[i].maxX);
      stream.write(mConvexHulls[i].minY);
      stream.write(mConvexHulls[i].maxY);
      stream.write(mConvexHulls[i].minZ);
      stream.write(mConvexHulls[i].maxZ);
      stream.write(mConvexHulls[i].surfaceStart);
      stream.write(mConvexHulls[i].surfaceCount);
      stream.write(mConvexHulls[i].planeStart);
      stream.write(mConvexHulls[i].polyListPlaneStart);
      stream.write(mConvexHulls[i].polyListPointStart);
      stream.write(mConvexHulls[i].polyListStringStart);

      if (smFileVersion >= 12)
         stream.write(mConvexHulls[i].staticMesh);
   }

   stream.write(mConvexHullEmitStrings.size());
   stream.write(mConvexHullEmitStrings.size(), mConvexHullEmitStrings.address());

   stream.write(mHullIndices.size());
   for(i = 0; i < mHullIndices.size(); i++)
      stream.write(mHullIndices[i]);

   stream.write(mHullPlaneIndices.size());
   for(i = 0; i < mHullPlaneIndices.size(); i++)
      stream.write(mHullPlaneIndices[i]);

   stream.write(mHullEmitStringIndices.size());
   for(i = 0; i < mHullEmitStringIndices.size(); i++)
      stream.write(mHullEmitStringIndices[i]);

   stream.write(mHullSurfaceIndices.size());
   for(i = 0; i < mHullSurfaceIndices.size(); i++)
      stream.write(mHullSurfaceIndices[i]);

   stream.write(mPolyListPlanes.size());
   for(i = 0; i < mPolyListPlanes.size(); i++)
      stream.write(mPolyListPlanes[i]);

   stream.write(mPolyListPoints.size());
   for(i = 0; i < mPolyListPoints.size(); i++)
      stream.write(mPolyListPoints[i]);

   stream.write(mPolyListStrings.size());
   for(i = 0; i < mPolyListStrings.size(); i++)
      stream.write(mPolyListStrings[i]);

   // Coord bins
   for(i = 0; i < NumCoordBins * NumCoordBins; i++)
   {
      stream.write(mCoordBins[i].binStart);
      stream.write(mCoordBins[i].binCount);
   }
   stream.write(mCoordBinIndices.size());
   for(i = 0; i < mCoordBinIndices.size(); i++)
      stream.write(mCoordBinIndices[i]);
   stream.write(mCoordBinMode);

   // Ambient colors...
   stream.write(mBaseAmbient);
   stream.write(mAlarmAmbient);

   if (smFileVersion >= 10)
   {
      // Static meshes
      stream.write(mStaticMeshes.size());
      for (i = 0; i < mStaticMeshes.size(); i++)
         mStaticMeshes[i]->write(stream);
   }
   else
      stream.write(U32(0));

   if (smFileVersion >= 11)
   {
      // Normals
      stream.write(mNormals.size());
      for (i = 0; i < mNormals.size(); i++)
         mathWrite(stream, mNormals[i]);

      // TexMatrices
      stream.write(mTexMatrices.size());
      for (i = 0; i < mTexMatrices.size(); i++)
      {
         stream.write(mTexMatrices[i].T);
         stream.write(mTexMatrices[i].N);
         stream.write(mTexMatrices[i].B);
      }

      // TexMatIndices
      stream.write(mTexMatIndices.size());
      for (i = 0; i < mTexMatIndices.size(); i++)
         stream.write(mTexMatIndices[i]);
   }
   else
   {
      stream.write(U32(0));
      stream.write(U32(0));
   }

   //
   // Support for interior light map border sizes.
   //
   if(mLightMapBorderSize > 0)
   {
      stream.write(U32(1));//flag new block...
      stream.write(U32(mLightMapBorderSize));//new block data..

	  //future expansion under current block (avoid using too
	  //many of the above expansion slots by allowing nested
	  //blocks)...
      stream.write(U32(0));
   }
   else
   {
      stream.write(U32(0));
   }

   return stream.getStatus() == Stream::Ok;
}

bool Interior::readVehicleCollision(Stream& stream)
{
   AssertFatal(stream.hasCapability(Stream::StreamRead), "Interior::read: non-read capable stream passed");
   AssertFatal(stream.getStatus() == Stream::Ok, "Interior::read: Error, stream in inconsistent state");

   S32 i;

   // Version this stream.  We only load stream of the current version
   U32 fileVersion;
   stream.read(&fileVersion);
   if (fileVersion > smFileVersion)
   {
      Con::errorf(ConsoleLogEntry::General, "Interior::read: incompatible file version found.");
      return false;
   }

   U32 vectorSize;

   // Convex hulls
   stream.read(&vectorSize);
   mVehicleConvexHulls.setSize(vectorSize);
   for(i = 0; i < mVehicleConvexHulls.size(); i++)
   {
      stream.read(&mVehicleConvexHulls[i].hullStart);
      stream.read(&mVehicleConvexHulls[i].hullCount);
      stream.read(&mVehicleConvexHulls[i].minX);
      stream.read(&mVehicleConvexHulls[i].maxX);
      stream.read(&mVehicleConvexHulls[i].minY);
      stream.read(&mVehicleConvexHulls[i].maxY);
      stream.read(&mVehicleConvexHulls[i].minZ);
      stream.read(&mVehicleConvexHulls[i].maxZ);
      stream.read(&mVehicleConvexHulls[i].surfaceStart);
      stream.read(&mVehicleConvexHulls[i].surfaceCount);
      stream.read(&mVehicleConvexHulls[i].planeStart);
      stream.read(&mVehicleConvexHulls[i].polyListPlaneStart);
      stream.read(&mVehicleConvexHulls[i].polyListPointStart);
      stream.read(&mVehicleConvexHulls[i].polyListStringStart);
   }

   stream.read(&vectorSize);
   mVehicleConvexHullEmitStrings.setSize(vectorSize);
   stream.read(mVehicleConvexHullEmitStrings.size(), mVehicleConvexHullEmitStrings.address());

   stream.read(&vectorSize);
   mVehicleHullIndices.setSize(vectorSize);
   for(i = 0; i < mVehicleHullIndices.size(); i++)
      stream.read(&mVehicleHullIndices[i]);

   stream.read(&vectorSize);
   mVehicleHullPlaneIndices.setSize(vectorSize);
   for(i = 0; i < mVehicleHullPlaneIndices.size(); i++)
      stream.read(&mVehicleHullPlaneIndices[i]);

   stream.read(&vectorSize);
   mVehicleHullEmitStringIndices.setSize(vectorSize);
   for(i = 0; i < mVehicleHullEmitStringIndices.size(); i++)
      stream.read(&mVehicleHullEmitStringIndices[i]);

   stream.read(&vectorSize);
   mVehicleHullSurfaceIndices.setSize(vectorSize);
   for(i = 0; i < mVehicleHullSurfaceIndices.size(); i++)
      stream.read(&mVehicleHullSurfaceIndices[i]);

   stream.read(&vectorSize);
   mVehiclePolyListPlanes.setSize(vectorSize);
   for(i = 0; i < mVehiclePolyListPlanes.size(); i++)
      stream.read(&mVehiclePolyListPlanes[i]);

   stream.read(&vectorSize);
   mVehiclePolyListPoints.setSize(vectorSize);
   for(i = 0; i < mVehiclePolyListPoints.size(); i++)
      stream.read(&mVehiclePolyListPoints[i]);

   stream.read(&vectorSize);
   mVehiclePolyListStrings.setSize(vectorSize);
   for(i = 0; i < mVehiclePolyListStrings.size(); i++)
      stream.read(&mVehiclePolyListStrings[i]);

   stream.read(&vectorSize);
   mVehicleNullSurfaces.setSize(vectorSize);
   for(i = 0; i < mVehicleNullSurfaces.size(); i++)
   {
      stream.read(&mVehicleNullSurfaces[i].windingStart);
      stream.read(&mVehicleNullSurfaces[i].planeIndex);
      stream.read(&mVehicleNullSurfaces[i].surfaceFlags);

      // HH: Modified for Tribes 2 compatibility.
      // The same width rule Interior::read already applies to the interior's
      // own null surfaces: before version 13 a winding count is a byte.  This
      // function never had it, so it took three bytes too many per vehicle
      // null surface and lost the rest of the block.
      if (fileVersion >= 13)
         stream.read(&mVehicleNullSurfaces[i].windingCount);
      else
      {
         U8 count;
         stream.read(&count);
         mVehicleNullSurfaces[i].windingCount = (U32)count;
      }
   }

   stream.read(&vectorSize);
   mVehiclePoints.setSize(vectorSize);
   for(i = 0; i < mVehiclePoints.size(); i++)
      mathRead(stream, &mVehiclePoints[i].point);

   stream.read(&vectorSize);
   mVehiclePlanes.setSize(vectorSize);
   for(i = 0; i < mVehiclePlanes.size(); i++)
      mathRead(stream, &mVehiclePlanes[i]);

   stream.read(&vectorSize);
   mVehicleWindings.setSize(vectorSize);
   for(i = 0; i < mVehicleWindings.size(); i++)
   {
      stream.read(&mVehicleWindings[i]);
   }

   stream.read(&vectorSize);
   mVehicleWindingIndices.setSize(vectorSize);
   for(i = 0; i < mVehicleWindingIndices.size(); i++)
   {
      stream.read(&mVehicleWindingIndices[i].windingStart);
      stream.read(&mVehicleWindingIndices[i].windingCount);
   }

   return true;
}

bool Interior::writeVehicleCollision(Stream& stream) const
{
   AssertFatal(stream.hasCapability(Stream::StreamWrite), "Interior::write: non-write capable stream passed");
   AssertFatal(stream.getStatus() == Stream::Ok, "Interior::write: Error, stream in inconsistent state");

   U32 i;

   // Version this stream
   stream.write(smFileVersion);

   // Convex hulls
   stream.write(mVehicleConvexHulls.size());
   for(i = 0; i < mVehicleConvexHulls.size(); i++)
   {
      stream.write(mVehicleConvexHulls[i].hullStart);
      stream.write(mVehicleConvexHulls[i].hullCount);
      stream.write(mVehicleConvexHulls[i].minX);
      stream.write(mVehicleConvexHulls[i].maxX);
      stream.write(mVehicleConvexHulls[i].minY);
      stream.write(mVehicleConvexHulls[i].maxY);
      stream.write(mVehicleConvexHulls[i].minZ);
      stream.write(mVehicleConvexHulls[i].maxZ);
      stream.write(mVehicleConvexHulls[i].surfaceStart);
      stream.write(mVehicleConvexHulls[i].surfaceCount);
      stream.write(mVehicleConvexHulls[i].planeStart);
      stream.write(mVehicleConvexHulls[i].polyListPlaneStart);
      stream.write(mVehicleConvexHulls[i].polyListPointStart);
      stream.write(mVehicleConvexHulls[i].polyListStringStart);
   }

   stream.write(mVehicleConvexHullEmitStrings.size());
   stream.write(mVehicleConvexHullEmitStrings.size(), mVehicleConvexHullEmitStrings.address());

   stream.write(mVehicleHullIndices.size());
   for(i = 0; i < mVehicleHullIndices.size(); i++)
      stream.write(mVehicleHullIndices[i]);

   stream.write(mVehicleHullPlaneIndices.size());
   for(i = 0; i < mVehicleHullPlaneIndices.size(); i++)
      stream.write(mVehicleHullPlaneIndices[i]);

   stream.write(mVehicleHullEmitStringIndices.size());
   for(i = 0; i < mVehicleHullEmitStringIndices.size(); i++)
      stream.write(mVehicleHullEmitStringIndices[i]);

   stream.write(mVehicleHullSurfaceIndices.size());
   for(i = 0; i < mVehicleHullSurfaceIndices.size(); i++)
      stream.write(mVehicleHullSurfaceIndices[i]);

   stream.write(mVehiclePolyListPlanes.size());
   for(i = 0; i < mVehiclePolyListPlanes.size(); i++)
      stream.write(mVehiclePolyListPlanes[i]);

   stream.write(mVehiclePolyListPoints.size());
   for(i = 0; i < mVehiclePolyListPoints.size(); i++)
      stream.write(mVehiclePolyListPoints[i]);

   stream.write(mVehiclePolyListStrings.size());
   for(i = 0; i < mVehiclePolyListStrings.size(); i++)
      stream.write(mVehiclePolyListStrings[i]);

   stream.write(mVehicleNullSurfaces.size());
   for(i = 0; i < mVehicleNullSurfaces.size(); i++)
   {
      stream.write(mVehicleNullSurfaces[i].windingStart);
      stream.write(mVehicleNullSurfaces[i].planeIndex);
      stream.write(mVehicleNullSurfaces[i].surfaceFlags);
      stream.write(mVehicleNullSurfaces[i].windingCount);
   }

   stream.write(mVehiclePoints.size());
   for(i = 0; i < mVehiclePoints.size(); i++)
      mathWrite(stream, mVehiclePoints[i].point);

   stream.write(mVehiclePlanes.size());
   for(i = 0; i < mVehiclePlanes.size(); i++)
      mathWrite(stream, mVehiclePlanes[i]);

   stream.write(mVehicleWindings.size());
   for(i = 0; i < mVehicleWindings.size(); i++)
      stream.write(mVehicleWindings[i]);

   stream.write(mVehicleWindingIndices.size());
   for(i = 0; i < mVehicleWindingIndices.size(); i++)
   {
      stream.write(mVehicleWindingIndices[i].windingStart);
      stream.write(mVehicleWindingIndices[i].windingCount);
   }

   return true;
}

bool Interior::readSurface(Stream& stream, Surface& surface, TexGenPlanes& texgens, const bool tgeInterior)
{
   // HH: The checks through this function were a variant test and are now a
   // corruption test.  They were how the caller guessed whether a version 0
   // file was TGE's or TGEA's, which they were never able to answer -- an index
   // can be in range and still be the wrong index.  The caller decides by
   // version now, so what these do is refuse a file whose surfaces point
   // outside the vectors they index, which is worth keeping and is all they
   // ever reliably did.
   stream.read(&surface.windingStart);

   if (surface.windingStart >= mWindings.size())
      return false;

   if (mFileVersion >= 13)
      stream.read(&surface.windingCount);
   else
   {
      U8 count;
      stream.read(&count);
      surface.windingCount = (U32)count;
   }

   if (surface.windingStart + surface.windingCount > mWindings.size())
      return false;

   stream.read(&surface.planeIndex);

   if (U32(surface.planeIndex & ~0x8000) >= mPlanes.size())
      return false;

   stream.read(&surface.textureIndex);

   if (surface.textureIndex >= mMaterialList->size())
      return false;

   stream.read(&surface.texGenIndex);

   if (surface.texGenIndex >= mTexGenEQs.size())
      return false;

   stream.read(&surface.surfaceFlags);
   stream.read(&surface.fanMask);

   // If reading the lightmap texgen fails then most likely this is a
   // TGE version 0 Interior (it gets offset by the "unused" read below
   if (readLMapTexGen(stream, texgens.planeX, texgens.planeY) == false)
      return false;

   stream.read(&surface.lightCount);
   stream.read(&surface.lightStateInfoStart);

   if (mFileVersion >= 13)
   {
      stream.read(&surface.mapOffsetX);
      stream.read(&surface.mapOffsetY);
      stream.read(&surface.mapSizeX);
      stream.read(&surface.mapSizeY);
   }
   else
   {
      U8 offX, offY, sizeX, sizeY;
      stream.read(&offX);
      stream.read(&offY);
      stream.read(&sizeX);
      stream.read(&sizeY);

      surface.mapOffsetX = (U32)offX;
      surface.mapOffsetY = (U32)offY;
      surface.mapSizeX = (U32)sizeX;
      surface.mapSizeY = (U32)sizeY;
   }

   if (!tgeInterior && (mFileVersion == 0 || mFileVersion == 5 || mFileVersion >= 12))
      stream.read(&surface.unused);

   if (mFileVersion == 5)
   {
      U32 brushId;
      stream.read(&brushId);
   }

   return true;
}

bool Interior::readLMapTexGen(Stream& stream, PlaneF& planeX, PlaneF& planeY)
{
   F32 genX[4];
   F32 genY[4];

   for(U32 i = 0; i < 4; i++)
   {
      genX[i] = 0.0f;
      genY[i] = 0.0f;
   }

   U16 finalWord;
   stream.read(&finalWord);
   stream.read(&genX[3]);
   stream.read(&genY[3]);

   // Unpack the final word.
   U32 logScaleY = (finalWord >> 0) & ((1 << 6) - 1);
   U32 logScaleX = (finalWord >> 6) & ((1 << 6) - 1);
   U16 stEnc     = (finalWord >> 13) & 7;

   S32 sc, tc;
   switch(stEnc)
   {
   case 0: sc = 0; tc = 1; break;
   case 1: sc = 0; tc = 2; break;
   case 2: sc = 1; tc = 0; break;
   case 3: sc = 1; tc = 2; break;
   case 4: sc = 2; tc = 0; break;
   case 5: sc = 2; tc = 1; break;

   default:
      sc = tc = -1;
      // This is potentially an invalid st coord encoding however *most* times
      // this is caused by attempting to load a TGE version 0 Interior
      return false;
   }

   U32 invScaleX = 1 << logScaleX;
   U32 invScaleY = 1 << logScaleY;

   genX[sc] = F32(1.0 / F64(invScaleX));
   genY[tc] = F32(1.0 / F64(invScaleY));

   planeX.x = genX[0];
   planeX.y = genX[1];
   planeX.z = genX[2];
   planeX.d = genX[3];
   planeY.x = genY[0];
   planeY.y = genY[1];
   planeY.z = genY[2];
   planeY.d = genY[3];

   return stream.getStatus() == Stream::Ok;
}

bool Interior::writeLMapTexGen(Stream& stream, const PlaneF& planeX, const PlaneF& planeY) const
{
   F32 genX[4], genY[4];

   genX[0] = planeX.x;
   genX[1] = planeX.y;
   genX[2] = planeX.z;
   genX[3] = planeX.d;
   genY[0] = planeY.x;
   genY[1] = planeY.y;
   genY[2] = planeY.z;
   genY[3] = planeY.d;

   // The tex gen for lmaps is a special case.
   //  there are only 4 parameters that matter,
   //  an inverse power of 2 in the x and y, and the
   //  fp offsets in x and y.  We can encode the
   //  scales completely in U16 and we'll just write out
   //  the offsets.  First, determine which coords we're
   //  writing...
   //
   S32 sc = -1;
   S32 tc = -1;
   if(genX[0] != 0.0)      sc = 0;
   else if(genX[1] != 0.0) sc = 1;
   else if(genX[2] != 0.0) sc = 2;

   if(genY[0] != 0.0)      tc = 0;
   else if(genY[1] != 0.0) tc = 1;
   else if(genY[2] != 0.0) tc = 2;
   AssertFatal(sc != -1 && tc != -1 && sc != tc, "Hm, something wrong here.");

   U32 invScaleX = U32((1.0f / genX[sc]) + 0.5);
   U32 invScaleY = U32((1.0f / genY[tc]) + 0.5);
   AssertISV(invScaleX && isPow2(invScaleX) && invScaleY && isPow2(invScaleY), "Not a power of 2?  Something wrong");

   U32 logScaleX = getBinLog2(invScaleX);
   U32 logScaleY = getBinLog2(invScaleY);
   AssertFatal(logScaleX < 63 && logScaleY < 63, "Error, you've set the lightmap scale WAAYYY to high!");

   // We need 3 bits to encode sc and tc, which leaves us 6 bits for logScaleX
   //  and logScaleY
   S16 stEnc = -1;
   if(sc == 0 && tc == 1) stEnc = 0;
   else if(sc == 0 && tc == 2) stEnc = 1;
   else if(sc == 1 && tc == 0) stEnc = 2;
   else if(sc == 1 && tc == 2) stEnc = 3;
   else if(sc == 2 && tc == 0) stEnc = 4;
   else if(sc == 2 && tc == 1) stEnc = 5;
   AssertFatal(stEnc != -1, avar("Hm.  This should never happen. (%d, %d)", sc, tc));

   U16 finalWord = U16(stEnc) << 13;
   finalWord       |= logScaleX << 6;
   finalWord       |= logScaleY << 0;

   stream.write(finalWord);
   stream.write(genX[3]);
   stream.write(genY[3]);

   return stream.getStatus() == Stream::Ok;
}

bool Interior::writePlaneVector(Stream& stream) const
{
   // This is pretty slow, but who cares?
   //
   Vector<Point3F> uniqueNormals(mPlanes.size());
   Vector<U16>  uniqueIndices(mPlanes.size());

   U32 i;

   for(i = 0; i < mPlanes.size(); i++)
   {
      bool inserted = false;
      for(U32 j = 0; j < uniqueNormals.size(); j++)
      {
         if(mPlanes[i] == uniqueNormals[j])
         {
            // Hah!  Already have this one...
            uniqueIndices.push_back(j);
            inserted = true;
            break;
         }
      }

      if(inserted == false)
      {
         // Gotta do it ourselves...
         uniqueIndices.push_back(uniqueNormals.size());
         uniqueNormals.push_back(Point3F(mPlanes[i].x, mPlanes[i].y, mPlanes[i].z));
      }
   }

   // Ok, what we have now, is a list of unique normals, a set of indices into
   //  that vector, and the distances that we still have to write out by hand.
   //  Hop to it!
   stream.write(uniqueNormals.size());
   for(i = 0; i < uniqueNormals.size(); i++)
      mathWrite(stream, uniqueNormals[i]);

   stream.write(mPlanes.size());
   for(i = 0; i < mPlanes.size(); i++)
   {
      stream.write(uniqueIndices[i]);
      stream.write(mPlanes[i].d);
   }

   return(stream.getStatus() == Stream::Ok);
}

bool Interior::readPlaneVector(Stream& stream)
{
   U32 vectorSize;

   stream.read(&vectorSize);
   Point3F* normals = new Point3F[vectorSize];
   U32 i;
   for(i = 0; i < vectorSize; i++)
      mathRead(stream, &normals[i]);

   U16 index;
   stream.read(&vectorSize);
   mPlanes.setSize(vectorSize);
   for(i = 0; i < mPlanes.size(); i++)
   {
      stream.read(&index);
      stream.read(&mPlanes[i].d);
      mPlanes[i].x = normals[index].x;
      mPlanes[i].y = normals[index].y;
      mPlanes[i].z = normals[index].z;
   }

   delete [] normals;

   return(stream.getStatus() == Stream::Ok);
}


bool Interior::getUnifiedZone(const U32 index, S32* zone)
{
   if(isBSPLeafIndex(index))
   {
      if(isBSPSolidLeaf(index))
         *zone = -1;
      else
         *zone = S32(getBSPEmptyLeafZone(index));
      return true;
   }
   else
   {
      S32 frontZone, backZone;
      bool frontUnified = getUnifiedZone(mBSPNodes[index].frontIndex, &frontZone);
      bool backUnified  = getUnifiedZone(mBSPNodes[index].backIndex, &backZone);
      if(frontUnified && backUnified)
      {
         if(frontZone == backZone)
         {
            *zone = frontZone;
            return true;
         }
         else
         {
            if(frontZone == -1 || backZone == -1)
            {
               // DMMFIX: Once the interior file format is able to distinguish
               //  between structural and detail nodes in the runtime bsp,
               //  we can make this work a little better.
               return false;
            }
            else
            {
               // Not equal, and neither is -1, no unified zone possible
               return false;
            }
         }
      }
      else
      {
         return false;
      }
   }
}

void Interior::truncateZoneNode(const U32 index)
{
   S32 unifiedZone;
   bool unified = getUnifiedZone(index, &unifiedZone);
   if(unified)
   {
      // Aha!
      if(isBSPLeafIndex(index))
         return;

      if(unifiedZone == -1)
         mBSPNodes[index].terminalZone = U16(0xFFFF);
      else
         mBSPNodes[index].terminalZone = U16(0x8000) | U16(unifiedZone);
   }
   else
   {
      // Sigh.
      if(isBSPLeafIndex(mBSPNodes[index].frontIndex) == false)
         truncateZoneNode(mBSPNodes[index].frontIndex);
      if(isBSPLeafIndex(mBSPNodes[index].backIndex) == false)
         truncateZoneNode(mBSPNodes[index].backIndex);
   }
}

void Interior::truncateZoneTree()
{
   for(U32 i = 0; i < mBSPNodes.size(); i++)
   {
      mBSPNodes[i].terminalZone = 0;
   }

   if(mBSPNodes.size() > 0)
      truncateZoneNode(0);
}


void Interior::setupZonePlanes()
{
   U16* temp = new U16[mPlanes.size() * mZones.size()];
   U32  tempSize = 0;

   for(U32 i = 0; i < mZones.size(); i++)
   {
      Zone& rZone = mZones[i];

      BitVector usedPlanes;
      usedPlanes.setSize(mPlanes.size());
      usedPlanes.clear();

      U32 j;
      for(j = 0; j < rZone.surfaceCount; j++)
      {
         Surface& rSurface = mSurfaces[mZoneSurfaces[rZone.surfaceStart + j]];
         usedPlanes.set(getPlaneIndex(rSurface.planeIndex));
      }

      rZone.planeStart = tempSize;
      for(j = 0; j < mPlanes.size(); j++)
      {
         if(usedPlanes.test(j))
         {
            AssertFatal(tempSize < mPlanes.size() * mZones.size(), "Error, out of bounds plane list!");
            temp[tempSize++] = j;
         }
      }
      rZone.planeCount = tempSize - rZone.planeStart;
   }

   mZonePlanes.setSize(tempSize);
   for(U32 j = 0; j < tempSize; j++)
      mZonePlanes[j] = temp[j];

   delete [] temp;
}
