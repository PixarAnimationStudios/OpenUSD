//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_SDR_SDF_TYPE_INDICATOR_H
#define PXR_USD_SDR_SDF_TYPE_INDICATOR_H

/// \file sdr/sdfTypeIndicator.h

#include "pxr/pxr.h"
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
class SdrSdfTypeIndicator
{
public:
    /// Default constructor.
    SDR_API
    SdrSdfTypeIndicator();

    /// Constructor. The sdfType must be TokenArray or Token if hasSdfTypeMapping
    /// is set to false.
    SDR_API
    SdrSdfTypeIndicator(
        const SdfValueTypeName& sdfType,
        const TfToken& sdrType,
        bool hasSdfTypeMapping = true);

    /// Gets the original Sdr property type associated with the represented
    /// Sdr property type to Sdf type mapping.
    SDR_API
    TfToken GetSdrType() const;

    /// Whether an exact Sdf type exists for the represented Sdr property type
    /// to Sdf type mapping.
    SDR_API
    bool HasSdfType() const;

    /// Gets the Sdf type associated with the represented Sdr property type to
    /// Sdf type mapping. If there is no valid Sdf type, either
    /// SdfValueTypeNames->TokenArray or SdfValueTypeNames->Token is returned.
    SDR_API
    SdfValueTypeName GetSdfType() const;

    /// Equality operation
    SDR_API
    bool operator==(const SdrSdfTypeIndicator &rhs) const;

    /// Inequality operation
    SDR_API
    bool operator!=(const SdrSdfTypeIndicator &rhs) const;

private:
    SdfValueTypeName _sdfType;
    TfToken _sdrType;
    bool _hasSdfTypeMapping;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDR_SDF_TYPE_INDICATOR_H
