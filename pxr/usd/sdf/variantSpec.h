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
#ifndef PXR_USD_SDF_VARIANT_SPEC_H
#define PXR_USD_SDF_VARIANT_SPEC_H

/// \file sdf/variantSpec.h

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"
#include "pxr/usd/sdf/declareSpec.h"
#include "pxr/usd/sdf/proxyTypes.h"
#include "pxr/usd/sdf/spec.h"
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

SDF_DECLARE_HANDLES(SdfLayer);
SDF_DECLARE_HANDLES(SdfPrimSpec);
SDF_DECLARE_HANDLES(SdfVariantSpec);
SDF_DECLARE_HANDLES(SdfVariantSetSpec);

class SdfPath;

/// \class SdfVariantSpec 
///
/// Represents a single variant in a variant set.
///
/// A variant contains a prim.  This prim is the root prim of the variant.
///
/// SdfVariantSpecs are value objects.  This means they are immutable 
/// once created and they are passed by copy-in APIs.  To change a variant 
/// spec, you make a new one and replace the existing one.
///
class SdfVariantSpec : public SdfSpec
{
    SDF_DECLARE_SPEC(SdfVariantSpec, SdfSpec);

public:
    ///
    /// \name Spec construction
    /// @{

    /// Constructs a new instance.
    SDF_API
    static SdfVariantSpecHandle New(const SdfVariantSetSpecHandle& owner,
                                    const std::string& name);

    /// @}

    /// \name Name
    /// @{

    /// Returns the name of this variant.
    SDF_API
    std::string GetName() const;

    /// Returns the name of this variant.
    SDF_API
    TfToken GetNameToken() const;

    /// @}
    /// \name Namespace hierarchy
    /// @{

    /// Return the SdfVariantSetSpec that owns this variant.
    SDF_API
    SdfVariantSetSpecHandle GetOwner() const;

    /// Get the prim spec owned by this variant.
    SDF_API
    SdfPrimSpecHandle GetPrimSpec() const;

    /// Returns the nested variant sets.
    ///
    /// The result maps variant set names to variant sets.  Variant sets
    /// may be removed through the proxy.
    SDF_API
    SdfVariantSetsProxy GetVariantSets() const;

    /// Returns list of variant names for the given variant set.
    SDF_API
    std::vector<std::string> GetVariantNames(const std::string& name) const;

    /// @}
};

/// Convenience function to create a variant spec for a given variant set and
/// a prim at the given path with.
///
/// The function creates the prim spec if it doesn't exist already and any
/// necessary parent prims, in the given layer.
///
/// It adds the variant set to the variant set list if it doesn't already exist.
///
/// It creates a variant spec with the given name under the specified variant
/// set if it doesn't already exist.
SDF_API SdfVariantSpecHandle SdfCreateVariantInLayer(
    const SdfLayerHandle &layer,
    const SdfPath &primPath,
    const std::string &variantSetName,
    const std::string &variantName );

PXR_NAMESPACE_CLOSE_SCOPE

#endif /* SDF_VARIANT_SPEC_H */
