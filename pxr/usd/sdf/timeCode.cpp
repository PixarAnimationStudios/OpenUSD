//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/sdf/timeCode.h"
#include "pxr/usd/sdf/layerOffset.h"

#include "pxr/base/tf/registryManager.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/arrayEdit.h"
#include "pxr/base/vt/value.h"
#include "pxr/base/vt/valueTransform.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(VtValue)
{
    VtRegisterTransform(
        +[](GfTimeCode const &timeCode, SdfLayerOffset const &offset) {
            return offset * timeCode;
        });

    VtRegisterTransform(
        +[](GfDuration const &duration, SdfLayerOffset const &offset) {
            return offset * duration;
        });
}

PXR_NAMESPACE_CLOSE_SCOPE
