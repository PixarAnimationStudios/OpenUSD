//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/esfUsd/attributeQuery.h"

PXR_NAMESPACE_OPEN_SCOPE

// EsfAttributeQuery should not reserve more space than necessary.
static_assert(sizeof(EsfUsd_AttributeQuery) == sizeof(EsfAttributeQuery));

EsfUsd_AttributeQuery::~EsfUsd_AttributeQuery() = default;

bool
EsfUsd_AttributeQuery::_IsValid() const
{
    return _attributeQuery.IsValid();
}

SdfPath
EsfUsd_AttributeQuery::_GetPath() const 
{
    return _attributeQuery.GetAttribute().GetPath();
}

void
EsfUsd_AttributeQuery::_Initialize()
{
    _attributeQuery = UsdAttributeQuery(_attributeQuery.GetAttribute());
}

bool
EsfUsd_AttributeQuery::_Get(VtValue *value, UsdTimeCode time) const
{
    return _attributeQuery.Get(value, time);
}

std::optional<TsSpline>
EsfUsd_AttributeQuery::_GetSpline() const
{
    if (!_attributeQuery.HasSpline()) {
        return std::nullopt;
    }

    return _attributeQuery.GetSpline();
}

bool
EsfUsd_AttributeQuery::_ValueMightBeTimeVarying() const
{
    return _attributeQuery.ValueMightBeTimeVarying();
}

bool
EsfUsd_AttributeQuery::_IsTimeVarying(UsdTimeCode from, UsdTimeCode to) const
{
    if (!_ValueMightBeTimeVarying()) {
        return false;
    }

    VtValue fromValue, toValue;
    _Get(&fromValue, from);
    _Get(&toValue, to);
    return fromValue != toValue;
}

PXR_NAMESPACE_CLOSE_SCOPE