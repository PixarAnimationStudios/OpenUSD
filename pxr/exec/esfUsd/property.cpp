//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/esfUsd/property.h"

#include "pxr/exec/esf/attribute.h"
#include "pxr/exec/esf/relationship.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/relationship.h"

PXR_NAMESPACE_OPEN_SCOPE

// EsfProperty should not reserve more space than necessary.
static_assert(sizeof(EsfUsd_Property) == sizeof(EsfProperty));

template <class InterfaceType, class UsdPropertyType>
EsfUsd_PropertyImpl<InterfaceType, UsdPropertyType>::~EsfUsd_PropertyImpl()
    = default;

template <class InterfaceType, class UsdPropertyType>
TfToken
EsfUsd_PropertyImpl<InterfaceType, UsdPropertyType>::_GetBaseName() const
{
    return this->_GetWrapped().GetBaseName();
}

template <class InterfaceType, class UsdPropertyType>
TfToken
EsfUsd_PropertyImpl<InterfaceType, UsdPropertyType>::_GetNamespace() const
{
    return this->_GetWrapped().GetNamespace();
}

// Explicit template instantiations
template class EsfUsd_PropertyImpl<EsfAttributeInterface, UsdAttribute>;
template class EsfUsd_PropertyImpl<EsfPropertyInterface, UsdProperty>;
template class EsfUsd_PropertyImpl<EsfRelationshipInterface, UsdRelationship>;

PXR_NAMESPACE_CLOSE_SCOPE
