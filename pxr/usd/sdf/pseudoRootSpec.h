//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_PSEUDO_ROOT_SPEC_H
#define PXR_USD_SDF_PSEUDO_ROOT_SPEC_H

/// \file sdf/pseudoRootSpec.h

#include "pxr/pxr.h"
#include "pxr/usd/sdf/declareSpec.h"
#include "pxr/usd/sdf/primSpec.h"

PXR_NAMESPACE_OPEN_SCOPE

SDF_DECLARE_HANDLES(SdfPseudoRootSpec);

class SdfPseudoRootSpec : public SdfPrimSpec
{
    SDF_DECLARE_SPEC(SdfPseudoRootSpec, SdfPrimSpec);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_PSEUDO_ROOT_SPEC_H
