//
// Copyright 2018 Pixar
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
#ifndef USDSKEL_INBETWEENSHAPE_H
#define USDSKEL_INBETWEENSHAPE_H

#include "pxr/pxr.h"
#include "pxr/usd/usdSkel/api.h"

#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usdSkel/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdSkelInbetweenShape
///
/// Schema wrapper for UsdAttribute for authoring and introspecting attributes
/// that serve as inbetween shapes of a UsdSkelBlendShape.
///
/// Inbetween shapes allow an explicit shape to be specified when the blendshape
/// to which it's bound is evaluated at a certain weight. For example, rather
/// than performing piecewise linear interpolation between a primary shape and
/// the rest shape at weight 0.5, an inbetween shape could be defined at the
/// weight. For weight values greater than 0.5, a shape would then be resolved
/// by linearly interpolating between the inbetween shape and the primary
/// shape, while for weight values less than or equal to 0.5, the shape is
/// resolved by linearly interpolating between the inbetween shape and the
/// primary shape.
class UsdSkelInbetweenShape
{
public:
    /// Default constructor returns an invalid inbetween shape.
    UsdSkelInbetweenShape() {}

    /// Speculative constructor that will produce a valid UsdSkelInbetweenShape
    /// when \p attr already represents an attribute that is an Inbetween, and
    /// produces an \em invalid Inbetween otherwise (i.e.
    /// \ref UsdSkelInbetweenShape_bool "operator bool()" will return false).
    ///
    /// Calling \c UsdSkelInbetweenShape::IsInbetween(attr) will return the same
    /// truth value as this constructor, but if you plan to subsequently use the
    /// Inbetween anyways, just use this constructor.
    USDSKEL_API
    explicit UsdSkelInbetweenShape(const UsdAttribute& attr);

    /// Return the location at which the shape is applied.
    USDSKEL_API
    float GetWeight() const;

    /// Set the location at which the shape is applied.
    USDSKEL_API
    bool SetWeight(float weight);

    /// Has weight been explicitly authored on this shape?
    ///
    /// \sa GetWeight()
    USDSKEL_API
    bool HasAuthoredWeight() const;

    USDSKEL_API
    bool GetOffsets(VtVec3fArray* offsets) const;

    USDSKEL_API
    bool SetOffsets(const VtVec3fArray& offsets) const;

    /// Test whether a given UsdAttribute represents a valid Inbetween, which
    /// implies that creating a UsdSkelInbetweenShape from the attribute will
    /// succeed.
    ///
    /// Succes implies that \c attr.IsDefined() is true.
    USDSKEL_API
    static bool IsInbetween(const UsdAttribute& attr);

    // ---------------------------------------------------------------
    /// \name UsdAttribute API
    // ---------------------------------------------------------------
    /// @{

    /// Allow UsdSkelInbetweenShape to auto-convert to UsdAttribute,
    /// so you can pass a UsdSkelInbetweenShape to any function that
    /// accepts a UsdAttribute or const-ref thereto.
    operator UsdAttribute const& () const { return _attr; }

    /// Explicit UsdAttribute extractor.
    UsdAttribute const &GetAttr() const { return _attr; }

    /// Return true if the wrapped UsdAttribute::IsDefined(), and in
    /// addition the attribute is identified as an Inbetween.
    bool IsDefined() const { return IsInbetween(_attr); }

    /// \anchor UsdSkelInbetweenShape_bool
    /// Return true if this Inbetween is valid for querying and
    /// authoring values and metadata, which is identically equivalent
    /// to IsDefined().
    explicit operator bool() const {
       return IsDefined() ? (bool)_attr : 0;
    }

    /// @}

private:
    friend class UsdSkelBlendShape;

    /// Validate that the given \p name is a valid attribute name for
    /// an inbetween.
    static bool _IsValidInbetweenName(const std::string& name,
                                      bool quiet=false);

    /// Validate that the given \p name contains the inbetweens namespace.
    /// Does not validate \p name as a legal property identifier.
    static bool _IsNamespaced(const TfToken& name);

    /// Return \p name prepended with the proper inbetween namespace, if
    /// it is not already prefixed.
    ///
    /// Does not validate \p name as a legal property identifier, but will
    /// verify that \p name contains no reserved keywords, and will return
    /// an empty TfToken if it does. If \p quiet is true, the verification
    /// will be silent.
    static TfToken _MakeNamespaced(const TfToken& name, bool quiet=false);

    static TfToken const &_GetNamespacePrefix();

    /// Factory for UsdBlendShape's use, so that we can encapsulate the
    /// logic of what discriminates an Inbetween in this calss, while
    /// preserving the pattern that attributes can only be created via
    /// their container objects.
    ///
    /// The name of the created attribute may or may not be the specified
    /// \p attrName, due to the possible need to apply property namespacing.
    ///
    /// \return an invalid Inbetween if we failed to create a valid
    /// attribute, o ra valid Inbetween otherwise. It is not an error
    /// to create over an existing, compatible attribute.
    ///
    /// \sa UsdPrim::CreateAttribute()
    static UsdSkelInbetweenShape _Create(const UsdPrim& prim,
                                         const TfToken& name);

    UsdAttribute _attr;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // USDSKEL_INBETWEENSHAPE_H
