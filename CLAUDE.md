# OpenTribes

A port of a 2012 Torque3D 1.2 snapshot to modern 64-bit Linux, replacing the
platform-specific subsystems with SDL3.

## New files must not carry a legal notice

This project is not associated with GarageGames. Do not put the
`Copyright (c) 2012 GarageGames, LLC` block, or any other legal notice, at the
top of a file you create. Start new source files at the include guard or the
first `#include`.

Do not copy the header across from a neighbouring file when adding to an
existing directory — most files here predate the port and carry it for that
reason.

This applies to files that are genuinely new. A file that is a rename or a
copy of original Torque source keeps the notice it came with; removing it from
a substantial copy would breach the MIT terms it was released under.
