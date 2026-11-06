// HH: Added for Tribes 2 compatibility.
#ifndef _COMMANDERICONDATA_H_
#define _COMMANDERICONDATA_H_

#ifndef _DATABLOCKST2_H_
#include "T3D/gameBase/dataBlocksT2.h"
#endif
#ifndef _GFXTEXTUREHANDLE_H_
#include "gfx/gfxTextureHandle.h"
#endif

/// What the commander map draws an object with: a base image and the ones it
/// lays over or swaps in for it when the object is selected or under the
/// mouse.  Each is a string, "type image overlay modulate", where type is
/// "static" or "animation"; an animation names a .dml list of frames and
/// adds how it plays and the milliseconds a frame lasts.
class CommanderIconData : public DataBlockT2
{
   typedef DataBlockT2 Parent;

public:
   enum State
   {
      Base,
      Active,
      Inactive,
      Select,
      Hilight,
      NumStates,
   };

   enum Play
   {
      Looping,
      FlipFlop,
      OneShot,
   };

   struct Image
   {
      Vector<GFXTexHandle> frames;
      /// Drawn over the base image rather than in place of it.
      bool overlay;
      /// Tinted: by the object's team for the base, by the map's selected or
      /// hilighted colour for those.
      bool modulate;
      Play play;
      U32  frameTime;

      Image() : overlay(false), modulate(false), play(Looping), frameTime(100) {}
      bool valid() const { return !frames.empty(); }
      GFXTextureObject *frame(U32 time) const;
   };

   StringTableEntry mImageNames[NumStates];

   CommanderIconData();

   bool unpackData(BitStream *stream, U32 classId);
   bool preload(bool server, String &errorStr);

   /// The images, loaded the first time they are asked for.
   const Image &getImage(State state);

   static void initPersistFields();

   DECLARE_CONOBJECT(CommanderIconData);

private:
   Image mImages[NumStates];
   bool  mLoaded;

   void loadImages();
};

#endif
