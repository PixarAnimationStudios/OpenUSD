//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/gf/half.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType) {
    // GfHalf is a type alias for pxr_half::half. Add a named alias to the TfType
    // system so it can be found by the name "GfHalf".
    TfType::Define<GfHalf>()
        .Alias(TfType::GetRoot(), "GfHalf");
}

PXR_NAMESPACE_CLOSE_SCOPE
