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
#include "console/simDatablock.h"
#include "console/simFieldDictionary.h"

#include "console/console.h"
#include "console/consoleInternal.h"
#include "console/engineAPI.h"
#include "T3D/gameBase/gameConnectionEvents.h"
#include "T3D/gameBase/gameConnection.h"
#include "T3D/gameBase/t2Layout.h"


IMPLEMENT_CO_DATABLOCK_V1(SimDataBlock);
SimObjectId SimDataBlock::sNextObjectId = DataBlockObjectIdFirst;
S32 SimDataBlock::sNextModifiedKey = 0;

ConsoleDocClass( SimDataBlock,
   "@brief \n"
   "@ingroup \n"
   
   "@section Datablock_Networking Datablocks and Networking\n"
   
   "@section Datablock_ClientSide Client-Side Datablocks\n"
);



//-----------------------------------------------------------------------------

SimDataBlock::SimDataBlock()
{
   setModDynamicFields(true);
   setModStaticFields(true);
}

//-----------------------------------------------------------------------------

bool SimDataBlock::onAdd()
{
   Parent::onAdd();

   // This initialization is done here, and not in the constructor,
   // because some jokers like to construct and destruct objects
   // (without adding them to the manager) to check what class
   // they are.
   modifiedKey = ++sNextModifiedKey;
   AssertFatal(sNextObjectId <= DataBlockObjectIdLast,
      "Exceeded maximum number of data blocks");

   // add DataBlock to the DataBlockGroup unless it is client side ONLY DataBlock
   if ( !isClientOnly() )
      if (SimGroup* grp = Sim::getDataBlockGroup())
         grp->addObject(this);
         
   Sim::getDataBlockSet()->addObject( this );

   return true;
}

//-----------------------------------------------------------------------------

// HH: Modified for Tribes 2 compatibility.
// Tribes 2 numbers datablocks from zero, so an id of zero means both "no id
// yet" and "the first datablock", and the range test this used -- which read
// an unnumbered object as client-only -- cannot fire. Every declared datablock
// kept the dynamic id the manager gave it instead, which left it out of
// DataBlockGroup, which is the group a server transmits.
void SimDataBlock::assignId()
{
   // The declaration this answers for always reaches it on a fresh object; one
   // already registered has the id it was registered with.
   if ( isProperlyAdded() )
      return;

   const SimObjectId id = sNextObjectId++;
   setId( id );

   // Zero is the one id registration reads as unset and replaces, so it has to
   // be forced -- as the datablocks arriving on a client are
   // (SimDataBlockEvent::process). Nothing registers between here and this
   // object's own registration: a datablock body holds fields, not
   // declarations.
   if ( !id )
      SimObject::setForcedId( id );
}

//-----------------------------------------------------------------------------

void SimDataBlock::onStaticModified(const char* slotName, const char* newValue)
{
   modifiedKey = sNextModifiedKey++;
}

//-----------------------------------------------------------------------------

// HH: Modified for Tribes 2 compatibility. The class publishes what it holds
// and the layout writes each field at its own node; see publishT2Data.
void SimDataBlock::packData(BitStream *stream, U32 classId)
{
   T2Layout::beginDataBlock();
   publishT2Data();
   T2Layout::writeDataBlock(stream, classId);
}

//-----------------------------------------------------------------------------

bool SimDataBlock::unpackData(BitStream *stream, U32 classId)
{
   return T2Layout::readDataBlock(stream, classId);
}

//-----------------------------------------------------------------------------

bool SimDataBlock::preload(bool, String&)
{
   return true;
}

//-----------------------------------------------------------------------------

void SimDataBlock::write(Stream &stream, U32 tabStop, U32 flags)
{
   // Only output selected objects if they want that.
   if((flags & SelectedOnly) && !isSelected())
      return;

   stream.writeTabs(tabStop);
   char buffer[1024];

   // Client side datablocks are created with 'new' while
   // regular server datablocks use the 'datablock' keyword.
   if ( isClientOnly() )
      dSprintf(buffer, sizeof(buffer), "new %s(%s) {\r\n", getClassName(), getName() ? getName() : "");
   else
      dSprintf(buffer, sizeof(buffer), "datablock %s(%s) {\r\n", getClassName(), getName() ? getName() : "");

   stream.write(dStrlen(buffer), buffer);
   writeFields(stream, tabStop + 1);

   stream.writeTabs(tabStop);
   stream.write(4, "};\r\n");
}

//=============================================================================
//    API.
//=============================================================================
// MARK: ---- API ----

//-----------------------------------------------------------------------------

DefineConsoleMethod( SimDataBlock, reloadOnLocalClient, void, (),,
   "Reload the datablock.  This can only be used with a local client configuration." )
{
   // Make sure we're running a local client.

   GameConnection* localClient = GameConnection::getLocalClientConnection();
   if( !localClient )
      return;

   // Do an in-place pack/unpack/preload.

   if( !object->preload( true, NetConnection::getErrorBuffer() ) )
   {
      Con::errorf( NetConnection::getErrorBuffer() );
      return;
   }

   U8 buffer[ 16384 ];
   BitStream stream( buffer, 16384 );

   const U32 classId = object->getClassId( localClient->getNetClassGroup() );

   object->packData( &stream, classId );
   stream.setPosition(0);
   object->unpackData( &stream, classId );

   if( !object->preload( false, NetConnection::getErrorBuffer() ) )
   {
      Con::errorf( NetConnection::getErrorBuffer() );
      return;
   }

   // Trigger a post-apply so that change notifications respond.
   object->inspectPostApply();
}

//-----------------------------------------------------------------------------

DefineConsoleFunction( preloadClientDataBlocks, void, (),,
   "Preload all datablocks in client mode.\n\n"
   "(Server parameter is set to false).  This will take some time to complete.")
{
   // we go from last to first because we cut 'n pasted the loop from deleteDataBlocks
   SimGroup *grp = Sim::getDataBlockGroup();
   String errorStr;
   for(S32 i = grp->size() - 1; i >= 0; i--)
   {
      AssertFatal(dynamic_cast<SimDataBlock*>((*grp)[i]), "Doh! non-datablock in datablock group!");
      SimDataBlock *obj = (SimDataBlock*)(*grp)[i];
      if (!obj->preload(false, errorStr))
         Con::errorf("Failed to preload client datablock, %s: %s", obj->getName(), errorStr.c_str());
   }
}

//-----------------------------------------------------------------------------

// HH: Added for Tribes 2 compatibility.
DefineConsoleFunction( t2RoundTrip, const char *, (SimDataBlock *block), (NULL),
   "@brief Write a datablock the Tribes 2 way, read it back into a fresh one "
   "of its class, and answer the fields that did not survive.\n\n"
   "The reader is the one measured against retail, so a field that comes back "
   "changed is one this engine would send a Tribes 2 client wrongly. The "
   "answer is \"bits=<n> ok\" or a list of field=sent/back, and the fields a "
   "Tribes 2 body does not carry are in it too -- the layout says which those "
   "are.\n\n@internal" )
{
   if( !block )
      return "no datablock";

   const U32 classId = block->getClassId( NetClassGroupGame );

   U8 buffer[ 32768 ];
   BitStream stream( buffer, sizeof( buffer ) );
   block->packData( &stream, classId );
   const U32 bits = stream.getPosition() * 8 + stream.getCurPos() % 8;

   SimObject *made = ( SimObject * ) ConsoleObject::create(
      NetClassGroupGame, NetClassTypeDataBlock, classId );
   SimDataBlock *back = dynamic_cast< SimDataBlock * >( made );
   if( !back )
   {
      delete made;
      return "could not make a second one of this class";
   }

   stream.setPosition( 0 );
   stream.clearCompressionPoint();
   const bool read = back->unpackData( &stream, classId );

   static char answer[ 4096 ];
   dSprintf( answer, sizeof( answer ), "bits=%u ", bits );
   if( !read )
   {
      dStrcat( answer, "the body would not read back" );
      delete back;
      return answer;
   }

   // Every field the class keeps, which is more than a body carries: the ones
   // a Tribes 2 body leaves out come back as the class's own defaults.
   U32 differing = 0;
   const AbstractClassRep::FieldList &fields = block->getClassRep()->mFieldList;
   for( U32 i = 0; i < fields.size(); i++ )
   {
      const AbstractClassRep::Field &field = fields[ i ];
      if( field.type == AbstractClassRep::DeprecatedFieldType ||
          field.type == AbstractClassRep::StartGroupFieldType ||
          field.type == AbstractClassRep::EndGroupFieldType ||
          field.type == AbstractClassRep::StartArrayFieldType ||
          field.type == AbstractClassRep::EndArrayFieldType )
         continue;

      for( U32 e = 0; e < field.elementCount; e++ )
      {
         char index[ 16 ];
         dSprintf( index, sizeof( index ), "%u", e );
         const char *sent = block->getDataField( field.pFieldname,
                                                 field.elementCount > 1 ? index : NULL );
         const char *got  = back->getDataField( field.pFieldname,
                                                field.elementCount > 1 ? index : NULL );
         if( !dStrcmp( sent ? sent : "", got ? got : "" ) )
            continue;

         differing++;
         char one[ 192 ];
         dSprintf( one, sizeof( one ), "%s%s=%.40s/%.40s", differing > 1 ? " " : "",
                   field.pFieldname, sent ? sent : "", got ? got : "" );
         if( dStrlen( answer ) + dStrlen( one ) < sizeof( answer ) - 1 )
            dStrcat( answer, one );
      }
   }

   // The fields a class never declared, which is where a script's values land
   // for a class this engine has only stubbed. Without these a stub looks like
   // it loses nothing because it is holding nothing the comparison can see.
   if( SimFieldDictionary *dynamic = block->getFieldDictionary() )
   {
      SimFieldDictionaryIterator each( dynamic );
      while( SimFieldDictionary::Entry *entry = *each )
      {
         ++each;

         const char *sent = entry->value;
         const char *got  = back->getDataField( entry->slotName, NULL );
         if( !dStrcmp( sent ? sent : "", got ? got : "" ) )
            continue;

         differing++;
         char one[ 192 ];
         dSprintf( one, sizeof( one ), "%s%s=%.40s/%.40s", differing > 1 ? " " : "",
                   entry->slotName, sent ? sent : "", got ? got : "" );
         if( dStrlen( answer ) + dStrlen( one ) < sizeof( answer ) - 1 )
            dStrcat( answer, one );
      }
   }

   if( !differing )
      dStrcat( answer, "ok" );

   delete back;
   return answer;
}

DefineConsoleFunction( deleteDataBlocks, void, (),,
   "Delete all the datablocks we've downloaded.\n\n"
   "This is usually done in preparation of downloading a new set of datablocks, "
   "such as occurs on a mission change, but it's also good post-mission cleanup." )
{
   // delete from last to first:
   SimGroup *grp = Sim::getDataBlockGroup();
   grp->deleteAllObjects();
   SimDataBlock::sNextObjectId = DataBlockObjectIdFirst;
   SimDataBlock::sNextModifiedKey = 0;
}
