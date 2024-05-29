//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/sdf/opaqueValue.h"

#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/vt/array.h"

#include <iostream>


PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<SdfOpaqueValue>();
    // Even though we don't support an opaque[] type in scene description, there
    // is still code that assumes that any scene-description value type has a
    // TfType-registered array type too, so we register it here as well.
    TfType::Define<VtArray<SdfOpaqueValue>>();
}

std::ostream &
operator<<(std::ostream &s, SdfOpaqueValue const &)
{
    return s << "OpaqueValue";
}

PXR_NAMESPACE_CLOSE_SCOPE
