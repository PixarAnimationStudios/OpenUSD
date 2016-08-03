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
/// \file sdf/variantSpec.h

#ifndef SDF_VARIANTSPEC_H
#define SDF_VARIANTSPEC_H

#include "pxr/usd/sdf/api.h"
#include "pxr/usd/sdf/declareSpec.h"
#include "pxr/usd/sdf/spec.h"
#include <string>

SDF_DECLARE_HANDLES(SdfLayer);
SDF_DECLARE_HANDLES(SdfPrimSpec);
SDF_DECLARE_HANDLES(SdfVariantSpec);
SDF_DECLARE_HANDLES(SdfVariantSetSpec);

class SdfPath;

///
/// \class SdfVariantSpec 
/// \brief Represents a single variant in a variant set.
///
/// A variant contains a prim.  This prim is the root prim of the variant.
///
/// SdfVariantSpecs are value objects.  This means they are immutable 
/// once created and they are passed by copy-in APIs.  To change a variant 
/// spec, you make a new one and replace the existing one.
///
class SDF_API SdfVariantSpec : public SdfSpec
{
    SDF_DECLARE_SPEC(SdfSchema, SdfSpecTypeVariant,
                     SdfVariantSpec, SdfSpec);

public:
    ///
    /// \name Spec construction
    /// @{

    /// \brief Constructs a new instance.
    static SdfVariantSpecHandle New(const SdfVariantSetSpecHandle& owner,
                                    const std::string& name);

    /// @}

    /// \name Name
    /// @{

    /// \brief Returns the name of this variant.
    std::string GetName() const;

    /// \brief Returns the name of this variant.
    TfToken GetNameToken() const;

    /// @}
    /// \name Namespace hierarchy
    /// @{

    /// \brief Return the SdfVariantSetSpec that owns this variant.
    SdfVariantSetSpecHandle GetOwner() const;

    /// \brief Get the prim spec owned by this variant.
    SdfPrimSpecHandle GetPrimSpec() const;
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

#endif /* SDF_VARIANT_SPEC_H */
