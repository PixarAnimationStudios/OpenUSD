# The Art of C++ : PEGTL (Parsing Expression Grammar Template Library)

[PEGTL](https://github.com/taocpp/PEGTL) is a library for creating
parsers according to a Parsing Expression Grammar.

## Version

v3.2.7

## Setup

Refer to the [Installing and Using](https://github.com/taocpp/PEGTL/blob/3.x/doc/Installing-and-Using.md)
documentation

## Copy headers

1. Copy PEGTL-3.2.7/include/tao/... to pxr/base/pegtl/...

2. Replace Macro Names

Change macro names from TAO_PEGTL_ to PXR_PEGTL_.  You can do this in a
bash-like shell with the following:

`sed -i 's/TAO_PEGTL_/PXR_PEGTL_/g' $(find -name '[^.]*.[hc]pp')`

3. Update CMakeLists.txt and any other build configuration files.

You can get a full list of the header files by running:

`find -type f`

4. Update pxr/base/pegtl/pegtl/config.hpp

This file defines the namespace name that PEGTL uses:

```
// Define PXR_PEGTL_NAMESPACE based on the internal namespace to isolate
// it from other versions of USD/PEGTL in client code.
#include "pxr/pxr.h"

#if PXR_USE_NAMESPACES
#define PXR_PEGTL_NAMESPACE PXR_INTERNAL_NS ## _pegtl
#else
#define PXR_PEGTL_NAMESPACE pxr_pegtl
#endif
```

