# IES utilities

Utilities for reading and using .ies files (IESNA LM-63 Format), which are used
to describe lights.

The files `ies.h` and `ies.cpp` are originally from
[Cycles](https://docs.blender.org/manual/en/latest/render/cycles/index.html),
which is part of the larger
[Blender](https://projects.blender.org/blender/blender/) project, though with
a the less restrictive Apache 2.0 license - see:

- https://www.blender.org/about/license/

## Version

v4.1.1 ( 53c49589f311bd3c9e3ea8f62b3fa8fe8e5d2c8c )

## Setup

When updating IES, the following steps should be followed:

1. Copy `intern/cycles/util/ies.h` and `intern/cycles/util/ies.cpp` over the
   copies in `pxr/imaging/plugin/hdEmbree/pxrIES`.
2. Apply `pxr-IES.patch` to update the source files with modifications for USD,
   ie, from the USD repo root folder:

   ```sh
   patch -p1 -i pxr/imaging/plugin/hdEmbree/pxrIES/pxr-IES.patch
   ```
3. Commit your changes, noting the exact version of blender that the new ies
   files were copied from.