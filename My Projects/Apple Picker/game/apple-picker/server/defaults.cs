//-----------------------------------------------------------------------------
// Server-side default console values.
//
// core/scripts/server/defaults.cs is the same list this file used to spell out
// by hand, so exec it and override only the three lines that describe this
// game.  It also carries the kick/ban and time-limit prefs that TGE's version
// did not have and that core's kickban.cs reads.
//-----------------------------------------------------------------------------

exec("core/scripts/server/defaults.cs");

$Pref::Server::Name = "Apple Picker";
$Pref::Server::Info = "Walk around and pick up five apples.";

$Pref::Server::ConnectionError =
   "You do not have the correct version of Apple Picker or the related art "@
   "needed to connect to this server, please contact the server operator to "@
   "obtain the latest version of this game.";
