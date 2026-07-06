//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/gf/timeCode.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/type.h"

#include <ostream>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType) {
    TfType::Define<GfTimeCode>();
}

std::ostream&
operator<<(std::ostream& out, const GfTimeCode& tc)
{
    return out << tc.GetValue();
}

PXR_NAMESPACE_CLOSE_SCOPE
