// HH: Added for Tribes 2 compatibility.
#ifndef _SURFACELIGHTINGT2_H_
#define _SURFACELIGHTINGT2_H_

#ifndef _MPOINT3_H_
#include "math/mPoint3.h"
#endif
#ifndef _COLOR_H_
#include "core/color.h"
#endif
#ifndef _SIMOBJECT_H_
#include "console/simObject.h"
#endif

class SceneObject;
class LightInfo;
class InteriorInstance;
class TerrainBlock;
struct RayInfo;

/// Tribes 2 lights a shape by the surface beneath it (0x0058fa80).
///
/// Inside an interior -- one above it and one below, each within 100 --
/// the sun is put out, and the shape is lit by an ambient light the
/// colour of the interior's lightmap under it (0x0058f8f3).  Within 10
/// of a terrain, the sun is dimmed by how bright the terrain's lightmap
/// is under it (0x0058f9be).  Anywhere else, and on terrain, the sun's
/// colour is cut by its own ambient (0x00573bb2).  The colour eases
/// towards a new surface's at 0.2 every 100ms.
class SurfaceLightingT2
{
public:
   SurfaceLightingT2();

   /// The light to put in the sun's place for obj this frame, or NULL to
   /// leave the lights as they are.
   LightInfo* getLight( SceneObject *obj );

private:
   enum Source
   {
      SourceInterior,
      SourceTerrain,
   };

   void _query( const Point3F &pos );
   void _sampleInterior( InteriorInstance *instance, const RayInfo &ray );
   bool _sampleTerrain( TerrainBlock *terrain, const RayInfo &ray );
   bool _getColor( ColorF *color );

   /// Where the surface was last asked about; it is asked again only once
   /// the shape has moved.
   Point3F mQueryPos;
   bool mQueried;

   bool mUseInfo;
   Source mSource;

   ColorF mDefaultColor;
   ColorF mAlarmColor;

   /// Set while the interior under the shape has an alarm state.
   SimObjectPtr<SimObject> mInterior;

   bool mHasLastColor;
   ColorF mLastColor;
   U32 mLastTime;

   U32 mFrame;
   LightInfo *mLight;
};

#endif // _SURFACELIGHTINGT2_H_
