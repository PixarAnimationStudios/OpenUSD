//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_ESF_USD_RELATIONSHIP_H
#define PXR_EXEC_ESF_USD_RELATIONSHIP_H

#include "pxr/pxr.h"

#include "pxr/exec/esfUsd/property.h"

#include "pxr/exec/esf/relationship.h"
#include "pxr/usd/usd/relationship.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Implementation of EsfRelationshipInterface that wraps a UsdRelationship.
class EsfUsd_Relationship
    : public EsfUsd_PropertyImpl<EsfRelationshipInterface, UsdRelationship>
{
public:
    ~EsfUsd_Relationship() override;

    /// Copies the provided \p relationship into this instance.
    EsfUsd_Relationship(const UsdRelationship &relationship)
        : EsfUsd_PropertyImpl<EsfRelationshipInterface, UsdRelationship>(
            relationship) {}

    /// Moves the provided \p relationship into this instance.
    EsfUsd_Relationship(UsdRelationship &&relationship)
        : EsfUsd_PropertyImpl<EsfRelationshipInterface, UsdRelationship>(
            std::move(relationship)) {}

private:
    // EsfRelationshipInterface implementation.
    SdfPathVector _GetTargets() const final;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
