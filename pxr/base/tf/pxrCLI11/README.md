# CLI11

[CLI11](https://github.com/CLIUtils/CLI11) is a command line parsing library
used for USD's command line tooling.

## Version

v2.3.1 ( c2ea58c7f9bb2a1da2d3d7f5b462121ac6a07f16 )

## Setup

When updating CLI11, the following steps should be followed:

1. Generate the CLI11.hpp from the project, or grab it from the release.
2. Move `CLI11.hpp` to `CLI11.h` in keeping with the rest of this repos standards.
3. Add `#include "pxr/pxr.h"` to the includes
4. Add `PXR_NAMESPACE_OPEN_SCOPE` before the opening of `namespace CLI`
5. Add `PXR_NAMESPACE_CLOSE_SCOPE` after the end of the file.