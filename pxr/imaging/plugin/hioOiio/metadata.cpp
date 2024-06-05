//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/arch/pragmas.h"

ARCH_PRAGMA_PUSH
ARCH_PRAGMA_MACRO_REDEFINITION // due to Python copysign
#include <OpenImageIO/imageio.h>
ARCH_PRAGMA_POP

PXR_NAMESPACE_OPEN_SCOPE

class TfToken;
class VtValue;

OIIO_NAMESPACE_USING

bool
HioOIIO_ExtractCustomMetadata(
    const ImageSpec &imagespec,
    TfToken const & key,
    VtValue * value)
{
    return false;
}

PXR_NAMESPACE_CLOSE_SCOPE

