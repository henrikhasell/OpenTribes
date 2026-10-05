#include "platform/platform.h"
#include "T3D/gameBase/dataBlocksMiscT2.h"

#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "console/simBase.h"
#include "core/stringTable.h"
#include "core/strings/stringFunctions.h"
#include "T3D/gameBase/t2Fields.h"
#include "T3D/gameBase/t2ShapePath.h"
#include "ts/tsShapeConstruct.h"


IMPLEMENT_CO_DATABLOCK_V1( CommanderIconData );
IMPLEMENT_CO_DATABLOCK_V1( CannedChatItem );
IMPLEMENT_CO_DATABLOCK_V1( SensorData );
IMPLEMENT_CO_DATABLOCK_V1( RunningLightData );
IMPLEMENT_CO_DATABLOCK_V1( JetEffectData );
IMPLEMENT_CO_DATABLOCK_V1( TSShapeConstructorT2 );

ConsoleDocClass( CommanderIconData,
   "@brief The five images a Tribes 2 commander map icon is drawn from.\n\n"
   "@internal" );

ConsoleDocClass( CannedChatItem,
   "@brief One line of the Tribes 2 chat menu.\n\n@internal" );

ConsoleDocClass( SensorData,
   "@brief What a Tribes 2 sensor can pick up.  Nothing of it is sent to a "
   "client; the sensor group and its colour arrive as events instead.\n\n"
   "@internal" );

ConsoleDocClass( RunningLightData,
   "@brief A light on a Tribes 2 vehicle's hull.\n\n@internal" );

ConsoleDocClass( JetEffectData,
   "@brief The flame out of a jetting Tribes 2 armour or vehicle.\n\n"
   "@internal" );

ConsoleDocClass( TSShapeConstructorT2,
   "@brief Which .dsq animations belong to a shape, as a Tribes 2 server "
   "says it.  Builds one of this tree's own TSShapeConstructors.\n\n"
   "@internal" );


//-----------------------------------------------------------------------------

CommanderIconData::CommanderIconData()
   : mBaseImage( StringTable->EmptyString() ),
     mActiveImage( StringTable->EmptyString() ),
     mInactiveImage( StringTable->EmptyString() ),
     mSelectImage( StringTable->EmptyString() ),
     mHilightImage( StringTable->EmptyString() )
{
}

/// Take the next string into a string table entry, or leave it alone.
static bool takeName( T2Fields &f, StringTableEntry &out )
{
   char text[ 256 ];
   if( !f.takeString( text, sizeof( text ) ) )
      return false;

   out = StringTable->insert( text );
   return true;
}

void CommanderIconData::readT2Fields( T2Reader &reader )
{
   Parent::readT2Fields( reader );

   T2Fields f( reader );

   // Five strings in a row, none of them optional, so each take stands on its
   // own node and a stop part way needs no handling beyond not filling the
   // rest.
   takeName( f, mBaseImage );
   takeName( f, mActiveImage );
   takeName( f, mInactiveImage );
   takeName( f, mSelectImage );
   takeName( f, mHilightImage );
}


//-----------------------------------------------------------------------------

CannedChatItem::CannedChatItem()
   : mChatText( StringTable->EmptyString() )
{
}

void CannedChatItem::readT2Fields( T2Reader &reader )
{
   Parent::readT2Fields( reader );

   // The take is here even though this tree's layout has nothing to give it.
   // Asking at the end of a layout is the ordinary absent-field case and costs
   // nothing, and it means the member fills itself the day the table is fixed
   // upstream rather than needing this file revisited.  See the header.
   T2Fields f( reader );
   takeName( f, mChatText );
}


//-----------------------------------------------------------------------------

/// Nothing, and the header says why: a Tribes 2 SensorData body is empty on
/// the wire.  The override carries that fact where the next agent will look
/// for it rather than leaving the class looking unfinished.
void SensorData::readT2Fields( T2Reader &reader )
{
   Parent::readT2Fields( reader );
}


//-----------------------------------------------------------------------------

RunningLightData::RunningLightData()
   : mRadius( 0.0f ),
     mColor( 1.0f, 1.0f, 1.0f, 1.0f ),
     mType( 0.0f ),
     mLength( 0.0f ),
     mNodeName( StringTable->EmptyString() ),
     mDirection( 0.0f, 1.0f, 0.0f ),
     mOffset( 0.0f, 0.0f, 0.0f ),
     mTexture( StringTable->EmptyString() )
{
}

void RunningLightData::readT2Fields( T2Reader &reader )
{
   Parent::readT2Fields( reader );

   T2Fields f( reader );

   ColorI packed;
   U32 type = 0;
   if( !f.takeRawFloat( mRadius ) ||
       !f.takeColor( packed ) ||
       !f.takeInt( 32, type ) ||
       !f.takeRawFloat( mLength ) ||
       !takeName( f, mNodeName ) ||
       !f.takeRawPoint( mDirection ) ||
       !f.takeRawPoint( mOffset ) )
      return;

   mType = S32( type );

   // Four bytes on the wire; the game holds it as a ColorF.
   mColor = packed;

   // The texture is the one optional field, so by the time the cursor could be
   // standing on anything else the layout has ended.
   takeName( f, mTexture );
}


//-----------------------------------------------------------------------------

JetEffectData::JetEffectData()
   : mCoolColor( 1.0f, 1.0f, 1.0f, 1.0f ),
     mHotColor( 1.0f, 1.0f, 1.0f, 1.0f ),
     mActivateTime( 0.0f ),
     mDeactivateTime( 0.0f ),
     mLength( 0.0f ),
     mWidth( 0.0f ),
     mSpeed( 0.0f ),
     mStretch( 0.0f ),
     mYOffset( 0.0f ),
     mTexture( StringTable->EmptyString() )
{
}

void JetEffectData::readT2Fields( T2Reader &reader )
{
   Parent::readT2Fields( reader );

   T2Fields f( reader );

   ColorI cool, hot;
   if( !f.takeColor( cool ) || !f.takeColor( hot ) )
      return;

   mCoolColor = cool;
   mHotColor = hot;

   if( !f.takeRawFloat( mActivateTime ) ||
       !f.takeRawFloat( mDeactivateTime ) ||
       !f.takeRawFloat( mLength ) ||
       !f.takeRawFloat( mWidth ) ||
       !f.takeRawFloat( mSpeed ) ||
       !f.takeRawFloat( mStretch ) ||
       !f.takeRawFloat( mYOffset ) )
      return;

   takeName( f, mTexture );
}


//-----------------------------------------------------------------------------

TSShapeConstructorT2::TSShapeConstructorT2()
   : mShape( StringTable->EmptyString() ),
     mSequences( __FILE__, __LINE__ )
{
}

void TSShapeConstructorT2::readT2Fields( T2Reader &reader )
{
   Parent::readT2Fields( reader );

   T2Fields f( reader );

   if( !takeName( f, mShape ) )
      return;

   // The count is the seven bits the cursor spent choosing between the loop
   // and the end, so it is not a field to take -- but nothing is lost by that,
   // because how many strings arrive is the count.  The body is one node, so
   // standing on it is what says another iteration is due.
   StringTableEntry sequence;
   while( f.atNode( 4 ) && takeName( f, sequence ) )
      mSequences.push_back( sequence );
}

bool TSShapeConstructorT2::onAdd()
{
   if( !Parent::onAdd() )
      return false;

   if( !mShape[ 0 ] || mSequences.empty() )
      return true;

   // The shape and every .dsq beside it are named bare, and both live under
   // shapes/, so the same rule puts the directory back on each.
   const String path = shapePathT2( mShape );

   // A client that already has a TSShapeConstructor for this shape -- one
   // defined in script -- is extended rather than duplicated: a second object
   // on the same shape is refused by TSShapeConstructor::onAdd, with a
   // complaint, and a mission sends thirteen of these.
   TSShapeConstructor *ctor = TSShapeConstructor::findShapeConstructor( path );
   const bool mine = !ctor;
   if( mine )
   {
      // Built through the default constructor and the field, not through
      // TSShapeConstructor(const String&): that one sets the path and leaves
      // mShape uninitialised, where the default constructor nulls it, and
      // _onLoad() reads mShape before it writes it.
      static StringTableEntry sBaseShape = StringTable->insert( "baseShape" );
      ctor = new TSShapeConstructor();
      ctor->setDataField( sBaseShape, NULL, path.c_str() );
   }

   // Through the field rather than the vector behind it: `sequence` takes the
   // "<file> <name>" pair in exactly the form Tribes 2 sends it, and splitting
   // it here to put it back together would be two copies of one rule.
   static StringTableEntry sSequence = StringTable->insert( "sequence" );
   for( U32 i = 0; i < mSequences.size(); i++ )
   {
      const char *pair = mSequences[ i ];
      const char *space = dStrchr( pair, ' ' );
      if( !space )
         continue;

      char file[ 256 ];
      const U32 length = getMin( U32( space - pair ), U32( sizeof( file ) - 1 ) );
      dStrncpy( file, pair, length );
      file[ length ] = 0;

      char qualified[ 512 ];
      dSprintf( qualified, sizeof( qualified ), "%s%s",
                shapePathT2( file ).c_str(), space );

      ctor->setDataField( sSequence, NULL, qualified );
   }

   // Registered last, so the sequences are in place for the onAdd that applies
   // them to a shape this client has already loaded.
   if( mine && !ctor->registerObject() )
   {
      delete ctor;
      return true;
   }

   return true;
}
