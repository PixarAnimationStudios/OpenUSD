//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/sdf/timeCode.h"

#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/type.h"

#include "pxr/base/vt/array.h"
#include "pxr/base/vt/value.h"

#include <ostream>

PXR_NAMESPACE_OPEN_SCOPE

// Register this class with the TfType registry
// Array registration included to facilitate Sdf/Types and Sdf/ParserHelpers
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<SdfTimeCode>();
    TfType::Define< VtArray<SdfTimeCode> >();
}

TF_REGISTRY_FUNCTION(VtValue)
{
    VtValue::RegisterSimpleBidirectionalCast<double, SdfTimeCode>();
}

std::ostream& 
operator<<(std::ostream& out, const SdfTimeCode& ap)
{
    return out << ap.GetValue();
}

PXR_NAMESPACE_CLOSE_SCOPE
