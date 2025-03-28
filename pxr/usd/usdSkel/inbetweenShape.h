//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_SKEL_INBETWEEN_SHAPE_H
#define PXR_USD_USD_SKEL_INBETWEEN_SHAPE_H

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
    bool GetWeight(float* weight) const;

    /// Set the location at which the shape is applied.
    USDSKEL_API
    bool SetWeight(float weight) const;

    /// Has a weight value been explicitly authored on this shape?
    ///
    /// \sa GetWeight()
    USDSKEL_API
    bool HasAuthoredWeight() const;

    /// Get the point offsets corresponding to this shape.
    USDSKEL_API
    bool GetOffsets(VtVec3fArray* offsets) const;

    /// Set the point offsets corresponding to this shape.
    USDSKEL_API
    bool SetOffsets(const VtVec3fArray& offsets) const;

    /// Returns a valid normal offsets attribute if the shape has normal
    /// offsets. Returns an invalid attribute otherwise.
    USDSKEL_API
    UsdAttribute GetNormalOffsetsAttr() const;

    /// Returns the existing normal offsets attribute if the shape has
    /// normal offsets, or creates a new one.
    USDSKEL_API
    UsdAttribute
    CreateNormalOffsetsAttr(const VtValue &defaultValue = VtValue()) const;

    /// Get the normal offsets authored for this shape.
    /// Normal offsets are optional, and may be left unspecified.
    USDSKEL_API
    bool GetNormalOffsets(VtVec3fArray* offsets) const;
    
    /// Set the normal offsets authored for this shape.
    USDSKEL_API
    bool SetNormalOffsets(const VtVec3fArray& offsets) const;

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

    bool operator==(const UsdSkelInbetweenShape& o) const {
        return _attr == o._attr;
    }
    
    bool operator!=(const UsdSkelInbetweenShape& o) const {
        return !(*this == o);
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

    static const TfToken& _GetNamespacePrefix();

    static const TfToken& _GetNormalOffsetsSuffix();

    UsdAttribute _GetNormalOffsetsAttr(bool create) const;

    /// Factory for UsdBlendShape's use, so that we can encapsulate the
    /// logic of what discriminates an Inbetween in this class, while
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

#endif // PXR_USD_USD_SKEL_INBETWEEN_SHAPE_H
