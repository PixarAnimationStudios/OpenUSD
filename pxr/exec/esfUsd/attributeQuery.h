//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_ESF_USD_ATTRIBUTE_QUERY_H
#define PXR_EXEC_ESF_USD_ATTRIBUTE_QUERY_H

#include "pxr/pxr.h"

#include "pxr/exec/esf/attributeQuery.h"
#include "pxr/usd/usd/attributeQuery.h"

#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

/// Implementation of EsfAttributeQueryInterface that wraps a UsdAttributeQuery.
class EsfUsd_AttributeQuery : public EsfAttributeQueryInterface
{
public:
    ~EsfUsd_AttributeQuery() override;

    /// Moves the provided \p attribute into this instance.
    explicit EsfUsd_AttributeQuery(UsdAttributeQuery &&attributeQuery)
        : _attributeQuery(std::move(attributeQuery)) {}

private:
    // EsfAttributeQueryInterface implementation.
    bool _IsValid() const final;
    SdfPath _GetPath() const final;
    void _Initialize() final;
    bool _Get(VtValue *value, UsdTimeCode time) const final;
    std::optional<TsSpline> _GetSpline() const final;
    bool _ValueMightBeTimeVarying() const final;
    bool _IsTimeVarying(UsdTimeCode from, UsdTimeCode to) const final;

private:
    UsdAttributeQuery _attributeQuery;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
