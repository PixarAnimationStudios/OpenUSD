//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/sdf/mapEditor.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/schema.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

Sdf_MapEditorBase::Sdf_MapEditorBase(
    const SdfSpecHandle& owner, const TfToken& field)
    : _owner(owner)
    , _field(field)
{
}

std::string
Sdf_MapEditorBase::GetLocation() const
{
    return TfStringPrintf("field '%s' in <%s>",
                          _field.GetText(), _owner->GetPath().GetText());
}

SdfSpecHandle
Sdf_MapEditorBase::GetOwner() const
{
    return _owner;
}

bool
Sdf_MapEditorBase::IsExpired() const
{
    return !_owner;
}

SdfAllowed
Sdf_MapEditorBase::_IsValidKey(const VtValueRef& key) const
{
    if (const SdfSchema::FieldDefinition* def =
            _owner->GetSchema().GetFieldDefinition(_field)) {
        return def->IsValidMapKey(key);
    }
    return true;
}

SdfAllowed
Sdf_MapEditorBase::_IsValidValue(const VtValueRef& value) const
{
    if (const SdfSchema::FieldDefinition* def =
            _owner->GetSchema().GetFieldDefinition(_field)) {
        return def->IsValidMapValue(value);
    }
    return true;
}

void
Sdf_MapEditorBase::_WriteToSpec(const VtValueRef& value)
{
    TfAutoMallocTag2 tag("Sdf", "Sdf_MapEditorBase::_WriteToSpec");

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
