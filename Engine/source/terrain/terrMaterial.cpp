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
#include "terrain/terrMaterial.h"
#include "console/consoleTypes.h"
#include "gfx/bitmap/gBitmap.h"


IMPLEMENT_CONOBJECT( TerrainMaterial );

ConsoleDocClass( TerrainMaterial,
	"@brief The TerrainMaterial class orginizes the material settings "
	"for a single terrain material layer.\n\n"

	"@note You should not be creating TerrainMaterials by hand in code. "
	"All TerrainMaterials should be created in the editors, as intended "
	"by the system.\n\n"

	"@tsexample\n"
	"// Created by the Terrain Painter tool in the World Editor\n"
	"new TerrainMaterial()\n"
	"{\n"
	"	internalName = \"grass1\";\n"
	"	diffuseMap = \"art/terrains/Test/grass1\";\n"
	"	detailMap = \"art/terrains/Test/grass1_d\";\n"
	"	detailSize = \"10\";\n"
	"	isManaged = \"1\";\n"
	"	detailBrightness = \"1\";\n"
	"	Enabled = \"1\";\n"
	"	diffuseSize = \"200\";\n"
	"};\n"
	"@endtsexample\n\n"

	"@see Materials\n"

	"@ingroup enviroMisc\n");

TerrainMaterial::TerrainMaterial()
   :  mSideProjection( false ),
      mDiffuseSize( 500.0f ),
      mDetailSize( 5.0f ),
      mDetailStrength( 1.0f ),
      mDetailDistance( 50.0f ),
      mParallaxScale( 0.0f )
{
}

TerrainMaterial::~TerrainMaterial()
{
}

void TerrainMaterial::initPersistFields()
{
   addField( "diffuseMap", TypeStringFilename, Offset( mDiffuseMap, TerrainMaterial ), "Base texture for the material" );
   addField( "diffuseSize", TypeF32, Offset( mDiffuseSize, TerrainMaterial ), "Used to scale the diffuse map to the material square" );

   addField( "normalMap", TypeStringFilename, Offset( mNormalMap, TerrainMaterial ), "Bump map for the material" );
   
   addField( "detailMap", TypeStringFilename, Offset( mDetailMap, TerrainMaterial ), "Detail map for the material" );
   addField( "detailSize", TypeF32, Offset( mDetailSize, TerrainMaterial ), "Used to scale the detail map to the material square" );

   addField( "detailStrength", TypeF32, Offset( mDetailStrength, TerrainMaterial ), "Exponentially sharpens or lightens the detail map rendering on the material" );
   addField( "detailDistance", TypeF32, Offset( mDetailDistance, TerrainMaterial ), "Changes how far camera can see the detail map rendering on the material" );
   addField( "useSideProjection", TypeBool, Offset( mSideProjection, TerrainMaterial ),"Makes that terrain material project along the sides of steep "
	   "slopes instead of projected downwards");
   addField( "parallaxScale", TypeF32, Offset( mParallaxScale, TerrainMaterial ), "Used to scale the height from the normal map to give some self "
	   "occlusion effect (aka parallax) to the terrain material" );

   Parent::initPersistFields();

   // Gotta call this at least once or it won't get created!
   Sim::getTerrainMaterialSet();
}

bool TerrainMaterial::onAdd()
{
   if ( !Parent::onAdd() )
      return false;

   SimSet *set = Sim::getTerrainMaterialSet();

   // Make sure we have an internal name set.
   if ( !mInternalName || !mInternalName[0] )
      Con::warnf( "TerrainMaterial::onAdd() - No internal name set!" );
   else
   {
      SimObject *object = set->findObjectByInternalName( mInternalName );
      if ( object )
         Con::warnf( "TerrainMaterial::onAdd() - Internal name collision; '%s' already exists!", mInternalName );
   }

   set->addObject( this );

   return true;
}

TerrainMaterial* TerrainMaterial::getWarningMaterial()
{ 
   return findOrCreate( NULL );
}

TerrainMaterial* TerrainMaterial::findOrCreate( const char *nameOrPath )
{
   SimSet *set = Sim::getTerrainMaterialSet();
   
   if ( !nameOrPath || !nameOrPath[0] )
      nameOrPath = "warning_material";

   // See if we can just find it.
   TerrainMaterial *mat = dynamic_cast<TerrainMaterial*>( set->findObjectByInternalName( StringTable->insert( nameOrPath ) ) );
   if ( mat )
      return mat;

   // We didn't find it... so see if its a path to a
   // file.  If it is lets assume its the texture.
   if ( GBitmap::sFindFiles( nameOrPath, NULL ) )
   {
      mat = new TerrainMaterial();
      mat->setInternalName( nameOrPath );
      mat->mDiffuseMap = nameOrPath;
      mat->registerObject();
      Sim::getRootGroup()->addObject( mat );
      return mat;
   }

   // HH: Added for Tribes 2 compatibility.
   // A Tribes 2 terrain names a material, not a file: retail resolves it by
   // formula (0x00794ac4, "textures/terrain/%s.png") with a leading "terrain."
   // dropped, and falls back on textures/terrain/Default.
   String name( nameOrPath );
   if ( name.startsWith( "terrain." ) )
      name = name.substr( 8 );

   // The name's own dots are part of it -- setFileName keeps them where
   // constructing a Path from the joined string would read the last one as an
   // extension and look for `LushWorld.png`.
   Torque::Path t2Path;
   t2Path.setPath( "textures/terrain" );
   t2Path.setFileName( name );

   // Keep the extension the search found: anything that reads the map back
   // has to build a Path from it, and that reads a dot as one.
   Vector<Torque::Path> found;
   if ( !GBitmap::sFindFiles( t2Path, &found ) )
   {
      t2Path.setFileName( "Default" );
      if ( GBitmap::sFindFiles( t2Path, &found ) )
         Con::warnf( "TerrainMaterial: no texture for '%s'; using the "
                     "terrain default.", nameOrPath );
   }

   if ( found.size() )
   {
      mat = new TerrainMaterial();
      mat->setInternalName( nameOrPath );
      mat->mDiffuseMap = found.first().getFullPath();
      mat->registerObject();
      Sim::getRootGroup()->addObject( mat );
      return mat;
   }

   // Ok... return a debug material then.
   mat = dynamic_cast<TerrainMaterial*>( set->findObjectByInternalName( StringTable->insert( "warning_material" ) ) );
   if ( !mat )
   {
      // This shouldn't happen.... the warning_texture should
      // have already been defined in script, but we put this
      // fallback here just in case it gets "lost".
      mat = new TerrainMaterial();
      mat->setInternalName( "warning_material" );
      mat->mDiffuseMap = "core/art/warnMat.png";
      mat->mDiffuseSize = 500;
      mat->mDetailMap = "core/art/warnMat.png";
      mat->mDetailSize = 5;
      mat->registerObject();
      
      Sim::getRootGroup()->addObject( mat );
   }

   return mat;
}
