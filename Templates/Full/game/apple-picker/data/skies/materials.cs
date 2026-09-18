//-----------------------------------------------------------------------------
// The sky cube from sky_day.dml, as a cubemap.
//
// TGE's Sky read the .dml as a flat material list and drew it as six quads.
// Sky::renderSkyBox walks sides 0..5 as four walls, then the top, then the
// bottom; Sky::initSkyData lays the top corners out counter-clockwise from
// (-r,-r), so side 0 spans SW to SE and the walls run south, east, north, west.
// Everything from index CloudMaterialOffset (7) on is a cloud layer, which is
// why the list has seven sky entries for six faces -- day_0007 sits in the
// unused seventh slot.
//
// The cubeFace order documented at gfx/sim/cubemapData.cpp:81-86 names its
// axes in the cubemap's own Y-up space, not Torque's Z-up world, so the two
// it calls -Y and +Y are the top and the bottom and the first four are the
// walls.  core/art/skies/night settles it: skybox_5 is cubeFace[4] and is all
// stars with no horizon, skybox_6 is cubeFace[5] and is flat ground colour.
//
//   day_0001   side 0   south wall   cubeFace[2]
//   day_0002   side 1   east wall    cubeFace[1]
//   day_0003   side 2   north wall   cubeFace[3]
//   day_0004   side 3   west wall    cubeFace[0]
//   day_0005   side 4   top          cubeFace[4]
//   day_0006   side 5   bottom       cubeFace[5]
//
// The cube_N.png faces are those six images turned to suit.  TGE drew each
// wall as its own quad with its own texcoords, so all four could be stored the
// same way up.  A cubemap is one sampler and the direction vector reaches it
// through cubeTrans, which takes Torque's Z-up world into the cubemap's Y-up
// space; that rotation lands on each side face differently, so the walls come
// out at four different angles.  No amount of reordering fixes it, which is
// why stock Torque skyboxes ship their faces pre-turned.  Measured by standing
// in the level and looking at each wall in turn:
//
//   cubeFace[0]  west    day_0004  rotated  90 CCW
//   cubeFace[1]  east    day_0002  rotated  90 CW
//   cubeFace[2]  north   day_0003  rotated 180
//   cubeFace[3]  south   day_0001  as-is
//   cubeFace[4]  top     day_0005  as-is
//   cubeFace[5]  bottom  cube_4    reused, never drawn
//
// The bottom is day_0005 because day_0006 is 4x4 where every other face is
// 256x256 -- TGE set renderBottomTexture = 0 and never drew it, so nobody
// noticed.  A cubemap needs all six faces the same size and one that does not
// match invalidates the whole thing, which then samples black in every
// direction, sky included.  SkyBox gets drawBottom = false for the same reason
// TGE had renderBottomTexture = 0, so this slot is never on screen.
//
// PNG rather than JPEG so that turning them does not re-compress an already
// lossy source.
//-----------------------------------------------------------------------------

singleton CubemapData( ApplePickerSkyCubemap )
{
   cubeFace[0] = "./cube_0";
   cubeFace[1] = "./cube_1";
   cubeFace[2] = "./cube_2";
   cubeFace[3] = "./cube_3";
   cubeFace[4] = "./cube_4";
   cubeFace[5] = "./cube_4";
};

singleton Material( ApplePickerSkyMat )
{
   cubemap = "ApplePickerSkyCubemap";
   materialTag0 = "Skies";
};
