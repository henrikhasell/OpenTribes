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
#include "lighting/basic/blTerrainSystem.h"

#include "core/bitVector.h"
#include "lighting/common/shadowVolumeBSP.h"
#include "lighting/lightingInterfaces.h"
#include "terrain/terrData.h"
#include "lighting/basic/basicLightManager.h"
#include "lighting/common/sceneLighting.h"
#include "gfx/bitmap/gBitmap.h"
#include "collision/collision.h"

extern SceneLighting* gLighting;


struct blTerrainChunk : public PersistInfo::PersistChunk
{
   typedef PersistInfo::PersistChunk Parent;

   blTerrainChunk();
   ~blTerrainChunk();

   GBitmap *mLightmap;

   bool read(Stream &);
   bool write(Stream &);
};

//------------------------------------------------------------------------------
// Class SceneLighting::TerrainChunk
//------------------------------------------------------------------------------
blTerrainChunk::blTerrainChunk()
{
   mChunkType = PersistChunk::TerrainChunkType;
   mLightmap = NULL;
}

blTerrainChunk::~blTerrainChunk()
{
   if(mLightmap)
      delete mLightmap;
}

//------------------------------------------------------------------------------

bool blTerrainChunk::read(Stream & stream)
{
   if(!Parent::read(stream))
      return(false);

   mLightmap = new GBitmap();
   return mLightmap->readBitmap("png",stream);
}

bool blTerrainChunk::write(Stream & stream)
{
   if(!Parent::write(stream))
      return(false);

   if(!mLightmap)
      return(false);

   if(!mLightmap->writeBitmap("png",stream))
      return(false);

   return(true);
}

class blTerrainProxy : public SceneLighting::ObjectProxy
{
protected:

   typedef ObjectProxy Parent;

   BitVector               mShadowMask;
   ShadowVolumeBSP *       mShadowVolume;
   ColorF *                mLightmap;

   /// The dimension of the lightmap in pixels.
   const U32 mLightMapSize;

   /// The dimension of the terrain height map sample array.
   const U32 mTerrainBlockSize;

   // HH: Added for Tribes 2 compatibility.
   /// Samples to a lexel per side, as a power of two.
   U32 mGenerateLevel;

   ColorF *sgBakedLightmap;
   Vector<LightInfo *> sgLights;
   bool sgMarkStaticShadow(void *terrainproxy, SceneObject *sceneobject, LightInfo *light);
   //void postLight(bool lastLight);

   void lightVector(LightInfo *);

   struct SquareStackNode
   {
      U8          mLevel;
      U16         mClipFlags;
      Point2I     mPos;
   };

   S32 testSquare(const Point3F &, const Point3F &, S32, F32, const Vector<PlaneF> &);
   bool markObjectShadow(ObjectProxy *);
   bool sgIsCorrectStaticObjectType(SceneObject *obj);

   inline ColorF _getValue( S32 row, S32 column );

public:

   blTerrainProxy(SceneObject * obj);
   ~blTerrainProxy();
   TerrainBlock * operator->() {return(static_cast<TerrainBlock*>(static_cast<SceneObject*>(mObj)));}
   TerrainBlock * getObject() {return(static_cast<TerrainBlock*>(static_cast<SceneObject*>(mObj)));}

   bool getShadowedSquares(const Vector<PlaneF> &, Vector<U16> &);

   // lighting
   void init();
   bool preLight(LightInfo *);
   void light(LightInfo *);

   // persist
   U32 getResourceCRC();
   bool setPersistInfo(PersistInfo::PersistChunk *);
   bool getPersistInfo(PersistInfo::PersistChunk *);

   virtual bool supportsShadowVolume();
   virtual void getClipPlanes(Vector<PlaneF>& planes);
   virtual void addToShadowVolume(ShadowVolumeBSP * shadowVolume, LightInfo * light, S32 level);

   // events
   //virtual void processTGELightProcessEvent(U32 curr, U32 max, LightInfo* currlight); 
   //virtual void processSGObjectProcessEvent(LightInfo* currLight);
};

//-------------------------------------------------------------------------------
// Class SceneLighting::TerrainProxy:
//-------------------------------------------------------------------------------
blTerrainProxy::blTerrainProxy( SceneObject *obj ) :
   Parent( obj ),
   mLightMapSize( getObject()->getLightMapSize() ),
   mTerrainBlockSize( getObject()->getBlockSize() ),
   mGenerateLevel( 0 ),   // HH: Added for Tribes 2 compatibility.
   mLightmap( NULL )
{
}

blTerrainProxy::~blTerrainProxy()
{
   delete [] mLightmap;
}

//-------------------------------------------------------------------------------
void blTerrainProxy::init()
{
   mLightmap = new ColorF[ mLightMapSize * mLightMapSize ];
   dMemset(mLightmap, 0, mLightMapSize * mLightMapSize * sizeof(ColorF));
   mShadowMask.setSize( mTerrainBlockSize * mTerrainBlockSize );

   // HH: Added for Tribes 2 compatibility.
   // How many samples a lexel gets per side, as a power of two
   // (`TerrainProxy::init`, 0x57b840).
   mGenerateLevel = mClamp( Con::getIntVariable( "$pref::sceneLighting::terrainGenerateLevel", 0 ), 0, 4 );
}

bool blTerrainProxy::preLight(LightInfo * light)
{
   if(!bool(mObj))
      return(false);

   if(light->getType() != LightInfo::Vector)
      return(false);

   mShadowMask.clear();
   return(true);
}

inline ColorF blTerrainProxy::_getValue( S32 row, S32 column )
{
   while( row < 0 )
      row += mLightMapSize;
   row = row % mLightMapSize;

   while( column < 0 )
      column += mLightMapSize;
   column = column % mLightMapSize;

   U32 offset = row * mLightMapSize + column;

   return mLightmap[offset];
}

bool blTerrainProxy::markObjectShadow(ObjectProxy * proxy)
{
   if (!proxy->supportsShadowVolume())
      return false;

   // setup the clip planes
   Vector<PlaneF> clipPlanes;
   proxy->getClipPlanes(clipPlanes);

   Vector<U16> shadowList;
   if(!getShadowedSquares(clipPlanes, shadowList))
      return(false);

   // set the correct bit
   for(U32 i = 0; i < shadowList.size(); i++)
      mShadowMask.set(shadowList[i]);

   return(true);
}

void blTerrainProxy::light(LightInfo * light)
{
   // If we don't have terrain or its not a directional
   // light then skip processing.
   TerrainBlock * terrain = getObject();
   if ( !terrain || light->getType() != LightInfo::Vector )
      return;

   S32 time = Platform::getRealMilliseconds();

   // reset
   mShadowVolume = new ShadowVolumeBSP;

   // HH: Modified for Tribes 2 compatibility.
   // Only an interior shadows a terrain: `TerrainProxy::light` (0x57bb90)
   // tests each lit object's type for one (0x57bc13) before marking the
   // squares it shadows and adding it to the shadow volume.
   for(ObjectProxy ** itr = gLighting->mLitObjects.begin(); itr != gLighting->mLitObjects.end(); itr++)
   {
      ObjectProxy* objproxy = *itr;
      if ( !objproxy->getObject() || !( objproxy->getObject()->getTypeMask() & InteriorObjectType ) )
         continue;

      if (markObjectShadow(objproxy))
         objproxy->addToShadowVolume(mShadowVolume, light, SceneLighting::SHADOW_DETAIL);
   }

   lightVector(light);

   // set the lightmap...
   terrain->clearLightMap();

   // HH: Modified for Tribes 2 compatibility.
   // Tribes 2 does not blur a terrain's lightmap: each lexel is already the
   // share of it the sun reaches (0x57bb13 hands lightVector's lexels
   // straight to the lightmap).
   //
   // It also keeps the lightmap at five bits a channel: each one is
   // clamped, scaled by 31 and truncated (`convertColor`, 0x574800), so a
   // lexel is up to a 31st darker than the light it was lit with. Each is
   // stored here as the colour those five bits stand for.
   auto fiveBits = []( F32 channel )
   {
      const U32 level = U32( mClampF( channel, 0.0f, 1.0f ) * 31.0f );
      return U8( ( level * 255 + 15 ) / 31 );
   };

   GBitmap *terrLightMap = terrain->getLightMap();
   for(U32 y = 0; y < mLightMapSize; y++)
   {
      for(U32 x = 0; x < mLightMapSize; x++)
      {
         const ColorF & lexel = mLightmap[x + y * mLightMapSize];

         ColorI color(255, 255, 255, 255);
         color.red   = fiveBits( lexel.red );
         color.green = fiveBits( lexel.green );
         color.blue  = fiveBits( lexel.blue );

         terrLightMap->setColor(x, y, color);
      }
   }

   delete mShadowVolume;

   Con::printf("    = terrain lit in %3.3f seconds", (Platform::getRealMilliseconds()-time)/1000.f);
}

//------------------------------------------------------------------------------
S32 blTerrainProxy::testSquare(const Point3F & min, const Point3F & max, S32 mask, F32 expand, const Vector<PlaneF> & clipPlanes)
{
   expand = 0;
   S32 retMask = 0;
   Point3F minPoint, maxPoint;
   for(S32 i = 0; i < clipPlanes.size(); i++)
   {
      if(mask & (1 << i))
      {
         if(clipPlanes[i].x > 0)
         {
            maxPoint.x = max.x;
            minPoint.x = min.x;
         }
         else
         {
            maxPoint.x = min.x;
            minPoint.x = max.x;
         }
         if(clipPlanes[i].y > 0)
         {
            maxPoint.y = max.y;
            minPoint.y = min.y;
         }
         else
         {
            maxPoint.y = min.y;
            minPoint.y = max.y;
         }
         if(clipPlanes[i].z > 0)
         {
            maxPoint.z = max.z;
            minPoint.z = min.z;
         }
         else
         {
            maxPoint.z = min.z;
            minPoint.z = max.z;
         }
         F32 maxDot = mDot(maxPoint, clipPlanes[i]);
         F32 minDot = mDot(minPoint, clipPlanes[i]);
         F32 planeD = clipPlanes[i].d;
         if(maxDot <= -(planeD + expand))
            return(U16(-1));
         if(minDot <= -planeD)
            retMask |= (1 << i);
      }
   }
   return(retMask);
}

bool blTerrainProxy::getShadowedSquares(const Vector<PlaneF> & clipPlanes, Vector<U16> & shadowList)
{
   TerrainBlock *terrain = getObject();
   if ( !terrain )
      return false;

   // HH: Modified for Tribes 2 compatibility.
   // The squares an interior's shadow can reach, walked down the terrain's
   // quadtree as Tribes 2 walks it (0x57c1d0). Only these lexels have their
   // lit share measured against the shadow volume.
   const TerrainFile *file = terrain->getFile();
   const U32 blockShift = getBinLog2( mTerrainBlockSize );

   Vector<SquareStackNode> stack;
   stack.setSize( blockShift * 4 + 1 );

   stack[0].mLevel = blockShift;
   stack[0].mClipFlags = 0xff;
   stack[0].mPos.set(0,0);

   U32 stackSize = 1;

   Point3F blockPos;
   terrain->getTransform().getColumn(3, &blockPos);
   const F32 squareSize = terrain->getSquareSize();

   bool marked = false;

   // push through all the levels of the quadtree
   while(stackSize)
   {
      SquareStackNode * node = &stack[stackSize - 1];

      S32 clipFlags = node->mClipFlags;
      Point2I pos = node->mPos;
      const TerrainSquare * sq = file->findSquare(node->mLevel, pos.x, pos.y);

      Point3F minPoint, maxPoint;
      minPoint.set(squareSize * pos.x + blockPos.x,
         squareSize * pos.y + blockPos.y,
         fixedToFloat(sq->minHeight) + blockPos.z);
      maxPoint.set(minPoint.x + squareSize * (1 << node->mLevel),
         minPoint.y + squareSize * (1 << node->mLevel),
         fixedToFloat(sq->maxHeight) + blockPos.z);

      // test the square against the current level
      if(clipFlags)
      {
         clipFlags = testSquare(minPoint, maxPoint, clipFlags, squareSize, clipPlanes);
         if(clipFlags == U16(-1))
         {
            stackSize--;
            continue;
         }
      }

      // shadowed?
      if(node->mLevel == 0)
      {
         marked = true;
         shadowList.push_back(pos.x + (pos.y << blockShift));
         stackSize--;
         continue;
      }

      // setup the next level of squares
      U8 nextLevel = node->mLevel - 1;
      S32 squareHalfSize = 1 << nextLevel;

      for(U32 i = 0; i < 4; i++)
      {
         node[i].mLevel = nextLevel;
         node[i].mClipFlags = clipFlags;
      }

      node[3].mPos = pos;
      node[2].mPos.set(pos.x + squareHalfSize, pos.y);
      node[1].mPos.set(pos.x, pos.y + squareHalfSize);
      node[0].mPos.set(pos.x + squareHalfSize, pos.y + squareHalfSize);

      stackSize += 3;
   }

   return marked;
}

// HH: Modified for Tribes 2 compatibility.
// Tribes 2's `TerrainProxy::lightVector` (0x57c330). Torque 3D cast a ray
// from each lexel and kept only whether it hit anything; the shader then
// lit the terrain itself. Tribes 2 bakes all of a lexel's light into it:
// the ambient, plus the sun by how squarely the ground faces it and by the
// share of the lexel no interior shadows. The terrain shadows itself by a
// running height swept along the sun, and the work is done at
// $pref::sceneLighting::terrainGenerateLevel samples a lexel per side,
// averaged.
void blTerrainProxy::lightVector(LightInfo * light)
{
   TerrainBlock* terrain = getObject();
   if (!terrain)
      return;

   Point3F lightDir = light->getDirection();
   lightDir.normalize();

   const ColorF & ambient = light->getAmbient();
   const ColorF & lightColor = light->getColor();

   // A sun straight overhead lights nothing.
   if(lightDir.x == 0 && lightDir.y == 0)
      return;

   const TerrainFile * file = terrain->getFile();

   const U32 blockSize = mTerrainBlockSize;
   const U32 blockShift = getBinLog2(blockSize);
   const U32 blockMask = blockSize - 1;
   const U32 lightmapShift = getBinLog2(mLightMapSize);

   const U32 generateDim = mLightMapSize << mGenerateLevel;
   const U32 generateShift = lightmapShift + mGenerateLevel;
   const U32 generateMask = generateDim - 1;

   const F32 squareSize = terrain->getSquareSize();
   const F32 stepSize = squareSize / F32(generateDim / blockSize);

   // Walk the samples in columns along whichever axis the sun travels
   // furthest in, so each column's shadow heights come from the last.
   F32 zStep;
   F32 frac;

   Point2I blockColStep;
   Point2I blockRowStep;
   Point2I blockFirstPos;
   Point2I lmFirstPos;

   if(mFabs(lightDir.x) >= mFabs(lightDir.y))
   {
      if(lightDir.x > 0)
      {
         zStep = lightDir.z / lightDir.x;
         frac = lightDir.y / lightDir.x;

         blockColStep.set(1, 0);
         blockRowStep.set(0, 1);
         blockFirstPos.set(0, 0);
         lmFirstPos.set(0, 0);
      }
      else
      {
         zStep = -lightDir.z / lightDir.x;
         frac = -lightDir.y / lightDir.x;

         blockColStep.set(-1, 0);
         blockRowStep.set(0, 1);
         blockFirstPos.set(blockMask, 0);
         lmFirstPos.set(mLightMapSize - 1, 0);
      }
   }
   else
   {
      if(lightDir.y > 0)
      {
         zStep = lightDir.z / lightDir.y;
         frac = lightDir.x / lightDir.y;

         blockColStep.set(0, 1);
         blockRowStep.set(1, 0);
         blockFirstPos.set(0, 0);
         lmFirstPos.set(0, 0);
      }
      else
      {
         zStep = -lightDir.z / lightDir.y;
         frac = -lightDir.x / lightDir.y;

         blockColStep.set(0, -1);
         blockRowStep.set(1, 0);
         blockFirstPos.set(0, blockMask);
         lmFirstPos.set(0, mLightMapSize - 1);
      }
   }
   zStep *= stepSize;

   S32 fracStep = -1;
   if(frac < 0)
   {
      fracStep = 1;
      frac = -frac;
   }
   const F32 oneMinusFrac = 1 - frac;

   // Samples to a square and to a lexel, per side.
   const U32 squareShift = generateShift - blockShift;
   const U32 lexelShift = generateShift - lightmapShift;

   const U32 squareStep = 1 << squareShift;
   const U32 squareMask = squareStep - 1;
   const U32 lexelStep = 1 << lexelShift;
   const U32 lexelNormalOffset = lexelStep >> 1;
   const U32 lexelMask = lexelStep - 1;

   Vector<F32> heightStore[2];
   heightStore[0].setSize(generateDim);
   heightStore[1].setSize(generateDim);
   F32 * heightArray = heightStore[0].address();
   F32 * nextHeightArray = heightStore[1].address();

   Vector<F32> terrainHeights[2];
   terrainHeights[0].setSize(blockSize);
   terrainHeights[1].setSize(blockSize);
   F32 * pTerrainHeights = terrainHeights[0].address();
   F32 * pNextTerrainHeights = terrainHeights[1].address();

   Vector<F32> terrainZRowStep;
   Vector<F32> nextTerrainZRowStep;
   Vector<F32> terrainZColStep;
   terrainZRowStep.setSize(blockSize);
   nextTerrainZRowStep.setSize(blockSize);
   terrainZColStep.setSize(blockSize);

   const F32 heightStep = 1.f / squareStep;
   U32 i;

   // The grid's own heights, the terrain repeating.
   auto gridHeight = [&]( const Point2I &p )
   {
      return fixedToFloat( file->getHeight( p.x & blockMask, p.y & blockMask ) );
   };

   // The slopes between one column of grid points and the next.
   auto fillSteps = [&]()
   {
      for(U32 j = 0; j < blockSize; j++)
      {
         terrainZRowStep[j] = (pTerrainHeights[(j+1) & blockMask] - pTerrainHeights[j]) * heightStep;
         nextTerrainZRowStep[j] = (pNextTerrainHeights[(j+1) & blockMask] - pNextTerrainHeights[j]) * heightStep;
         terrainZColStep[j] = (pNextTerrainHeights[j] - pTerrainHeights[j]) * heightStep;
      }
   };

   // The height of the ground at a sample, on whichever of its square's two
   // triangles it is: the terrain splits a square on its 135 degree
   // diagonal where the square's coordinates differ in parity.
   auto groundHeight = [&]( const Point2I &bwalk, U32 xmask, U32 ymask, U32 bi, U32 binext, F32 &xstep, F32 &ystep )
   {
      if((bwalk.x ^ bwalk.y) & 1)
      {
         U32 xsub = squareStep - xmask;
         if(xsub > ymask)
         {
            xstep = terrainZColStep[bi];
            ystep = terrainZRowStep[bi];
            return pTerrainHeights[bi] + xmask * xstep + ymask * ystep;
         }

         xstep = -terrainZColStep[binext];
         ystep = nextTerrainZRowStep[bi];
         return pNextTerrainHeights[bi] + xsub * xstep + ymask * ystep;
      }

      if(xmask > ymask)
      {
         xstep = terrainZColStep[bi];
         ystep = nextTerrainZRowStep[bi];
      }
      else
      {
         xstep = terrainZColStep[binext];
         ystep = terrainZRowStep[bi];
      }
      return pTerrainHeights[bi] + xmask * xstep + ymask * ystep;
   };

   Point2I bp = blockFirstPos;
   for(i = 0; i < blockSize; i++)
   {
      pTerrainHeights[i] = gridHeight(bp);
      bp += blockRowStep;
   }

   bp = blockFirstPos + blockColStep;
   for(i = 0; i < blockSize; i++)
   {
      pNextTerrainHeights[i] = gridHeight(bp);
      bp += blockRowStep;
   }

   fillSteps();

   // The first column of shadow heights is the ground itself.
   for(i = 0; i < generateDim; i++)
   {
      U32 bi = i >> squareShift;
      heightArray[i] = pTerrainHeights[bi] + (i & squareMask) * terrainZRowStep[bi];
   }

   bp = blockFirstPos;
   if(generateDim == blockSize)
      bp += blockColStep;

   // Sweep the block once to carry the shadow heights round, the terrain
   // repeating, before the lighting pass starts.
   U32 x, y;
   for(x = 1; x < generateDim; x++)
   {
      U32 xmask = x & squareMask;

      if(!xmask)
      {
         F32 * tmp = pTerrainHeights;
         pTerrainHeights = pNextTerrainHeights;
         pNextTerrainHeights = tmp;

         bp += blockColStep;

         Point2I bwalk = bp;
         for(i = 0; i < blockSize; i++, bwalk += blockRowStep)
            pNextTerrainHeights[i] = gridHeight(bwalk);

         fillSteps();
      }

      Point2I bwalk = bp - blockRowStep;
      for(y = 0; y < generateDim; y++)
      {
         U32 ymask = y & squareMask;
         if(!ymask)
            bwalk += blockRowStep;

         U32 bi = y >> squareShift;
         U32 binext = (bi + 1) & blockMask;

         F32 xstep, ystep;
         F32 height = groundHeight(bwalk, xmask, ymask, bi, binext, xstep, ystep);

         F32 intHeight = heightArray[y] * oneMinusFrac + heightArray[(y + fracStep) & generateMask] * frac + zStep;
         nextHeightArray[y] = getMax(height, intHeight);
      }

      F32 * swapHeights = heightArray;
      heightArray = nextHeightArray;
      nextHeightArray = swapHeights;
   }

   const F32 lexelDim = squareSize * F32(blockSize) / F32(mLightMapSize);

   // A sample's normal is its square's corner normals, each weighted by
   // how near the sample is to it.
   Vector<Point3F> normals[2];
   normals[0].setSize(blockSize);
   normals[1].setSize(blockSize);
   Point3F * pNormals = normals[0].address();
   Point3F * pNextNormals = normals[1].address();

   Vector<F32> normTable;
   normTable.setSize(squareStep * squareStep * 4);

   const Point2F corners[4] = {
      Point2F(0.f, 0.f),
      Point2F(1.f, 0.f),
      Point2F(1.f, 1.f),
      Point2F(0.f, 1.f)
   };

   U32 idx = 0;
   const F32 step = 1.f / squareStep;
   Point2F pos(0.f, 0.f);
   for(x = 0; x < squareStep; x++, pos.x += step, pos.y = 0.f)
   {
      for(y = 0; y < squareStep; y++, pos.y += step)
      {
         for(i = 0; i < 4; i++, idx++)
            normTable[idx] = 1.f - getMin(Point2F(pos - corners[i]).len(), 1.f);
      }
   }

   auto gridNormal = [&]( const Point2I &p, Point3F &out )
   {
      out.set(0.f, 0.f, 1.f);
      terrain->getNormal(Point2F(p.x * squareSize, p.y * squareSize), &out, true, false);
   };

   bp = blockFirstPos;
   for(x = 0; x < blockSize; x++)
   {
      pNextTerrainHeights[x] = gridHeight(bp);
      gridNormal(bp, pNextNormals[x]);
      bp += blockRowStep;
   }

   // get swapped on first pass
   pTerrainHeights = terrainHeights[1].address();
   pNextTerrainHeights = terrainHeights[0].address();

   Point3F worldOffset;
   terrain->getTransform().getColumn(3, &worldOffset);

   const F32 ratio = F32(1 << lexelShift);
   const F32 inverseRatioSquared = 1.f / (ratio * ratio);

   Vector<F32> lightScale;
   lightScale.setSize(mLightMapSize);

   // walk it...
   bp = blockFirstPos - blockColStep;
   for(x = 0; x < generateDim; x++)
   {
      U32 xmask = x & squareMask;
      U32 lxmask = x & lexelMask;

      if(!xmask)
      {
         bp += blockColStep;

         Point3F * temp = pNormals;
         pNormals = pNextNormals;
         pNextNormals = temp;

         Point2I bwalk = bp + blockColStep;
         for(i = 0; i < blockSize; i++)
         {
            gridNormal(bwalk, pNextNormals[i]);
            bwalk += blockRowStep;
         }

         F32 * tmp = pTerrainHeights;
         pTerrainHeights = pNextTerrainHeights;
         pNextTerrainHeights = tmp;

         bwalk = bp + blockColStep;
         for(i = 0; i < blockSize; i++, bwalk += blockRowStep)
            pNextTerrainHeights[i] = gridHeight(bwalk);

         fillSteps();
      }

      // A new column of lexels starts fully lit.
      if(!lxmask)
         for(i = 0; i < mLightMapSize; i++)
            lightScale[i] = 1.f;

      Point2I bwalk = bp - blockRowStep;
      for(y = 0; y < generateDim; y++)
      {
         U32 lymask = y & lexelMask;
         U32 ymask = y & squareMask;
         if(!ymask)
            bwalk += blockRowStep;

         U32 bi = y >> squareShift;
         U32 binext = (bi + 1) & blockMask;

         F32 xstep, ystep;
         F32 height = groundHeight(bwalk, xmask, ymask, bi, binext, xstep, ystep);

         F32 intHeight = heightArray[y] * oneMinusFrac + heightArray[(y + fracStep) & generateMask] * frac + zStep;

         U32 lsi = y >> lexelShift;

         Point2I lmPos = lmFirstPos + blockColStep * (x >> lexelShift) + blockRowStep * lsi;

         ColorF & col = mLightmap[lmPos.x + (lmPos.y << lightmapShift)];

         // Measure the share of a lexel an interior leaves lit, once, at
         // its first sample, where its square is in an interior's shadow.
         Point2I terrPos = lmPos;
         terrPos.x >>= lightmapShift - blockShift;
         terrPos.y >>= lightmapShift - blockShift;

         if(!lxmask && !lymask && mShadowMask.test(terrPos.x + (terrPos.y << blockShift)))
         {
            U32 nidx = (xmask + lexelNormalOffset + ((ymask + lexelNormalOffset) << squareShift)) << 2;

            Point3F normal = pNormals[bi] * normTable[nidx++];
            normal += pNormals[binext] * normTable[nidx++];
            normal += pNextNormals[binext] * normTable[nidx++];
            normal += pNextNormals[bi] * normTable[nidx];
            normal.normalize();

            nextHeightArray[y] = height;
            F32 colorScale = -mDot(normal, lightDir);

            if(colorScale > 0.f)
            {
               Point2F wPos(lmPos.x * lexelDim + worldOffset.x,
                            lmPos.y * lexelDim + worldOffset.y);

               F32 xh = xstep * ratio;
               F32 yh = ystep * ratio;
               F32 z = height + worldOffset.z;

               ShadowVolumeBSP::SVPoly * poly = mShadowVolume->createPoly();
               poly->mWindingCount = 4;
               poly->mWinding[0].set(wPos.x, wPos.y, z);
               poly->mWinding[1].set(wPos.x + lexelDim, wPos.y, z + xh);
               poly->mWinding[2].set(wPos.x + lexelDim, wPos.y + lexelDim, z + xh + yh);
               poly->mWinding[3].set(wPos.x, wPos.y + lexelDim, z + yh);
               poly->mPlane.set(poly->mWinding[0], poly->mWinding[1], poly->mWinding[2]);

               F32 lexelSize = mShadowVolume->getPolySurfaceArea(poly);
               F32 intensity = mShadowVolume->getClippedSurfaceArea(poly) / lexelSize;
               lightScale[lsi] = mClampF(intensity, 0.f, 1.f);
            }
            else
               lightScale[lsi] = 0.f;
         }

         // Above the swept shadow height the sun reaches the ground.
         if(height >= intHeight)
         {
            U32 nidx = (xmask + (ymask << squareShift)) << 2;

            Point3F normal = pNormals[bi] * normTable[nidx++];
            normal += pNormals[binext] * normTable[nidx++];
            normal += pNextNormals[binext] * normTable[nidx++];
            normal += pNextNormals[bi] * normTable[nidx];
            normal.normalize();

            nextHeightArray[y] = height;
            F32 colorScale = -mDot(normal, lightDir);

            if(colorScale > 0.f)
               col += ambient + lightColor * colorScale * lightScale[lsi];
            else
               col += ambient;
         }
         else
         {
            nextHeightArray[y] = intHeight;
            col += ambient;
         }
      }

      F32 * swapHeights = heightArray;
      heightArray = nextHeightArray;
      nextHeightArray = swapHeights;
   }

   // Average each lexel's samples.
   for(i = 0; i < mLightMapSize * mLightMapSize; i++)
   {
      mLightmap[i] *= inverseRatioSquared;
      mLightmap[i].clamp();
   }
}

//--------------------------------------------------------------------------
U32 blTerrainProxy::getResourceCRC()
{
   TerrainBlock * terrain = getObject();
   if(!terrain)
      return(0);
   return(terrain->getCRC());
}

//--------------------------------------------------------------------------
bool blTerrainProxy::setPersistInfo(PersistInfo::PersistChunk * info)
{
   if(!Parent::setPersistInfo(info))
      return(false);

   blTerrainChunk * chunk = dynamic_cast<blTerrainChunk*>(info);
   AssertFatal(chunk, "blTerrainProxy::setPersistInfo: invalid info chunk!");

   TerrainBlock * terrain = getObject();
   if(!terrain || !terrain->getLightMap())
      return(false);

   // HH: Added for Tribes 2 compatibility.
   // A cache lit at another lightmap size is a cache for some other terrain
   // setup; relight rather than stretch it over this one.
   if(!chunk->mLightmap || chunk->mLightmap->getWidth() != mLightMapSize ||
      chunk->mLightmap->getHeight() != mLightMapSize)
      return(false);

   terrain->setLightMap( new GBitmap( *chunk->mLightmap) );

   return(true);
}

bool blTerrainProxy::getPersistInfo(PersistInfo::PersistChunk * info)
{
   if(!Parent::getPersistInfo(info))
      return(false);

   blTerrainChunk * chunk = dynamic_cast<blTerrainChunk*>(info);
   AssertFatal(chunk, "blTerrainProxy::getPersistInfo: invalid info chunk!");

   TerrainBlock * terrain = getObject();
   if(!terrain || !terrain->getLightMap())
      return(false);

   if(chunk->mLightmap) delete chunk->mLightmap;

   chunk->mLightmap = new GBitmap(*terrain->getLightMap());

   return(true);
}

bool blTerrainProxy::supportsShadowVolume()
{
   return false;
}

void blTerrainProxy::getClipPlanes(Vector<PlaneF>& planes)
{

}

void blTerrainProxy::addToShadowVolume(ShadowVolumeBSP * shadowVolume, LightInfo * light, S32 level)
{

}



void blTerrainSystem::init()
{
}

U32 blTerrainSystem::addObjectType()
{
   return TerrainObjectType;
}

SceneLighting::ObjectProxy* blTerrainSystem::createObjectProxy(SceneObject* obj, SceneLighting::ObjectProxyList* sceneObjects)
{
   if ((obj->getTypeMask() & TerrainObjectType) != 0)
      return new blTerrainProxy(obj);
   else
      return NULL;
}

PersistInfo::PersistChunk* blTerrainSystem::createPersistChunk(const U32 chunkType)
{
   if (chunkType == PersistInfo::PersistChunk::TerrainChunkType)
      return new blTerrainChunk();
   else
      return NULL;
}

bool blTerrainSystem::createPersistChunkFromProxy(SceneLighting::ObjectProxy* objproxy, PersistInfo::PersistChunk **ret)
{
   if (dynamic_cast<blTerrainProxy*>(objproxy) != NULL)
   {
      *ret = new blTerrainChunk();
      return true;
   } else {
      return NULL;
   }
}

// Given a ray, this will return the color from the lightmap of this object, return true if handled
bool blTerrainSystem::getColorFromRayInfo(const RayInfo & collision, ColorF& result) const
{
   TerrainBlock *terrain = dynamic_cast<TerrainBlock *>(collision.object);
   if (!terrain)
      return false;

   Point2F uv;
   F32 terrainlength = (F32)terrain->getBlockSize();
   Point3F pos = terrain->getPosition();
   uv.x = (collision.point.x - pos.x) / terrainlength;
   uv.y = (collision.point.y - pos.y) / terrainlength;

   // similar to x = x & width...
   uv.x = uv.x - F32(U32(uv.x));
   uv.y = uv.y - F32(U32(uv.y));
   const GBitmap* lightmap = terrain->getLightMap();
   if (!lightmap)
      return false;

   result = lightmap->sampleTexel(uv.x, uv.y);
   // terrain lighting is dim - look into this (same thing done in shaders)...
   result *= 2.0f;
   return true;
}
