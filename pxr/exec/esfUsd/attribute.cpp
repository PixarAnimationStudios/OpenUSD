//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/esfUsd/attribute.h"

#include "pxr/exec/esfUsd/attributeQuery.h"

#include "pxr/exec/esf/attribute.h"
#include "pxr/usd/sdf/valueTypeName.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/attributeQuery.h"

PXR_NAMESPACE_OPEN_SCOPE

// EsfAttribute should not reserve more space than necessary.
static_assert(sizeof(EsfUsd_Attribute) == sizeof(EsfAttribute));

EsfUsd_Attribute::~EsfUsd_Attribute() = default;

SdfValueTypeName
EsfUsd_Attribute::_GetValueTypeName() const
{
    return _GetWrapped().GetTypeName();
}

EsfAttributeQuery
EsfUsd_Attribute::_GetQuery() const
{
    return {
        std::in_place_type<EsfUsd_AttributeQuery>,
        UsdAttributeQuery(_GetWrapped())
    };
}

PXR_NAMESPACE_CLOSE_SCOPE