//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
/// \file MapEditProxy.cpp

#include "pxr/usd/sdf/mapEditProxy.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/schema.h"

#include "pxr/base/vt/valueRef.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

SdfMapEditProxyBase::SdfMapEditProxyBase(
    const SdfSpecHandle& owner, const TfToken& field)
    : _owner(owner)
    , _field(field)
{
}

std::string
SdfMapEditProxyBase::_Location() const
{
    if (!_owner) {
        return std::string();
    }

    return TfStringPrintf("field '%s' in <%s>",
                          _field.GetText(), _owner->GetPath().GetText());
}

bool
SdfMapEditProxyBase::_IsExpired() const
{
    return !_field.IsEmpty() && !_owner;
}

SdfAllowed
SdfMapEditProxyBase::_IsValidKey(const VtValueRef& key) const
{
    if (const SdfSchema::FieldDefinition* def =
            _owner->GetSchema().GetFieldDefinition(_field)) {
        return def->IsValidMapKey(key);
    }
    return true;
}

SdfAllowed
SdfMapEditProxyBase::_IsValidValue(const VtValueRef& value) const
{
    if (const SdfSchema::FieldDefinition* def =
            _owner->GetSchema().GetFieldDefinition(_field)) {
        return def->IsValidMapValue(value);
    }
    return true;
}

void
SdfMapEditProxyBase::_WriteToSpec(const VtValueRef& value)
{
    TfAutoMallocTag tag("Sdf", "SdfMapEditProxyBase::_WriteToSpec");

    if (TF_VERIFY(_owner)) {
        if (value.IsEmpty()) {
            _owner->ClearField(_field);
        }
        else {
            _owner->SetField(_field, value);
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
