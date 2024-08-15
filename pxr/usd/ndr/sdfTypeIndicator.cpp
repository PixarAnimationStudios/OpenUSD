//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/ndr/sdfTypeIndicator.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/valueTypeName.h"

PXR_NAMESPACE_OPEN_SCOPE

NdrSdfTypeIndicator::NdrSdfTypeIndicator()
    : _sdfType(SdfValueTypeNames->Token),
      _ndrType(TfToken()),
      _hasSdfTypeMapping(false) {}

NdrSdfTypeIndicator::NdrSdfTypeIndicator(
    const SdfValueTypeName& sdfType,
    const TfToken& ndrType,
    bool hasSdfTypeMapping)
    : _sdfType(sdfType),
      _ndrType(ndrType),
      _hasSdfTypeMapping(hasSdfTypeMapping) {}

TfToken
NdrSdfTypeIndicator::GetNdrType() const {
    return _ndrType;
}

bool
NdrSdfTypeIndicator::HasSdfType() const {
    return _hasSdfTypeMapping;
}

SdfValueTypeName
NdrSdfTypeIndicator::GetSdfType() const {
    return _sdfType;
}

bool
NdrSdfTypeIndicator::operator==(const NdrSdfTypeIndicator &rhs) const {
    return _sdfType == rhs._sdfType && _ndrType == rhs._ndrType;
}

bool
NdrSdfTypeIndicator::operator!=(const NdrSdfTypeIndicator &rhs) const {
    return !operator==(rhs);
}

PXR_NAMESPACE_CLOSE_SCOPE
