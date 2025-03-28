//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_NDR_SDF_TYPE_INDICATOR_H
#define PXR_USD_NDR_SDF_TYPE_INDICATOR_H

/// \file ndr/sdfTypeIndicator.h
///
/// \deprecated
/// All Ndr objects are deprecated in favor of the corresponding Sdr objects
/// in sdr/registry.h

#include "pxr/pxr.h"
#include "pxr/usd/ndr/api.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/sdf/valueTypeName.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class NdrSdfTypeIndicator
/// Represents a mapping from an Ndr Property type to Sdf type.
///
/// If an exact mapping exists from Ndr Property type to Sdf type, HasSdfType()
/// will return true, and GetSdfType() will return the Sdf type resulting from
/// the conversion. GetNdrType() will return the original Ndr Property type. 
///
/// If a mapping doesn't exist from Ndr Property type to Sdf type, HasSdfType()
/// will return false, and GetSdfType() will return either
/// SdfValueTypeNames->TokenArray or SdfValueTypeNames->Token. GetNdrType()
/// will return the original Ndr Property type.
///
/// \deprecated
/// Deprecated in favor of SdrSdfTypeIndicator
class NdrSdfTypeIndicator
{
public:
    /// Default constructor.
    NDR_API
    NdrSdfTypeIndicator();

    /// Constructor. The sdfType must be TokenArray or Token if hasSdfTypeMapping
    /// is set to false.
    NDR_API
    NdrSdfTypeIndicator(
        const SdfValueTypeName& sdfType,
        const TfToken& ndrType,
        bool hasSdfTypeMapping = true);

    /// Gets the original Ndr property type associated with the represented
    /// Ndr property type to Sdf type mapping.
    ///
    /// \deprecated
    /// Deprecated in favor of GetSdrType
    NDR_API
    TfToken GetNdrType() const;

    /// Gets the original Sdr property type associated with the represented
    /// Sdr property type to Sdf type mapping.
    NDR_API
    TfToken GetSdrType() const;

    /// Whether an exact Sdf type exists for the represented Ndr property type
    /// to Sdf type mapping.
    NDR_API
    bool HasSdfType() const;

    /// Gets the Sdf type associated with the represented Ndr property type to
    /// Sdf type mapping. If there is no valid Sdf type, either
    /// SdfValueTypeNames->TokenArray or SdfValueTypeNames->Token is returned.
    NDR_API
    SdfValueTypeName GetSdfType() const;

    /// Equality operation
    NDR_API
    bool operator==(const NdrSdfTypeIndicator &rhs) const;

    /// Inequality operation
    NDR_API
    bool operator!=(const NdrSdfTypeIndicator &rhs) const;

private:
    SdfValueTypeName _sdfType;
    TfToken _ndrType;
    bool _hasSdfTypeMapping;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_NDR_SDF_TYPE_INDICATOR_H
