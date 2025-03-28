//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_SDR_SDF_TYPE_INDICATOR_H
#define PXR_USD_SDR_SDF_TYPE_INDICATOR_H

/// \file sdr/sdfTypeIndicator.h
///
/// \note
/// All Ndr objects are deprecated in favor of the corresponding Sdr objects
/// in this file. All existing pxr/usd/ndr implementations will be moved to
/// pxr/usd/sdr.

#include "pxr/pxr.h"
#include "pxr/usd/ndr/sdfTypeIndicator.h"
#include "pxr/usd/sdr/api.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/sdf/valueTypeName.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class SdrSdfTypeIndicator
/// 
/// Represents a mapping from an Sdr Property type to Sdf type.
///
/// If an exact mapping exists from Sdr Property type to Sdf type, HasSdfType()
/// will return true, and GetSdfType() will return the Sdf type resulting from
/// the conversion. GetSdrType() will return the original Sdr Property type. 
///
/// If a mapping doesn't exist from Sdr Property type to Sdf type, HasSdfType()
/// will return false, and GetSdfType() will return either
/// SdfValueTypeNames->TokenArray or SdfValueTypeNames->Token. GetSdrType()
/// will return the original Sdr Property type.
using SdrSdfTypeIndicator = NdrSdfTypeIndicator;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDR_SDF_TYPE_INDICATOR_H
