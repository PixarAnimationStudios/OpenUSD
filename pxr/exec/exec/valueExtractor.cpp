//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/valueExtractor.h"

#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

VtValue
Exec_ValueExtractor::_ExtractInvalid(
    const VdfVector &, const VdfMask::Bits &)
{
    TF_CODING_ERROR("Attempted to extract with invalid extractor");
    return VtValue();
}

PXR_NAMESPACE_CLOSE_SCOPE
