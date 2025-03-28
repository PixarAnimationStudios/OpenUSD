//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXTRAS_USD_EXAMPLES_USD_OBJ_TRANSLATOR_H
#define PXR_EXTRAS_USD_EXAMPLES_USD_OBJ_TRANSLATOR_H
#include "pxr/pxr.h"
#include "pxr/base/tf/declarePtrs.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(SdfLayer);

/// Return an anonymous (in-memory-only) layer with data from \p objStream
/// translated to Usd.
SdfLayerRefPtr
UsdObjTranslateObjToUsd(const class UsdObjStream &objStream);


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_EXTRAS_USD_EXAMPLES_USD_OBJ_TRANSLATOR_H
