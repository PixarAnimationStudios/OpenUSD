//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
/// \file PseudoRootSpec.cpp

#include "pxr/pxr.h"
#include "pxr/usd/sdf/pseudoRootSpec.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

SDF_DEFINE_SPEC(
    SdfSchema, SdfSpecTypePseudoRoot, SdfPseudoRootSpec, SdfPrimSpec);

PXR_NAMESPACE_CLOSE_SCOPE
