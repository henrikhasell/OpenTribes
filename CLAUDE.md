# OpenTribes

A port of a 2012 Torque3D 1.2 snapshot to modern 64-bit Linux, replacing the
platform-specific subsystems with SDL3.

## New files must not carry a legal notice

This project is not associated with GarageGames. Do not put the
`Copyright (c) 2012 GarageGames, LLC` block, or any other legal notice, at the
top of a file you create. Start new source files at the marker below, the
include guard, or the first `#include`.

Do not copy the header across from a neighbouring file when adding to an
existing directory — most files here predate the port and carry it for that
reason.

This applies to files that are genuinely new. A file that is a rename of one
already in this repository keeps the notice it came with.

## New headers say what they are for

A header this port adds — as opposed to one that came with Torque 3D — opens
with this line, above the include guard:

```
// HH: Added for Tribes 2 compatibility.
```

It is what separates a header the port wrote to run Tribes 2 from the several
thousand the engine already had, and it goes on every new header rather than
only the obviously game-facing ones.

Headers brought over from TGE take that tree's marker below instead, and a
header the port added for the platform work rather than for the game — the SDL
and GL layers — takes neither: it is not there for Tribes 2 and should not say
it is. A `.cpp` takes nothing; the header beside it already says it.

## Code brought over from TGE

Source taken from the Torque Game Engine tree does not keep its banner either.
Strip it and start the file with:

```
// HH: imported from TGE
```

If the file could not be brought over byte-identically, say so on the next
line:

```
// (minor edits)
```

Describe the change instead of writing `minor edits` when it was not minor.
