//-----------------------------------------------------------------------------
// The game: five apples on the plain, and a victory box on the fifth.
//
// An ItemData, not the StaticShapeData that tutorial.base's logoitem.cs used
// for this same shape, and that is the whole point: only ItemObjectType is in
// sServerCollisionContactMask (game/player.cc:92), so only an Item gets a
// walk-over collision out of Player::findContact.  A StaticShape would just be
// scenery you bump into.
//
// Modelled on starter.fps's HealthPatch (server/scripts/health.cs:49-76): the
// same maxInventory = 0 walk-over patch, with no inventory system behind it.
// Nothing here calls incInventory, so none of inventory.cs is needed.
//
// Three hooks, all through the package, so this file plus its one exec line in
// server/init.cs is the whole feature -- the same shape as animation-test's
// server/switch.cs:
//
//   onServerCreated              registers the datablock.  It has to be here
//                                and not at init time -- every destroyServer()
//                                ends in deleteDataBlocks().  A `datablock`
//                                declaration is an ordinary statement in the
//                                grammar (console/CMDgram.y:181), so it is legal
//                                inside a function.
//   onMissionLoaded              places the apples, once MissionGroup and
//                                MissionCleanup exist and the Terrain is up for
//                                getTerrainHeight().
//   GameConnection::onClientEnterGame  tells a joining client the score, so its
//                                HUD is right from its first frame.
//
// Re-exec is harmless: activatePackage returns early if the package is already
// active (console/consoleInternal.cc:955).
//-----------------------------------------------------------------------------

// Where the apples go, as mission x/y.  Z comes from the terrain, so this works
// on a hillside too.  A ring of radius 10 about the origin, with the mission's
// drop point 18m south of it looking north -- so the whole game is in frame the
// moment the loading screen comes down.
$ApplePicker::spawn[0] = "0 10";
$ApplePicker::spawn[1] = "-9.51 3.09";
$ApplePicker::spawn[2] = "-5.88 -8.09";
$ApplePicker::spawn[3] = "5.88 -8.09";
$ApplePicker::spawn[4] = "9.51 3.09";
$ApplePicker::spawnCount = 5;

// torque_logo.dts is about 2m on a side at scale 1 -- the same size as the
// player.  0.6 brings it down to something you pick up rather than walk into.
//
// The height matters more than it looks.  Pickup is
// getWorldBox().isOverlapped(item->getWorldBox()) (game/player.cc:2786) against
// the player's OWN collision box, which runs from the feet up:
// PlayerData::pickupRadius does NOT widen it, despite appearances -- findContact
// computes an expanded box at player.cc:2742 and then never uses it.  So an
// apple hovering above head height would be unreachable.  Scale and height are
// the two knobs; pickupRadius is not one.
$ApplePicker::scale  = "0.6 0.6 0.6";
$ApplePicker::height = 0.9;


package applePicker {

function onServerCreated()
{
   Parent::onServerCreated();
   applePickerRegister();
}

function onMissionLoaded()
{
   Parent::onMissionLoaded();
   applePickerPlace();
}

function GameConnection::onClientEnterGame(%this)
{
   Parent::onClientEnterGame(%this);
   applePickerTell(%this);
}

}; // applePicker package

activatePackage(applePicker);


//-----------------------------------------------------------------------------

function applePickerRegister()
{
   datablock ItemData(AppleItem)
   {
      // Shows up under "shapes" in the Mission Editor's Creator pane.
      category = "Apple Picker";

      // tutorial.base's own art, copied in with the rest of the mod.  "~"
      // resolves to the codeblock's mod root, i.e. apple-picker.
      shapeFile = "~/data/shapes/3dtorquelogo/torque_logo.dts";

      mass = 1;
      friction = 1;
      elasticity = 0.3;

      // No pickup, no throw -- this is a walk-over patch, and nothing in this
      // mod has an inventory.  HealthPatch's line.
      maxInventory = 0;

      pickupName = "an apple";
   };
}

// What the Creator pane calls to place one by hand.  Same as starter.fps's
// (server/scripts/item.cs:118-129): the editor always makes them static and
// rotating.  Handy for trying positions -- drag one out, read the coordinates
// off the Inspector, paste them into $ApplePicker::spawn[] above.
function ItemData::create(%data)
{
   return new Item() {
      dataBlock = %data;
      static = true;
      rotate = true;
   };
}


//-----------------------------------------------------------------------------

function applePickerPlace()
{
   $ApplePicker::picked = 0;
   $ApplePicker::total  = 0;

   // MissionCleanup, not MissionGroup: these are placed by script every load,
   // and an object in MissionGroup would be written into the .mis the next time
   // someone saved from the Mission Editor -- after which it would be created
   // twice, once by the file and once by this.
   %group = new SimGroup(ApplePickerGroup);
   MissionCleanup.add(%group);

   for (%i = 0; %i < $ApplePicker::spawnCount; %i++)
   {
      %x = getWord($ApplePicker::spawn[%i], 0);
      %y = getWord($ApplePicker::spawn[%i], 1);
      %z = getTerrainHeight(%x SPC %y) + $ApplePicker::height;

      // No rotation field: Item::setTransform (game/item.cc:396-410) throws an
      // authored rotation away when rotate is true and substitutes the spin.
      %apple = new Item() {
         dataBlock = AppleItem;
         position  = %x SPC %y SPC %z;
         scale     = $ApplePicker::scale;
         static    = true;   // at rest from the first tick: floats, no gravity
         rotate    = true;   // client-side spin, so it reads as a pickup
      };

      // An Item whose datablock failed to preload -- a missing or unreadable
      // .dts -- does not register, and a failed registerObject leaves the object
      // unadded rather than throwing.  Say so here rather than leaving an empty
      // patch of terrain and one line in console.log to find later.
      if (!isObject(%apple))
      {
         error("applePickerPlace: could not create an Item for " @ AppleItem.shapeFile);
         continue;
      }

      %group.add(%apple);
      $ApplePicker::total++;
   }

   // The count the HUD shows is what was actually created, never the 5 asked
   // for, so a shape that failed to load cannot leave a game that says 5 and
   // can only ever reach 4.
   applePickerTellAll();
}


//-----------------------------------------------------------------------------
// Walk-over pickup.
//
// ShapeBase::notifyCollision (game/shapeBase.cc:2803-2827) calls onCollision on
// BOTH datablocks: first the player's with (%obj = player, %col = item), then
// the item's with (%obj = item, %col = player).  This is the second one.
//
// Server side only -- sClientCollisionContactMask has no ItemObjectType -- so
// there is no ghost firing this a second time.
//
// Watch for typos in the name: Con::execute returns "" silently for a missing
// datablock method (console/console.cc:926-934), so a misspelling here produces
// no error at all, just apples you walk through.  probe.py's smoke picks all
// five up for exactly that reason.

function AppleItem::onCollision(%this, %obj, %col, %vec, %vecLen)
{
   // queueCollision re-stamps every CollisionTimeoutValue = 250ms for as long
   // as the boxes overlap, so this fires more than once per apple.  The flag is
   // what stops one apple counting twice.
   if (%obj.picked)
      return;

   // Nothing else in this mission is a ShapeBase, but say what is meant.
   if (%col.getClassName() !$= "Player")
      return;

   %obj.picked = true;
   $ApplePicker::picked++;

   // Deleting the object inside its own collision callback is the stock path --
   // ItemData::onPickup does exactly this (starter.fps/server/scripts/item.cs:110)
   // -- and notifyCollision guards it: it holds a SimObjectPtr, and this
   // callback is the last thing it does with the item.
   %obj.delete();

   applePickerTellAll();

   if ($ApplePicker::picked >= $ApplePicker::total)
      applePickerWin();
}


//-----------------------------------------------------------------------------
// Talking to the client.  Both the counter and MessageBoxOK live on the client,
// so the server reaches them with commandToClient; the matching
// clientCmdAppleCount / clientCmdAppleVictory are in client/applePicker.cs.

function applePickerTell(%client)
{
   commandToClient(%client, 'AppleCount', $ApplePicker::picked, $ApplePicker::total);
}

function applePickerTellAll()
{
   for (%i = 0; %i < ClientGroup.getCount(); %i++)
      applePickerTell(ClientGroup.getObject(%i));
}

function applePickerWin()
{
   for (%i = 0; %i < ClientGroup.getCount(); %i++)
      commandToClient(ClientGroup.getObject(%i), 'AppleVictory', $ApplePicker::total);
}
