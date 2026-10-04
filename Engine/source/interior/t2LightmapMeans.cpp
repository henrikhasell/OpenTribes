// HH: Added for Tribes 2 compatibility.
//
// How bright each interior's lightmaps actually are, asked of the running
// client rather than of a picture.
//
// A .dif holds only the base bake, and split by surface it is dim -- exterior
// means of 25 to 49 of 255 across the six buildings Dust to Dust loads, with
// between half and three quarters of each one's surfaces under 8.  The shader
// multiplies the texture by that, so the buildings draw black.  Tribes 2
// computes the rest per mission and caches it, and the cache is a *delta*:
// blInteriorProxy::setPersistInfo installs it with
// `combine(base, cached, GFXTOPAdd)`, so an installed lightmap is the base bake
// plus what the mission's lighting pass added.
//
// That makes the question numeric, which matters because the alternative is a
// screenshot and a screenshot of an unfocused window is of whatever frame it
// last chose to draw.  Retail's own caches average 84.4 of 255 over Talus's 69
// maps with none near black; the base bakes average under 50.  So a mean is
// the measurement that separates the lit case from the unlit one, and it is
// the same quantity in both.
//
// Reads the per-instance handles rather than the base ones, because the base
// set is shared by every instance of a building and is exactly what the
// install replaces.
#include "platform/platform.h"
#include "console/console.h"
#include "console/engineAPI.h"
#include "console/simBase.h"
#include "core/strings/stringFunctions.h"
#include "gfx/bitmap/gBitmap.h"
#include "interior/interior.h"
#include "interior/interiorInstance.h"
#include "interior/interiorLMManager.h"
#include "scene/sceneObject.h"
#include "scene/sceneContainer.h"

static bool bitmapMean( GBitmap *bmp, F64 &mean )
{
   if ( !bmp )
      return false;

   const U32 w = bmp->getWidth();
   const U32 h = bmp->getHeight();
   if ( !w || !h )
      return false;

   F64 total = 0.0;
   for ( U32 y = 0; y < h; y++ )
      for ( U32 x = 0; x < w; x++ )
      {
         ColorI c;
         bmp->getColor( x, y, c );
         total += ( F64 )( c.red + c.green + c.blue ) / 3.0;
      }

   mean = total / ( F64 )( w * h );
   return true;
}

// Asked of the client container rather than of the Sim tree.  A ghost is a
// registered object but it is not a child of RootGroup, so walking the groups
// finds none of them -- which is what the first version of this did, reporting
// zero interiors in a scene that had just lit ten of them.  This is the query
// SceneLighting::light itself uses to find the same objects.
static void collectInteriors( Vector<InteriorInstance *> &out )
{
   SimpleQueryList objects;
   gClientContainer.findObjects( InteriorObjectType,
                                 &SimpleQueryList::insertionCallback, &objects );

   for ( SceneObject **itr = objects.mList.begin(); itr != objects.mList.end(); itr++ )
      if ( InteriorInstance *inst = dynamic_cast<InteriorInstance *>( *itr ) )
         out.push_back( inst );
}

DefineEngineFunction( t2LightmapMeans, const char *, (), ,
   "@brief Report the mean brightness of every interior's installed lightmaps.\n\n"
   "One line per InteriorInstance in the client scene, echoed as T2LM, then a "
   "T2LM-DONE line carrying the number of interiors, the number of maps and the "
   "mean of the per-map means.  A map is counted as near black below a mean of "
   "8, which is the threshold the base bakes were measured against.\n\n"
   "Returns the summary line.\n\n"
   "@internal" )
{
   Vector<InteriorInstance *> insts;
   collectInteriors( insts );

   U32 interiors = 0, maps = 0, black = 0;
   F64 total = 0.0;

   for ( U32 n = 0; n < insts.size(); n++ )
   {
      InteriorInstance *inst = insts[ n ];
      const LM_HANDLE instanceHandle = inst->getLMHandle();

      U32 mine = 0, mineBlack = 0, slots = 0, live = 0;
      F64 mineTotal = 0.0;

      for ( U32 d = 0; d < inst->getNumDetailLevels(); d++ )
      {
         Interior *detail = inst->getDetailLevel( d );
         if ( !detail )
            continue;

         Vector<GFXTexHandle> &handles =
            gInteriorLMManager.getHandles( detail->getLMHandle(), instanceHandle );

         slots += handles.size();

         for ( U32 i = 0; i < handles.size(); i++ )
         {
            if ( !handles[ i ].isValid() )
               continue;
            live++;

            // A lightmap that reached the GPU has had its bitmap freed, so a
            // null here is an uploaded texture rather than a missing map --
            // which is why maps can be zero while slots and live are not, and
            // why all three are reported instead of the mean alone.
            F64 mean = 0.0;
            if ( !bitmapMean( handles[ i ]->getBitmap(), mean ) )
               continue;

            mine++;
            mineTotal += mean;
            if ( mean < 8.0 )
               mineBlack++;
         }
      }

      interiors++;
      maps += mine;
      black += mineBlack;
      total += mineTotal;

      Con::printf( "T2LM %s slots=%d live=%d measured=%d mean=%.1f nearBlack=%d",
                   inst->getInteriorFileName(), slots, live, mine,
                   mine ? mineTotal / ( F64 )mine : 0.0, mineBlack );
   }

   char *out = Con::getReturnBuffer( 128 );
   dSprintf( out, 128, "T2LM-DONE interiors=%d maps=%d meanOfMeans=%.1f nearBlack=%d",
             interiors, maps, maps ? total / ( F64 )maps : 0.0, black );
   Con::printf( "%s", out );
   return out;
}
