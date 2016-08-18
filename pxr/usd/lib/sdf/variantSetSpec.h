//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef SDF_VARIANTSETSPEC_H
#define SDF_VARIANTSETSPEC_H

/// \file sdf/variantSetSpec.h

#include "pxr/usd/sdf/api.h"
#include "pxr/usd/sdf/declareSpec.h"
#include "pxr/usd/sdf/spec.h"
#include "pxr/usd/sdf/proxyTypes.h"
#include "pxr/usd/sdf/types.h"

#include <iosfwd>
#include <map>
#include <string>
#include <vector>

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
    SDF_DECLARE_SPEC(SdfSchema, SdfSpecTypeVariantSet,
                     SdfVariantSetSpec, SdfSpec);

public:
    ///
    /// \name Spec construction
    /// @{

    /// Constructs a new instance.
    SDF_API
    static SdfVariantSetSpecHandle
    New(const SdfPrimSpecHandle& prim, const std::string& name);

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

    /// Returns the prim that this variant set belongs to.
    SDF_API
    SdfPrimSpecHandle GetOwner() const;

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

#endif // SD_VARIANTSETSPEC_H
