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
#include "lighting/common/scenePersist.h"

#include "lighting/lightingInterfaces.h"
#include "scene/sceneManager.h"
#include "lighting/lightManager.h"


U32 PersistInfo::smFileVersion   = 0x11;

// HH: Added for Tribes 2 compatibility.
// Tribes 2's cached mission lighting is this same format one version back.
// Retail's own GameData has two of them -- Talus_d615e81a.ml and
// Training4_2e3c4309.ml -- and both open 16 where this engine writes 17, with
// everything under the version matching field for field: a chunk count, a
// Mission chunk first carrying its CRC, then a Terrain chunk, then the
// interiors.  Rejecting them on the version alone throws away the only lit
// interior lightmaps on the disk.
static const U32 smTribes2FileVersion = 0x10;

bool PersistInfo::isReadableVersion(U32 version)
{
	return(version == smFileVersion || version == smTribes2FileVersion);
}

PersistInfo::~PersistInfo()
{
	for(U32 i = 0; i < mChunks.size(); i++)
		delete mChunks[i];
}

//------------------------------------------------------------------------------
bool PersistInfo::read(Stream & stream)
{
	U32 version;
	if(!stream.read(&version) || !isReadableVersion(version))
		return(false);

	mVersion = version;

	U32 numChunks;
	if(!stream.read(&numChunks))
		return(false);

	if(numChunks == 0)
		return(false);

	// read in all the chunks
	for(U32 i = 0; i < numChunks; i++)
	{
		U32 chunkType;
		if(!stream.read(&chunkType))
			return(false);

		// MissionChunk must be first chunk
		if(i == 0 && chunkType != PersistChunk::MissionChunkType)
			return(false);
		if(i != 0 && chunkType == PersistChunk::MissionChunkType)
			return(false);

      // Create the right chunk for the system 
      bool bChunkFound = false; 
      SceneLightingInterfaces sli = LIGHTMGR->getSceneLightingInterface()->mAvailableSystemInterfaces;
      for(SceneLightingInterface** itr = sli.begin(); itr != sli.end() && !bChunkFound; itr++)
      {
         PersistInfo::PersistChunk* pc = (*itr)->createPersistChunk(chunkType);
         if (pc != NULL)
         {
            mChunks.push_back(pc);
            bChunkFound = true;            
         }
      }
         
      if (!bChunkFound)
      {
		   // create the chunk
		   switch(chunkType)
		   {
		   case PersistChunk::MissionChunkType:
			   mChunks.push_back(new PersistInfo::MissionChunk);
			   break;

		   default:
			   return(false);
			   break;
		   }
      }

		// load the chunk info
		if(!mChunks[i]->read(stream))
			return(false);
	}

	return(true);
}

bool PersistInfo::write(Stream & stream)
{
	if(!stream.write(smFileVersion))
		return(false);

	if(!stream.write((U32)mChunks.size()))
		return(false);

	for(U32 i = 0; i < mChunks.size(); i++)
	{
		if(!stream.write(mChunks[i]->mChunkType))
			return(false);
		if(!mChunks[i]->write(stream))
			return(false);
	}

	return(true);
}

//------------------------------------------------------------------------------
// Class SceneLighting::PersistInfo::PersistChunk
//------------------------------------------------------------------------------
bool PersistInfo::PersistChunk::read(Stream & stream)
{
	if(!stream.read(&mChunkCRC))
		return(false);
	return(true);
}

bool PersistInfo::PersistChunk::write(Stream & stream)
{
	if(!stream.write(mChunkCRC))
		return(false);
	return(true);
}

//------------------------------------------------------------------------------
// Class SceneLighting::PersistInfo::MissionChunk
//------------------------------------------------------------------------------
PersistInfo::MissionChunk::MissionChunk()
{
	mChunkType = PersistChunk::MissionChunkType;
}