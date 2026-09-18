# SDL3 Migration Plan

Target: build and run the `opentribes` branch (Torque3D 1.2, `9afd7949`, 2012-09-20) on
modern 64-bit Linux by replacing every platform-specific subsystem with SDL3 and its
satellite libraries, and dropping DirectX.

This is a clean-room migration. No code is carried back from later upstream branches.

## Scope

**Deleted** (~43,000 lines):

| Component | Lines | Reason |
|---|---:|---|
| `platformWin32/` | 14,475 | Replaced by SDL3 |
| `platformX86UNIX/` | 12,346 | Replaced by SDL3 |
| `platformMac/` | 5,864 | Replaced by SDL3 |
| `platformPOSIX/` | 727 | Replaced by SDL3 |
| `gfx/D3D9/` | 9,312 | Windows-only |
| `shaderGen/HLSL/` | 5,373 | Windows-only |

**Retained and rewritten:** `platform/` (18,678 lines) keeps its public interface —
`platformNet.h`, `platformFont.h`, `platformInput.h`, `platformTimer.h`, `platformVolume.h`,
`event.h` — and gains a single SDL3 implementation behind it.

**Retained as-is:** `gfx/gl/` (17,062 lines) and `shaderGen/GLSL/` (5,027 lines) become the
only render path.

## Library decisions

Three of the target libraries are not clean drop-ins. Stated once here; the plan proceeds
on the recommended resolution unless you say otherwise.

**Audio: OpenAL only. SDL3_mixer is not used.** SDL3_mixer cannot satisfy the SFX contract —
`sfx/sfxVoice.h:187-202` declares `setTransform(MatrixF)`, `setVelocity(VectorF)` and
`setMinMaxDistance()` as *pure* virtuals, and `sfx/sfxDevice.h:174-198` adds distance models,
Doppler factor and multi-listener support. SDL3_mixer offers basic positional panning and
attenuation, so implementing `SFXDevice` on it means stubbing most of that away.

The existing `sfx/openal/` backend (1,783 lines) already implements all of it:
`setTransform` (`sfxALVoice.cpp:216`), `setVelocity` (`:206`), `setMinMaxDistance` (`:187`),
`setVolume` (`:231`), `setPitch` (`:238`), `setRolloffFactor` (`:254`), `setCone` (`:245`),
and on the device `setListener` (`sfxALDevice.cpp:126`), `setDistanceModel` (`:149`),
`setDopplerFactor` (`:174`), `setRolloffFactor` (`:191`).

It is also almost entirely portable already. The only platform-specific files in the audio
stack are `sfx/openal/win32/LoadOAL.cpp` and `sfx/openal/mac/LoadOAL.mac.cpp`, runtime dlopen
shims that build the `OPENALFNTABLE`. Linking OpenAL Soft directly deletes both, so the
"no platform files" goal holds. openal-soft is fetched and built by CMake like everything else.

Decoding needs no new library either: `sfx/media/sfxVorbisStream.cpp` and `sfxWavStream.cpp`
sit on vendored libogg/libvorbis — portable C — and already produce the raw PCM OpenAL wants.

*Known gap, pre-existing:* `setReverb`/`resetReverb` are unimplemented. `aldlist.cpp:90`
detects `ALC_EXT_EFX` and sets a `SFXALEFX` caps flag that is never used, so `SFXEnvironment`
reverb is already dead in this backend. OpenAL Soft supports EFX — this is a feature to add
later, not a regression introduced by the migration.

**SDL3_net is a much narrower API than `platformNet.h`**, which is a full BSD-socket surface
(`openSocket`/`bind`/`listen`/`accept`/`connect`/`sendto`/`recvfrom`) plus asynchronous DNS
and `JournaledSignal` events for deterministic replay. Expect to implement the journaling
and `NetAddress` semantics by hand on top of SDL3_net rather than mapping one-to-one.

**SDL3_image does not handle DDS.** It replaces the six loaders in `gfx/bitmap/loaders/`
(bmp, gif, jpeg, mng, png, tga). `gfx/bitmap/ddsLoader.cpp`, `ddsFile.h` and `ddsUtils.cpp`
stay — DDS is GPU compressed-texture handling bound to `GFXFormat`, not image decode.

## Dependencies

**Policy: every dependency is fetched and built by CMake. Nothing comes from the system.**
No `find_package` for project dependencies, no `pkg-config`, no reliance on what happens to
be installed. `FetchContent_Declare` with `GIT_TAG` pinned to an exact release tag or commit
SHA — never a branch name — so a given checkout always produces the same dependency tree.
Set `FETCHCONTENT_UPDATES_DISCONNECTED=ON` for fast reconfigures, and expose
`FETCHCONTENT_SOURCE_DIR_<name>` overrides so an offline or air-gapped build can point at
local clones.

### Fetched

| Dependency | Upstream | Used for |
|---|---|---|
| SDL3 | `libsdl-org/SDL` | window, input, events, threads, TLS, filesystem, dlopen, CPU info |
| SDL3_image | `libsdl-org/SDL_image` | `gfx/bitmap/loaders/` |
| SDL3_ttf | `libsdl-org/SDL_ttf` | `PlatformFont` |
| SDL3_net | `libsdl-org/SDL_net` | `platformNet` |
| openal-soft | `kcat/openal-soft` | `SFXDevice` |
| zlib | `madler/zlib` | `core/util/zip/`, `gfx/gFont.cpp` |
| libogg, libvorbis | `xiph/ogg`, `xiph/vorbis` | `sfx/media/` decode |
| libtheora | `xiph/theora` | `core/ogg/` video playback |

All build as static subprojects. Disable their tests, examples and tools
(`ALSOFT_EXAMPLES`, `ALSOFT_UTILS`, `SDL_TEST_LIBRARY`, and equivalents) — exact option
names must be verified against each pinned tag, since they drift between releases.

### The six unmaintained libraries — replacement assessment

These have no maintained upstream to fetch, or upstream moved to an incompatible successor.
Each was assessed by actual call-site count, not by reputation.

**The dependency graph decides most of this.** `modules/collada.inc` defines
`DOM_INCLUDE_TINYXML` and `PCRE_STATIC` — `collada_dom` drags in both TinyXML *and* PCRE.
And PCRE has **zero** consumers anywhere in `Engine/source`; its only user in the entire tree
is `Engine/lib/collada/src/dae/daeURI.cpp`. So PCRE is not an engine dependency at all. Drop
COLLADA and it vanishes on its own.

| Library | Real call sites | Replacement | Verdict |
|---|---|---|---|
| `pcre` | 0 in engine (only `collada_dom`'s `daeURI.cpp`) | none needed | **Delete transitively** with COLLADA |
| `convexDecomp` | 1 — `ts/tsMeshFit.cpp` | **V-HACD** (`kmammou/v-hacd`) | **Swap.** Same code lineage (Ratcliff → HACD → V-HACD), better output, and it is editor-time collision fitting, not a runtime path |
| `tinyxml` | 5 — `console/SimXMLDocument.{h,cpp}`, `ts/tsShapeConstruct.cpp`, `ts/collada/colladaUtils.{h,cpp}` | **TinyXML-2** (same author) | **Swap.** Mechanical rename (`TiXmlDocument` → `tinyxml2::XMLDocument`). Drops to 3 files if COLLADA goes |
| `squish` | 2 — `gfx/bitmap/ddsUtils.cpp`, `ts/tsLastDetail.cpp` | **bc7enc_rdo** or ISPC Texture Compressor | **Swap.** Faster, better quality, adds BC7 |
| `collada_dom` | 6,129 lines across 16 files in `ts/collada/` | **Assimp**, or move import offline | **Judgment call** — see below |
| `opcode` | 5 — `ts/tsCollision.cpp`, `ts/tsMesh.cpp`, `ts/tsShapeInstance.{h,cpp}`, `forest/forestCollision.cpp`, `T3D/tsStatic.cpp` | `madmann91/bvh`, Embree, Bullet's `btOptimizedBvh` | **Defer** — see below |

**COLLADA.** Enabled by default (`Torque3D.php:122`), but it is an *asset import* path, not a
runtime one. Converting DAE to DTS offline with Assimp's CLI removes `collada_dom`, PCRE and
6,129 lines of `ts/collada/` from the engine entirely — by far the largest single reduction
available here. Keep in-engine import only if authoring workflow genuinely depends on it, in
which case Assimp is the replacement and `ts/collada/` gets rewritten against a different API.

**OPCODE is the risky one, and it is not free to keep.** It is runtime collision — ray and box
queries against `TSMesh` triangles — so a swap changes gameplay behavior and needs A/B
validation against the old implementation. But keeping it is not zero-cost either:
`OPC_OptimizedTree.cpp` packs node pointers into `udword` (32-bit) in three places, and
`OPC_SweepAndPrune.cpp` does it once more. That is a Phase 1 blocker regardless of whether the
library stays. Recommendation: repair in place for Phase 1, revisit after Phase 4 when there
is a running build to A/B against. If it is replaced, `madmann91/bvh` is the closest structural
match to what OPCODE provides.

**Recommended for this migration:** TinyXML-2, V-HACD and bc7enc_rdo are low-risk swaps with
small surfaces — do them. Drop COLLADA from the runtime and let PCRE go with it. Keep OPCODE
until Phase 4.

A scan for `Torque`/`GarageGames`/`TORQUE_` markers across these found almost none
(`squish` 1/30 files, `bullet` 7/403, the rest zero), so they are close to stock — but that is
weak evidence, not proof they are unpatched. Confirm before swapping any for an upstream.

### Deleted outright

`Engine/lib/lpng`, `ljpeg`, `lmng`, `lungif` (superseded by SDL3_image, Phase 6);
`Engine/lib/openal` (seven directories of per-platform OpenAL *headers* — `FreeBSD`, `LINUX`,
`macCarb`, `macosx`, `OpenBSD`, `win32`, `X86UNIX` — all obsolete once openal-soft is linked
directly, Phase 8); `Engine/lib/sdl` (32-bit SDL 1.2.5 binary); `d3dx8dll`.

`Engine/lib/zlib`, `libogg`, `libvorbis` and `libtheora` are deleted as vendored copies and
re-acquired through `FetchContent` — same libraries, current versions, one acquisition path.

`bullet` and `physX` are optional modules (`modules/bullet.inc`, `physX.inc`) and are not in
the `Full` configuration. Leave them out of the initial build; if Bullet is wanted later, note
that this is Bullet 2.7x and `bulletphysics/bullet3` is an API break.

### Caveats

**SDL3 still needs system development headers at build time.** Its X11, Wayland, ALSA and
PipeWire backends compile against those headers even though SDL dlopens the libraries at
runtime. "No system installs" holds for the dependency tree; it cannot hold at the
OS-integration boundary. Decide per backend which to enable and document the required headers.

**zlib needs a single owner.** SDL3_image's vendored path pulls its own libpng, which pulls
its own zlib, colliding with the `zlib` target fetched above. Pick one: either disable
SDL3_image's vendoring and hand it our zlib/libpng targets, or let it vendor and drop zlib
from the fetch list. Do not let both exist — duplicate target names fail configure, and
duplicate symbols fail the link.

**First configure is expensive.** SDL3 plus three satellites plus openal-soft plus the Xiph
stack is a large source fetch and a long cold build. Budget for it once; `FetchContent`
caches into the build tree afterward.

---

## Phase 0 — Replace the build system with CMake

The PHP `projectGenerator` cannot run here: Smarty 2.6.18 uses the PHP4 constructor
`function Smarty()` (`Smarty.class.php:568`) and `each()` (`Smarty_Compiler.class.php:560`),
both removed in PHP 8.0, against the system's PHP 8.5.9. The bundled `Engine/bin/php/php.exe`
is a Windows i386 binary.

Rather than stand up a PHP 5.6 container, write a fresh `CMakeLists.txt`. This also disposes
of, in one step: the missing `project.linux.conf` / `libs.linux.conf`; the hardcoded `-m32` in
`makeApp.tpl:14-16`; the `LDLIBS := -lstdc++` link line with no hook for platform libraries;
and the case-sensitivity bugs in `core.inc` (`shaderGen/glsl` and `shaderGen/hlsl` are
requested lowercase, but the directories are `shaderGen/GLSL` and `shaderGen/HLSL`;
`core.inc:135` requests `gfx/D3D`, which does not exist).

- Author `CMakeLists.txt` enumerating the retained engine source set, 64-bit, GL-only.
- Wire up `FetchContent` for the fetched dependency set, with pinned tags and the
  `FETCHCONTENT_SOURCE_DIR_<name>` offline overrides. Resolve the zlib ownership question here
  rather than discovering it at link time in Phase 6.
- Build the retained `Engine/lib/` sources as static targets, minus `d3dx8dll`.
- Set `-std=gnu++11` for engine C++ and `-std=gnu17` for vendored C — GCC 16.1.1 defaults to
  C23, where implicit function declarations are hard errors, which the 2012 zlib/png/jpeg
  copies will trip immediately.

**Checkpoint:** configures, and compilation starts.

### Removing PHP entirely

All of it goes. **220 PHP files, 18 launcher scripts and 39 Windows binaries**, with no
runtime consequence — nothing the shipped game executes touches PHP. There is zero `.php`
under any `Templates/*/game/`, and the only occurrences of "php" in `Engine/source` are a
"PHP style arrays" comment in `console/arrayObject.cpp` and a `garagegames.com` URL.

| Removed | Count | Replaced by |
|---|---:|---|
| `.php` — generator core, Smarty 2.6.18, classes, libs | 91 | CMake |
| `.tpl` — Smarty templates for VS2008/2010/2012, Xcode, makefiles, web | 49 | CMake generators (Ninja, Make, VS, Xcode all come free) |
| `.inc` — `modules/*.inc` source-set definitions, `btargets/targets.inc` | 33 | `target_sources` in CMake |
| `.conf` — per-project configs across 4 templates, `dae2dts`, `Engine/lib`, `Engine/bin/tools/web` | 46 | CMake cache variables and options |
| `generateProjects.{bat,command}`, `compile.bat`, `generateAllProjects.bat` | 18 | `cmake --build` |
| `Engine/bin/*.exe`, `*.dll` — bundled `php.exe`, `php5ts.dll`, bison, flex, nasm, uuidgen | 39 | System toolchain; see below |

Two files matched the extension sweep but are not PHP and are unrelated:
`Engine/bin/tools/nsis/app/Examples/Plugin/extdll.inc` (NSIS) and
`Engine/lib/lmng/contrib/kylix/mngview/mngview.conf` (Kylix).

**The one thing to preserve: the TorqueScript parser.** `console/CMDgram.y` and `CMDscan.l`
are regenerated by `bison.bat` and `generateCompiler.bat` — Windows batch, not PHP, but they
die with the rest of the Windows tooling. The generated `cmdgram.cpp` and `CMDscan.cpp` are
committed. Keep those as the default build input so bison and flex are not required to build,
and add an optional CMake target that regenerates them for the rare case the grammar changes.

`Tools/dae2dts/` goes too if COLLADA is dropped per the dependency assessment above, taking
its two `.conf` files with it. The `webDeploy` configs and `templates/web/` go regardless —
NPAPI and ActiveX are dead browser technology.

## Phase 1 — 64-bit foundation

Do this before mass compilation; it changes types that everything downstream depends on.

- **There is no x86_64 case in the CPU detection at all.** `platform/types.gcc.h` tested
  `i386`, `__ppc__` and `SN_TARGET_PS3`, then `#error "GCC: Unsupported Target CPU"`. Since
  `i386` is undefined on a 64-bit target, the original tree could not be compiled for x86_64
  even in principle. This blocks everything else in this phase.
- `platform/types.posix.h:32` defines `typedef unsigned int dsize_t;` — 32 bits. `dsize_t`
  appears in 55 files and is the engine's pointer/allocation-size type. Redefine as
  `uintptr_t`. (Note: `typesX86UNIX.h` carries the same bug but is *dead* — `types.gcc.h`
  includes `types.posix.h` on Linux, never the X86UNIX header.)
- `platform/types.gcc.h` dispatched on the bare `linux` macro, which only exists in GNU
  dialects and disappears under a strict `-std=c++NN`. Use `__linux__`.
- Collapse the type headers. `platform/` carries `typesX86UNIX.h`, `typesWin32.h`,
  `typesPPC.h`, `types.mac.h`, `types.posix.h`, `types.ppc.h`, `types.xenon.h`,
  `types.codewarrior.h`, `types.visualc.h`, `types.gcc.h`, `typesLinux.h`. Reduce to one
  `types.h` over `<cstdint>`.
- Audit pointer-to-`U32`/`S32` casts (~19 files by first pass; treat as a lower bound —
  a full audit needs `-Wpointer-to-int-cast` on a compiling tree).
- Remove 32-bit inline asm. `platform/profiler.cpp`, `platform/platformCPUCount.cpp`,
  `platform/platform.h` and `core/ogg/oggTheoraDecoder.cpp` contain asm blocks that survive
  the platform-directory deletion. Replace with intrinsics or portable C. The worst is
  `profiler.cpp`'s `endHighResolutionTimer`, which binds the `time` pointer to `%ecx` and
  dereferences it at 4-byte offsets — it truncates the pointer on x86_64 rather than failing
  loudly. `__rdtsc()` from `<x86intrin.h>` is correct on both widths.
- Drop `TORQUE_SUPPORTS_NASM`. The `.asm` sources it gated (`math/mMathSSE.cpp`,
  `math/mMathAMD.cpp`) are 32-bit.
- Exclude libpng's `pnggccrd.c` and `pngvcrd.c` from the build — libpng 1.2's hand-written
  32-bit x86 MMX paths, which cannot assemble on x86_64. The portable C fallbacks replace them.
- Repair OPCODE's pointer packing. `Engine/lib/opcode/OPC_OptimizedTree.cpp` stores node
  pointers in `udword` (32-bit) in three places, and `OPC_SweepAndPrune.cpp` once more. This
  must be fixed whether or not OPCODE is later replaced.

**Checkpoint:** `sizeof(void*) == 8` throughout; no 32-bit asm outside deleted directories.

## Phase 2 — Delete, then stub

- Delete `platformWin32/`, `platformX86UNIX/`, `platformMac/`, `platformPOSIX/`,
  `gfx/D3D9/`, `shaderGen/HLSL/`, `sfx/dsound/`, `sfx/xaudio/`.
- Remove `TORQUE_OS_WIN32`/`TORQUE_OS_MAC` branches and the D3D device registration in
  `gfx/gfxInit.cpp`.
- Provide empty stub implementations for every symbol in the `platform/` headers.

**Checkpoint:** the engine compiles and links against stubs. Runs and does nothing.
This is the bottom of the curve — everything after it adds capability back.

## Phase 3 — SDL3 platform core

One implementation directory, `platform/sdl/`, covering what the three deleted directories did:
window creation and lifecycle, the event loop feeding `platform/event.h`, input
(`platformInput.h`), timers, threads and TLS, filesystem and volumes (`platformVolume.h`),
dynamic library loading (`platformDlibrary.h`), CPU info, message boxes, and clipboard.

`platform/platformRedBook.h` (CD audio) has no modern equivalent — retire the interface.

**Checkpoint:** an empty SDL3 window appears and responds to input events.

## Phase 4 — GL-only rendering

- Wire `gfx/gl`, `gfx/gl/ggl`, `gfx/gl/ggl/generated` and `shaderGen/GLSL` as the sole path.
- Replace the GGL loader's GLX binding with SDL3's `SDL_GL_GetProcAddress`, removing the last
  X11 dependency.
- Make the GL device the default in `gfx/gfxInit.cpp`.

**Checkpoint:** first rendered frame.

## Phase 5 — SDL3_ttf

Replace `platformWin32/winFont.cpp`, `platformMac/macCarbFont.cpp` and
`platformX86UNIX/x86UNIXFont.client.cpp` with one SDL3_ttf implementation of the
`PlatformFont` interface behind `platform/platformFont.cpp`. `gfx/gFont.cpp` and
`gfx/gfxFontRenderBatcher.cpp` consume it unchanged. Cleanest substitution in the project.

## Phase 6 — SDL3_image

Replace the six loaders in `gfx/bitmap/loaders/` with SDL3_image feeding `GBitmap`. Keep the
DDS path. Delete `Engine/lib/lpng`, `ljpeg`, `lmng` and `lungif` — SDL3_image supplies its own
decoders, so nothing in the tree still needs them.

## Phase 7 — SDL3_net

Reimplement `platform/platformNet.cpp` (1,051 lines) and `platformNetAsync.cpp` (187 lines)
on SDL3_net, preserving the `platformNet.h` interface, the `JournaledSignal` events, and
`NetAddress` semantics.

## Phase 8 — Audio

The smallest phase, since the backend already works and is already portable.

- Link OpenAL Soft directly; delete `sfx/openal/win32/LoadOAL.cpp` and
  `sfx/openal/mac/LoadOAL.mac.cpp` along with the `OPENALFNTABLE` indirection in `LoadOAL.h`.
- Delete `sfx/dsound/` (1,293) and `sfx/xaudio/` (1,343) as Windows-only, and `sfx/fmod/`
  (4,729) as a proprietary SDK dependency.
- Keep `sfx/media/`, now decoding against the fetched libogg/libvorbis targets.
- Optional follow-on: implement `setReverb` over `ALC_EXT_EFX`, wiring up the caps flag
  `aldlist.cpp:90` already sets, to bring `SFXEnvironment` to life.

## Phase 9 — Shaders and finish

The 58 missing GLSL shaders were taken from the master branch rather than translated, and
the post effect chain now composites: gamma, FXAA and chromatic aberration render correctly.
SSAO, depth of field and light rays do not, for the reason under Known issues below — they
need the deferred prepass, not a different shader.

The gap as it stood:

| Area | Missing |
|---|---:|
| `postFx/dof` | 11 |
| `postFx/hdr` | 10 |
| `postFx` | 9 |
| `postFx/mlaa` | 6 |
| `postFx/ssao` | 5 |
| `postFx/edgeaa` | 4 |
| `fixedFunction` | 4 |
| `common` | 3 |
| `postFx/lightRay`, `postFx/fxaa`, `lighting/advanced` | 2 each |

The `justfile` that this phase ends with already exists. What remains of Phase 9 is the
deferred path, tracked under Known issues rather than here.

---

## Known issues

Found while landing Phase 4 and sharpened during Phase 9. None blocks the remaining phases,
but all three are real and none has an owner yet.

### The engine crashes on shutdown

Every exit ends in `pure virtual method called` / `terminate called without an active
exception`, after prefs are written and with the exit code set to 134. It predates the GL
work — reproduced by stashing the Phase 4 changes, rebuilding and running the base branch —
so it is most likely a destruction-order problem among the statically owned singletons.

The practical cost is that every automated run looks like a failure, which will hide real
crashes in Phases 6 and 7. Worth fixing before them for that reason alone.

### Advanced Lighting

This is what blocks SSAO, depth of field and light rays, and it is worth being precise about
why, because the obvious reading is wrong.

**Those three effects are not missing a port.** Their GLSL compiles. They ask for the
`#prepass` named target, which only `RenderPrePassMgr` registers
(`renderPrePassMgr.cpp:92`). Basic Lighting *can* create that bin, and the code that would is
commented out — in this tree and, byte for byte, in the master branch too:

```cpp
/*
// If you would like to use forward shading, and have a linear depth pre-pass
// than un-comment this code block.
if ( !prePassBin ) { ... }
*/
```

So master does not implement these effects differently. They are deferred-only there as well;
master simply has a working deferred path. Anyone looking to master for a different technique
will not find one.

**The deferred path now works.** `AdvancedLightManager::isCompatible` requires pixel shader
3.0 (master also checks `R16F` render target support). The GL device reported 2.0 until Phase
9 raised it, so the deferred path could never be selected. Four bugs stood between that and a
correct frame, all of them in GL-only code that the 2012 tree shipped half finished and that
the D3D9 sibling next to it gets right:

- **`gTapRotationTex` had no texture unit.** `softShadow.glsl` declares it and the HLSL pins
  it to `register(S3)`, but the Advanced Lighting `ShaderData` blocks never listed it in
  `samplerNames` — the only thing that assigns a unit in GL. The handle reported register -1,
  which `ProcessedCustomMaterial` passed to `GFXDevice::setTexture`, whose only bounds check
  is an `AssertFatal` that compiles out in release. That was the segfault.
- **`DeferredRTLightingFeatGLSL` named its screen-space uniform `renderTargetParams`.**
  Nothing in the engine sets that name; it sets `$rtParams<N>`, indexed by the material's
  texture slot. `DeferredRTLightingFeatHLSL` keeps that index in `mLastTexIndex` and builds
  the name from it; the GLSL copy hardcoded a literal instead, so the uniform stayed zero and
  every sample of the light buffer collapsed to uv (0,0). The same feature also dropped the
  `uv.y = 1 - uv.y` line its HLSL sibling and master both have.
- **`DeferredBumpFeatGLSL::processPix` dropped `&& !fd.features[MFT_SpecularMap]`** from the
  condition its `setTexData` still carries. A material with both a specular and a normal map
  therefore declared a `bumpMap` sampler that nothing bound, and every sampler after it —
  `lightInfoBuffer` included — was off by one. Terrain lit correctly, shapes did not.
- **`getUVFromSSPos` in `farFrustumQuad.glsl` had its Y flip commented out**, and in the wrong
  position relative to the rtParams scale. The HLSL beside it and master's GLSL both flip
  before scaling.

With those fixed the deferred path renders a frame indistinguishable from Basic Lighting, and
`isCompatible` no longer refuses.

**Shadows are the remaining gap.** The PSSM pass runs and the shadow map is a correct 2x2
atlas of terrain depth, every shadow constant handle on the vector light shader resolves, and
switching `$pref::Shadows::disable` does change the light buffer. But the change is faint
self-shadow acne rather than a cast shadow, so the lookup is landing in the wrong depth range.
That is what to pick up next, along with SSAO, depth of field and light rays, which the
deferred path no longer blocks.

### Shaders still ask for the compatibility profile

Everything GL now goes through a core-profile loader, but the shaders are still
`#version 150 compatibility` and the context SDL hands us is whatever the driver defaults to.
Dropping `compatibility` is the last step to being genuinely core, and it is not a one-line
change:

- of 114 GLSL files, 45 still use `varying`, 28 write `gl_FragColor` and 17 call
  `texture2D()`; these become `in`/`out`, a declared fragment output, and `texture()`. The
  shaders taken from master in Phase 9 are already written the core way, so the remainder is
  what this branch wrote or inherited
- the GLSL shader generator emits the same constructs and has to change with them
- a core profile requires a vertex array object bound for every draw, and nothing binds one

The post effect shaders have landed, so this no longer has to wait on them. Worth pairing
with the deferred path work instead, since both walk the same tree.

---

## Sequencing note

Phases 0-2 are mechanical and fast. Phase 3 is the first hard one. The milestone worth
optimizing for is **end of Phase 4** — a window with a rendered frame proves the build
system, the 64-bit conversion, the platform layer and the GL path all at once, and everything
after it is independently testable subsystem work.

Realistically a multi-month effort for one developer. Phases 5-8 can proceed in parallel
once Phase 4 lands, as they touch disjoint subsystems.

## Cleanup before starting

Remove artifacts from an earlier master-branch CMake configure: `build/`,
`My Projects/OpenTribes/`, `Engine/lib/libogg/include/ogg/config_types.h`.
