//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_USDUI_PROPERTY_HINTS_H
#define PXR_USD_USDUI_PROPERTY_HINTS_H

/// \file usdUI/propertyHints.h

#include "pxr/usd/usdUI/objectHints.h"
#include "pxr/usd/usd/property.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdUIPropertyHints
///
/// A "schema-like" wrapper that provides API for retrieving and authoring UI
/// hint values within the \c uiHints dictionary field on a UsdProperty
/// instance.
///
/// UsdUIPropertyHints is "schema-like" in that it interprets fields belonging
/// to a core object type (in this case UsdProperty, but see also
/// UsdUIObjectHints, UsdUIPrimHints, and UsdUIAttributeHints), and provides
/// convenient API for using those fields. However, it is not formally a schema
/// and does not derive from UsdSchemaBase.
///
/// See \ref usdUI_hintsOverview for an overview of UI hints.
class UsdUIPropertyHints : public UsdUIObjectHints
{
public:
    /// Default constructor that creates an invalid hints object.
    ///
    /// Calling "set" operations on this object will post errors. "Get"
    /// operations will return fallback values.
    USDUI_API
    UsdUIPropertyHints();

    /// Construct a hints object for the given UsdProperty \p prop.
    USDUI_API
    explicit UsdUIPropertyHints(const UsdProperty& prop);

    /// Return the property that this hints instance is interpreting.
    UsdProperty GetProperty() const { return _prop; }

    /// Return the property's display group, indicating which group it should
    /// appear under in the UI.
    ///
    /// Backwards compatibility note: If no display group is stored in the
    /// property's \c uiHints dictionary, the value of
    /// UsdProperty::GetDisplayGroup (which has been deprecated) will be
    /// returned. This fallback behavior is temporary, and will be removed in
    /// a future release.
    USDUI_API
    std::string GetDisplayGroup() const;

    /// Set the object's display group to \p group. Return \c true if
    /// successful.
    ///
    /// Backwards compatibility note: this function always writes to the
    /// \c uiHints dictionary. It does not call UsdObject::SetDisplayGroup,
    /// which has been deprecatd.
    USDUI_API
    bool SetDisplayGroup(const std::string& group);

    /// Return the property's "shown if" expression string.
    ///
    /// This expression, when evaluated, determines whether the property should
    /// be shown or hidden in the UI.
    USDUI_API
    std::string GetShownIf() const;

    /// Set the property's "shown if" expression string. Return \c true if
    /// successful.
    USDUI_API
    bool SetShownIf(const std::string& shownIf);

private:
    UsdProperty _prop;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
