//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usdImaging/usdImaging/prototypeSceneIndexUtils.h"

#include "pxr/imaging/hd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

static const TfToken::Set nonRenderablePrimTypes {
    HdPrimTypeTokens->material
};

namespace UsdImaging_PrototypeSceneIndexUtils
{

bool
IsRenderablePrimType(const TfToken &primType)
{
    if (primType.IsEmpty()) {
        return false;
    }

    return nonRenderablePrimTypes.count(primType) == 0;
}

}

PXR_NAMESPACE_CLOSE_SCOPE
