//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/esfUsd/relationship.h"

#include "pxr/exec/esf/relationship.h"
#include "pxr/usd/usd/relationship.h"

PXR_NAMESPACE_OPEN_SCOPE

// EsfRelationship should not reserve more space than necessary.
static_assert(sizeof(EsfUsd_Relationship) == sizeof(EsfRelationship));

EsfUsd_Relationship::~EsfUsd_Relationship() = default;

SdfPathVector
EsfUsd_Relationship::_GetTargets() const
{
    SdfPathVector targets;
    _GetWrapped().GetTargets(&targets);
    return targets;
}

PXR_NAMESPACE_CLOSE_SCOPE
