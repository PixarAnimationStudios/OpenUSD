//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_GEOM_UTIL_TOKENS_H
#define PXR_IMAGING_GEOM_UTIL_TOKENS_H

/// \file geomUtil/tokens.h

#include "pxr/pxr.h"
#include "pxr/imaging/geomUtil/api.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE


#define GEOMUTIL_INTERPOLATION_TOKENS \
    (constant) \
    (uniform) \
    (vertex) \

TF_DECLARE_PUBLIC_TOKENS(GeomUtilInterpolationTokens,
    GEOMUTIL_API, GEOMUTIL_INTERPOLATION_TOKENS);


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_GEOM_UTIL_TOKENS_H
