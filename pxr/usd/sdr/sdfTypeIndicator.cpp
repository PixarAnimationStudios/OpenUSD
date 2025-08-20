//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/sdr/sdfTypeIndicator.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/valueTypeName.h"

PXR_NAMESPACE_OPEN_SCOPE

SdrSdfTypeIndicator::SdrSdfTypeIndicator()
    : _sdfType(SdfValueTypeNames->Token),
      _sdrType(TfToken()),
      _hasSdfTypeMapping(false) {}

SdrSdfTypeIndicator::SdrSdfTypeIndicator(
    const SdfValueTypeName& sdfType,
    const TfToken& sdrType,
    bool hasSdfTypeMapping)
    : _sdfType(sdfType),
      _sdrType(sdrType),
      _hasSdfTypeMapping(hasSdfTypeMapping) {}

TfToken
SdrSdfTypeIndicator::GetSdrType() const {
    return _sdrType;
}

bool
SdrSdfTypeIndicator::HasSdfType() const {
    return _hasSdfTypeMapping;
}

SdfValueTypeName
SdrSdfTypeIndicator::GetSdfType() const {
    return _sdfType;
}

bool
SdrSdfTypeIndicator::operator==(const SdrSdfTypeIndicator &rhs) const {
    return _sdfType == rhs._sdfType && _sdrType == rhs._sdrType;
}

bool
SdrSdfTypeIndicator::operator!=(const SdrSdfTypeIndicator &rhs) const {
    return !operator==(rhs);
}

PXR_NAMESPACE_CLOSE_SCOPE
