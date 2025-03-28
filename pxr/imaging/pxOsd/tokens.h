//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_PX_OSD_TOKENS_H
#define PXR_IMAGING_PX_OSD_TOKENS_H

/// \file pxOsd/tokens.h

#include "pxr/pxr.h"
#include "pxr/imaging/pxOsd/api.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE


#define PXOSD_OPENSUBDIV_TOKENS  \
    (all)                        \
    (none)                       \
    (cornersOnly)                \
    (cornersPlus1)               \
    (cornersPlus2)               \
    (boundaries)                 \
    (bilinear)                   \
    (catmullClark)               \
    (loop)                       \
    (edgeOnly)                   \
    (edgeAndCorner)              \
    (uniform)                    \
    (chaikin)                    \
    (leftHanded)                 \
    (rightHanded)                \
    (smooth)

TF_DECLARE_PUBLIC_TOKENS(PxOsdOpenSubdivTokens,
                         PXOSD_API, PXOSD_OPENSUBDIV_TOKENS);


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXOSD_REFINER_FACTORY_H
