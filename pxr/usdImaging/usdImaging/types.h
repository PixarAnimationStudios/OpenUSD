//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_TYPES_H
#define PXR_USD_IMAGING_USD_IMAGING_TYPES_H

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Given to an invalidation call to indicate whether the property was
/// added or removed or whether one of its fields changed.
///
enum class UsdImagingPropertyInvalidationType
{
    Update,
    Resync
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
