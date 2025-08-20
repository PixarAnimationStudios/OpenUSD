//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_ESF_USD_ATTRIBUTE_H
#define PXR_EXEC_ESF_USD_ATTRIBUTE_H

#include "pxr/pxr.h"

#include "pxr/exec/esfUsd/property.h"

#include "pxr/exec/esf/attribute.h"
#include "pxr/exec/esf/attributeQuery.h"
#include "pxr/usd/usd/attribute.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Implementation of EsfAttributeInterface that wraps a UsdAttribute.
class EsfUsd_Attribute
    : public EsfUsd_PropertyImpl<EsfAttributeInterface, UsdAttribute>
{
public:
    ~EsfUsd_Attribute() override;

    /// Copies the provided \p attribute into this instance.
    EsfUsd_Attribute(const UsdAttribute &attribute)
        : EsfUsd_PropertyImpl<EsfAttributeInterface, UsdAttribute>(
            attribute) {}

    /// Moves the provided \p attribute into this instance.
    EsfUsd_Attribute(UsdAttribute &&attribute)
        : EsfUsd_PropertyImpl<EsfAttributeInterface, UsdAttribute>(
            std::move(attribute)) {}

private:
    // EsfAttributeInterface implementation.
    SdfValueTypeName _GetValueTypeName() const final;
    EsfAttributeQuery _GetQuery() const final;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
