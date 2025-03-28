//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_VARIANT_SET_SPEC_H
#define PXR_USD_SDF_VARIANT_SET_SPEC_H

/// \file sdf/variantSetSpec.h

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"
#include "pxr/usd/sdf/declareSpec.h"
#include "pxr/usd/sdf/spec.h"
#include "pxr/usd/sdf/proxyTypes.h"
#include "pxr/usd/sdf/types.h"

#include <iosfwd>
#include <map>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// \class SdfVariantSetSpec 
///
/// Represents a coherent set of alternate representations for part of a
/// scene.
///
/// An SdfPrimSpec object may contain one or more named SdfVariantSetSpec
/// objects that define variations on the prim.
///
/// An SdfVariantSetSpec object contains one or more named SdfVariantSpec
/// objects. It may also define the name of one of its variants to be used by
/// default. 
///
/// When a prim references another prim, the referencing prim may specify
/// one of the variants from each of the variant sets of the target prim.
/// The chosen variant from each set (or the default variant from those sets
/// that the referencing prim does not explicitly specify) is composited 
/// over the target prim, and then the referencing prim is composited over 
/// the result.
///
class SdfVariantSetSpec : public SdfSpec
{
    SDF_DECLARE_SPEC(SdfVariantSetSpec, SdfSpec);

public:
    ///
    /// \name Spec construction
    /// @{

    /// Constructs a new instance.
    SDF_API
    static SdfVariantSetSpecHandle
    New(const SdfPrimSpecHandle& prim, const std::string& name);

    /// Constructs a new instance.
    SDF_API
    static SdfVariantSetSpecHandle
    New(const SdfVariantSpecHandle& prim, const std::string& name);

    /// @}

    /// \name Name
    /// @{

    /// Returns the name of this variant set.
    SDF_API
    std::string GetName() const;

    /// Returns the name of this variant set.
    SDF_API
    TfToken GetNameToken() const;

    /// @}
    /// \name Namespace hierarchy
    /// @{

    /// Returns the prim or variant that this variant set belongs to.
    SDF_API
    SdfSpecHandle GetOwner() const;

    /// @}
    /// \name Variants
    /// @{

    /// Returns the variants as a map.
    SDF_API
    SdfVariantView GetVariants() const;

    /// Returns the variants as a vector.
    SDF_API
    SdfVariantSpecHandleVector GetVariantList() const;

    /// Removes \p variant from the list of variants.
    ///
    /// If the variant set does not currently own \p variant, no action
    /// is taken.
    SDF_API
    void RemoveVariant(const SdfVariantSpecHandle& variant);

    /// @}
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // SD_VARIANTSETSPEC_H
