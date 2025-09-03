//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_GEOM_XFORMABLE_H
#define PXR_EXEC_EXEC_GEOM_XFORMABLE_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/execGeom/api.h"

#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

#define EXEC_GEOM_XFORMABLE_TOKENS      \
    (computeLocalToWorldTransform)      \
    ((transform, "xformOps:transform"))

TF_DECLARE_PUBLIC_TOKENS(
    ExecGeomXformableTokens, EXECGEOM_API, EXEC_GEOM_XFORMABLE_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
