// HH: imported from TGE
// (not a copy: the same class ported to the GFX layer, storms dropped)

#include "platform/platform.h"
#include "environment/sky.h"

#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "core/stream/bitStream.h"
#include "core/stream/fileStream.h"
#include "core/volume.h"
#include "gfx/gfxDebugEvent.h"
#include "gfx/gfxDrawUtil.h"
#include "gfx/gfxTransformSaver.h"
#include "materials/materialList.h"
#include "math/mathIO.h"
#include "renderInstance/renderPassManager.h"
#include "scene/sceneManager.h"
#include "scene/sceneRenderState.h"

#define HORIZON         0.0f
#define OFFSET_HEIGHT   60.0f
#define FOG_BAN_DETAIL  8
#define RAD             (2 * M_PI)

IMPLEMENT_CO_NETOBJECT_V1(Sky);

bool Sky::smCloudsOn    = true;
bool Sky::smSkyOn       = true;
S32  Sky::smNumCloudsOn = MAX_NUM_LAYERS;

F32 Cloud::mRadius;


//---------------------------------------------------------------------------
// Winding clipper, from the engine this came from.  Clips a convex polygon
// against a plane, keeping the front half.
//---------------------------------------------------------------------------
static void sgUtil_clipToPlane( Point3F *points, U32 &rNumPoints, const PlaneF &rPlane )
{
   S32 start = -1;
   for( U32 i = 0; i < rNumPoints; i++ )
   {
      if( rPlane.whichSide( points[i] ) == PlaneF::Front )
      {
         start = i;
         break;
      }
   }

   // Nothing was in front of the plane...
   if( start == -1 )
   {
      rNumPoints = 0;
      return;
   }

   Point3F finalPoints[128];
   U32 numFinalPoints = 0;

   U32 baseStart = start;
   U32 end       = ( start + 1 ) % rNumPoints;

   while( end != baseStart )
   {
      const Point3F &rStartPoint = points[start];
      const Point3F &rEndPoint   = points[end];

      PlaneF::Side fSide = rPlane.whichSide( rStartPoint );
      PlaneF::Side eSide = rPlane.whichSide( rEndPoint );

      S32 code = fSide * 3 + eSide;
      switch( code )
      {
      case 4:   // f f
      case 3:   // f o
      case 1:   // o f
      case 0:   // o o
         // No clipping required
         finalPoints[numFinalPoints++] = points[start];
         start = end;
         end   = ( end + 1 ) % rNumPoints;
         break;

      case 2:   // f b
         {
            // Emit the front point, insert the intersection, and advance to
            // the first point that is in front or on.
            finalPoints[numFinalPoints++] = points[start];

            Point3F vector = rEndPoint - rStartPoint;
            F32 t = -( rPlane.distToPlane( rStartPoint ) / mDot( rPlane, vector ) );

            finalPoints[numFinalPoints++] = rStartPoint + ( vector * t );

            U32 endSeek = ( end + 1 ) % rNumPoints;
            while( rPlane.whichSide( points[endSeek] ) == PlaneF::Back )
               endSeek = ( endSeek + 1 ) % rNumPoints;

            end   = endSeek;
            start = ( end + ( rNumPoints - 1 ) ) % rNumPoints;

            const Point3F &rNewStartPoint = points[start];
            const Point3F &rNewEndPoint   = points[end];

            vector = rNewEndPoint - rNewStartPoint;
            t = -( rPlane.distToPlane( rNewStartPoint ) / mDot( rPlane, vector ) );

            points[start] = rNewStartPoint + ( vector * t );
         }
         break;

      case -1:  // o b
         {
            finalPoints[numFinalPoints++] = points[start];

            U32 endSeek = ( end + 1 ) % rNumPoints;
            while( rPlane.whichSide( points[endSeek] ) == PlaneF::Back )
               endSeek = ( endSeek + 1 ) % rNumPoints;

            end   = endSeek;
            start = ( end + ( rNumPoints - 1 ) ) % rNumPoints;

            const Point3F &rNewStartPoint = points[start];
            const Point3F &rNewEndPoint   = points[end];

            Point3F vector = rNewEndPoint - rNewStartPoint;
            F32 t = -( rPlane.distToPlane( rNewStartPoint ) / mDot( rPlane, vector ) );

            points[start] = rNewStartPoint + ( vector * t );
         }
         break;

      case -2:  // b f
      case -3:  // b o
      case -4:  // b b
         start = end;
         end   = ( end + 1 ) % rNumPoints;
         break;

      default:
         AssertFatal( false, "sgUtil_clipToPlane - bad clip code" );
      }
   }

   finalPoints[numFinalPoints++] = points[start];

   AssertFatal( numFinalPoints >= 3, "Error, this shouldn't happen!" );
   AssertFatal( numFinalPoints <= 128, "Error, too many points!" );

   rNumPoints = numFinalPoints;
   for( U32 i = 0; i < rNumPoints; i++ )
      points[i] = finalPoints[i];
}


//---------------------------------------------------------------------------
Sky::Sky()
{
   mNumCloudLayers = 0;
   mTypeMask |= EnvironmentObjectType;
   mNetFlags.set( Ghostable | ScopeAlways );

   mVisibleDistance = 250;
   mFogDistance     = 250;
   mFogColor.set( 0.6f, 0.6f, 0.6f, 1.0f );
   mNumFogVolumes   = 0;
   mFogLine         = 0.0f;
   mRadius          = 1.0f;
   mLastVisDisMod   = 0.0f;

   mMaterialListName = StringTable->insert( "" );

   for( U32 j = 0; j < MaxFogVolumes; j++ )
   {
      mFogVolumes[j].visibleDistance = -1;
      mFogVolumes[j].percentage = 1.0f;
      mFogVolumes[j].minHeight = 0.0f;
      mFogVolumes[j].maxHeight = 0.0f;
      mFogVolumes[j].color.set( mFogColor.red, mFogColor.green, mFogColor.blue, 1.0f );
      mFogStorm[j] = false;
   }

   mSkyTexturesOn   = true;
   mRenderBoxBottom = false;
   mNoRenderBans    = false;
   mSolidFillColor.set( 0.0f, 0.0f, 0.0f, 0.0f );

   for( U32 i = 0; i < MAX_NUM_LAYERS; i++ )
   {
      mCloudHeight[i] = 0.0f;
      mCloudSpeed[i]  = 0.0001f;
   }

   mWindVelocity.set( 1.0f, 0.0f, 0.0f );
   mWindDir.set( 0.0f, 1.0f );
   mEffectPrecip = true;

   mRealFogColor.set( 128, 128, 128, 255 );
   mRealSkyColor.set( 0, 0, 0, 255 );
}

//---------------------------------------------------------------------------
void Sky::initPersistFields()
{
   addGroup( "Media" );
   addField( "materialList", TypeFilename, Offset( mMaterialListName, Sky ),
      "The .dml listing the six sky box faces, the environment map and the cloud layers." );
   endGroup( "Media" );

   addGroup( "Clouds" );
   addField( "cloudHeightPer", TypeF32, Offset( mCloudHeight, Sky ), MAX_NUM_LAYERS,
      "Height of each cloud layer as a fraction of the sky radius." );
   addField( "cloudSpeed1", TypeF32, Offset( mCloudSpeed[0], Sky ), "" );
   addField( "cloudSpeed2", TypeF32, Offset( mCloudSpeed[1], Sky ), "" );
   addField( "cloudSpeed3", TypeF32, Offset( mCloudSpeed[2], Sky ), "" );
   endGroup( "Clouds" );

   addGroup( "Visibility" );
   addField( "visibleDistance", TypeF32, Offset( mVisibleDistance, Sky ), "" );
   endGroup( "Visibility" );

   addGroup( "Fog" );
   addField( "fogDistance", TypeF32, Offset( mFogDistance, Sky ),
      "Distance at which the haze ramp starts; it reaches full at visibleDistance." );
   addField( "fogColor", TypeColorF, Offset( mFogColor, Sky ), "" );
   addField( "fogStorm1", TypeBool, Offset( mFogStorm[0], Sky ), "" );
   addField( "fogStorm2", TypeBool, Offset( mFogStorm[1], Sky ), "" );
   addField( "fogStorm3", TypeBool, Offset( mFogStorm[2], Sky ), "" );
   addField( "fogVolume1", TypePoint3F, Offset( mFogVolumes[0], Sky ),
      "visibleDistance, minHeight and maxHeight of a fog slab." );
   addField( "fogVolume2", TypePoint3F, Offset( mFogVolumes[1], Sky ), "" );
   addField( "fogVolume3", TypePoint3F, Offset( mFogVolumes[2], Sky ), "" );
   addField( "fogVolumeColor1", TypeColorF, Offset( mFogVolumes[0].color, Sky ), "" );
   addField( "fogVolumeColor2", TypeColorF, Offset( mFogVolumes[1].color, Sky ), "" );
   addField( "fogVolumeColor3", TypeColorF, Offset( mFogVolumes[2].color, Sky ), "" );
   endGroup( "Fog" );

   addGroup( "Wind" );
   addField( "windVelocity", TypePoint3F, Offset( mWindVelocity, Sky ), "" );
   addField( "windEffectPrecipitation", TypeBool, Offset( mEffectPrecip, Sky ), "" );
   endGroup( "Wind" );

   addGroup( "Misc" );
   addField( "SkySolidColor", TypeColorF, Offset( mSolidFillColor, Sky ), "" );
   addField( "useSkyTextures", TypeBool, Offset( mSkyTexturesOn, Sky ), "" );
   addField( "renderBottomTexture", TypeBool, Offset( mRenderBoxBottom, Sky ), "" );
   addField( "noRenderBans", TypeBool, Offset( mNoRenderBans, Sky ), "" );
   endGroup( "Misc" );

   Parent::initPersistFields();
}

void Sky::consoleInit()
{
   Con::addVariable( "pref::CloudsOn", TypeBool, &smCloudsOn, "" );
   Con::addVariable( "pref::NumCloudLayers", TypeS32, &smNumCloudsOn, "" );
   Con::addVariable( "pref::SkyOn", TypeBool, &smSkyOn, "" );
}

//---------------------------------------------------------------------------
bool Sky::onAdd()
{
   if( !Parent::onAdd() )
      return false;

   mObjBox.minExtents.set( -1e9, -1e9, -1e9 );
   mObjBox.maxExtents.set(  1e9,  1e9,  1e9 );
   resetWorldBox();

   // Find out how many fog layers we have.
   for( mNumFogVolumes = 0; mNumFogVolumes < MaxFogVolumes; mNumFogVolumes++ )
      if( mFogVolumes[mNumFogVolumes].visibleDistance == -1 ||
          mFogVolumes[mNumFogVolumes].visibleDistance == 0 )
         break;

   // Storm fog layers are off by default, others are on.
   for( U32 i = 0; i < mNumFogVolumes; ++i )
      mFogVolumes[i].percentage = mFogStorm[i] ? 0.0f : 1.0f;

   if( isClientObject() )
   {
      if( !loadDml() )
         return false;

      initStateBlocks();
      initSkyData();
   }
   else
      setWindVelocity( mWindVelocity );

   addToScene();
   setVisibility();
   return true;
}

//---------------------------------------------------------------------------
void Sky::onRemove()
{
   removeFromScene();
   Parent::onRemove();
}

//---------------------------------------------------------------------------
void Sky::initStateBlocks()
{
   // The sky box and the bans sit at the far edge of the world and are drawn
   // before anything else, so neither reads nor writes depth.  The faces are
   // clamped because the seam between two of them is a texture edge; the
   // clouds wrap because their coordinates scroll past 1 forever.
   GFXStateBlockDesc d;
   d.setCullMode( GFXCullNone );
   d.setZReadWrite( false, false );
   d.samplersDefined = true;
   d.samplers[0] = GFXSamplerStateDesc::getClampLinear();
   mSkyStateBlock = GFX->createStateBlock( d );

   GFXStateBlockDesc b;
   b.setCullMode( GFXCullNone );
   b.setZReadWrite( false, false );
   b.setBlend( true, GFXBlendSrcAlpha, GFXBlendInvSrcAlpha );
   mBanStateBlock = GFX->createStateBlock( b );

   GFXStateBlockDesc c;
   c.setCullMode( GFXCullNone );
   c.setZReadWrite( false, false );
   c.setBlend( true, GFXBlendSrcAlpha, GFXBlendInvSrcAlpha );
   c.samplersDefined = true;
   c.samplers[0] = GFXSamplerStateDesc::getWrapLinear();
   mCloudStateBlock = GFX->createStateBlock( c );
}

//---------------------------------------------------------------------------
bool Sky::loadDml()
{
   if( !mMaterialListName || !mMaterialListName[0] )
      return true;

   char path[1024];
   dStrcpy( path, mMaterialListName );
   char *p = dStrrchr( path, '/' );
   if( p )
      *p = 0;
   else
      path[0] = 0;

   mNumCloudLayers = 0;

   MaterialList materials;
   FileStream stream;
   if( !stream.open( mMaterialListName, Torque::FS::File::Read ) )
   {
      Con::warnf( "Sky material list is missing: %s", mMaterialListName );
      return false;
   }

   materials.read( stream );
   stream.close();

   for( S32 x = 0; x < 6 && x < (S32)materials.size(); ++x )
   {
      String name = String::ToString( "%s/%s", path, materials.getMaterialName( x ).c_str() );
      mSkyHandle[x].set( name, &GFXDefaultStaticDiffuseProfile, "Sky" );
   }

   for( S32 x = 0; x + CloudMaterialOffset < (S32)materials.size() && x < MAX_NUM_LAYERS;
        ++x, ++mNumCloudLayers )
   {
      String name = String::ToString( "%s/%s", path,
         materials.getMaterialName( x + CloudMaterialOffset ).c_str() );
      GFXTexHandle tex( name, &GFXDefaultStaticDiffuseProfile, "SkyCloud" );
      mCloudLayer[x].setTexture( tex );
   }

   return true;
}

//---------------------------------------------------------------------------
void Sky::initSkyData()
{
   calcPoints();
   mWindDir = Point2F( mWindVelocity.x, mWindVelocity.y );
   mWindDir.normalize();
   for( S32 i = 0; i < MAX_NUM_LAYERS; ++i )
   {
      mCloudLayer[i].setHeights( mCloudHeight[i], mCloudHeight[i] - 0.05f, 0.05f );
      mCloudLayer[i].setSpeed( mWindDir * mCloudSpeed[i] );
      mCloudLayer[i].setPoints();
   }
   setVisibility();
}

//---------------------------------------------------------------------------
void Sky::calcPoints()
{
   S32 x, y, xval = 1, yval = -1;
   F32 textureDem;

   F32 visDisMod = mVisibleDistance;
   if( gClientSceneGraph )
      visDisMod = gClientSceneGraph->getVisibleDistance();
   mRadius = visDisMod * 0.95f;

   Cloud::setRadius( mRadius );

   Point3F tpt( 1, 1, 1 );
   tpt.normalize( mRadius );

   mPoints[0] = mPoints[4] = Point3F( -tpt.x, -tpt.y, tpt.z );
   mPoints[5] = mPoints[9] = Point3F( -tpt.x, -tpt.y, -tpt.z );

   for( x = 1; x < 4; ++x )
   {
      mPoints[x]     = Point3F( tpt.x * xval, tpt.y * yval, tpt.z );
      mPoints[x + 5] = Point3F( tpt.x * xval, tpt.y * yval, -tpt.z );

      if( yval > 0 && xval > 0 )
         xval *= -1;
      if( yval < 0 )
         yval *= -1;
   }

   mFogLine = 0.0f;
   for( U32 i = 0; i < mNumFogVolumes; ++i )
      mFogLine = ( mFogVolumes[i].maxHeight > mFogLine ) ? mFogVolumes[i].maxHeight : mFogLine;

   textureDem = 512;
   if( mSkyHandle[0].isValid() )
      textureDem = mSkyHandle[0].getWidth();

   for( y = 0; y < 2; ++y )
      for( x = 0; x < 2; ++x )
      {
         mTexCoord[x + ( y * 2 )].set( x, y );
         mTexCoord[x + ( y * 2 )] *= ( textureDem - 1.0f ) / textureDem;
         mTexCoord[x + ( y * 2 )] += Point2F( 0.5f / textureDem, 0.5f / textureDem );
      }

   mSpherePt = mSkyBoxPt = mPoints[1];
   mSpherePt.set( mSpherePt.x, 0.0f, mSpherePt.z );
   mSpherePt.normalize( mSkyBoxPt.x );
   mTopCenterPt.set( 0.0f, 0.0f, mSkyBoxPt.z );
}

//---------------------------------------------------------------------------
void Sky::setVisibility()
{
   mRealFogColor.red   = S32( mCeil( mFogColor.red   * 255.0f ) );
   mRealFogColor.green = S32( mCeil( mFogColor.green * 255.0f ) );
   mRealFogColor.blue  = S32( mCeil( mFogColor.blue  * 255.0f ) );
   mRealFogColor.alpha = 255;

   mRealSkyColor.red   = S32( mSolidFillColor.red   * 255.0f );
   mRealSkyColor.green = S32( mSolidFillColor.green * 255.0f );
   mRealSkyColor.blue  = S32( mSolidFillColor.blue  * 255.0f );

   // The sky box is clipped at the height the fog reaches and nothing is drawn
   // below that line, so whatever the canvas was cleared to shows through from
   // the horizon up to the bans.  The original engine never cleared the colour
   // buffer at all and let the gap keep the previous frame, which worked only
   // because the gap is, by construction, solid fog.  Clearing to the fog
   // colour gets the same picture without depending on what was there before.
   extern ColorI gCanvasClearColor;
   gCanvasClearColor = mRealFogColor;

   SceneManager *scene = isClientObject() ? gClientSceneGraph : gServerSceneGraph;
   if( !scene )
      return;

   scene->setVisibleDistance( mVisibleDistance );

   // The two halves of the original fog, handed to the shaders as they were
   // written rather than reshaped into this engine's exponential curve.
   //
   //   haze   a quadratic ramp from fogDistance out to visibleDistance
   //   band   the length of the eye ray inside a fog slab, over the slab's
   //          own visible distance
   //
   // density stays at zero so the exponential term contributes nothing; a
   // level driven by a LevelInfo instead leaves hazeScale and bandVisDist at
   // zero and gets only the exponential, exactly as before.
   FogData fog;
   fog.color = mFogColor;
   fog.density = 0.0f;
   fog.densityOffset = mFogDistance;
   fog.atmosphereHeight = 0.0f;
   fog.hazeScale = ( mVisibleDistance > mFogDistance )
      ? 1.0f / ( mVisibleDistance - mFogDistance )
      : 0.0f;

   if( mNumFogVolumes > 0 )
   {
      fog.bandMin     = mFogVolumes[0].minHeight;
      fog.bandMax     = mFogVolumes[0].maxHeight;
      fog.bandVisDist = mFogVolumes[0].visibleDistance * mFogVolumes[0].percentage;
   }

   if( mNumFogVolumes > 1 )
      Con::warnf( "Sky: %d fog volumes, only the first reaches the shaders; the "
                  "rest still set the height the sky is fogged to.", mNumFogVolumes );

   scene->setFogData( fog );
}

//---------------------------------------------------------------------------
void Sky::setWindVelocity( const Point3F &vel )
{
   mWindVelocity = vel;
   if( isServerObject() )
      setMaskBits( WindMask );
}

Point3F Sky::getWindVelocity()
{
   return mWindVelocity;
}

//---------------------------------------------------------------------------
void Sky::updateVisibility()
{
   setVisibility();
   setMaskBits( VisibilityMask );
}

void Sky::inspectPostApply()
{
   Parent::inspectPostApply();
   updateVisibility();
}

//---------------------------------------------------------------------------
U32 Sky::packUpdate( NetConnection *conn, U32 mask, BitStream *stream )
{
   U32 retMask = Parent::packUpdate( conn, mask, stream );

   if( stream->writeFlag( mask & InitMask ) )
   {
      stream->writeString( mMaterialListName );

      stream->write( mFogColor.red );
      stream->write( mFogColor.green );
      stream->write( mFogColor.blue );

      stream->write( mNumFogVolumes );
      for( U32 i = 0; i < mNumFogVolumes; i++ )
      {
         stream->write( mFogVolumes[i].visibleDistance );
         stream->write( mFogVolumes[i].minHeight );
         stream->write( mFogVolumes[i].maxHeight );
         stream->write( mFogVolumes[i].percentage );
         stream->write( mFogVolumes[i].color.red );
         stream->write( mFogVolumes[i].color.green );
         stream->write( mFogVolumes[i].color.blue );
      }

      stream->write( mFogDistance );
      stream->write( mVisibleDistance );

      for( U32 i = 0; i < MAX_NUM_LAYERS; i++ )
      {
         stream->write( mCloudHeight[i] );
         stream->write( mCloudSpeed[i] );
      }

      stream->write( mSolidFillColor.red );
      stream->write( mSolidFillColor.green );
      stream->write( mSolidFillColor.blue );

      stream->writeFlag( mSkyTexturesOn );
      stream->writeFlag( mRenderBoxBottom );
      stream->writeFlag( mNoRenderBans );

      mathWrite( *stream, mWindVelocity );
   }

   return retMask;
}

//---------------------------------------------------------------------------
void Sky::unpackUpdate( NetConnection *conn, BitStream *stream )
{
   Parent::unpackUpdate( conn, stream );

   if( stream->readFlag() )
   {
      char buf[1024];
      stream->readString( buf );
      mMaterialListName = StringTable->insert( buf );
      loadDml();

      stream->read( &mFogColor.red );
      stream->read( &mFogColor.green );
      stream->read( &mFogColor.blue );
      mFogColor.alpha = 1.0f;

      stream->read( &mNumFogVolumes );
      for( U32 i = 0; i < mNumFogVolumes && i < MaxFogVolumes; i++ )
      {
         stream->read( &mFogVolumes[i].visibleDistance );
         stream->read( &mFogVolumes[i].minHeight );
         stream->read( &mFogVolumes[i].maxHeight );
         stream->read( &mFogVolumes[i].percentage );
         stream->read( &mFogVolumes[i].color.red );
         stream->read( &mFogVolumes[i].color.green );
         stream->read( &mFogVolumes[i].color.blue );
         mFogVolumes[i].color.alpha = 1.0f;
      }

      stream->read( &mFogDistance );
      stream->read( &mVisibleDistance );

      for( U32 i = 0; i < MAX_NUM_LAYERS; i++ )
      {
         stream->read( &mCloudHeight[i] );
         stream->read( &mCloudSpeed[i] );
      }

      stream->read( &mSolidFillColor.red );
      stream->read( &mSolidFillColor.green );
      stream->read( &mSolidFillColor.blue );
      mSolidFillColor.alpha = 1.0f;

      mSkyTexturesOn   = stream->readFlag();
      mRenderBoxBottom = stream->readFlag();
      mNoRenderBans    = stream->readFlag();

      mathRead( *stream, &mWindVelocity );

      if( isProperlyAdded() )
      {
         initStateBlocks();
         initSkyData();
      }
   }
}

//---------------------------------------------------------------------------
void Sky::prepRenderImage( SceneRenderState *state )
{
   if( state->isShadowPass() )
      return;

   ObjectRenderInst *ri = state->getRenderPass()->allocInst<ObjectRenderInst>();
   ri->renderDelegate.bind( this, &Sky::_renderObject );
   ri->type = RenderPassManager::RIT_Sky;
   ri->defaultKey = 10;
   ri->defaultKey2 = 0;
   state->getRenderPass()->addInst( ri );
}

void Sky::_renderObject( ObjectRenderInst *ri, SceneRenderState *state, BaseMatInstance *mi )
{
   GFXDEBUGEVENT_SCOPE( Sky_RenderObject, ColorF::WHITE );
   GFXTransformSaver saver;

   // Everything below is in sky space: axis-aligned with the world, origin
   // following the camera.  The scene's view transform is in the world matrix
   // rather than the view matrix -- guiTSControl sets the view matrix to the
   // identity and SkyBox reads GFX->getWorldMatrix() as the scene view -- so
   // this has to compose onto what is there.  Setting it instead replaces the
   // camera's rotation with a translation, and the sky then turns with the
   // player.
   MatrixF camPosMat( MatrixF::Identity );
   camPosMat.setPosition( state->getCameraPosition() );
   GFX->multWorld( camPosMat );

   render( state->getCameraPosition() );
}

//---------------------------------------------------------------------------
void Sky::render( const Point3F &camPos )
{
   F32 banHeights[2] = { -( mSpherePt.z - 1 ), -( mSpherePt.z - 1 ) };
   F32 alphaBan[2] = { 0.0f, 0.0f };
   F32 depthInFog = 0.0f;

   if( gClientSceneGraph )
   {
      F32 currentVisDis = gClientSceneGraph->getVisibleDistance();
      if( mLastVisDisMod != currentVisDis )
      {
         calcPoints();
         for( S32 i = 0; i < MAX_NUM_LAYERS; ++i )
            mCloudLayer[i].setPoints();

         mLastVisDisMod = currentVisDis;
      }
   }

   if( mNumFogVolumes )
      depthInFog = -( camPos.z - mFogLine );

   // Calculates alpha values and ban heights
   if( depthInFog > 0.0f )
      calcAlphas_Heights( camPos.z, banHeights, alphaBan, depthInFog );
   else // Not in fog so setup default values
   {
      alphaBan[0] = 0.0f;
      alphaBan[1] = 0.0f;
      banHeights[0] = HORIZON;
      banHeights[1] = banHeights[0] + OFFSET_HEIGHT;
   }

   // If the lower ban is at the top of the box then no clipping plane is needed
   if( banHeights[0] >= mSpherePt.z )
      banHeights[0] = banHeights[1] = mSpherePt.z;

   // Renders the 6 sides of the sky box
   if( alphaBan[1] < 1.0f || mNumFogVolumes == 0 )
      renderSkyBox( banHeights[0], alphaBan[1] );

   // If completely fogged out then no need to render
   if( alphaBan[1] < 1.0f || depthInFog < 0.0f )
   {
      if( smCloudsOn && smSkyOn )
      {
         F32 ang   = mAtan2( banHeights[0], mSkyBoxPt.x );
         F32 xyval = mSin( ang );
         F32 zval  = mCos( ang );
         PlaneF planes[4];
         planes[0] = PlaneF(  xyval,  0.0f,   zval, 0.0f );
         planes[1] = PlaneF( -xyval,  0.0f,   zval, 0.0f );
         planes[2] = PlaneF(  0.0f,   xyval,  zval, 0.0f );
         planes[3] = PlaneF(  0.0f,  -xyval,  zval, 0.0f );

         S32 numRender = ( smNumCloudsOn > mNumCloudLayers ) ? mNumCloudLayers : smNumCloudsOn;
         for( S32 x = 0; x < numRender; ++x )
            mCloudLayer[x].render( Sim::getCurrentTime(), planes, mCloudStateBlock );
      }

      if( !mNoRenderBans )
      {
         Point3F banPoints[2][MAX_BAN_POINTS];
         Point3F cornerPoints[MAX_BAN_POINTS];

         calcBans( banHeights, banPoints, cornerPoints );
         renderBans( alphaBan, banHeights, banPoints, cornerPoints );
      }
   }
}

//---------------------------------------------------------------------------
void Sky::calcAlphas_Heights( F32 zCamPos, F32 *banHeights, F32 *alphaBan, F32 depthInFog )
{
   F32 sideA, lower = 0.0f, upper = 0.0f;
   F32 visValue, visDis = 0.0f;
   F32 visRatio, ratioVal, setVis;

   visDis = setVis = mVisibleDistance +
      ( mFogVolumes[mNumFogVolumes - 1].visibleDistance - mVisibleDistance ) *
         mFogVolumes[mNumFogVolumes - 1].percentage;

   for( S32 x = 0; x < (S32)mNumFogVolumes - 1; ++x )
   {
      F32 distance = mVisibleDistance +
         ( mFogVolumes[x].visibleDistance - mVisibleDistance ) * mFogVolumes[x].percentage;
      if( distance < setVis )
      {
         visValue = ( zCamPos < mFogVolumes[x].minHeight )
            ? mFogVolumes[x].maxHeight - mFogVolumes[x].minHeight
            : mFogVolumes[x].maxHeight - zCamPos;
         if( visValue > 0.0f )
         {
            ratioVal = setVis / distance;
            visRatio = visValue / distance;
            visDis -= ( distance * visRatio ) * ratioVal;
         }
      }
   }

   // Calculate upper height
   if( visDis > 0.0f )
      upper = ( mSkyBoxPt.x * depthInFog ) / ( visDis * 0.2f );

   banHeights[1] = mSpherePt.z;

   if( upper < mSpherePt.z )
   {
      banHeights[1] = upper;
      if( banHeights[1] < OFFSET_HEIGHT )
         banHeights[1] = OFFSET_HEIGHT + HORIZON;
   }

   if( visDis > depthInFog )
   {
      sideA = mSqrt( ( visDis * visDis ) - ( depthInFog * depthInFog ) );
      lower = ( mSkyBoxPt.x * depthInFog ) / sideA;

      // Calculate lower height
      banHeights[0] = mSpherePt.z;
      if( lower < mSpherePt.z )
         banHeights[0] = lower;

      if( banHeights[0] == mSpherePt.z && banHeights[1] == mSpherePt.z )
      {
         F32 temp = ( ( lower - mSpherePt.z ) * ( sideA / depthInFog ) );
         if( temp <= mSkyBoxPt.x )
            alphaBan[1] = temp / mSkyBoxPt.x;
         else
            alphaBan[1] = 1.0f;

         alphaBan[0] = 1.0f;
      }
      else
      {
         alphaBan[0] = banHeights[0] / mSpherePt.z;
         alphaBan[1] = 0.0f;
      }
   }
   else
   {
      alphaBan[1] = alphaBan[0] = 1.0f;
      banHeights[0] = banHeights[1] = mSpherePt.z;
   }

   banHeights[0] *= mFogVolumes[0].percentage;
   banHeights[1] *= mFogVolumes[0].percentage;

   if( banHeights[1] < OFFSET_HEIGHT )
      banHeights[1] = OFFSET_HEIGHT + HORIZON;
}

//---------------------------------------------------------------------------
void Sky::setRenderPoints( Point3F *renderPoints, S32 index )
{
   renderPoints[0] = mPoints[index];
   renderPoints[1] = mPoints[index + 1];
   renderPoints[2] = mPoints[index + 6];
   renderPoints[3] = mPoints[index + 5];
}

void Sky::calcTexCoords( Point2F *texCoords, Point3F *renderPoints, S32 index )
{
   for( S32 x = 0; x < 4; ++x )
      texCoords[x] = mTexCoord[x];

   S32 length = (S32)( mFabs( mPoints[index].z ) + mFabs( mPoints[index + 5].z ) );
   F32 per = mPoints[index].z - renderPoints[3].z;

   texCoords[3].y = texCoords[2].y = ( per / length );
}

//---------------------------------------------------------------------------
void Sky::renderSkyBox( F32 lowerBanHeight, F32 alphaBanUpper )
{
   S32 side, index = 0, val;
   U32 numPoints;
   Point3F renderPoints[128];
   Point2F texCoords[4];

   ColorI color = mSkyTexturesOn && smSkyOn ? ColorI::WHITE : mRealSkyColor;
   color.alpha = 255;

   GFX->setStateBlock( mSkyStateBlock );


   for( side = 0; side < ( mRenderBoxBottom ? 6 : 5 ); ++side )
   {
      if( !( lowerBanHeight != mSpherePt.z || ( side == 4 && alphaBanUpper < 1.0f ) ) )
         continue;
      if( !mSkyHandle[side].isValid() )
         continue;

      GFXVertexBufferHandle<GFXVertexPCT> vb( GFX, 4, GFXBufferTypeVolatile );
      GFXVertexPCT *v = vb.lock();
      if( !v )
         return;

      if( side < 4 )
      {
         numPoints = 4;
         setRenderPoints( renderPoints, index );
         if( !mNoRenderBans )
            sgUtil_clipToPlane( renderPoints, numPoints,
                                PlaneF( 0.0f, 0.0f, 1.0f, -lowerBanHeight ) );
         if( !numPoints )
         {
            vb.unlock();
            ++index;
            continue;
         }

         calcTexCoords( texCoords, renderPoints, index );

         // The original walked the quad 0,1,3,2 against points 0,1,2,3.
         static const S32 texOrder[4] = { 0, 1, 3, 2 };
         for( S32 i = 0; i < 4; ++i )
         {
            v[i].point = renderPoints[i];
            v[i].texCoord = texCoords[texOrder[i]];
            v[i].color = color;
         }
         ++index;
      }
      else
      {
         S32 first = 3;
         val = -1;
         if( side == 5 )
         {
            first = 5;
            val = 1;
         }

         static const S32 texOrder[4] = { 0, 1, 3, 2 };
         for( S32 i = 0; i < 4; ++i )
         {
            v[i].point = mPoints[first + ( i * val )];
            v[i].texCoord = mTexCoord[texOrder[i]];
            v[i].color = color;
         }
      }

      vb.unlock();

      GFX->setTexture( 0, mSkyHandle[side] );
      GFX->setVertexBuffer( vb );
      GFX->setupGenericShaders( GFXDevice::GSModColorTexture );
      GFX->drawPrimitive( GFXTriangleFan, 0, 2 );
   }
}

//---------------------------------------------------------------------------
void Sky::calcBans( F32 *banHeights, Point3F banPoints[][MAX_BAN_POINTS], Point3F *cornerPoints )
{
   F32 incRad = RAD / F32( FOG_BAN_DETAIL * 2 );
   MatrixF ban;
   Point4F point;
   S32 index, x;
   F32 height = banHeights[0];

   F32 value = banHeights[0] / mSkyBoxPt.z;
   F32 mulVal = -( mSqrt( 1 - ( value * value ) ) ); // lowerBan multiple
   index = 0;

   // Calculates the upper and lower bans
   for( x = 0; x < 2; ++x )
   {
      for( F32 angle = 0.0f; angle <= RAD + incRad; angle += incRad )
      {
         ban.set( Point3F( 0.0f, 0.0f, angle ) );
         point.set( mulVal * mSkyBoxPt.x, 0.0f, 0.0f, 1.0f );
         ban.mul( point );
         banPoints[x][index++].set( point.x, point.y, height );
      }
      height = banHeights[1];
      value  = banHeights[1] / mSkyBoxPt.x;
      mulVal = -( mSqrt( 1 - ( value * value ) ) ); // upperBan multiple
      index = 0;
   }

   // Calculates the filler points needed between the lower ban and the clipping plane
   index = 2;
   cornerPoints[0].set( mPoints[3].x, mPoints[3].y, banHeights[0] - 1 );
   cornerPoints[1].set( mPoints[3].x, 0.0f, banHeights[0] - 1 );

   for( x = 0; x < ( FOG_BAN_DETAIL / 2 ) + 1; ++x )
      cornerPoints[index++] = banPoints[0][x];
   cornerPoints[index].set( 0.0f, mPoints[3].y, banHeights[0] - 1 );
}

//---------------------------------------------------------------------------
void Sky::renderBans( F32 *alphaBan, F32 *banHeights, Point3F banPoints[][MAX_BAN_POINTS],
                      Point3F *cornerPoints )
{
   const U32 ringCount = FOG_BAN_DETAIL * 2 + 1;

   ColorI solid( mRealFogColor.red, mRealFogColor.green, mRealFogColor.blue, 255 );
   ColorI outer( mRealFogColor.red, mRealFogColor.green, mRealFogColor.blue,
                 U8( alphaBan[0] * 255 ) );
   ColorI inner( mRealFogColor.red, mRealFogColor.green, mRealFogColor.blue,
                 U8( alphaBan[1] * 255 ) );

   GFX->setStateBlock( mBanStateBlock );


   // Renders the side bans
   if( banHeights[0] < mSpherePt.z )
   {
      GFXVertexBufferHandle<GFXVertexPC> vb( GFX, ringCount * 2, GFXBufferTypeVolatile );
      GFXVertexPC *v = vb.lock();
      if( !v )
         return;

      for( U32 x = 0; x < ringCount; ++x )
      {
         v[x * 2].point     = banPoints[0][x];
         v[x * 2].color     = solid;
         v[x * 2 + 1].point = banPoints[1][x];
         v[x * 2 + 1].color = outer;
      }
      vb.unlock();

      GFX->setVertexBuffer( vb );
      GFX->setupGenericShaders( GFXDevice::GSColor );
      GFX->drawPrimitive( GFXTriangleStrip, 0, ringCount * 2 - 2 );
   }

   // Renders the top ban
   {
      GFXVertexBufferHandle<GFXVertexPC> vb( GFX, ringCount + 1, GFXBufferTypeVolatile );
      GFXVertexPC *v = vb.lock();
      if( !v )
         return;

      v[0].point = mTopCenterPt;
      v[0].color = inner;
      for( U32 x = 0; x < ringCount; ++x )
      {
         v[x + 1].point = banPoints[1][x];
         v[x + 1].color = outer;
      }
      vb.unlock();

      GFX->setVertexBuffer( vb );
      GFX->setupGenericShaders( GFXDevice::GSColor );
      GFX->drawPrimitive( GFXTriangleFan, 0, ringCount - 1 );
   }

   // Renders the filler, four times around
   {
      const U32 fillCount = FOG_BAN_DETAIL;

      GFXVertexBufferHandle<GFXVertexPC> vb( GFX, fillCount, GFXBufferTypeVolatile );
      GFXVertexPC *v = vb.lock();
      if( !v )
         return;

      for( U32 x = 0; x < fillCount; ++x )
      {
         v[x].point = cornerPoints[x];
         v[x].color = solid;
      }
      vb.unlock();

      GFX->setVertexBuffer( vb );

      for( S32 side = 0; side < 4; ++side )
      {
         MatrixF rot( EulerF( 0.0f, 0.0f, mDegToRad( 90.0f * side ) ) );
         GFX->pushWorldMatrix();
         GFX->multWorld( rot );
         GFX->setupGenericShaders( GFXDevice::GSColor );
         GFX->drawPrimitive( GFXTriangleFan, 0, fillCount - 2 );
         GFX->popWorldMatrix();
      }
   }
}


//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// Cloud
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
Cloud::Cloud()
{
   mBaseOffset.set( 0, 0 );
   mTextureScale.set( 1, 1 );
   mCenterHeight = 0.5f;
   mInnerHeight = 0.45f;
   mEdgeHeight = 0.4f;
   mLastTime = 0;
   mOffset = 0;
   mSpeed.set( 1, 1 );
   for( S32 i = 0; i < 25; ++i )
      mAlpha[i] = 1.0f;
   mRadius = 1.0f;
}

//---------------------------------------------------------------------------
void Cloud::updateCoord()
{
   mBaseOffset += mSpeed * mOffset;
   if( mSpeed.x < 0 )
      mBaseOffset.x -= mCeil( mBaseOffset.x );
   else
      mBaseOffset.x -= mFloor( mBaseOffset.x );
   if( mSpeed.y < 0 )
      mBaseOffset.y -= mCeil( mBaseOffset.y );
   else
      mBaseOffset.y -= mFloor( mBaseOffset.y );
}

//---------------------------------------------------------------------------
void Cloud::setHeights( F32 cHeight, F32 iHeight, F32 eHeight )
{
   mCenterHeight = cHeight;
   mInnerHeight = iHeight;
   mEdgeHeight = eHeight;
}

void Cloud::setTexture( GFXTexHandle texture )
{
   if( texture.isValid() )
      mCloudHandle = texture;
}

void Cloud::setSpeed( Point2F speed )
{
   mSpeed = speed;
}

//---------------------------------------------------------------------------
void Cloud::setPoints()
{
   S32 x, y;
   F32 xyDiff = ( mRadius - -mRadius ) / 4;
   F32 cDis  = mRadius * mCenterHeight,
       upDis = mRadius * mInnerHeight,
       edgeZ = mRadius * mEdgeHeight;

   F32 zValue[25] = { edgeZ, edgeZ, edgeZ, edgeZ, edgeZ,
                      edgeZ, upDis, upDis, upDis, edgeZ,
                      edgeZ, upDis, cDis,  upDis, edgeZ,
                      edgeZ, upDis, upDis, upDis, edgeZ,
                      edgeZ, edgeZ, edgeZ, edgeZ, edgeZ };

   for( y = 0; y < 5; ++y )
      for( x = 0; x < 5; ++x )
         mPoints[y * 5 + x].set( -mRadius + ( xyDiff * x ), mRadius - ( xyDiff * y ),
                                 zValue[y * 5 + x] );

   // Used to modify the corners of the cloud layers to make them one plane.
   Point3F vec = ( mPoints[5] + ( ( mPoints[1] - mPoints[5] ) * 0.5f ) ) - mPoints[6];
   mPoints[0] = mPoints[6] + ( vec * 2.0f );

   vec = ( mPoints[9] + ( ( mPoints[3] - mPoints[9] ) * 0.5f ) ) - mPoints[8];
   mPoints[4] = mPoints[8] + ( vec * 2.0f );

   vec = ( mPoints[21] + ( ( mPoints[15] - mPoints[21] ) * 0.5f ) ) - mPoints[16];
   mPoints[20] = mPoints[16] + ( vec * 2.0f );

   vec = ( mPoints[23] + ( ( mPoints[19] - mPoints[23] ) * 0.5f ) ) - mPoints[18];
   mPoints[24] = mPoints[18] + ( vec * 2.0f );

   calcAlpha();
}

//---------------------------------------------------------------------------
void Cloud::calcAlpha()
{
   for( S32 i = 0; i < 25; ++i )
   {
      mAlpha[i] = 1.3f - ( ( mPoints[i] - Point3F( 0, 0, mPoints[i].z ) ).len() ) / mRadius;
      if( mAlpha[i] < 0.4f )
         mAlpha[i] = 0.0f;
      else if( mAlpha[i] > 0.8f )
         mAlpha[i] = 1.0f;
   }
}

//---------------------------------------------------------------------------
void Cloud::setRenderPoints( Point3F *renderPoints, Point2F *renderTexPoints,
                             F32 *renderAlpha, S32 index )
{
   static const S32 offset[4] = { 0, 5, 6, 1 };
   for( S32 x = 0; x < 4; ++x )
   {
      renderPoints[x] = mPoints[index + offset[x]];
      renderTexPoints[x] = mTexCoords[index + offset[x]];
      renderAlpha[x] = mAlpha[index + offset[x]];
   }
}

//---------------------------------------------------------------------------
void Cloud::render( U32 currentTime, PlaneF *planes, GFXStateBlock *sb )
{
   if( !mCloudHandle.isValid() )
      return;

   mOffset = 1.0f;
   if( mLastTime != 0 )
      mOffset = ( currentTime - mLastTime ) / 32.0f;
   mLastTime = (F32)currentTime;

   updateCoord();

   for( S32 x = 0; x < 5; x++ )
      for( S32 y = 0; y < 5; y++ )
         mTexCoords[y * 5 + x].set( x * mTextureScale.x + mBaseOffset.x,
                                    y * mTextureScale.y - mBaseOffset.y );

   GFX->setStateBlock( sb );
   GFX->setTexture( 0, mCloudHandle );

   U32 numPoints;
   Point3F renderPoints[128];
   Point2F renderTexPoints[128];
   F32 renderAlpha[128];

   for( S32 i = 0; i < 4; ++i )
   {
      S32 start = i * 5;
      for( S32 j = 0; j < 4; ++j )
      {
         numPoints = 4;
         setRenderPoints( renderPoints, renderTexPoints, renderAlpha, start );
         for( S32 k = 0; k < 4; ++k )
            clipToPlane( renderPoints, renderTexPoints, renderAlpha, numPoints, planes[k] );

         if( numPoints >= 3 )
         {
            GFXVertexBufferHandle<GFXVertexPCT> vb( GFX, numPoints, GFXBufferTypeVolatile );
            GFXVertexPCT *v = vb.lock();
            if( v )
            {
               for( U32 k = 0; k < numPoints; ++k )
               {
                  v[k].point = renderPoints[k];
                  v[k].texCoord = renderTexPoints[k];
                  v[k].color = ColorI( 255, 255, 255, U8( renderAlpha[k] * 255 ) );
               }
               vb.unlock();

               GFX->setVertexBuffer( vb );
               GFX->setupGenericShaders( GFXDevice::GSModColorTexture );
               GFX->drawPrimitive( GFXTriangleFan, 0, numPoints - 2 );
            }
         }

         ++start;
      }
   }
}

//---------------------------------------------------------------------------
void Cloud::clipToPlane( Point3F *points, Point2F *texPoints, F32 *alphaPoints,
                         U32 &rNumPoints, const PlaneF &rPlane )
{
   S32 start = -1;
   for( U32 i = 0; i < rNumPoints; i++ )
   {
      if( rPlane.whichSide( points[i] ) == PlaneF::Front )
      {
         start = i;
         break;
      }
   }

   // Nothing was in front of the plane...
   if( start == -1 )
   {
      rNumPoints = 0;
      return;
   }

   U32 numFinalPoints = 0;
   Point3F finalPoints[128];
   Point2F finalTexPoints[128];
   F32 finalAlpha[128];

   U32 baseStart = start;
   U32 end       = ( start + 1 ) % rNumPoints;

   while( end != baseStart )
   {
      const Point3F &rStartPoint = points[start];
      const Point3F &rEndPoint   = points[end];

      const Point2F &rStartTexPoint = texPoints[start];
      const Point2F &rEndTexPoint   = texPoints[end];

      PlaneF::Side fSide = rPlane.whichSide( rStartPoint );
      PlaneF::Side eSide = rPlane.whichSide( rEndPoint );

      S32 code = fSide * 3 + eSide;
      switch( code )
      {
      case 4:   // f f
      case 3:   // f o
      case 1:   // o f
      case 0:   // o o
         finalAlpha[numFinalPoints] = alphaPoints[start];
         finalPoints[numFinalPoints] = points[start];
         finalTexPoints[numFinalPoints++] = texPoints[start];

         start = end;
         end   = ( end + 1 ) % rNumPoints;
         break;

      case 2:   // f b
         {
            finalAlpha[numFinalPoints] = alphaPoints[start];
            finalPoints[numFinalPoints] = points[start];
            finalTexPoints[numFinalPoints++] = texPoints[start];

            Point3F vector = rEndPoint - rStartPoint;
            F32 t = -( rPlane.distToPlane( rStartPoint ) / mDot( rPlane, vector ) );

            finalAlpha[numFinalPoints] =
               alphaPoints[start] + ( ( alphaPoints[end] - alphaPoints[start] ) * t );
            finalPoints[numFinalPoints] = rStartPoint + ( vector * t );

            Point2F texVec = rEndTexPoint - rStartTexPoint;
            finalTexPoints[numFinalPoints++] = rStartTexPoint + ( texVec * t );

            U32 endSeek = ( end + 1 ) % rNumPoints;
            while( rPlane.whichSide( points[endSeek] ) == PlaneF::Back )
               endSeek = ( endSeek + 1 ) % rNumPoints;

            end   = endSeek;
            start = ( end + ( rNumPoints - 1 ) ) % rNumPoints;

            const Point3F &rNewStartPoint = points[start];
            const Point3F &rNewEndPoint   = points[end];

            const Point2F &rNewStartTexPoint = texPoints[start];
            const Point2F &rNewEndTexPoint   = texPoints[end];

            vector = rNewEndPoint - rNewStartPoint;
            t = -( rPlane.distToPlane( rNewStartPoint ) / mDot( rPlane, vector ) );

            alphaPoints[start] =
               alphaPoints[start] + ( ( alphaPoints[end] - alphaPoints[start] ) * t );
            points[start] = rNewStartPoint + ( vector * t );

            texVec = rNewEndTexPoint - rNewStartTexPoint;
            texPoints[start] = rNewStartTexPoint + ( texVec * t );
         }
         break;

      case -1:  // o b
         {
            finalAlpha[numFinalPoints] = alphaPoints[start];
            finalPoints[numFinalPoints] = points[start];
            finalTexPoints[numFinalPoints++] = texPoints[start];

            U32 endSeek = ( end + 1 ) % rNumPoints;
            while( rPlane.whichSide( points[endSeek] ) == PlaneF::Back )
               endSeek = ( endSeek + 1 ) % rNumPoints;

            end   = endSeek;
            start = ( end + ( rNumPoints - 1 ) ) % rNumPoints;

            const Point3F &rNewStartPoint = points[start];
            const Point3F &rNewEndPoint   = points[end];

            const Point2F &rNewStartTexPoint = texPoints[start];
            const Point2F &rNewEndTexPoint   = texPoints[end];

            Point3F vector = rNewEndPoint - rNewStartPoint;
            F32 t = -( rPlane.distToPlane( rNewStartPoint ) / mDot( rPlane, vector ) );

            alphaPoints[start] =
               alphaPoints[start] + ( ( alphaPoints[end] - alphaPoints[start] ) * t );
            points[start] = rNewStartPoint + ( vector * t );

            Point2F texVec = rNewEndTexPoint - rNewStartTexPoint;
            texPoints[start] = rNewStartTexPoint + ( texVec * t );
         }
         break;

      case -2:  // b f
      case -3:  // b o
      case -4:  // b b
         start = end;
         end   = ( end + 1 ) % rNumPoints;
         break;

      default:
         AssertFatal( false, "Cloud::clipToPlane - bad clip code" );
      }
   }

   finalAlpha[numFinalPoints] = alphaPoints[start];
   finalPoints[numFinalPoints] = points[start];
   finalTexPoints[numFinalPoints++] = texPoints[start];

   rNumPoints = numFinalPoints;
   for( U32 i = 0; i < rNumPoints; i++ )
   {
      points[i] = finalPoints[i];
      texPoints[i] = finalTexPoints[i];
      alphaPoints[i] = finalAlpha[i];
   }
}


//---------------------------------------------------------------------------
DefineEngineMethod( Sky, getWindVelocity, const char *, (), ,
   "Returns the wind velocity as an \"x y z\" string." )
{
   Point3F vel = object->getWindVelocity();
   String ret = String::ToString( "%g %g %g", vel.x, vel.y, vel.z );
   char *buf = Con::getReturnBuffer( ret.length() + 1 );
   dStrcpy( buf, ret.c_str() );
   return buf;
}

DefineEngineMethod( Sky, setWindVelocity, void, ( F32 x, F32 y, F32 z ), ,
   "Sets the wind velocity." )
{
   if( object->isClientObject() )
      return;

   object->setWindVelocity( Point3F( x, y, z ) );
}
