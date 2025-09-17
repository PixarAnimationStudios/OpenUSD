//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_USDUI_PRIM_HINTS_H
#define PXR_USD_USDUI_PRIM_HINTS_H

/// \file usdUI/primHints.h

#include "pxr/usd/usdUI/objectHints.h"
#include "pxr/usd/usd/prim.h"

#include "pxr/base/vt/dictionary.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdUIPrimHints
///
/// A "schema-like" wrapper that provides API for retrieving and authoring UI
/// hint values within the \c uiHints dictionary metadata field on a UsdPrim
/// instance.
///
/// UsdUIPrimHints is "schema-like" in that it interprets fields belonging to
/// a core object type (in this case UsdPrim, but see also UsdUIObjectHints,
/// UsdUIPropertyHints, and UsdUIAttributeHints), and provides convenient API
/// for using those fields. However, it is not formally a schema and does not
/// derive from UsdSchemaBase.
///
/// See \ref usdUI_hintsOverview for an overview of UI hints.
class UsdUIPrimHints : public UsdUIObjectHints
{
public:
    /// Default constructor that creates an invalid hints object.
    ///
    /// Calling "set" operations on this object will post errors. "Get"
    /// operations will return fallback values.
    USDUI_API
    UsdUIPrimHints();

    /// Construct a hints object for the given \p prim.
    USDUI_API
    explicit UsdUIPrimHints(const UsdPrim& prim);

    /// Return the prim that this hints instance is interpreting.
    UsdPrim GetPrim() const { return _prim; }

    /// Return the prim's display group expansion dictionary.
    ///
    /// This dictionary is keyed by group name and holds boolean values
    /// indicating whether groups should be expanded or collapsed by default.
    USDUI_API
    VtDictionary GetDisplayGroupsExpanded() const;

    /// Set the prim's display group expansion dictionary. Return \c true if
    /// successful.
    ///
    /// Note that since this field is dictionary-valued, its composed value will
    /// be the combination of all its entries as specified across all relevant
    /// edit targets. Overrides occur per-entry rather than the dictionary as a
    /// whole.
    USDUI_API
    bool SetDisplayGroupsExpanded(const VtDictionary& expanded);

    /// Return whether the display group named by \p group should be expanded
    /// by default.
    USDUI_API
    bool GetDisplayGroupExpanded(const std::string& group) const;

    /// Set whether the display group named by \p group should be expanded
    /// by default. Return \c true if successful.
    USDUI_API
    bool SetDisplayGroupExpanded(
        const std::string& group,
        bool expanded);

    /// Return the prim's display group "shown if" dictionary.
    ///
    /// This dictionary is keyed by group name and holds expression strings
    /// that, when evaluated, determine whether the corresponding groups should
    /// be shown or hidden in the UI.
    ///
    /// \sa UsdUIPropertyHints::GetShownIf()
    USDUI_API
    VtDictionary GetDisplayGroupsShownIf() const;

    /// Set the prim's display group "shown if" dictionary to \p shownIf. Return
    /// \c true if successful.
    ///
    /// Note that since this field is dictionary-valued, its composed value will
    /// be the combination of all its entries as specified across all relevant
    /// edit targets. Overrides occur per-entry rather than the dictionary as a
    /// whole.
    ///
    /// \sa UsdUIPropertyHints::SetShownIf()
    USDUI_API
    bool SetDisplayGroupsShownIf(const VtDictionary& shownIf);

    /// Return the "shown if" expression string for the named \p group.
    USDUI_API
    std::string GetDisplayGroupShownIf(const std::string& group) const;

    /// Set the "shown if" expression string for the named \p group to
    /// \p shownIf. Return \c true if successful.
    USDUI_API
    bool SetDisplayGroupShownIf(
        const std::string& group,
        const std::string& shownIf);

private:
    UsdPrim _prim;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
