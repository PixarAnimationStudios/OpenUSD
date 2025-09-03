//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/esfUsd/object.h"

#include "pxr/exec/esfUsd/attribute.h"
#include "pxr/exec/esfUsd/prim.h"
#include "pxr/exec/esfUsd/relationship.h"
#include "pxr/exec/esfUsd/stage.h"

#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/token.h"
#include "pxr/exec/esf/attribute.h"
#include "pxr/exec/esf/object.h"
#include "pxr/exec/esf/prim.h"
#include "pxr/exec/esf/property.h"
#include "pxr/exec/esf/relationship.h"
#include "pxr/exec/esf/stage.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/object.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/property.h"
#include "pxr/usd/usd/relationship.h"

#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

// EsfObject should not reserve more space than necessary.
static_assert(sizeof(EsfUsd_Object) == sizeof(EsfObject));

template <class InterfaceType, class UsdObjectType>
EsfUsd_ObjectImpl<InterfaceType, UsdObjectType>::~EsfUsd_ObjectImpl()
    = default;

template <class InterfaceType, class UsdObjectType>
bool
EsfUsd_ObjectImpl<InterfaceType, UsdObjectType>::_IsValid() const
{
    return _GetWrapped().IsValid();
}

template <class InterfaceType, class UsdObjectType>
TfToken
EsfUsd_ObjectImpl<InterfaceType, UsdObjectType>::_GetName() const
{
    return _GetWrapped().GetName();
}

template <class InterfaceType, class UsdObjectType>
EsfPrim
EsfUsd_ObjectImpl<InterfaceType, UsdObjectType>::_GetPrim() const
{
    return {std::in_place_type<EsfUsd_Prim>, _GetWrapped().GetPrim()};
}

template <class InterfaceType, class UsdObjectType>
EsfStage
EsfUsd_ObjectImpl<InterfaceType, UsdObjectType>::_GetStage() const
{
    return {std::in_place_type<EsfUsd_Stage>, _GetWrapped().GetStage()};
}

template <class InterfaceType, class UsdObjectType>
EsfSchemaConfigKey
EsfUsd_ObjectImpl<InterfaceType, UsdObjectType>::_GetSchemaConfigKey() const
{
    // We use the address of the UsdPrimTypeInfo as the schema config key, since
    // it is unique to the set of types and applied schemas for the prim and it
    // is stable, since it is guaranteed to stay alive at least as long as the
    // UsdStage.
    if constexpr (std::is_same_v<UsdPrim, UsdObjectType>) {
        return EsfObjectInterface::CreateSchemaConfigKey(
            &_GetWrapped().GetPrimTypeInfo());
    } else {
        return EsfObjectInterface::CreateSchemaConfigKey(
            &_GetWrapped().GetPrim().GetPrimTypeInfo());
    }
}

template <class InterfaceType, class UsdObjectType>
VtValue
EsfUsd_ObjectImpl<InterfaceType, UsdObjectType>::_GetMetadata(
    const TfToken &key) const
{
    VtValue value;
    if (!_GetWrapped().GetMetadata(key, &value)) {
        TF_CODING_ERROR("Invalid metadata key '%s'", key.GetText());
    }
    return value;
}

template <class InterfaceType, class UsdObjectType>
bool
EsfUsd_ObjectImpl<InterfaceType, UsdObjectType>::_IsValidMetadataKey(
    const TfToken &key) const
{
    const SdfSchema &schema = SdfSchema::GetInstance();
    const SdfSchema::SpecDefinition *spec = nullptr;

    // If the wrapped type is a concrete type, we can avoid additional virtual
    // function calls.
    if constexpr (std::is_same_v<UsdPrim, UsdObjectType>) {
        spec = schema.GetSpecDefinition(SdfSpecTypePrim);
    }
    else if constexpr (std::is_same_v<UsdAttribute, UsdObjectType>) {
        spec = schema.GetSpecDefinition(SdfSpecTypeAttribute);
    }
    else if constexpr (std::is_same_v<UsdRelationship, UsdObjectType>) {
        spec = schema.GetSpecDefinition(SdfSpecTypeRelationship);
    }
    else {
        // Otherwise, the wrapped type is abstract (UsdObject or UsdProperty),
        // so we need to call virtual functions to determine the type.
        if (IsPrim()) {
            spec = schema.GetSpecDefinition(SdfSpecTypePrim);
        }
        else if (IsAttribute()) {
            spec = schema.GetSpecDefinition(SdfSpecTypeAttribute);
        }
        else if (IsRelationship()) {
            spec = schema.GetSpecDefinition(SdfSpecTypeRelationship);
        }
    }

    if (!TF_VERIFY(spec, "Unexpected object type")) {
        return false;
    }

    return spec->IsMetadataField(key);
}

template <class InterfaceType, class UsdObjectType>
TfType
EsfUsd_ObjectImpl<InterfaceType, UsdObjectType>::_GetMetadataValueType(
    const TfToken &key) const
{
    // TODO: This will be cleaner when we have SdfSchema API that lets us
    // directly qurey the value type for a given field.
    const SdfSchemaBase::FieldDefinition *const fieldDef =
        SdfSchema::GetInstance().GetFieldDefinition(key);
    if (!fieldDef) {
        TF_CODING_ERROR("Invalid metadata key '%s'", key.GetText());
        return TfType();
    }

    static TfType vtValueType = TfType::Find<VtValue>();

    // An empty value indicates a value type of VtValue; otherwise, we return
    // the fallback value held type.
    const VtValue fallback = fieldDef->GetFallbackValue();
    return fallback.IsEmpty() ? vtValueType : fallback.GetType();
}

template <class InterfaceType, class UsdObjectType>
bool
EsfUsd_ObjectImpl<InterfaceType, UsdObjectType>::IsPrim() const
{
    return _GetWrapped().template Is<UsdPrim>();
}

template <class InterfaceType, class UsdObjectType>
bool
EsfUsd_ObjectImpl<InterfaceType, UsdObjectType>::IsAttribute() const
{
    return _GetWrapped().template Is<UsdAttribute>();
}

template <class InterfaceType, class UsdObjectType>
bool
EsfUsd_ObjectImpl<InterfaceType, UsdObjectType>::IsRelationship() const
{
    return _GetWrapped().template Is<UsdRelationship>();
}

template <class InterfaceType, class UsdObjectType>
EsfObject
EsfUsd_ObjectImpl<InterfaceType, UsdObjectType>::AsObject() const
{
    return {
        std::in_place_type<EsfUsd_Object>,
        _GetWrapped().template As<UsdObject>()
    };
}

template <class InterfaceType, class UsdObjectType>
EsfPrim
EsfUsd_ObjectImpl<InterfaceType, UsdObjectType>::AsPrim() const
{
    return {
        std::in_place_type<EsfUsd_Prim>,
        _GetWrapped().template As<UsdPrim>()
    };
}

template <class InterfaceType, class UsdObjectType>
EsfAttribute
EsfUsd_ObjectImpl<InterfaceType, UsdObjectType>::AsAttribute() const
{
    return {
        std::in_place_type<EsfUsd_Attribute>,
        _GetWrapped().template As<UsdAttribute>()
    };
}

template <class InterfaceType, class UsdObjectType>
EsfRelationship
EsfUsd_ObjectImpl<InterfaceType, UsdObjectType>::AsRelationship() const
{
    return {
        std::in_place_type<EsfUsd_Relationship>,
        _GetWrapped().template As<UsdRelationship>()
    };
}

// Explicit template instantiations.
template class EsfUsd_ObjectImpl<EsfAttributeInterface, UsdAttribute>;
template class EsfUsd_ObjectImpl<EsfObjectInterface, UsdObject>;
template class EsfUsd_ObjectImpl<EsfPrimInterface, UsdPrim>;
template class EsfUsd_ObjectImpl<EsfPropertyInterface, UsdProperty>;
template class EsfUsd_ObjectImpl<EsfRelationshipInterface, UsdRelationship>;

PXR_NAMESPACE_CLOSE_SCOPE
