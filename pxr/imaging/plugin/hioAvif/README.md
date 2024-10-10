# hioAvif plugin

This library implements reading AVIF image files for Hio.

The AV1 Image Format is a royalty-free open-source picture format, with modern compression, flexible color specification, high dynamic range values, 
depth images and alpha channels, and support for layered and sequential images.

The supported feature set here is currently restricted to single frame images,
which are decoded to linear Rec709 RGB or RGBA if an alpha channel is present.

Reading is implemented through the use of libaom, and libavif.
YUV to RGB decoding is accomplished by libyuv.

libaom is the reference codec library created by the Alliance for Open Media.
libavif is a portable C implementation of the AV1 Image File Format.

libavif is obtained from the v1.0.4 tag release found here:
https://github.com/AOMediaCodec/libavif

libyuv was contained within the libavif release tag.

libaom is obtaned from the v3.8.3 tag release found here:
https://aomedia.googlesource.com/aom/

Please see AVIF_AOM_LICENSE.txt to learn about the licenses used by these
libraries.

The decoding, but not the encoding, portions of those libraries are used here, 
with minimal modification to accomodate the OpenUSD build environment. In
particular, the following files have been renamed:

aom/av1/common/scale.c --> aom/av1/common/aom_scale.c
AVIF/src/obu.c --> AVIF/src/avif_obu.c
AVIF/src/src-libyuv/scale.c --> AVIF/src/src-libyuv/yuv_scale.c

In addition to this renaming, the `#include` directives within the interred
source code have been modified to be compatible with the OpenUSD build system
which requires qualified paths in certain cases that would otherwise require
local relative includes, currently unsupported by the build systems across all
build configurations.

For example, this path

```
#include "aom_image.h"
```

becomes

```
#include "pxr/imaging/plugin/hioAvif/aom/aom_image.h"
```

Due to the large number of rewritten includes, neither a patch for reference,
nor an elaboration of the changes are included here. 

There are no other modifications to the interred library code beyond this, and
the code is in all other respects used as is.


