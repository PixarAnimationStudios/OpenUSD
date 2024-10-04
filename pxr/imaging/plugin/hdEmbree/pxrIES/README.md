# IES utilities

Utilities for reading and using .ies files (IESNA LM-63 Format), which are used
to describe lights.

The files `ies.h` and `ies.cpp` are originally from
[Cycles](https://www.cycles-renderer.org/), a path-traced renderer that is a
spinoff of the larger [Blender](https://projects.blender.org/blender/blender/)
project, though available with in own repository, and via the Apache 2.0
license:

- https://projects.blender.org/blender/cycles
- https://projects.blender.org/blender/cycles/src/branch/main/LICENSE

## Version

v4.1.1 ( 234fa733d30a0e49cd10b2c92091500103a1150a )

## Setup

When updating IES, the following steps should be followed:

1. Copy `src/util/ies.h` and `src/util/ies.cpp` over the
   copies in `pxr/imaging/plugin/hdEmbree/pxrIES`.
2. Apply `pxr-IES.patch` to update the source files with modifications for USD,
   ie, from the USD repo root folder:

   ```sh
   patch -p1 -i pxr/imaging/plugin/hdEmbree/pxrIES/pxr-IES.patch
   ```
3. Commit your changes, noting the exact version of blender that the new ies
   files were copied from.