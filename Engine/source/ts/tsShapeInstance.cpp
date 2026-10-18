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
#include "ts/tsShapeInstance.h"

#include "ts/tsLastDetail.h"
#include "ts/tsMaterialList.h"
#include "console/consoleTypes.h"
#include "ts/tsDecal.h"
#include "platform/profiler.h"
#include "core/frameAllocator.h"
#include "gfx/gfxDevice.h"
#include "materials/materialManager.h"
#include "materials/materialFeatureTypes.h"
#include "materials/sceneData.h"
#include "materials/matInstance.h"
#include "materials/materialDefinition.h"
#include "scene/sceneRenderState.h"
#include "gfx/primBuilder.h"
#include "gfx/gfxDrawUtil.h"
#include "core/module.h"
// HH: Added for Tribes 2 compatibility.
#include "renderInstance/renderPassManager.h"


MODULE_BEGIN( TSShapeInstance )

   MODULE_INIT
   {
      Con::addVariable("$pref::TS::detailAdjust", TypeF32, &TSShapeInstance::smDetailAdjust,
         "@brief User perference for scaling the TSShape level of detail.\n"
         "The smaller the value the closer the camera must get to see the "
         "highest LOD.  This setting can have a huge impact on performance in "
         "mesh heavy scenes.  The default value is 1.\n"
         "@ingroup Rendering\n" );

      Con::addVariable("$pref::TS::skipLoadDLs", TypeS32, &TSShape::smNumSkipLoadDetails,
         "@brief User perference which causes TSShapes to skip loading higher lods.\n"
         "This potentialy reduces the GPU resources and materials generated as well as "
         "limits the LODs rendered.  The default value is 0.\n"
         "@see $pref::TS::skipRenderDLs\n"
         "@ingroup Rendering\n" );

      Con::addVariable("$pref::TS::skipRenderDLs", TypeS32, &TSShapeInstance::smNumSkipRenderDetails,
         "@brief User perference which causes TSShapes to skip rendering higher lods.\n"
         "This will reduce the number of draw calls and triangles rendered and improve "
         "rendering performance when proper LODs have been created for your models. "
         "The default value is 0.\n"
         "@see $pref::TS::skipLoadDLs\n"
         "@ingroup Rendering\n" );

      Con::addVariable("$pref::TS::smallestVisiblePixelSize", TypeF32, &TSShapeInstance::smSmallestVisiblePixelSize,
         "@brief User perference which sets the smallest pixel size at which TSShapes will skip rendering.\n"
         "This will force all shapes to stop rendering when they get smaller than this size. "
         "The default value is -1 which disables it.\n"
         "@ingroup Rendering\n" );

      Con::addVariable("$pref::TS::maxInstancingVerts", TypeS32, &TSMesh::smMaxInstancingVerts,
         "@brief Enables mesh instancing on non-skin meshes that have less that this count of verts.\n"
         "The default value is 200.  Higher values can degrade performance.\n"
         "@ingroup Rendering\n" );
   }

MODULE_END;


F32                           TSShapeInstance::smDetailAdjust = 1.0f;
F32                           TSShapeInstance::smSmallestVisiblePixelSize = -1.0f;
S32                           TSShapeInstance::smNumSkipRenderDetails = 0;

F32                           TSShapeInstance::smLastScreenErrorTolerance = 0.0f;
F32                           TSShapeInstance::smLastScaledDistance = 0.0f;
F32                           TSShapeInstance::smLastPixelSize = 0.0f;

Vector<QuatF>                 TSShapeInstance::smNodeCurrentRotations(__FILE__, __LINE__);
Vector<Point3F>               TSShapeInstance::smNodeCurrentTranslations(__FILE__, __LINE__);
Vector<F32>                   TSShapeInstance::smNodeCurrentUniformScales(__FILE__, __LINE__);
Vector<Point3F>               TSShapeInstance::smNodeCurrentAlignedScales(__FILE__, __LINE__);
Vector<TSScale>               TSShapeInstance::smNodeCurrentArbitraryScales(__FILE__, __LINE__);
Vector<MatrixF>               TSShapeInstance::smNodeLocalTransforms(__FILE__, __LINE__);
TSIntegerSet                  TSShapeInstance::smNodeLocalTransformDirty;

Vector<TSThread*>             TSShapeInstance::smRotationThreads(__FILE__, __LINE__);
Vector<TSThread*>             TSShapeInstance::smTranslationThreads(__FILE__, __LINE__);
Vector<TSThread*>             TSShapeInstance::smScaleThreads(__FILE__, __LINE__);

//-------------------------------------------------------------------------------------
// constructors, destructors, initialization
//-------------------------------------------------------------------------------------

TSShapeInstance::TSShapeInstance( const Resource<TSShape> &shape, bool loadMaterials )
{
   VECTOR_SET_ASSOCIATION(mMeshObjects);
   VECTOR_SET_ASSOCIATION(mNodeTransforms);
   VECTOR_SET_ASSOCIATION(mNodeReferenceRotations);
   VECTOR_SET_ASSOCIATION(mNodeReferenceTranslations);
   VECTOR_SET_ASSOCIATION(mNodeReferenceUniformScales);
   VECTOR_SET_ASSOCIATION(mNodeReferenceScaleFactors);
   VECTOR_SET_ASSOCIATION(mNodeReferenceArbitraryScaleRots);
   VECTOR_SET_ASSOCIATION(mThreadList);
   VECTOR_SET_ASSOCIATION(mTransitionThreads);

   mShapeResource = shape;
   mShape = mShapeResource;
   buildInstanceData( mShape, loadMaterials );
}

TSShapeInstance::TSShapeInstance( TSShape *shape, bool loadMaterials )
{
   VECTOR_SET_ASSOCIATION(mMeshObjects);
   VECTOR_SET_ASSOCIATION(mNodeTransforms);
   VECTOR_SET_ASSOCIATION(mNodeReferenceRotations);
   VECTOR_SET_ASSOCIATION(mNodeReferenceTranslations);
   VECTOR_SET_ASSOCIATION(mNodeReferenceUniformScales);
   VECTOR_SET_ASSOCIATION(mNodeReferenceScaleFactors);
   VECTOR_SET_ASSOCIATION(mNodeReferenceArbitraryScaleRots);
   VECTOR_SET_ASSOCIATION(mThreadList);
   VECTOR_SET_ASSOCIATION(mTransitionThreads);

   mShapeResource = NULL;
   mShape = shape;
   buildInstanceData( mShape, loadMaterials );
}

TSShapeInstance::~TSShapeInstance()
{
   mMeshObjects.clear();

   while (mThreadList.size())
      destroyThread(mThreadList.last());

   setMaterialList(NULL);

   delete [] mDirtyFlags;
}

void TSShapeInstance::buildInstanceData(TSShape * _shape, bool loadMaterials)
{
   mShape = _shape;

   debrisRefCount = 0;

   mCurrentDetailLevel = 0;
   mCurrentIntraDetailLevel = 1.0f;

   // all triggers off at start
   mTriggerStates = 0;

   //
   mAlphaAlways = false;
   mAlphaAlwaysValue = 1.0f;

   // material list...
   mMaterialList = NULL;
   mOwnMaterialList = false;

   //
   mData = 0;
   mScaleCurrentlyAnimated = false;

   if(loadMaterials)
      setMaterialList(mShape->materialList);

   // set up node data
   initNodeTransforms();

   // add objects to trees
   initMeshObjects();

   // set up subtree data
   S32 ss = mShape->subShapeFirstNode.size(); // we have this many subtrees
   mDirtyFlags = new U32[ss];

   mGroundThread = NULL;
   mCurrentDetailLevel = 0;

   animateSubtrees();

   // Construct billboards if not done already
   if ( loadMaterials && mShapeResource )
      mShape->setupBillboardDetails( mShapeResource.getPath().getFullPath() );
}

void TSShapeInstance::initNodeTransforms()
{
   // set up node data
   S32 numNodes = mShape->nodes.size();
   mNodeTransforms.setSize(numNodes);
}

void TSShapeInstance::initMeshObjects()
{
   // add objects to trees
   S32 numObjects = mShape->objects.size();
   mMeshObjects.setSize(numObjects);
   for (S32 i=0; i<numObjects; i++)
   {
      const TSObject * obj = &mShape->objects[i];
      MeshObjectInstance * objInst = &mMeshObjects[i];

      // hook up the object to it's node and transforms.
      objInst->mTransforms = &mNodeTransforms;
      objInst->nodeIndex = obj->nodeIndex;

      // set up list of meshes
      if (obj->numMeshes)
         objInst->meshList = &mShape->meshes[obj->startMeshIndex];
      else
         objInst->meshList = NULL;

      objInst->object = obj;
      objInst->forceHidden = false;
   }

   // HH: Added for Tribes 2 compatibility.
   mDecalObjects.setSize(mShape->decals.size());
   for (S32 i=0; i<mDecalObjects.size(); i++)
   {
      mDecalObjects[i].decal = i;
      mDecalObjects[i].frame = -1;
   }
}

void TSShapeInstance::setMaterialList( TSMaterialList *matList )
{
   // get rid of old list
   if ( mOwnMaterialList )
      delete mMaterialList;

   mMaterialList = matList;
   mOwnMaterialList = false;

   // If the material list is already be mapped then
   // don't bother doing the initializing a second time.
   // Note: only check the last material instance as this will catch both
   // uninitialised lists, as well as initialised lists that have had new
   // materials appended
   if ( mMaterialList && !mMaterialList->getMaterialInst( mMaterialList->size()-1 ) )
   {
      mMaterialList->setTextureLookupPath( mShapeResource.getPath().getPath() );
      mMaterialList->mapMaterials();
      Material::sAllowTextureTargetAssignment = true;
      initMaterialList();
      Material::sAllowTextureTargetAssignment = false;
   }

   // HH: Added for Tribes 2 compatibility.
   bindIflMaterials();
}

void TSShapeInstance::cloneMaterialList( const FeatureSet *features )
{
   if ( mOwnMaterialList )
      return;

   mMaterialList = new TSMaterialList(mMaterialList);
   initMaterialList( features );

   mOwnMaterialList = true;
}

void TSShapeInstance::initMaterialList( const FeatureSet *features )
{
   // If we don't have features then use the default.
   if ( !features )
      features = &MATMGR->getDefaultFeatures();

   // HH: Added for Tribes 2 compatibility.
   // An ifl slot shares a frame's instance, so the sharing is undone around the
   // walk.
   releaseIflMaterials();

   // Initialize the materials.
   mMaterialList->initMatInstances( *features, mShape->getVertexFormat() );

   bindIflMaterials();

   // TODO: It would be good to go thru all the meshes and
   // pre-create all the active material hooks for shadows,
   // reflections, and instancing.  This would keep these
   // hiccups from happening at runtime.
}

// HH: Added for Tribes 2 compatibility.
//
/// Whether this instance is one of the ifl material's own frames.
static bool isIflFrame( const TSMaterialList *list,
                        const TSShape::IflMaterial &ifl,
                        const BaseMatInstance *inst )
{
   for ( S32 i = 0; i < ifl.numFrames; i++ )
      if ( list->getMaterialInst( ifl.firstFrame + i ) == inst )
         return true;

   return false;
}

void TSShapeInstance::releaseIflMaterials()
{
   const Vector<TSShape::IflMaterial> &ifls = mShape->iflMaterials;
   if ( ifls.empty() || !mMaterialList )
      return;

   for ( S32 i = 0; i < ifls.size(); i++ )
   {
      const TSShape::IflMaterial &ifl = ifls[i];
      if ( !ifl.numFrames || ifl.materialSlot < 0 ||
           ifl.firstFrame + ifl.numFrames > (S32)mMaterialList->size() )
         continue;


      BaseMatInstance *own = mMaterialList->getMaterialInst( ifl.materialSlot );
      if ( own && !isIflFrame( mMaterialList, ifl, own ) )
         delete own;

      mMaterialList->setMaterialInst( NULL, ifl.materialSlot );
   }
}

void TSShapeInstance::bindIflMaterials()
{
   const Vector<TSShape::IflMaterial> &ifls = mShape->iflMaterials;
   if ( ifls.empty() || !mMaterialList )
      return;

   mIflFrame.setSize( ifls.size() );

   for ( S32 i = 0; i < ifls.size(); i++ )
   {
      const TSShape::IflMaterial &ifl = ifls[i];

      mIflFrame[i] = -1;
      if ( !ifl.numFrames || ifl.materialSlot < 0 ||
           ifl.firstFrame + ifl.numFrames > (S32)mMaterialList->size() )
         continue;

      mIflFrame[i] = 0;
      mMaterialList->setMaterialInst(
         mMaterialList->getMaterialInst( ifl.firstFrame ), ifl.materialSlot );
   }
}

void TSShapeInstance::animateIfls()
{
   const Vector<TSShape::IflMaterial> &ifls = mShape->iflMaterials;
   if ( ifls.empty() || ifls.size() != mIflFrame.size() || !mMaterialList )
      return;


   const F32 now = (F32)Sim::getCurrentTime() / 1000.0f;

   for ( S32 i = 0; i < ifls.size(); i++ )
   {
      if ( mIflFrame[i] < 0 )
         continue;

      const TSShape::IflMaterial &ifl = ifls[i];

      // An ifl is timed on the exporter's timeline, which a sequence occupies
      // from toolBegin for its own duration, so it plays at the file's rate.
      S32 frame = -1;
      for ( S32 j = 0; j < mThreadList.size(); j++ )
      {
         TSThread *th = mThreadList[j];
         const TSShape::Sequence *seq = th->getSequence();
         if ( !seq->iflMatters.test( i ) )
            continue;

         frame = mShape->getIflFrame(
            ifl, th->getPos() * seq->duration + seq->toolBegin );
         break;
      }

      // Nothing animates it, so it runs at the rate its own file asks for.
      if ( frame < 0 )
         frame = mShape->getIflFrame( ifl, now );

      if ( frame == mIflFrame[i] )
         continue;

      mIflFrame[i] = frame;
      mMaterialList->setMaterialInst(
         mMaterialList->getMaterialInst( ifl.firstFrame + frame ),
         ifl.materialSlot );
   }
}

void TSShapeInstance::reSkin( String newBaseName, String oldBaseName )
{
   if( newBaseName.isEmpty() )
      newBaseName = "base";
   if( oldBaseName.isEmpty() )
      oldBaseName = "base";

   if ( newBaseName.equal( oldBaseName, String::NoCase ) )
      return;

   const U32 oldBaseNameLength = oldBaseName.length();

   // Make our own copy of the materials list from the resource if necessary
   if (ownMaterialList() == false)
      cloneMaterialList();

   TSMaterialList* pMatList = getMaterialList();
   pMatList->setTextureLookupPath( mShapeResource.getPath().getPath() );

   // Cycle through the materials
   const Vector<String> &materialNames = pMatList->getMaterialNameList();
   for ( S32 i = 0; i < materialNames.size(); i++ )
   {
      // Try changing base
      const String &pName = materialNames[i];

      String::SizeType fileAt = 0;
      for ( String::SizeType c = 0; c < pName.length(); c++ )
         if ( pName[ c ] == '/' || pName[ c ] == '\\' )
            fileAt = c + 1;

      const String file = pName.substr( fileAt, pName.length() - fileAt );

      if ( file.compare( oldBaseName, oldBaseNameLength, String::NoCase ) == 0 )
      {
         String newName( pName );
         newName.replace( fileAt, oldBaseNameLength, newBaseName );
         pMatList->renameMaterial( i, newName );
      }
   }

   // Initialize the material instances
   initMaterialList();
}

//-------------------------------------------------------------------------------------
// Render & detail selection
//-------------------------------------------------------------------------------------

void TSShapeInstance::renderDebugNormals( F32 normalScalar, S32 dl )
{
   if ( dl < 0 )
      return;

   AssertFatal( dl >= 0 && dl < mShape->details.size(),
      "TSShapeInstance::renderDebugNormals() - Bad detail level!" );

   static GFXStateBlockRef sb;
   if ( sb.isNull() )
   {
      GFXStateBlockDesc desc;
      desc.setCullMode( GFXCullNone );
      desc.setZReadWrite( true );
      desc.zWriteEnable = false;
      desc.vertexColorEnable = true;

      sb = GFX->createStateBlock( desc );
   }
   GFX->setStateBlock( sb );

   const TSDetail *detail = &mShape->details[dl];
   const S32 ss = detail->subShapeNum;
   if ( ss < 0 )
      return;

   const S32 start = mShape->subShapeFirstObject[ss];
   const S32 end   = start + mShape->subShapeNumObjects[ss];

   for ( S32 i = start; i < end; i++ )
   {
      MeshObjectInstance *meshObj = &mMeshObjects[i];
      if ( !meshObj )
         continue;

      const MatrixF &meshMat = meshObj->getTransform();

      // Then go through each TSMesh...
      U32 m = 0;
      for( TSMesh *mesh = meshObj->getMesh(m); mesh != NULL; mesh = meshObj->getMesh(m++) )
      {
         // and pull out the list of normals.
         const U32 numNrms = mesh->mNumVerts;
         PrimBuild::begin( GFXLineList, 2 * numNrms );
         for ( U32 n = 0; n < numNrms; n++ )
         {
            Point3F norm = mesh->mVertexData[n].normal();
            Point3F vert = mesh->mVertexData[n].vert();

            meshMat.mulP( vert );
            meshMat.mulV( norm );

            // Then render them.
            PrimBuild::color4f( mFabs( norm.x ), mFabs( norm.y ), mFabs( norm.z ), 1.0f );
            PrimBuild::vertex3fv( vert );
            PrimBuild::vertex3fv( vert + (norm * normalScalar) );
         }

         PrimBuild::end();
      }
   }
}

void TSShapeInstance::renderDebugNodes()
{
   GFXDrawUtil *drawUtil = GFX->getDrawUtil();
   ColorI color( 255, 0, 0, 255 );

   GFXStateBlockDesc desc;
   desc.setBlend( false );
   desc.setZReadWrite( false, false );

   for ( U32 i = 0; i < mNodeTransforms.size(); i++ )
      drawUtil->drawTransform( desc, mNodeTransforms[i], NULL, NULL );
}

void TSShapeInstance::listMeshes( const String &state ) const
{
   if ( state.equal( "All", String::NoCase ) )
   {
      for ( U32 i = 0; i < mMeshObjects.size(); i++ )
      {
         const MeshObjectInstance &mesh = mMeshObjects[i];
         Con::warnf( "meshidx %3d, %8s, %s", i, ( mesh.forceHidden ) ? "Hidden" : "Visible", mShape->getMeshName(i).c_str() );         
      }
   }
   else if ( state.equal( "Hidden", String::NoCase ) )
   {
      for ( U32 i = 0; i < mMeshObjects.size(); i++ )
      {
         const MeshObjectInstance &mesh = mMeshObjects[i];
         if ( mesh.forceHidden )
            Con::warnf( "meshidx %3d, %8s, %s", i, "Visible", mShape->getMeshName(i).c_str() );         
      }
   }
   else if ( state.equal( "Visible", String::NoCase ) )
   {
      for ( U32 i = 0; i < mMeshObjects.size(); i++ )
      {
         const MeshObjectInstance &mesh = mMeshObjects[i];
         if ( !mesh.forceHidden )
            Con::warnf( "meshidx %3d, %8s, %s", i, "Hidden", mShape->getMeshName(i).c_str() );         
      }
   }
   else
   {
      Con::warnf( "TSShapeInstance::listMeshes( %s ) - only All/Hidden/Visible are valid parameters." );
   }
}

void TSShapeInstance::render( const TSRenderState &rdata )
{
   if (mCurrentDetailLevel<0)
      return;

   PROFILE_SCOPE( TSShapeInstance_Render );

   // alphaIn:  we start to alpha-in next detail level when intraDL > 1-alphaIn-alphaOut
   //           (finishing when intraDL = 1-alphaOut)
   // alphaOut: start to alpha-out this detail level when intraDL > 1-alphaOut
   // NOTE:
   //   intraDL is at 1 when if shape were any closer to us we'd be at dl-1,
   //   intraDL is at 0 when if shape were any farther away we'd be at dl+1
   F32 alphaOut = mShape->alphaOut[mCurrentDetailLevel];
   F32 alphaIn  = mShape->alphaIn[mCurrentDetailLevel];
   F32 saveAA = mAlphaAlways ? mAlphaAlwaysValue : 1.0f;

   /// This first case is the single detail level render.
   if ( mCurrentIntraDetailLevel > alphaIn + alphaOut )
      render( rdata, mCurrentDetailLevel, mCurrentIntraDetailLevel );
   else if ( mCurrentIntraDetailLevel > alphaOut )
   {
      // draw this detail level w/ alpha=1 and next detail level w/
      // alpha=1-(intraDl-alphaOut)/alphaIn

      // first draw next detail level
      if ( mCurrentDetailLevel + 1 < mShape->details.size() && mShape->details[ mCurrentDetailLevel + 1 ].size > 0.0f )
      {
         setAlphaAlways( saveAA * ( alphaIn + alphaOut - mCurrentIntraDetailLevel ) / alphaIn );
         render( rdata, mCurrentDetailLevel + 1, 0.0f );
      }

      setAlphaAlways( saveAA );
      render( rdata, mCurrentDetailLevel, mCurrentIntraDetailLevel );
   }
   else
   {
      // draw next detail level w/ alpha=1 and this detail level w/
      // alpha = 1-intraDL/alphaOut

      // first draw next detail level
      if ( mCurrentDetailLevel + 1 < mShape->details.size() && mShape->details[ mCurrentDetailLevel + 1 ].size > 0.0f )
         render( rdata, mCurrentDetailLevel+1, 0.0f );

      setAlphaAlways( saveAA * mCurrentIntraDetailLevel / alphaOut );
      render( rdata, mCurrentDetailLevel, mCurrentIntraDetailLevel );
      setAlphaAlways( saveAA );
   }
}

void TSShapeInstance::setMeshForceHidden( const char *meshName, bool hidden )
{
   Vector<MeshObjectInstance>::iterator iter = mMeshObjects.begin();
   for ( ; iter != mMeshObjects.end(); iter++ )
   {
      S32 nameIndex = iter->object->nameIndex;
      const char *name = mShape->names[ nameIndex ];

      if ( dStrcmp( meshName, name ) == 0 )
      {
         iter->forceHidden = hidden;
         return;
      }
   }
}

void TSShapeInstance::setMeshForceHidden( S32 meshIndex, bool hidden )
{
   AssertFatal( meshIndex > -1 && meshIndex < mMeshObjects.size(),
      "TSShapeInstance::setMeshForceHidden - Invalid index!" );
                  
   mMeshObjects[meshIndex].forceHidden = hidden;
}

void TSShapeInstance::render( const TSRenderState &rdata, S32 dl, F32 intraDL )
{
   AssertFatal( dl >= 0 && dl < mShape->details.size(),"TSShapeInstance::render" );

   S32 i;

   const TSDetail * detail = &mShape->details[dl];
   S32 ss = detail->subShapeNum;
   S32 od = detail->objectDetailNum;

   // if we're a billboard detail, draw it and exit
   if ( ss < 0 )
   {
      PROFILE_SCOPE( TSShapeInstance_RenderBillboards );
      
      if ( !rdata.isNoRenderTranslucent() && ( TSLastDetail::smCanShadow || !rdata.getSceneState()->isShadowPass() ) )
         mShape->billboardDetails[ dl ]->render( rdata, mAlphaAlways ? mAlphaAlwaysValue : 1.0f );

      return;
   }

   // run through the meshes   
   S32 start = rdata.isNoRenderNonTranslucent() ? mShape->subShapeFirstTranslucentObject[ss] : mShape->subShapeFirstObject[ss];
   S32 end   = rdata.isNoRenderTranslucent() ? mShape->subShapeFirstTranslucentObject[ss] : mShape->subShapeFirstObject[ss] + mShape->subShapeNumObjects[ss];
   for (i=start; i<end; i++)
   {
      // following line is handy for debugging, to see what part of the shape that it is rendering
      // const char *name = mShape->names[ mMeshObjects[i].object->nameIndex ];
      mMeshObjects[i].render( od, mMaterialList, rdata, mAlphaAlways ? mAlphaAlwaysValue : 1.0f );
   }

   // HH: Added for Tribes 2 compatibility.
   // Tribes 2 turns decals off for a shape it draws over (0x00694493).
   if ( !rdata.getOverrideMaterial() )
      renderDecals( rdata, od, mAlphaAlways ? mAlphaAlwaysValue : 1.0f );
}

// HH: Added for Tribes 2 compatibility.
// Tribes 2 blends every decal by its texture's alpha whatever the material
// says, so one that is not translucent is drawn with a translucent copy.
static BaseMatInstance *getDecalMatInstance( BaseMatInstance *matInst,
                                             const GFXVertexFormat *format )
{
   Material *mat = dynamic_cast<Material*>( matInst->getMaterial() );
   if ( !mat || mat->isTranslucent() )
      return matInst;

   struct Entry { Material *source; BaseMatInstance *decal; };
   static Vector<Entry> sDecalMats;
   for ( U32 i = 0; i < sDecalMats.size(); i++ )
      if ( sDecalMats[i].source == mat )
         return sDecalMats[i].decal;

   Entry entry;
   entry.source = mat;
   entry.decal = matInst;

   Material *decal = MATMGR->allocateAndRegister(
      String::ToString( "TSDecalMaterial%d", sDecalMats.size() + 1 ) );
   if ( decal )
   {
      decal->mAutoGenerated = true;
      decal->mDiffuseMapFilename[0] = mat->mDiffuseMapFilename[0];
      decal->mTranslucent = true;
      decal->mTranslucentBlendOp = Material::LerpAlpha;
      decal->mTranslucentZWrite = false;

      BaseMatInstance *inst = decal->createMatInstance();
      if ( inst && inst->init( MATMGR->getDefaultFeatures(), format ) )
         entry.decal = inst;
      else
         SAFE_DELETE( inst );
   }

   sDecalMats.push_back( entry );
   return entry.decal;
}

// HH: Added for Tribes 2 compatibility.
TSShapeInstance::DecalObjectInstance::DecalObjectInstance()
   : decal(0), frame(-1), mBuiltFrame(-1), mBuiltDetail(-1), mBuiltSkinCount(0)
{
}

// HH: Added for Tribes 2 compatibility.
// A decal frame is a run of primitives over its object's own vertices, with
// a pair of planes placing its texture.  The texture coordinates come from
// the unposed mesh, so a mark stays where it is put as the armour moves.
void TSShapeInstance::renderDecals( const TSRenderState &rdata, S32 od, F32 alpha )
{
   const SceneRenderState *state = rdata.getSceneState();
   if ( !mDecalObjects.size() || !state || state->isShadowPass() ||
        rdata.isNoRenderTranslucent() || !mMaterialList )
      return;

   RenderPassManager *renderPass = state->getRenderPass();

   for ( S32 i = 0; i < mDecalObjects.size(); i++ )
   {
      DecalObjectInstance &inst = mDecalObjects[i];
      if ( inst.frame < 0 )
         continue;

      const TSShape::Decal &decal = mShape->decals[inst.decal];
      if ( od >= decal.numMeshes || decal.objectIndex < 0 ||
           decal.objectIndex >= mMeshObjects.size() )
         continue;

      TSDecalMesh *decalMesh = mShape->decalMeshes[decal.startMeshIndex + od];
      if ( !decalMesh || !decalMesh->targetMesh ||
           inst.frame >= decalMesh->startPrimitive.size() ||
           inst.frame >= decalMesh->texgenS.size() ||
           inst.frame >= decalMesh->texgenT.size() )
         continue;

      MeshObjectInstance &target = mMeshObjects[decal.objectIndex];
      TSMesh *mesh = decalMesh->targetMesh;
      if ( target.forceHidden || target.visible * alpha <= 0.01f ||
           target.getMesh( od ) != mesh || !mesh->mVertexData.isReady() )
         continue;

      if ( inst.mBuiltFrame != inst.frame || inst.mBuiltDetail != od ||
           inst.mBuiltSkinCount != target.mSkinCount ||
           !inst.mVertexBuffer.isValid() )
      {
         inst.mBuiltFrame = inst.frame;
         inst.mBuiltDetail = od;
         inst.mBuiltSkinCount = target.mSkinCount;

         const S32 first = decalMesh->startPrimitive[inst.frame];
         const S32 last = inst.frame + 1 < decalMesh->startPrimitive.size()
                          ? decalMesh->startPrimitive[inst.frame + 1]
                          : decalMesh->primitives.size();

         static Vector<U16> tris;
         tris.clear();
         for ( S32 p = getMax( first, 0 ); p < last && p < decalMesh->primitives.size(); p++ )
         {
            const TSDrawPrimitive &draw = decalMesh->primitives[p];
            if ( draw.start < 0 || draw.start + draw.numElements > decalMesh->indices.size() )
               continue;

            const U16 *idx = &decalMesh->indices[draw.start];
            const S32 n = draw.numElements;
            switch ( draw.matIndex & TSDrawPrimitive::TypeMask )
            {
               case TSDrawPrimitive::Triangles:
                  for ( S32 k = 0; k + 2 < n; k += 3 )
                  {
                     tris.push_back( idx[k] );
                     tris.push_back( idx[k + 1] );
                     tris.push_back( idx[k + 2] );
                  }
                  break;

               case TSDrawPrimitive::Strip:
                  for ( S32 k = 0; k + 2 < n; k++ )
                  {
                     tris.push_back( idx[k + ( k & 1 )] );
                     tris.push_back( idx[k + 1 - ( k & 1 )] );
                     tris.push_back( idx[k + 2] );
                  }
                  break;

               case TSDrawPrimitive::Fan:
                  for ( S32 k = 1; k + 1 < n; k++ )
                  {
                     tris.push_back( idx[0] );
                     tris.push_back( idx[k] );
                     tris.push_back( idx[k + 1] );
                  }
                  break;

               default:
                  break;
            }
         }

         const S32 firstVert = mesh->vertsPerFrame * getMax( target.frame, 0 );
         const Vector<Point3F> *unposed = NULL;
         if ( mesh->getMeshType() == TSMesh::SkinMeshType )
            unposed = &( (TSSkinMesh*)mesh )->batchData.initialVerts;

         const U32 vertSize = mesh->mVertexData.vertSize();
         const U32 count = tris.size();
         bool ok = count > 0;
         for ( U32 v = 0; ok && v < count; v++ )
            ok = firstVert + tris[v] < (S32)mesh->mVertexData.size();

         if ( !ok )
         {
            inst.mVertexBuffer = TSVertexBufferHandle();
            continue;
         }

         if ( !inst.mVertexBuffer.isValid() || inst.mVertexBuffer->mNumVerts != count )
            inst.mVertexBuffer.set( GFX, vertSize, mesh->getVertexFormat(), count,
                                    GFXBufferTypeDynamic );

         const Point4F &planeS = decalMesh->texgenS[inst.frame];
         const Point4F &planeT = decalMesh->texgenT[inst.frame];

         U8 *out = (U8*)inst.mVertexBuffer.lock();
         if ( !out )
         {
            inst.mVertexBuffer = TSVertexBufferHandle();
            continue;
         }

         for ( U32 v = 0; v < count; v++ )
         {
            const S32 at = firstVert + tris[v];
            dMemcpy( out + v * vertSize, &mesh->mVertexData[at], vertSize );

            const Point3F &p = unposed && tris[v] < unposed->size()
                               ? (*unposed)[tris[v]]
                               : mesh->mVertexData[at].vert();
            TSMesh::__TSMeshVertexBase *vert = (TSMesh::__TSMeshVertexBase*)( out + v * vertSize );
            vert->tvert( Point2F( planeS.x * p.x + planeS.y * p.y + planeS.z * p.z + planeS.w,
                                  planeT.x * p.x + planeT.y * p.y + planeT.z * p.z + planeT.w ) );
         }
         inst.mVertexBuffer.unlock();

         if ( !inst.mPrimitiveBuffer.isValid() || inst.mPrimitiveBuffer->mIndexCount != count )
         {
            inst.mPrimitiveBuffer.set( GFX, count, 1, GFXBufferTypeStatic );

            U16 *indices = NULL;
            GFXPrimitive *prims = NULL;
            inst.mPrimitiveBuffer.lock( &indices, &prims );
            for ( U32 v = 0; v < count; v++ )
               indices[v] = (U16)v;
            prims[0].type = GFXTriangleList;
            prims[0].numPrimitives = count / 3;
            prims[0].startIndex = 0;
            prims[0].startVertex = 0;
            prims[0].minIndex = 0;
            prims[0].numVertices = count;
            inst.mPrimitiveBuffer.unlock();
         }
      }

      if ( !inst.mVertexBuffer.isValid() || !inst.mPrimitiveBuffer.isValid() )
         continue;

      BaseMatInstance *matInst = mMaterialList->getMaterialInst( decalMesh->materialIndex & TSDrawPrimitive::MaterialMask );
      if ( matInst )
         matInst = getDecalMatInstance( matInst, mesh->getVertexFormat() );
      matInst = state->getOverrideMaterial( matInst );
      if ( !matInst || !matInst->isValid() )
         continue;

      MatrixF objToWorld( GFX->getWorldMatrix() );
      objToWorld.mul( target.getTransform() );

      MeshRenderInst *ri = renderPass->allocInst<MeshRenderInst>();
      ri->type = RenderPassManager::RIT_Mesh;
      if ( matInst->getMaterial()->isTranslucent() )
      {
         ri->type = RenderPassManager::RIT_Translucent;
         ri->translucentSort = true;
      }
      ri->sortDistSq = ( objToWorld.getPosition() - state->getCameraPosition() ).lenSquared();
      ri->objectToWorld = renderPass->allocUniqueXform( objToWorld );
      ri->worldToCamera = renderPass->allocSharedXform( RenderPassManager::View );
      ri->projection = renderPass->allocSharedXform( RenderPassManager::Projection );
      ri->vertBuff = &inst.mVertexBuffer;
      ri->primBuff = &inst.mPrimitiveBuffer;
      ri->primBuffIndex = 0;
      ri->matInst = matInst;
      ri->defaultKey = matInst->getStateHint();
      ri->defaultKey2 = (U32)(uintptr_t)ri->vertBuff;
      ri->materialHint = rdata.getMaterialHint();
      ri->visibility = target.visible * alpha;
      ri->cubemap = rdata.getCubemap();

      if ( matInst->isForwardLit() && rdata.getLightQuery() )
         rdata.getLightQuery()->getLights( ri->lights, 8 );

      renderPass->addInst( ri );
   }
}

void TSShapeInstance::setCurrentDetail( S32 dl, F32 intraDL )
{
   PROFILE_SCOPE( TSShapeInstance_setCurrentDetail );

   mCurrentDetailLevel = mClamp( dl, -1, mShape->mSmallestVisibleDL );
   mCurrentIntraDetailLevel = intraDL > 1.0f ? 1.0f : (intraDL < 0.0f ? 0.0f : intraDL);

   // Restrict the chosen detail level by cutoff value.
   if ( smNumSkipRenderDetails > 0 && mCurrentDetailLevel >= 0 )
   {
      S32 cutoff = getMin( smNumSkipRenderDetails, mShape->mSmallestVisibleDL );
      if ( mCurrentDetailLevel < cutoff )
      {
         mCurrentDetailLevel = cutoff;
         mCurrentIntraDetailLevel = 1.0f;
      }
   }
}

S32 TSShapeInstance::setDetailFromPosAndScale(  const SceneRenderState *state,
                                                const Point3F &pos, 
                                                const Point3F &scale )
{
   VectorF camVector = pos - state->getDiffuseCameraPosition();
   F32 dist = getMax( camVector.len(), 0.01f );
   F32 invScale = ( 1.0f / getMax( getMax( scale.x, scale.y ), scale.z ) );

   return setDetailFromDistance( state, dist * invScale );
}

S32 TSShapeInstance::setDetailFromDistance( const SceneRenderState *state, F32 scaledDistance )
{
   PROFILE_SCOPE( TSShapeInstance_setDetailFromDistance );

   // For debugging/metrics.
   smLastScaledDistance = scaledDistance;

   // Shortcut if the distance is really close or negative.
   if ( scaledDistance <= 0.0f )
   {
      mShape->mDetailLevelLookup[0].get( mCurrentDetailLevel, mCurrentIntraDetailLevel );
      return mCurrentDetailLevel;
   }

   // The pixel scale is used the linearly scale the lod
   // selection based on the viewport size.
   //
   // The original calculation from TGEA was...
   //
   // pixelScale = viewport.extent.x * 1.6f / 640.0f;
   //
   // Since we now work on the viewport height, assuming
   // 4:3 aspect ratio, we've changed the reference value
   // to 300 to be more compatible with legacy shapes.
   //
   const F32 pixelScale = state->getViewport().extent.y / 300.0f;

   // This is legacy DTS support for older "multires" based
   // meshes.  The original crossbow weapon uses this.
   //
   // If we have more than one detail level and the maxError
   // is non-negative then we do some sort of screen error 
   // metric for detail selection.
   //
   if ( mShape->mUseDetailFromScreenError )
   {
      // The pixel size of 1 meter at the input distance.
      F32 pixelRadius = state->projectRadius( scaledDistance, 1.0f ) * pixelScale;
      static const F32 smScreenError = 5.0f;
      return setDetailFromScreenError( smScreenError / pixelRadius );
   }

   // We're inlining SceneRenderState::projectRadius here to 
   // skip the unnessasary divide by zero protection.
   F32 pixelRadius = ( mShape->radius / scaledDistance ) * state->getWorldToScreenScale().y * pixelScale;
   F32 pixelSize = pixelRadius * smDetailAdjust;

   if (  pixelSize > smSmallestVisiblePixelSize && 
         pixelSize <= mShape->mSmallestVisibleSize )
      pixelSize = mShape->mSmallestVisibleSize + 0.01f;

   // For debugging/metrics.
   smLastPixelSize = pixelSize;

   // Clamp it to an acceptable range for the lookup table.
   U32 index = (U32)mClampF( pixelSize, 0, mShape->mDetailLevelLookup.size() - 1 );

   // Check the lookup table for the detail and intra detail levels.
   mShape->mDetailLevelLookup[ index ].get( mCurrentDetailLevel, mCurrentIntraDetailLevel );

   // Restrict the chosen detail level by cutoff value.
   if ( smNumSkipRenderDetails > 0 && mCurrentDetailLevel >= 0 )
   {
      S32 cutoff = getMin( smNumSkipRenderDetails, mShape->mSmallestVisibleDL );
      if ( mCurrentDetailLevel < cutoff )
      {
         mCurrentDetailLevel = cutoff;
         mCurrentIntraDetailLevel = 1.0f;
      }
   }

   return mCurrentDetailLevel;
}

S32 TSShapeInstance::setDetailFromScreenError( F32 errorTolerance )
{
   PROFILE_SCOPE( TSShapeInstance_setDetailFromScreenError );

   // For debugging/metrics.
   smLastScreenErrorTolerance = errorTolerance;

   // note:  we use 10 time the average error as the metric...this is
   // more robust than the maxError...the factor of 10 is to put average error
   // on about the same scale as maxError.  The errorTOL is how much
   // error we are able to tolerate before going to a more detailed version of the
   // shape.  We look for a pair of details with errors bounding our errorTOL,
   // and then we select an interpolation parameter to tween betwen them.  Ok, so
   // this isn't exactly an error tolerance.  A tween value of 0 is the lower poly
   // model (higher detail number) and a value of 1 is the higher poly model (lower
   // detail number).

   // deal with degenerate case first...
   // if smallest detail corresponds to less than half tolerable error, then don't even draw
   F32 prevErr;
   if ( mShape->mSmallestVisibleDL < 0 )
      prevErr = 0.0f;
   else
      prevErr = 10.0f * mShape->details[mShape->mSmallestVisibleDL].averageError * 20.0f;
   if ( mShape->mSmallestVisibleDL < 0 || prevErr < errorTolerance )
   {
      // draw last detail
      mCurrentDetailLevel = mShape->mSmallestVisibleDL;
      mCurrentIntraDetailLevel = 0.0f;
      return mCurrentDetailLevel;
   }

   // this function is a little odd
   // the reason is that the detail numbers correspond to
   // when we stop using a given detail level...
   // we search the details from most error to least error
   // until we fit under the tolerance (errorTOL) and then
   // we use the next highest detail (higher error)
   for (S32 i = mShape->mSmallestVisibleDL; i >= 0; i-- )
   {
      F32 err0 = 10.0f * mShape->details[i].averageError;
      if ( err0 < errorTolerance )
      {
         // ok, stop here

         // intraDL = 1 corresponds to fully this detail
         // intraDL = 0 corresponds to the next lower (higher number) detail
         mCurrentDetailLevel = i;
         mCurrentIntraDetailLevel = 1.0f - (errorTolerance - err0) / (prevErr - err0);
         return mCurrentDetailLevel;
      }
      prevErr = err0;
   }

   // get here if we are drawing at DL==0
   mCurrentDetailLevel = 0;
   mCurrentIntraDetailLevel = 1.0f;
   return mCurrentDetailLevel;
}

//-------------------------------------------------------------------------------------
// Object (MeshObjectInstance & PluginObjectInstance) render methods
//-------------------------------------------------------------------------------------

void TSShapeInstance::ObjectInstance::render( S32, TSMaterialList *, const TSRenderState &rdata, F32 alpha )
{
   AssertFatal(0,"TSShapeInstance::ObjectInstance::render:  no default render method.");
}

void TSShapeInstance::MeshObjectInstance::render(  S32 objectDetail, 
                                                   TSMaterialList *materials, 
                                                   const TSRenderState &rdata, 
                                                   F32 alpha )
{
   PROFILE_SCOPE( TSShapeInstance_MeshObjectInstance_render );

   if ( forceHidden || ( ( visible * alpha ) <= 0.01f ) )
      return;

   TSMesh *mesh = getMesh(objectDetail);
   if ( !mesh )
      return;

   const MatrixF &transform = getTransform();

   if ( rdata.getCuller() )
   {
      Box3F box( mesh->getBounds() );
      transform.mul( box );
      if ( rdata.getCuller()->isCulled( box ) )
         return;
   }

   GFX->pushWorldMatrix();
   GFX->multWorld( transform );

   mesh->setFade( visible * alpha );

   // HH: Added for Tribes 2 compatibility.
   // The blaster's sight is a cycle of material frames and stood still.
   mesh->setFrames( frame, matFrame );

   // Pass a hint to the mesh that time has advanced and that the
   // skin is dirty and needs to be updated.  This should result
   // in the skin only updating once per frame in most cases.
   const U32 currTime = Sim::getCurrentTime();
   bool isSkinDirty = currTime != mLastTime;

   mesh->render(  materials, 
                  rdata, 
                  isSkinDirty,
                  *mTransforms, 
                  mVertexBuffer,
                  mPrimitiveBuffer );

   // HH: Added for Tribes 2 compatibility.
   if ( isSkinDirty )
      mSkinCount++;

   // Update the last render time.
   mLastTime = currTime;

   GFX->popWorldMatrix();
}

TSShapeInstance::MeshObjectInstance::MeshObjectInstance() 
   : meshList(0), object(0), frame(0), matFrame(0),
     visible(1.0f), forceHidden(false), mLastTime( 0 ), mSkinCount( 0 )
{
}

void TSShapeInstance::prepCollision()
{
   PROFILE_SCOPE( TSShapeInstance_PrepCollision );

   // Iterate over all our meshes and call prepCollision on them...
   for(S32 i=0; i<mShape->meshes.size(); i++)
   {
      if(mShape->meshes[i])
         mShape->meshes[i]->prepOpcodeCollision();
   }
}

