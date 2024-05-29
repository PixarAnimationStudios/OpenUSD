//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/geomSubset.h"

PXR_NAMESPACE_OPEN_SCOPE

bool operator==(const HdGeomSubset& lhs, const HdGeomSubset& rhs)
{
    return lhs.type == rhs.type
        && lhs.id == rhs.id
        && lhs.materialId == rhs.materialId
        && lhs.indices == rhs.indices;
}

PXR_NAMESPACE_CLOSE_SCOPE

