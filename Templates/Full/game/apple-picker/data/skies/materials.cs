//-----------------------------------------------------------------------------
// The sky cube from sky_day.dml, as a cubemap.
//
// TGE's Sky read the .dml as a flat material list and drew it as six quads.
// Sky::renderSkyBox walks sides 0..5 as four walls, then the top, then the
// bottom, and Sky::initSkyData lays the top corners out counter-clockwise from
// (-r,-r), so side 0 spans SW to SE and the walls run south, east, north, west.
// mSkyHandle takes entries 0..5; EnvMapMaterialOffset is 6, so day_0007 is the
// environment map rather than a face, and CloudMaterialOffset is 7, so the two
// cloud1 entries after it are the cloud layers.
//
//   day_0001  south    day_0003  north    day_0005  top
//   day_0002  east     day_0004  west     day_0006  bottom
//
// The cube_N.png faces are those images turned to suit.  TGE drew each wall as
// its own quad with its own texcoords -- mTexCoord is (0,0),(1,0),(0,1),(1,1)
// with (0,0) at the wall's top-left -- so all four could be stored the same way
// up.  A cubemap is one sampler reached through cubeTrans, which takes Torque's
// Z-up world into the cubemap's Y-up space, and that rotation lands on each
// side face differently.  No reordering fixes it, which is why stock Torque
// skyboxes ship their faces pre-turned.
//
// Which index faces which way, and how far each is turned, was measured rather
// than reasoned about: six numbered faces, each a flat colour with a triangle
// pointing at its own top edge and a marker in its top-left corner, then
// photographed from all six directions in the level.
//
//   cubeFace[0]  east    day_0002  rotated  90 CCW
//   cubeFace[1]  west    day_0004  rotated  90 CW
//   cubeFace[2]  north   day_0003  rotated 180
//   cubeFace[3]  south   day_0001  as-is
//   cubeFace[4]  top     day_0005  as-is
//   cubeFace[5]  bottom  cube_4    reused, never drawn
//
// That test also showed the corner marker landing on the far side from where
// it sits in the source, so the cubemap is mirrored when viewed from inside.
// TGE mirrored too -- on the south wall it puts u = 0 at the west corner, which
// is on your right when you face south -- so the two cancel and the faces need
// no flip, only the turn.
//
// The bottom is cube_4 because day_0006 is 4x4 where every other face is
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
