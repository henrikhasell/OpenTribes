#include "platform/platform.h"
#include "gui/shell/shellMenuCtrl.h"

#include "console/console.h"
#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"
#include "gfx/gFont.h"
#include "gui/controls/guiBackgroundCtrl.h"
#include "gui/core/guiArrayCtrl.h"
#include "gui/core/guiCanvas.h"
#include "sfx/sfxSystem.h"
#include "sfx/sfxTrack.h"

//-----------------------------------------------------------------------------
// The pressed button, drawn again over the dimmed screen.

class ShellLaunchDummy : public GuiControl
{
   typedef GuiControl Parent;

   ShellLaunchMenu *mMenu;

public:
   ShellLaunchDummy( ShellLaunchMenu *menu = NULL ) : mMenu( menu ) {}

   DECLARE_CONOBJECT( ShellLaunchDummy );

   void follow()
   {
      GuiControl *parent = mMenu->getParent();
      if ( parent )
         resize( parent->localToGlobalCoord( mMenu->getPosition() ), mMenu->getExtent() );
   }

   void parentResized( const RectI &, const RectI & ) override
   {
      follow();
   }

   void onRender( Point2I offset, const RectI & ) override
   {
      mMenu->renderPressed( offset );
   }
};

IMPLEMENT_CONOBJECT( ShellLaunchDummy );

ConsoleDocClass( ShellLaunchDummy,
   "@brief The LAUNCH button as it is drawn while its menu is open.\n\n"
   "@ingroup GuiShell"
);

//-----------------------------------------------------------------------------
// The column that rises from the button before the menu shows in it.

class ShellVWipeCtrl : public GuiControl
{
   typedef GuiControl Parent;

   ShellLaunchMenu *mMenu;

   S32 mRate;
   U32 mLastTime;

public:
   GFXTexHandle mFill;
   GFXTexHandle mBar;
   bool mWiping;

   ShellVWipeCtrl( ShellLaunchMenu *menu = NULL )
      : mMenu( menu ), mRate( 0 ), mLastTime( 0 ), mWiping( true ) {}

   DECLARE_CONOBJECT( ShellVWipeCtrl );

   bool onWake() override
   {
      if ( !Parent::onWake() )
         return false;

      // Fast enough to cover twice the height above it in a second, so the
      // column reaches the top of the screen in half of one.
      mRate = getPosition().y * 2;
      mLastTime = Platform::getRealMilliseconds();
      return true;
   }

   void onSleep() override
   {
      Parent::onSleep();

      mFill = NULL;
      mBar = NULL;
      mWiping = false;
   }

   void onPreRender() override
   {
      if ( getPosition().y > 0 )
      {
         const U32 now = Platform::getRealMilliseconds();
         const S32 step = S32( ( now - mLastTime ) * U32( mRate ) / 1000 );

         mLastTime = now;

         if ( step > 0 )
            resize( Point2I( getPosition().x, getPosition().y - step ),
                    Point2I( getWidth(), getHeight() + step ) );
      }
      else if ( mWiping )
      {
         mWiping = false;
         mMenu->onWipeDone();
      }

      setUpdate();
   }

   void onRender( Point2I offset, const RectI &updateRect ) override
   {
      if ( mFill && mBar )
      {
         GFXDrawUtil *draw = GFX->getDrawUtil();

         draw->clearBitmapModulation();
         draw->drawBitmapStretch( mFill, RectI( offset, getExtent() ) );

         if ( mWiping )
            draw->drawBitmapStretch( mBar,
               RectI( offset.x, offset.y, getWidth(), mBar->getHeight() ) );
      }

      renderChildControls( offset, updateRect );
   }
};

IMPLEMENT_CONOBJECT( ShellVWipeCtrl );

ConsoleDocClass( ShellVWipeCtrl,
   "@brief The column the LAUNCH menu wipes up the screen as it opens.\n\n"
   "@ingroup GuiShell"
);

//-----------------------------------------------------------------------------
// The entries, at the foot of the column.

class ShellLaunchList : public GuiArrayCtrl
{
   typedef GuiArrayCtrl Parent;

   ShellLaunchMenu *mMenu;

   enum { BottomMargin = 60, TextX = 6, TextY = 2 };

public:
   GFXTexHandle mRollover;
   GFXTexHandle mSelected;
   GFXTexHandle mSeparator;

   ShellLaunchList( ShellLaunchMenu *menu = NULL ) : mMenu( menu ) {}

   DECLARE_CONOBJECT( ShellLaunchList );

   void setCellSize( const Point2I &size ) { mCellSize = size; }

   void place()
   {
      GuiControl *parent = getParent();
      if ( !parent )
         return;

      const S32 rows = mMenu->entries().size();
      const S32 height = mCellSize.y * rows;

      mSize.set( 1, rows );
      resize( Point2I( getPosition().x, parent->getHeight() - height - BottomMargin ),
              Point2I( mCellSize.x, height ) );
      setUpdate();
   }

   void parentResized( const RectI &, const RectI & ) override
   {
      place();
   }

   void onCellSelected( Point2I cell ) override
   {
      const Vector< ShellLaunchMenu::Entry > &entries = mMenu->entries();

      if ( cell.y >= 0 && cell.y < entries.size() && !entries[ cell.y ].separator )
         mMenu->close( entries[ cell.y ].id );
   }

   void onMouseDown( const GuiEvent &event ) override
   {
      if ( cursorInControl() )
         Parent::onMouseDown( event );
      else
         mMenu->close( -1 );
   }

   void select( S32 row )
   {
      if ( row < 0 || row >= mMenu->entries().size() )
         mSelectedCell.set( -1, -1 );
      else
         mSelectedCell.set( 0, row );

      setUpdate();
   }

   bool onKeyDown( const GuiEvent &event ) override
   {
      if ( !mVisible || !mActive || !mAwake )
         return true;

      const Vector< ShellLaunchMenu::Entry > &entries = mMenu->entries();
      const S32 count = entries.size();
      S32 row = mSelectedCell.y;

      switch ( event.keyCode )
      {
         case KEY_UP:
            if ( row == 0 )
               break;

            row = row == -1 ? count - 1 : row - 1;
            while ( row >= 0 && entries[ row ].separator )
               row--;

            select( row );
            break;

         case KEY_DOWN:
            if ( row == count - 1 )
               break;

            row = row == -1 ? 0 : row + 1;
            while ( row < count && entries[ row ].separator )
               row++;

            select( row );
            break;

         case KEY_RETURN:
         case KEY_SPACE:
            cellSelected( mSelectedCell );
            break;

         case KEY_ESCAPE:
            mMenu->close( -1 );
            break;

         default:
            break;
      }

      return true;
   }

   void onRenderCell( Point2I offset, Point2I cell, bool selected, bool mouseOver ) override
   {
      const Vector< ShellLaunchMenu::Entry > &entries = mMenu->entries();

      if ( cell.y >= entries.size() )
         return;

      const ShellLaunchMenu::Entry &entry = entries[ cell.y ];

      GFXDrawUtil *draw = GFX->getDrawUtil();
      draw->clearBitmapModulation();

      if ( entry.separator && mSeparator )
      {
         const S32 y = ( mCellSize.y - S32( mSeparator->getHeight() ) ) / 2;
         draw->drawBitmap( mSeparator, Point2I( offset.x, offset.y + y ) );
         return;
      }

      const RectI bar( offset, mCellSize );

      if ( selected && mSelected )
         draw->drawBitmapStretch( mSelected, bar );
      else if ( mouseOver && mRollover )
         draw->drawBitmapStretch( mRollover, bar );

      const ColorI &color = selected ? mProfile->mFontColorSEL
                            : mouseOver ? mProfile->mFontColorHL
                                        : mProfile->mFontColor;

      if ( !mFont || !entry.text )
         return;

      draw->setBitmapModulation( color );
      draw->drawText( mFont, Point2I( offset.x + TextX, offset.y + TextY ), entry.text );
   }
};

IMPLEMENT_CONOBJECT( ShellLaunchList );

ConsoleDocClass( ShellLaunchList,
   "@brief The entries of an open LAUNCH menu.\n\n"
   "@ingroup GuiShell"
);

//-----------------------------------------------------------------------------

IMPLEMENT_CONOBJECT( ShellLaunchMenu );

ConsoleDocClass( ShellLaunchMenu,
   "@brief The LAUNCH menu at the left of Tribes 2's launch bar.\n\n"

   "A fixed button with a fixed label.  Pressed, it dims the screen, wipes "
   "a column up from the button to the top of the screen and lists its "
   "entries at the foot of that column.  Its art is the profile's "
   "bitmapBase and the two lit versions beside it.\n\n"

   "@ingroup GuiShell"
);

ShellLaunchMenu::ShellLaunchMenu()
{
   mButtonFontType = StringTable->insert( "Verdana Bold" );
   mButtonFontSize = 14;
   mButtonFontColor.set( 5, 244, 217, 255 );
   mButtonFontColorHL.set( 60, 140, 140, 255 );

   mRolloverBarBitmap = StringTable->insert( "gui/shll_launch_rol" );
   mSelectedBarBitmap = StringTable->insert( "gui/shll_launch_act" );
   mSeparatorBitmap = StringTable->insert( "gui/shll_launch_sep" );
   mVWipeFillBitmap = StringTable->insert( "gui/shll_wpvfield" );
   mVWipeBarBitmap = StringTable->insert( "gui/shll_wipefill" );

   mOpen = false;
   mOver = false;

   mBackground = NULL;
   mWipe = NULL;
   mList = NULL;
}

void ShellLaunchMenu::initPersistFields()
{
   addField( "buttonFontType", TypeString, Offset( mButtonFontType, ShellLaunchMenu ),
      "Font for the label on the button." );
   addField( "buttonFontSize", TypeS32, Offset( mButtonFontSize, ShellLaunchMenu ),
      "Size of the label on the button." );
   addField( "buttonFontColor", TypeColorI, Offset( mButtonFontColor, ShellLaunchMenu ),
      "Colour of the label at rest." );
   addField( "buttonFontColorHL", TypeColorI, Offset( mButtonFontColorHL, ShellLaunchMenu ),
      "Colour of the label while the mouse is over the button or the menu is open." );
   addField( "rolloverBarBitmap", TypeString, Offset( mRolloverBarBitmap, ShellLaunchMenu ),
      "Art behind the entry the mouse is over." );
   addField( "selectedBarBitmap", TypeString, Offset( mSelectedBarBitmap, ShellLaunchMenu ),
      "Art behind the entry the keyboard is on." );
   addField( "separatorBitmap", TypeString, Offset( mSeparatorBitmap, ShellLaunchMenu ),
      "Art for a divider between entries." );
   addField( "vWipeFillBmp", TypeString, Offset( mVWipeFillBitmap, ShellLaunchMenu ),
      "Art the column is filled with; its width is the column's." );
   addField( "vWipeBarBmp", TypeString, Offset( mVWipeBarBitmap, ShellLaunchMenu ),
      "Art at the head of the column while it rises." );

   Parent::initPersistFields();
}

bool ShellLaunchMenu::onWake()
{
   if ( !Parent::onWake() )
      return false;

   mSkin.setBase( mProfile->mBitmapBase );

   if ( mButtonFontType && mButtonFontType[ 0 ] && mButtonFontSize > 0 )
      mButtonFont = GFont::create( mButtonFontType, mButtonFontSize,
                                   Con::getVariable( "$GUI::fontCacheDirectory" ) );

   return true;
}

void ShellLaunchMenu::onSleep()
{
   close( -1 );

   Parent::onSleep();

   mButtonFont = NULL;
   mOver = false;
}

GFont *ShellLaunchMenu::buttonFont()
{
   return mButtonFont ? (GFont*)mButtonFont : (GFont*)mProfile->mFont;
}

GFXTexHandle ShellLaunchMenu::art()
{
   mSkin.setBase( mProfile->mBitmapBase );

   GFXTexHandle tex = mSkin.get( mOpen ? "_act" : mOver ? "_rol" : "" );
   return tex ? tex : mSkin.get( "" );
}

void ShellLaunchMenu::clear()
{
   mEntries.clear();
}

void ShellLaunchMenu::addEntry( const char *text, S32 id )
{
   Entry entry;
   entry.text = StringTable->insert( text ? text : "", true );
   entry.id = id;
   entry.separator = false;
   mEntries.push_back( entry );
}

void ShellLaunchMenu::addSeparator()
{
   Entry entry;
   entry.text = NULL;
   entry.id = 0;
   entry.separator = true;
   mEntries.push_back( entry );
}

void ShellLaunchMenu::renderPressed( Point2I offset )
{
   GFXDrawUtil *draw = GFX->getDrawUtil();
   draw->clearBitmapModulation();

   mSkin.setBase( mProfile->mBitmapBase );

   GFXTexHandle tex = mSkin.get( "_act" );
   if ( tex )
      draw->drawBitmap( tex, offset );

   const char *label = getText();
   GFont *font = buttonFont();

   if ( !font || !label || !label[ 0 ] )
      return;

   const Point2I at( offset.x + ( getWidth() - S32( font->getStrWidth( label ) ) ) / 2 + 1,
                     offset.y + mProfile->mTextOffset.y + 1 );

   draw->setBitmapModulation( mButtonFontColorHL );
   draw->drawText( font, at, label );
   draw->clearBitmapModulation();
}

void ShellLaunchMenu::onRender( Point2I offset, const RectI & )
{
   GFXDrawUtil *draw = GFX->getDrawUtil();
   draw->clearBitmapModulation();

   GFXTexHandle tex = art();
   if ( tex )
      draw->drawBitmap( tex, offset );

   const char *label = getText();
   GFont *font = buttonFont();

   if ( !font || !label || !label[ 0 ] )
      return;

   // The label sinks a pixel down and right while the menu is open.
   const S32 sink = mOpen ? 1 : 0;

   const Point2I at( offset.x + ( getWidth() - S32( font->getStrWidth( label ) ) ) / 2 + sink,
                     offset.y + mProfile->mTextOffset.y + sink );

   draw->setBitmapModulation( mOpen || mOver ? mButtonFontColorHL : mButtonFontColor );
   draw->drawText( font, at, label );
   draw->clearBitmapModulation();
}

void ShellLaunchMenu::onMouseDown( const GuiEvent & )
{
   if ( mVisible && mActive && mAwake )
      open();
}

void ShellLaunchMenu::onMouseEnter( const GuiEvent &event )
{
   Parent::onMouseEnter( event );

   mOver = true;

   if ( mActive )
      if ( SFXTrack *over = mProfile->getSoundButtonOver() )
         SFX->playOnce( over );
}

void ShellLaunchMenu::onMouseLeave( const GuiEvent &event )
{
   Parent::onMouseLeave( event );

   mOver = false;
}

bool ShellLaunchMenu::onKeyDown( const GuiEvent &event )
{
   if ( event.keyCode == KEY_RETURN && !event.modifier )
   {
      if ( mVisible && mActive && mAwake )
         open();
      return true;
   }

   return Parent::onKeyDown( event );
}

void ShellLaunchMenu::open()
{
   // Where the column starts, from the button's top left.
   enum { WipeX = -6, WipeY = 13 };

   if ( mOpen )
      return;

   GuiControl *parent = getParent();
   GuiCanvas *root = getRoot();

   if ( !parent || !root )
      return;

   const Point2I at = parent->localToGlobalCoord( getPosition() );

   ShellLaunchDummy *dummy = new ShellLaunchDummy( this );
   dummy->setMinExtent( Point2I( 0, 0 ) );
   dummy->setPosition( at );
   dummy->setExtent( getExtent() );

   mWipe = new ShellVWipeCtrl( this );
   mWipe->mFill.set( mVWipeFillBitmap, &GFXDefaultPersistentProfile, avar( "%s() - fill", __FUNCTION__ ) );
   mWipe->mBar.set( mVWipeBarBitmap, &GFXDefaultPersistentProfile, avar( "%s() - bar", __FUNCTION__ ) );
   mWipe->setMinExtent( Point2I( 0, 0 ) );
   mWipe->setPosition( at + Point2I( WipeX, WipeY ) );
   mWipe->setExtent( Point2I( mWipe->mFill ? mWipe->mFill->getWidth() : 0, 1 ) );

   // A background that draws itself draws its children twice, which is
   // what gives the column and its entries their strength.
   GuiBackgroundCtrl *background = new GuiBackgroundCtrl;
   background->mDraw = true;
   mBackground = background;

   GuiControlProfile *back;
   if ( Sim::findObject( "DlgBackProfile", back ) )
      mBackground->setControlProfile( back );

   mBackground->setMinExtent( Point2I( 0, 0 ) );
   mBackground->setPosition( Point2I( 0, 0 ) );
   mBackground->setExtent( root->getExtent() );
   mBackground->setField( "horizSizing", "width" );
   mBackground->setField( "vertSizing", "height" );

   dummy->registerObject();
   mWipe->registerObject();
   mBackground->registerObject();

   mBackground->addObject( dummy );
   mBackground->addObject( mWipe );

   root->pushDialogControl( mBackground, 99 );

   if ( SFXTrack *down = mProfile->getSoundButtonDown() )
      SFX->playOnce( down );

   mOpen = true;
}

void ShellLaunchMenu::onWipeDone()
{
   enum { Inset = 10, CellPadding = 4 };

   if ( !mOpen || !mWipe )
      return;

   ShellLaunchList *list = new ShellLaunchList( this );
   list->setControlProfile( mProfile );
   list->setMinExtent( Point2I( 0, 0 ) );

   if ( mRolloverBarBitmap && mRolloverBarBitmap[ 0 ] )
      list->mRollover.set( mRolloverBarBitmap, &GFXDefaultPersistentProfile, avar( "%s() - rollover", __FUNCTION__ ) );
   if ( mSelectedBarBitmap && mSelectedBarBitmap[ 0 ] )
      list->mSelected.set( mSelectedBarBitmap, &GFXDefaultPersistentProfile, avar( "%s() - selected", __FUNCTION__ ) );
   if ( mSeparatorBitmap && mSeparatorBitmap[ 0 ] )
      list->mSeparator.set( mSeparatorBitmap, &GFXDefaultPersistentProfile, avar( "%s() - separator", __FUNCTION__ ) );

   const S32 width = mWipe->getWidth() - Inset * 2;
   GFont *font = mProfile->mFont;
   const S32 height = font ? S32( font->getHeight() ) + CellPadding : CellPadding;

   list->setCellSize( Point2I( width, height ) );
   list->setPosition( Point2I( Inset, 0 ) );
   list->setExtent( Point2I( width, height ) );

   list->registerObject();
   mWipe->addObject( list );

   list->place();
   list->setFirstResponder();
   list->mouseLock();

   mList = list;
}

void ShellLaunchMenu::close( S32 id )
{
   if ( !mOpen )
      return;

   mOpen = false;

   GuiCanvas *root = getRoot();

   if ( mList )
      mList->mouseUnlock();

   if ( root )
      root->popDialogControl( mBackground );

   // The list may be the one asking; let it return before it goes.
   mBackground->safeDeleteObject();
   mBackground = NULL;
   mWipe = NULL;
   mList = NULL;

   if ( mAwake )
      setFirstResponder();

   for ( S32 i = 0; i < mEntries.size(); i++ )
   {
      if ( !mEntries[ i ].separator && mEntries[ i ].id == id )
      {
         Con::executef( this, "onSelect", Con::getIntArg( id ), mEntries[ i ].text );
         break;
      }
   }
}

DefineEngineMethod( ShellLaunchMenu, clear, void, (), ,
   "@brief Take every entry off the menu.\n\n@ingroup GuiShell" )
{
   object->clear();
}

DefineEngineMethod( ShellLaunchMenu, add, void, ( S32 id, const char *text ), ,
   "@brief Put an entry on the end of the menu.\n\n"

   "Tribes 2 gives the id first, where Torque's own popup gives the text "
   "first.\n\n"

   "@ingroup GuiShell" )
{
   object->addEntry( text, id );
}

DefineEngineMethod( ShellLaunchMenu, addSeparator, void, (), ,
   "@brief Put a divider on the end of the menu.\n\n@ingroup GuiShell" )
{
   object->addSeparator();
}
