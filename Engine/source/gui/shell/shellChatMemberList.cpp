#include "platform/platform.h"
#include "gui/shell/shellChatMemberList.h"

#include "console/console.h"
#include "console/engineAPI.h"
#include "core/strings/stringFunctions.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"
#include "gfx/gFont.h"

IMPLEMENT_CONOBJECT( ShellChatMemberList );

ConsoleDocClass( ShellChatMemberList,
   "@brief The list of who is in a Tribes 2 chat channel.\n\n"

   "A row's text is a run of <tribe:0> and <tribe:1> sections rather than "
   "plain text: 0 is drawn in the nick colours and 1 in the tribe colours, in "
   "the order script wrote them.  A style puts a row in a colour of its own "
   "and hangs up to two badges off its right-hand end.\n\n"

   "@ingroup GuiShell"
);

static const char *smTribeOpen = "<tribe:";
static const char *smTribeClose = "</tribe>";

ShellChatMemberList::ShellChatMemberList()
{
}

bool ShellChatMemberList::onWake()
{
   if ( !Parent::onWake() )
      return false;

   const char *stem = mProfile->mBitmapBase;

   if ( stem && stem[ 0 ] )
   {
      char name[ 512 ];

      dSprintf( name, sizeof( name ), "%s_rol", stem );
      mRollover.set( name, &GFXDefaultPersistentProfile,
                     avar( "%s() - rollover", __FUNCTION__ ) );

      dSprintf( name, sizeof( name ), "%s_act", stem );
      mSelectedBar.set( name, &GFXDefaultPersistentProfile,
                        avar( "%s() - selected", __FUNCTION__ ) );
   }

   return true;
}

void ShellChatMemberList::onSleep()
{
   Parent::onSleep();

   mRollover = NULL;
   mSelectedBar = NULL;
}

const ShellChatMemberList::Style *ShellChatMemberList::styleFor( S32 row ) const
{
   if ( row < 0 || row >= mRowStyle.size() )
      return NULL;

   for ( U32 i = 0; i < mStyles.size(); i++ )
      if ( mStyles[ i ].id == mRowStyle[ row ] )
         return &mStyles[ i ];

   return NULL;
}

ColorI ShellChatMemberList::runColor( S32 run, S32 state, const Style *style ) const
{
   if ( style && style->given[ state ] )
      return style->color[ state ];

   static const U32 nick[ StateCount ] = { 4, 5, 6 };
   static const U32 tribe[ StateCount ] = { 7, 8, 9 };

   if ( run == RunNick )
      return mProfile->mFontColors[ nick[ state ] ];

   if ( run == RunTribe )
      return mProfile->mFontColors[ tribe[ state ] ];

   return state == Selected ? mProfile->mFontColorSEL
        : state == Highlight ? mProfile->mFontColorHL
                             : mProfile->mFontColor;
}

void ShellChatMemberList::onRenderCell( Point2I offset, Point2I cell,
                                        bool selected, bool mouseOver )
{
   if ( cell.y < 0 || cell.y >= mList.size() )
      return;

   const Entry &entry = mList[ cell.y ];

   GFXDrawUtil *draw = GFX->getDrawUtil();
   draw->clearBitmapModulation();

   if ( entry.active )
   {
      GFXTexHandle bar = selected ? mSelectedBar : mouseOver ? mRollover : GFXTexHandle();

      if ( bar )
         draw->drawBitmapStretch( bar, RectI( offset, mCellSize ) );
   }

   const Style *style = styleFor( cell.y );

   if ( style )
   {
      if ( style->left )
         draw->drawBitmap( style->left,
            Point2I( offset.x + mCellSize.x - 0x24, offset.y ) );

      if ( style->right )
         draw->drawBitmap( style->right,
            Point2I( offset.x + mCellSize.x - 0x12, offset.y ) );
   }

   if ( !entry.text || !entry.text[ 0 ] || !mProfile->mFont )
      return;

   const S32 state = !entry.active ? Normal
                     : selected ? Selected
                     : mouseOver ? Highlight
                                 : Normal;

   GFont *font = mProfile->mFont;

   Point2I pen( offset.x + mProfile->mTextOffset.x,
                offset.y + mProfile->mTextOffset.y );

   const char *at = entry.text;

   while ( *at )
   {
      const char *open = dStrstr( at, smTribeOpen );

      if ( !open )
         break;

      if ( open != at )
      {
         draw->setBitmapModulation( runColor( RunPlain, state, style ) );
         pen.x = draw->drawTextN( font, pen, at, U32( open - at ),
                                   mProfile->mFontColors );
      }

      const S32 run = open[ 7 ] == '1' ? RunTribe : RunNick;

      const char *body = open + 9;
      const char *close = dStrstr( body, smTribeClose );

      const U32 length = close ? U32( close - body ) : dStrlen( body );

      if ( length )
      {
         draw->setBitmapModulation( runColor( run, state, style ) );
         pen.x = draw->drawTextN( font, pen, body, length, mProfile->mFontColors );
      }

      at = close ? close + dStrlen( smTribeClose ) : body + length;
   }

   if ( *at )
   {
      draw->setBitmapModulation( runColor( RunPlain, state, style ) );
      draw->drawTextN( font, pen, at, dStrlen( at ), mProfile->mFontColors );
   }
}

bool ShellChatMemberList::addStyle( S32 id, const char *left, const char *right,
                                    const char *color, const char *colorHL,
                                    const char *colorSEL )
{
   for ( U32 i = 0; i < mStyles.size(); i++ )
      if ( mStyles[ i ].id == id )
         return false;

   mStyles.increment();

   Style &style = mStyles.last();
   style.id = id;

   if ( left && left[ 0 ] )
      style.left.set( left, &GFXDefaultPersistentProfile,
                      avar( "%s() - left", __FUNCTION__ ) );

   if ( right && right[ 0 ] )
      style.right.set( right, &GFXDefaultPersistentProfile,
                       avar( "%s() - right", __FUNCTION__ ) );

   const char *given[ StateCount ] = { color, colorHL, colorSEL };

   for ( U32 i = 0; i < StateCount; i++ )
   {
      S32 r = 0, g = 0, b = 0, a = 255;

      const S32 read = given[ i ]
         ? dSscanf( given[ i ], "%d %d %d %d", &r, &g, &b, &a ) : -1;

      style.given[ i ] = read >= 3;
      style.color[ i ] = ColorI( r, g, b, read == 4 ? a : 255 );
   }

   return true;
}

S32 ShellChatMemberList::getRowStyle( S32 row ) const
{
   return row >= 0 && row < mRowStyle.size() ? mRowStyle[ row ] : -1;
}

void ShellChatMemberList::setRowStyle( S32 row, S32 style )
{
   if ( row < 0 )
      return;

   while ( mRowStyle.size() <= row )
      mRowStyle.push_back( -1 );

   mRowStyle[ row ] = style;
   setUpdate();
}

DefineEngineMethod( ShellChatMemberList, addStyle, bool,
   ( S32 id, const char *left, const char *right, const char *color,
     const char *colorHL, const char *colorSEL ), ,
   "@brief Register a look rows can be put in.\n\n"
   "@ingroup GuiShell" )
{
   return object->addStyle( id, left, right, color, colorHL, colorSEL );
}

DefineEngineMethod( ShellChatMemberList, getRowStyle, S32, ( S32 row ), ,
   "@brief The style the row at this index is in, or -1.\n\n"
   "@ingroup GuiShell" )
{
   return object->getRowStyle( row );
}

DefineEngineMethod( ShellChatMemberList, getRowStyleById, S32, ( S32 id ), ,
   "@brief The style the row with this id is in, or -1.\n\n"
   "@ingroup GuiShell" )
{
   return object->getRowStyle( object->findEntryById( id ) );
}

DefineEngineMethod( ShellChatMemberList, setRowStyle, void, ( S32 row, S32 style ), ,
   "@brief Put the row at this index in a style.\n\n"
   "@ingroup GuiShell" )
{
   object->setRowStyle( row, style );
}

DefineEngineMethod( ShellChatMemberList, setRowStyleById, void, ( S32 id, S32 style ), ,
   "@brief Put the row with this id in a style.\n\n"
   "@ingroup GuiShell" )
{
   object->setRowStyle( object->findEntryById( id ), style );
}
