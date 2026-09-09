//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/gf/duration.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/type.h"

#include <ostream>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType) {
    TfType::Define<GfDuration>();
}

std::ostream&
operator<<(std::ostream& out, const GfDuration& duration)
{
    return out << duration.GetValue();
}

PXR_NAMESPACE_CLOSE_SCOPE
