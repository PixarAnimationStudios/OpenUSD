//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_RI_RMAN_UTILITIES_H
#define PXR_USD_USD_RI_RMAN_UTILITIES_H

/// \file usdRi/rmanUtilities.h
/// Utilities for converting between USD encodings and Renderman encodings in
/// cases where there is a difference.

#include "pxr/pxr.h"
#include "pxr/usd/usdRi/api.h"
#include "pxr/usd/sdf/listOp.h"

PXR_NAMESPACE_OPEN_SCOPE

class TfToken;

/// Given a \p token representing a UsdGeom interpolate boundary value, returns
/// corresponding rman enum (converted to int).
USDRI_API
int UsdRiConvertToRManInterpolateBoundary(const TfToken &token);

/// Given the integer \p i that corresponds to an rman enum for interpolate
/// boundary condition, returns the equivalent UsdGeom token.
USDRI_API
const TfToken &UsdRiConvertFromRManInterpolateBoundary(int i);

/// Given a \p token representing a UsdGeom face-varying interpolate boundary 
/// value, returns corresponding rman enum (converted to int).
USDRI_API
int UsdRiConvertToRManFaceVaryingLinearInterpolation(const TfToken &token);

/// Given the integer \p i that corresponds to an rman enum for face-varying
/// interpolate boundary condition, returns the equivalent UsdGeom token.
USDRI_API
const TfToken &UsdRiConvertFromRManFaceVaryingLinearInterpolation(int i);

/// Given a \p token representing a UsdGeom Catmull-Clark triangle subdivision
/// rule value, returns corresponding rman enum (converted to int).
USDRI_API
int UsdRiConvertToRManTriangleSubdivisionRule(const TfToken &token);

/// Given the integer \p i that corresponds to an rman enum for a Catmull-
/// Clark triangle subdivision rule, returns the equivalent UsdGeom token.
USDRI_API
const TfToken &UsdRiConvertFromRManTriangleSubdivisionRule(int i);

/// Convert the given RenderMan set specification statement to an
/// equivalent SdfStringListOp form.
///
/// RenderMan specifies certain set operations using a string encoding.
/// The string form contains either a list of named groups, or a unary
/// operator ("+" or "-") followed by a list of named groups.
/// In set-algebra terms "+" is a union and "-" is a difference
/// operator.
///
/// This method converts the string form to an equivalent USD type,
/// SdfStringListOp.
///
/// The string representation is used implicitly for certain
/// attributes; see UsdRiDoesAttributeUseSetSpecification().
///
/// \note SdfStringListOp is more expressive than the RenderMan
///       grouping membership representation, so lossless
///       round-trip conversion is not possible in general.
///
/// \see SdfStringListOp::ApplyOperations()
///
USDRI_API
SdfStringListOp UsdRiConvertRManSetSpecificationToListOp(std::string const&);

/// Return true if an only if the given attribute name uses a
/// string set specification representation in the RenderMan interface.
///
/// Consult the RenderMan documentation for more details, but
/// at time of writing, this includes the following:
///
/// - grouping:membership
/// - lighting:excludesubset
/// - lighting:subset
/// - lightfilter:subset
///
USDRI_API
bool UsdRiDoesAttributeUseSetSpecification(TfToken const& attrName);

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_USD_USD_RI_RMAN_UTILITIES_H
