//-----------------------------------------------------------------------------
// Client-side default console values.
//-----------------------------------------------------------------------------

$pref::Player::Name = "Apple Picker";
$pref::Player::defaultFov = 90;
$pref::Player::zoomSpeed = 0;

$pref::Net::LagThreshold = 400;

$pref::shadows = "2";
$pref::HudMessageLogSize = 40;
$pref::ChatHudLength = 1;

$pref::Input::LinkMouseSensitivity = 1;
$pref::Input::KeyboardEnabled = 1;
$pref::Input::MouseEnabled = 1;
$pref::Input::JoystickEnabled = 0;
$pref::Input::KeyboardTurnSpeed = 0.1;

$pref::sceneLighting::cacheSize = 20000;
$pref::sceneLighting::purgeMethod = "lastCreated";
$pref::sceneLighting::cacheLighting = 1;
$pref::sceneLighting::terrainGenerateLevel = 1;

$pref::Terrain::DynamicLights = 1;
$pref::Terrain::enableDetails = 1;
$pref::Interior::TexturedFog = 0;

//-----------------------------------------------------------------------------
// Video.
//
// $pref::Video::allowOpenGL is MANDATORY on Linux and is the one pref that is
// fatal if missing.  platformVideo.cc:243: when the OpenGL device exists but
// this is falsy, setDevice() recurses into setDevice("D3D") -- which does not
// exist on this platform -- and the 640x480 retry re-enters the same branch,
// ending in ImmediateShutdown(1).  Con::getBoolVariable returns false for an
// undefined variable, so "unset" is the failing case.

$pref::Video::displayDevice = "OpenGL";
$pref::Video::allowOpenGL = 1;
$pref::Video::allowD3D = 1;          // inert on Linux, harmless
$pref::Video::preferOpenGL = 1;
$pref::Video::appliedPref = 0;
$pref::Video::disableVerticalSync = 1;
$pref::Video::monitorNum = 0;
$pref::Video::fullScreen = 0;
$pref::Video::windowedRes = "1280 720";
$pref::Video::resolution = "1280 720 32";
$pref::Video::screenShotFormat = "PNG";

$pref::OpenGL::force16BitTexture = "0";
$pref::OpenGL::forcePalettedTexture = "0";
$pref::OpenGL::maxHardwareLights = 3;
$pref::VisibleDistanceMod = 1.0;

//-----------------------------------------------------------------------------
// Editor.

$pref::Editor::visibleDistance = 2100;
$pref::SkyOn = 1;
$pref::CloudsOn = 1;

$Pref::ResourceManager::excludedDirectories = ".svn;CVS;.git";

//-----------------------------------------------------------------------------
// Developer telnet console.
//
// The engine has a full TCP console built in: telnetSetParameters() opens a
// listening socket, TelConsole->process() is pumped every frame from the main
// loop (engine/game/main.cc), and every line the console prints is mirrored to
// each connected client.  That turns a running engine into a REPL -- you can
// inspect and mutate a live editor session instead of paying a full reboot per
// question.  ../probe.sh drives it.
//
// Off by default.  Enable per run with `./run.sh -telnet 4711`, or set a port
// here to have it always on.
$pref::Telnet::Port = 0;

// Empty password: a bare newline grants full read/write access (the password
// compare in telnetConsole.cc matches "" against ""). That is only acceptable
// because of BindAddress below.
$pref::Telnet::ConsolePass = "";
$pref::Telnet::ListenPass = "";

// Bind every listening socket -- the telnet console included -- to loopback,
// so a debug console is never exposed to the network.  Net::bind() reads this
// (platformX86UNIX/x86UNIXNet.cc:691); empty means INADDR_ANY.  Clear it if you
// ever want this machine to host a real multiplayer server.
$Pref::Net::BindAddress = "127.0.0.1";

//-----------------------------------------------------------------------------
// Audio.

$pref::Audio::driver = "OpenAL";
$pref::Audio::forceMaxDistanceUpdate = 0;
$pref::Audio::environmentEnabled = 0;
$pref::Audio::masterVolume   = 0.8;
$pref::Audio::channelVolume1 = 0.8;
$pref::Audio::channelVolume2 = 0.8;
$pref::Audio::channelVolume3 = 0.8;
$pref::Audio::channelVolume4 = 0.8;
$pref::Audio::channelVolume5 = 0.8;
$pref::Audio::channelVolume6 = 0.8;
$pref::Audio::channelVolume7 = 0.8;
$pref::Audio::channelVolume8 = 0.8;
