// HH: Added for Tribes 2 compatibility.
#ifndef _SHELLSTATEBITMAP_H_
#define _SHELLSTATEBITMAP_H_

#ifndef _GUITYPES_H_
#include "gui/core/guiTypes.h"
#endif

class ShellStateBitmap
{
public:
   enum
   {
      MaxStates = 3,   ///< Resting, showing, under the mouse.
      MaxPieces = 3,   ///< Left cap, middle, right cap.
   };

private:
   GFXTexHandle mTexture;
   StringTableEntry mSource;

   RectI mCell[ MaxPieces ][ MaxStates ];
   S32 mPieces;

public:
   ShellStateBitmap();

   void update( GuiControlProfile *profile );

   void load( const char *name );

   bool isValid() const { return mPieces > 0; }

   S32 getPieceCount() const { return mPieces; }

   S32 getCapWidth() const
   {
      return mPieces >= MaxPieces ? mCell[ 0 ][ 0 ].extent.x + mCell[ 2 ][ 0 ].extent.x : 0;
   }

   S32 getStateHeight() const { return mPieces ? mCell[ 0 ][ 0 ].extent.y : 0; }

   void draw( S32 state, const RectI &bounds ) const;

   void drawCell( S32 state, const Point2I &at ) const;

private:
   void measure( GBitmap *bitmap );
};

#endif // _SHELLSTATEBITMAP_H_
