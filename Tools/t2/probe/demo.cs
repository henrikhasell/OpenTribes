// Retail names these three callbacks in its own exe, and calls them from
// inside the start-block decode.  Overriding them counts the decode without
// needing a frame to be drawn.
function demoUpdateDatablockProgress(%count, %total)
{
   $dbCalls = $dbCalls + 1;
   $dbCount = %count;
   $dbTotal = %total;
}

function clientReceivedDataBlock(%index, %total)
{
   $cbCalls = $cbCalls + 1;
}

function demoPlaybackComplete()
{
   $complete = $complete + 1;
}

function probeDemo()
{
   echo("PROBE dbCalls=" @ $dbCalls @ " dbCount=" @ $dbCount @
        " dbTotal=" @ $dbTotal @ " cbCalls=" @ $cbCalls @
        " complete=" @ $complete @ " conn=" @ isObject(ServerConnection));
}

// Find a connection anywhere in the Sim, not only in RootGroup.
function scanConns(%hi)
{
   %found = 0;
   for (%i = 1; %i < %hi; %i++)
   {
      if (!isObject(%i))
         continue;
      if (strstr(%i.getClassName(), "Connection") != -1)
      {
         echo("SCANCONN id=" @ %i @ " class=" @ %i.getClassName() @
              " name=" @ %i.getName() @ " count=" @ %i.getCount());
         %found++;
      }
   }
   echo("SCANCONN-DONE found=" @ %found);
}

// Tally the world classes a start block builds, by id rather than by group.
function scanGhosts(%hi)
{
   %classes = "";
   for (%i = 1; %i < %hi; %i++)
   {
      if (!isObject(%i))
         continue;
      %c = %i.getClassName();
      if (%c $= "Player" || %c $= "StaticShape" || %c $= "InteriorInstance" ||
          %c $= "TerrainBlock" || %c $= "WayPoint" || %c $= "SpawnSphere" ||
          %c $= "AIObjective" || %c $= "AudioEmitter" || %c $= "Item" ||
          %c $= "Camera" || %c $= "Sky" || %c $= "Sun" ||
          %c $= "Precipitation" || %c $= "MissionArea" ||
          %c $= "LinearProjectile" || %c $= "MissionMarker")
      {
         if ($g[%c] $= "") %classes = %classes @ " " @ %c;
         $g[%c] = $g[%c] + 1;
      }
   }
   %k = getWordCount(%classes);
   for (%i = 0; %i < %k; %i++)
   {
      %c = getWord(%classes, %i);
      echo("SCAN " @ %c @ " " @ $g[%c]);
   }
   echo("SCAN-DONE");
}
