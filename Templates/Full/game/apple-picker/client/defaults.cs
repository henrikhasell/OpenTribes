//-----------------------------------------------------------------------------
// Client-side default console values.
//
// Most of what this file used to set by hand is now in
// core/scripts/client/defaults.cs, so exec that first and only override what
// this game actually wants differently.  In particular the whole TGE video
// block is gone: $pref::Video::allowOpenGL, preferOpenGL, OpenGL::* and the
// 16-bit/paletted texture flags have no reader left in the engine, and the
// device is chosen by GFXInit::getBestAdapterChoice(), which falls through to
// the GL adapter on this platform whatever $pref::Video::displayDevice says.
//-----------------------------------------------------------------------------

exec("core/scripts/client/defaults.cs");

$pref::Player::Name = "Apple Picker";
$pref::Player::defaultFov = 90;
$pref::Player::zoomSpeed = 0;

$pref::Video::mode = "1280 720 false 32 60 4";

// This game runs on the Basic Lighting manager rather than letting
// $lightManager::defaults pick Advanced Lighting.  The deferred path is not in
// a usable state on this branch, and nothing here needs it -- one directional
// light, a terrain and six shapes.
$pref::lightManager = "Basic Lighting";


//-----------------------------------------------------------------------------
// Developer telnet console.
//
// The engine has a full TCP console built in: telnetSetParameters() opens a
// listening socket, TelnetConsole::process() is pumped every frame, and every
// line the console prints is mirrored to each connected client.  That turns a
// running engine into a REPL -- you can inspect and mutate a live session
// instead of paying a full reboot per question.
//
// Off by default.  Enable per run with `-telnet 4711`, or set a port here to
// have it always on.
$pref::Telnet::Port = 0;

// Empty password: a bare newline grants full read/write access (the password
// compare in telnetConsole.cpp matches "" against "").
//
// TGE's $Pref::Net::BindAddress, which pinned every listening socket to
// loopback, has no reader in this engine -- so an open port here is an open
// port on every interface.  Set a password if you enable one off a laptop.
$pref::Telnet::ConsolePass = "";
$pref::Telnet::ListenPass = "";
