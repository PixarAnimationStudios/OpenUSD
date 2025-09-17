//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_ESF_USD_PRIM_H
#define PXR_EXEC_ESF_USD_PRIM_H

#include "pxr/pxr.h"

#include "pxr/exec/esfUsd/object.h"

#include "pxr/exec/esf/prim.h"
#include "pxr/usd/usd/prim.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Implementation of EsfPrimInterface that wraps a UsdPrim.
class EsfUsd_Prim : public EsfUsd_ObjectImpl<EsfPrimInterface, UsdPrim>
{
public:
    ~EsfUsd_Prim() override;

    /// Copies the provided \p prim into this instance.
    EsfUsd_Prim(const UsdPrim &prim)
        : EsfUsd_ObjectImpl<EsfPrimInterface, UsdPrim>(prim) {}

    /// Moves the provided \p prim into this instance.
    EsfUsd_Prim(UsdPrim &&prim)
        : EsfUsd_ObjectImpl<EsfPrimInterface, UsdPrim>(std::move(prim)) {}

private:
    // EsfPrimInterface implementation.
    const TfTokenVector &_GetAppliedSchemas() const final;
    EsfAttribute _GetAttribute(
        const TfToken &attributeName) const final;
    EsfPrim _GetParent() const final;
    EsfRelationship _GetRelationship(
        const TfToken &relationshipName) const final;
    TfType _GetType() const final;
    bool IsPseudoRoot() const final;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
