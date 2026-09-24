#include "platform/platform.h"
#include "gui/shell/shellButtonCtrl.h"

#include "console/console.h"
#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"

IMPLEMENT_CONOBJECT( ShellBitmapButton );

ConsoleDocClass( ShellBitmapButton,
   "@brief A button skinned the way Tribes 2's shell skins one.\n\n"

   "Draws one band of its profile's art across itself, stretching what is "
   "between the end caps, and centres the label over it.\n\n"

   "@ingroup GuiShell"
);

ShellBitmapButton::ShellBitmapButton()
{
   mSimpleStyle = false;
}

void ShellBitmapButton::initPersistFields()
{
   addField( "simpleStyle", TypeBool, Offset( mSimpleStyle, ShellBitmapButton ),
      "Draw the art and the label only, without the frame a shell button otherwise carries." );

   Parent::initPersistFields();
}

bool ShellBitmapButton::onWake()
{
   if ( !Parent::onWake() )
      return false;

   mArt.update( mProfile );
   return true;
}

S32 ShellBitmapButton::artState()
{
   if ( !mActive )
      return 0;
   if ( mDepressed || mStateOn )
      return 1;
   if ( mMouseOver )
      return 2;

   return 0;
}

void ShellBitmapButton::onRender( Point2I offset, const RectI &updateRect )
{
   mArt.update( mProfile );

   GFXDrawUtil *draw = GFX->getDrawUtil();

   if ( mArt.isValid() )
   {
      draw->clearBitmapModulation();

      // simpleStyle is a different drawing altogether rather than a variation
      // on one: the state is drawn at the size it was drawn, where the control
      // starts, with no caps and nothing stretched.
      if ( mSimpleStyle )
         mArt.drawCell( artState(), offset );
      else
         mArt.draw( artState(), RectI( offset, getExtent() ) );
   }

   if ( mProfile->mFont && mButtonText && mButtonText[ 0 ] )
   {
      ColorI color = mProfile->mFontColor;
      if ( !mActive )
         color = mProfile->mFontColorNA;
      else if ( mDepressed || mStateOn )
         color = mProfile->mFontColorSEL;
      else if ( mMouseOver )
         color = mProfile->mFontColorHL;

      draw->setBitmapModulation( color );
      renderJustifiedText( offset + mProfile->mTextOffset,
                           getExtent() - mProfile->mTextOffset, mButtonText );
      draw->clearBitmapModulation();
   }

   renderChildControls( offset, updateRect );
}

//-----------------------------------------------------------------------------
IMPLEMENT_CONOBJECT( ShellTabGroupCtrl );

ConsoleDocClass( ShellTabGroupCtrl,
   "@brief The row of tabs across the top of Tribes 2's launch bar.\n\n"

   "The tabs belong to the control rather than being child controls: script "
   "adds one per screen the player opens and removes it again when they close "
   "it.  Selecting a tab calls onSelect, which is where the screen is put on "
   "the canvas.\n\n"

   "@ingroup GuiShell"
);

ShellTabGroupCtrl::ShellTabGroupCtrl()
{
   VECTOR_SET_ASSOCIATION( mTabs );
   VECTOR_SET_ASSOCIATION( mSets );

   mTabSpacing = 0;
   mMaxTabWidth = 0;
   mGlowOffset = 0;
   mStretchToFit = false;

   mMinTabWidth = 0;
   mTabThickness = 0;

   mSelected = -1;
   mMouseOver = -1;

   setExtent( 300, 32 );
}

void ShellTabGroupCtrl::initPersistFields()
{
   addField( "tabSpacing", TypeS32, Offset( mTabSpacing, ShellTabGroupCtrl ),
      "Gap left between one tab and the next." );
   addField( "maxTabWidth", TypeS32, Offset( mMaxTabWidth, ShellTabGroupCtrl ),
      "How wide one tab is allowed to grow." );
   addField( "glowOffset", TypeS32, Offset( mGlowOffset, ShellTabGroupCtrl ),
      "Where along the tab its attention marker sits." );
   addField( "stretchToFit", TypeBool, Offset( mStretchToFit, ShellTabGroupCtrl ),
      "Divide the whole row between the tabs, ignoring maxTabWidth." );

   Parent::initPersistFields();
}

bool ShellTabGroupCtrl::onWake()
{
   if ( !Parent::onWake() )
      return false;

   // The set every tab belongs to unless script says otherwise.  It is added
   // rather than special-cased so that the profile is just another set, and it
   // is added here rather than in the constructor because the profile is not
   // resolved until now -- by which time script's own onAdd has already
   // registered any sets of its own, which is why this one is not first.
   if ( mProfile->mBitmapBase && mProfile->mBitmapBase[ 0 ] )
      addSet( 0, mProfile->mBitmapBase, mProfile->mFontColor,
              mProfile->mFontColorHL, mProfile->mFontColorSEL );

   // The narrowest a tab may be is both its caps and room for one letter --
   // a tab squeezed past that is two caps touching with nothing between them.
   mMinTabWidth = 0;

   if ( mSets.size() && mSets[ 0 ].art.isValid() )
      mMinTabWidth = mSets[ 0 ].art.getCapWidth();

   if ( mProfile->mFont )
      mMinTabWidth += mProfile->mFont->getStrWidth( "W" );

   // A cap smaller than that is raised to it rather than fighting it.
   if ( mMaxTabWidth > 0 && mMaxTabWidth < mMinTabWidth )
      mMaxTabWidth = mMinTabWidth;

   // Across the bar a tab is the art's height less the glow's margin.  The art
   // carries that margin as transparent pixels along its outside edges -- in
   // gui/shll_horztabbutton the first seven rows and the first and last seven
   // columns of a resting tab are clear, and it is the lit state that fills
   // them -- so a tab set glowOffset inside the bar has its face where the bar
   // is and its glow room outside it.
   const S32 artThickness = mSets.size() ? mSets[ 0 ].art.getStateHeight() : 0;

   mTabThickness = getMax( artThickness - mGlowOffset, 0 );

   return true;
}

RectI ShellTabGroupCtrl::tabRect( S32 along, S32 size ) const
{
   // The glow's margin comes off every edge the row does not run along: the
   // tabs start that far in and stop that far short, which is what leaves the
   // art room to overhang the bar without being drawn outside it.
   return horizontal()
      ? RectI( mGlowOffset + along, mGlowOffset, size, mTabThickness )
      : RectI( mGlowOffset, mGlowOffset + along, mTabThickness, size );
}

S32 ShellTabGroupCtrl::findSet( S32 id ) const
{
   for ( S32 i = 0; i < mSets.size(); i++ )
      if ( mSets[ i ].id == id )
         return i;

   return -1;
}

const ShellTabGroupCtrl::Set *ShellTabGroupCtrl::setFor( const Tab &tab ) const
{
   if ( !mSets.size() )
      return NULL;

   const S32 index = findSet( tab.set );
   return &mSets[ index >= 0 ? index : 0 ];
}

bool ShellTabGroupCtrl::addSet( S32 id, const char *bitmap, const ColorI &fontColor,
                                const ColorI &fontColorHL, const ColorI &fontColorSEL )
{
   if ( findSet( id ) != -1 )
      return false;

   mSets.increment();

   Set &set = mSets.last();
   set.id = id;
   set.fontColor = fontColor;
   set.fontColorHL = fontColorHL;
   set.fontColorSEL = fontColorSEL;
   set.art.load( bitmap );

   setUpdate();
   return true;
}

void ShellTabGroupCtrl::removeSet( S32 index )
{
   if ( index >= 0 && index < mSets.size() )
   {
      mSets.erase( index );
      setUpdate();
   }
}

void ShellTabGroupCtrl::clearTabSets()
{
   mSets.clear();
   setUpdate();
}

S32 ShellTabGroupCtrl::tabExtent( S32 *remainder ) const
{
   if ( remainder )
      *remainder = 0;

   const S32 count = mTabs.size();
   if ( !count )
      return 0;

   // The glow is taken off the room before it is divided, not after: the art
   // is allowed to overhang the bar by that much at each end.
   const S32 span = ( horizontal() ? getWidth() : getHeight() ) - mGlowOffset * 2;

   // The spacing comes off every tab rather than off the gaps between them,
   // which leaves the last tab's worth of it spare at the end.
   S32 each = span / count - mTabSpacing;

   if ( each < mMinTabWidth )
      each = mMinTabWidth;

   // stretchToFit is what lets a tab grow past its cap to fill the bar.
   if ( !mStretchToFit && mMaxTabWidth > 0 && each > mMaxTabWidth )
      each = mMaxTabWidth;
   else if ( remainder )
   {
      // Whatever the division left over is handed out a pixel at a time to the
      // tabs at the front, so the row ends where the bar does.
      const S32 used = each * count + mTabSpacing * ( count - 1 );
      *remainder = getMax( span - used, 0 );
   }

   return getMax( each, 1 );
}

S32 ShellTabGroupCtrl::hitTab( const Point2I &point ) const
{
   S32 remainder = 0;
   const S32 each = tabExtent( &remainder );
   if ( each < 1 )
      return -1;

   // The row starts the glow's margin in, so anything before that is the bar
   // rather than a tab.
   S32 along = ( horizontal() ? point.x : point.y ) - mGlowOffset;
   if ( along < 0 )
      return -1;

   for ( S32 i = 0; i < mTabs.size(); i++ )
   {
      const S32 width = each + ( i < remainder ? 1 : 0 );
      if ( along < width )
         return i;

      along -= width + mTabSpacing;
      if ( along < 0 )
         return -1;
   }

   return -1;
}

void ShellTabGroupCtrl::addTab( S32 id, const char *text, S32 set )
{
   Tab tab;
   tab.id = id;
   tab.text = StringTable->insert( text ? text : "", true );
   tab.active = true;
   tab.set = set;

   mTabs.push_back( tab );

   // The first tab to arrive is the one showing: a bar with tabs and nothing
   // selected is a state Tribes 2's scripts never expect to see.
   if ( mSelected == -1 )
      setSelectedByIndex( mTabs.size() - 1 );

   setUpdate();
}

void ShellTabGroupCtrl::removeTabByIndex( S32 index )
{
   if ( index < 0 || index >= mTabs.size() )
      return;

   mTabs.erase( index );

   // Follow the tab, not the slot.  Closing one to the left of the open tab
   // must not silently change which screen is up.
   if ( mSelected == index )
      mSelected = -1;
   else if ( mSelected > index )
      mSelected--;

   setUpdate();
}

S32 ShellTabGroupCtrl::indexOfId( S32 id ) const
{
   for ( S32 i = 0; i < mTabs.size(); i++ )
      if ( mTabs[ i ].id == id )
         return i;

   return -1;
}

S32 ShellTabGroupCtrl::getTabId( S32 index ) const
{
   return ( index >= 0 && index < mTabs.size() ) ? mTabs[ index ].id : -1;
}

void ShellTabGroupCtrl::clear()
{
   mTabs.clear();

   mSelected = -1;
   mMouseOver = -1;

   setUpdate();
}

void ShellTabGroupCtrl::setTabText( S32 index, const char *text )
{
   if ( index >= 0 && index < mTabs.size() )
   {
      mTabs[ index ].text = StringTable->insert( text ? text : "", true );
      setUpdate();
   }
}

const char *ShellTabGroupCtrl::getTabText( S32 index ) const
{
   return ( index >= 0 && index < mTabs.size() ) ? mTabs[ index ].text : "";
}

void ShellTabGroupCtrl::setTabActive( S32 index, bool active )
{
   if ( index >= 0 && index < mTabs.size() )
   {
      mTabs[ index ].active = active;
      setUpdate();
   }
}

bool ShellTabGroupCtrl::isTabActive( S32 index ) const
{
   return ( index >= 0 && index < mTabs.size() ) ? mTabs[ index ].active : false;
}

S32 ShellTabGroupCtrl::getTabSet( S32 index ) const
{
   return ( index >= 0 && index < mTabs.size() ) ? mTabs[ index ].set : 0;
}

void ShellTabGroupCtrl::setSelectedByIndex( S32 index )
{
   if ( index < 0 || index >= mTabs.size() || !mTabs[ index ].active )
      return;

   mSelected = index;
   setUpdate();

   // onSelect is where the launch bar puts the screen on the canvas.  It is
   // handed the id and the label because that is what a Torque list callback
   // is handed; Tribes 2's own handler ignores both and reads getSelectedTab().
   Con::executef( this, "onSelect", Con::getIntArg( mTabs[ index ].id ), mTabs[ index ].text );
}

S32 ShellTabGroupCtrl::getSelectedId() const
{
   return getTabId( mSelected );
}

const char *ShellTabGroupCtrl::getSelectedText() const
{
   return getTabText( mSelected );
}

void ShellTabGroupCtrl::onMouseDown( const GuiEvent &event )
{
   const S32 tab = hitTab( globalToLocalCoord( event.mousePoint ) );
   if ( tab != -1 )
      setSelectedByIndex( tab );
}

void ShellTabGroupCtrl::onMouseMove( const GuiEvent &event )
{
   const S32 over = hitTab( globalToLocalCoord( event.mousePoint ) );
   if ( over != mMouseOver )
   {
      mMouseOver = over;
      setUpdate();
   }
}

void ShellTabGroupCtrl::onMouseLeave( const GuiEvent & )
{
   mMouseOver = -1;
   setUpdate();
}

void ShellTabGroupCtrl::onRender( Point2I offset, const RectI &updateRect )
{
   GFXDrawUtil *draw = GFX->getDrawUtil();

   S32 remainder = 0;
   const S32 each = tabExtent( &remainder );

   S32 along = 0;

   for ( S32 i = 0; i < mTabs.size(); i++ )
   {
      const Tab &entry = mTabs[ i ];
      const Set *set = setFor( entry );

      const S32 width = each + ( i < remainder ? 1 : 0 );

      RectI tab = tabRect( along, width );
      tab.point += offset;

      along += width + mTabSpacing;

      S32 state = 0;
      if ( i == mSelected )
         state = 1;
      else if ( i == mMouseOver && entry.active )
         state = 2;

      if ( set && set->art.isValid() )
      {
         // The art is drawn back out over the margin the tab was set in by, so
         // it lands on the bar's own edges: a resting tab's face stops at the
         // tab, and a lit one's glow fills the margin and overhangs its
         // neighbours rather than being clipped to its own rect.
         RectI art = tab;
         art.inset( -mGlowOffset, -mGlowOffset );

         draw->clearBitmapModulation();
         set->art.draw( state, art );
      }

      if ( mProfile->mFont && entry.text[ 0 ] )
      {
         // The colours come from the set too, all but the one for a tab that
         // cannot be selected: a set carries three, and Tribes 2 leaves the
         // inactive colour to the profile.
         ColorI color = set ? set->fontColor : mProfile->mFontColor;
         if ( !entry.active )
            color = mProfile->mFontColorNA;
         else if ( i == mSelected )
            color = set ? set->fontColorSEL : mProfile->mFontColorSEL;
         else if ( i == mMouseOver )
            color = set ? set->fontColorHL : mProfile->mFontColorHL;

         // The open tab's label sits a pixel down and right of where the
         // others sit, the way a pressed button's does.
         Point2I at = tab.point + mProfile->mTextOffset;
         if ( i == mSelected )
            at += Point2I( 1, 1 );

         draw->setBitmapModulation( color );
         renderJustifiedText( at, tab.extent - mProfile->mTextOffset, entry.text );
         draw->clearBitmapModulation();
      }
   }

   renderChildControls( offset, updateRect );
}

//-----------------------------------------------------------------------------
DefineEngineMethod( ShellTabGroupCtrl, tabCount, S32, (), ,
   "@brief How many tabs the bar is carrying.\n\n@ingroup GuiShell" )
{
   return object->getTabCount();
}

DefineEngineMethod( ShellTabGroupCtrl, addTab, void, ( S32 id, const char *text, S32 set ), ( 0 ),
   "@brief Put a tab on the end of the bar.\n\n"
   "@param id Script's own handle for the tab, which need not be unique.\n"
   "@param text The label.\n"
   "@param set Which group of tabs this one belongs to.\n\n"
   "@ingroup GuiShell" )
{
   object->addTab( id, text, set );
}

DefineEngineMethod( ShellTabGroupCtrl, removeTabByIndex, void, ( S32 index ), ,
   "@brief Take the tab at this position off the bar.\n\n@ingroup GuiShell" )
{
   object->removeTabByIndex( index );
}

DefineEngineMethod( ShellTabGroupCtrl, removeTab, void, ( S32 id ), ,
   "@brief Take the first tab carrying this id off the bar.\n\n@ingroup GuiShell" )
{
   object->removeTabByIndex( object->indexOfId( id ) );
}

DefineEngineMethod( ShellTabGroupCtrl, clear, void, (), ,
   "@brief Empty the bar.\n\n@ingroup GuiShell" )
{
   object->clear();
}

static ColorI shellParseColor( const char *text )
{
   S32 r = 255, g = 255, b = 255, a = 255;

   // Tribes 2 writes these three-wide -- "50 50 0" -- and means them opaque.
   if ( text && dSscanf( text, "%d %d %d %d", &r, &g, &b, &a ) < 3 )
      return ColorI( 255, 255, 255 );

   return ColorI( r, g, b, a );
}

DefineEngineMethod( ShellTabGroupCtrl, addSet, void,
   ( S32 id, const char *bitmap, const char *fontColor, const char *fontColorHL,
     const char *fontColorSEL ), ,
   "@brief Give a group of tabs its own art and colours.\n\n"

   "A tab added with this id draws from `bitmap` rather than the profile's, "
   "which is how one bar carries tabs that read as different kinds of thing.  "
   "An id already registered is left alone.\n\n"

   "@ingroup GuiShell" )
{
   object->addSet( id, bitmap, shellParseColor( fontColor ),
                   shellParseColor( fontColorHL ), shellParseColor( fontColorSEL ) );
}

DefineEngineMethod( ShellTabGroupCtrl, removeSet, void, ( S32 index ), ,
   "@brief Drop the set at this position.\n\n@ingroup GuiShell" )
{
   object->removeSet( index );
}

DefineEngineMethod( ShellTabGroupCtrl, clearTabSets, void, (), ,
   "@brief Drop every set, including the one taken from the profile.\n\n"
   "@ingroup GuiShell" )
{
   object->clearTabSets();
}

DefineEngineMethod( ShellTabGroupCtrl, getTabId, S32, ( S32 index ), ,
   "@brief The id the tab at this position was added with.\n\n@ingroup GuiShell" )
{
   return object->getTabId( index );
}

DefineEngineMethod( ShellTabGroupCtrl, getTabIndex, S32, ( S32 id ), ,
   "@brief Where the first tab carrying this id sits, or -1.\n\n@ingroup GuiShell" )
{
   return object->indexOfId( id );
}

DefineEngineMethod( ShellTabGroupCtrl, getTabText, const char *, ( S32 index ), ,
   "@brief The label on the tab at this position.\n\n@ingroup GuiShell" )
{
   return object->getTabText( index );
}

DefineEngineMethod( ShellTabGroupCtrl, setTabText, void, ( S32 index, const char *text ), ,
   "@brief Relabel the tab at this position.\n\n@ingroup GuiShell" )
{
   object->setTabText( index, text );
}

DefineEngineMethod( ShellTabGroupCtrl, isTabActive, bool, ( S32 index ), ,
   "@brief Whether the tab at this position can be selected.\n\n@ingroup GuiShell" )
{
   return object->isTabActive( index );
}

DefineEngineMethod( ShellTabGroupCtrl, setTabActive, void, ( S32 index, bool active ), ,
   "@brief Let the tab at this position be selected, or stop it being.\n\n@ingroup GuiShell" )
{
   object->setTabActive( index, active );
}

DefineEngineMethod( ShellTabGroupCtrl, getTabSetByIndex, S32, ( S32 index ), ,
   "@brief Which group the tab at this position belongs to.\n\n@ingroup GuiShell" )
{
   return object->getTabSet( index );
}

DefineEngineMethod( ShellTabGroupCtrl, getTabSet, S32, ( S32 id ), ,
   "@brief Which group the first tab carrying this id belongs to.\n\n@ingroup GuiShell" )
{
   return object->getTabSet( object->indexOfId( id ) );
}

DefineEngineMethod( ShellTabGroupCtrl, getSelectedTab, S32, (), ,
   "@brief Which position is showing, or -1.\n\n@ingroup GuiShell" )
{
   return object->getSelectedTab();
}

DefineEngineMethod( ShellTabGroupCtrl, setSelectedByIndex, void, ( S32 index ), ,
   "@brief Show the tab at this position.\n\n@ingroup GuiShell" )
{
   object->setSelectedByIndex( index );
}

DefineEngineMethod( ShellTabGroupCtrl, setSelected, void, ( S32 id ), ,
   "@brief Show the first tab carrying this id.\n\n@ingroup GuiShell" )
{
   object->setSelectedByIndex( object->indexOfId( id ) );
}

DefineEngineMethod( ShellTabGroupCtrl, getSelectedId, S32, (), ,
   "@brief The id of the tab showing.\n\n@ingroup GuiShell" )
{
   return object->getSelectedId();
}

DefineEngineMethod( ShellTabGroupCtrl, getSelectedText, const char *, (), ,
   "@brief The label on the tab showing.\n\n@ingroup GuiShell" )
{
   return object->getSelectedText();
}
