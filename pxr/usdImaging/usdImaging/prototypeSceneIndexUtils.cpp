//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usdImaging/usdImaging/prototypeSceneIndexUtils.h"

#include "pxr/imaging/hd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

static const TfToken::Set primTypeWhitelist {
    HdPrimTypeTokens->material
};
    
namespace UsdImaging_PrototypeSceneIndexUtils {

void
SetEmptyPrimType(TfToken * const primType)
{
    if (!primType) {
        return;
    }

    if (primTypeWhitelist.count(*primType) == 0) {
        *primType = TfToken();
    }
}

}

PXR_NAMESPACE_CLOSE_SCOPE
