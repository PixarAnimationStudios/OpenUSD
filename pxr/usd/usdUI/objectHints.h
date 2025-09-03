//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_USDUI_OBJECT_HINTS_H
#define PXR_USD_USDUI_OBJECT_HINTS_H

/// \file usdUI/objectHints.h

#include "pxr/usd/usd/object.h"
#include "pxr/usd/usdUI/api.h"

#include "pxr/base/tf/staticTokens.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// \hideinitializer
#define USDUI_HINT_KEYS                                     \
    ((UIHints, "uiHints"))                                  \
    ((DisplayName, "displayName"))                          \
    ((DisplayGroup, "displayGroup"))                        \
    ((Hidden, "hidden"))                                    \
    ((ShownIf, "shownIf"))                                  \
    ((ValueLabels, "valueLabels"))                          \
    ((ValueLabelsOrder, "valueLabelsOrder"))                \
    ((DisplayGroupsExpanded, "displayGroupsExpanded"))      \
    ((DisplayGroupsShownIf, "displayGroupsShownIf"))        \

/// \anchor UsdUIHintKeys
/// <b>UsdUIHintKeys</b> provides tokens for the various entries in the
/// \c uiHints dictionary metadata field. Named API corresponding to these
/// entries is provided by the hint access classes UsdUIObjectHints,
/// UsdUIPrimHints, UsdUIPropertyHints, and UsdUIAttributeHints. See
/// documentation there for field descriptions.
TF_DECLARE_PUBLIC_TOKENS(
    UsdUIHintKeys, USDUI_API, USDUI_HINT_KEYS);

/// \class UsdUIObjectHints
///
/// A "schema-like" wrapper that provides API for retrieving and authoring UI
/// hint values within the \c uiHints dictionary field on a UsdObject instance.
///
/// UsdUIObjectHints is "schema-like" in that it interprets fields belonging to
/// a core object type (in this case UsdObject, but see also UsdUIPrimHints,
/// UsdUIPropertyHints, and UsdUIAttributeHints), and provides convenient API
/// for using those fields. However, it is not formally a schema and does not
/// derive from UsdSchemaBase.
///
/// See \ref usdUI_hintsOverview for an overview of UI hints.
class UsdUIObjectHints
{
public:
    /// Default constructor that creates an invalid hints object.
    ///
    /// Calling "set" operations on this object will post errors. "Get"
    /// operations will return fallback values.
    USDUI_API
    UsdUIObjectHints();

    /// Construct a hints object for the given UsdObject \p obj.
    USDUI_API
    explicit UsdUIObjectHints(const UsdObject& obj);

    /// Return the object that this hints intance is interpreting.
    UsdObject GetObject() const { return _obj; }

    /// Return the object's display name, indicating how it should appear in
    /// the UI.
    ///
    /// Backwards compatibility note: If no display name is stored in the
    /// object's \c uiHints dictionary, the value of UsdObject::GetDisplayName
    /// (which has been deprecated) will be returned. This fallback behavior is
    /// temporary, and will be removed in a future release.
    USDUI_API
    std::string GetDisplayName() const;

    /// Set the object's display name to \p name. Return \c true if successful.
    ///
    /// Backwards compatibility note: this function always writes to the
    /// \c uiHints dictionary. It does not call UsdObject::SetDisplayName, which
    /// has been deprecated.
    USDUI_API
    bool SetDisplayName(const std::string& name);

    /// Return the object's hidden status, indicating whether or not it should
    /// be hidden from the UI.
    ///
    /// Backwards compatibility note: If no hidden status is stored in the
    /// object's \c uiHints dictionary, the value of UsdObject::GetHidden
    /// (which has been deprecated) will be returned. This fallback behavior
    /// is temporary, and will be removed in a future release.
    USDUI_API
    bool GetHidden() const;

    /// Set the object's hidden status to \p hidden. Return \c true if
    /// successful.
    ///
    /// Backwards compatibility note: this function always writes to the
    /// \c uiHints dictionary. It does not call UsdObject::SetHidden, which
    /// has been deprecated.
    USDUI_API
    bool SetHidden(bool hidden);

    /// Return \c true if this hints object is valid.
    explicit operator bool() const {
        return bool(_obj);
    }

    /// Equality operator.
    bool operator==(const UsdUIObjectHints& rhs) const {
        return _obj == rhs._obj;
    }

    /// Inequality operator.
    bool operator!=(const UsdUIObjectHints& rhs) const {
        return !(*this == rhs);
    }

protected:
    /// Combine \p key1 and \p key2 into a single token, separated by the
    /// namespace delimiter ':'.
    USDUI_API
    static TfToken _MakeKeyPath(
        const TfToken& key1,
        const TfToken& key2);

private:
    UsdObject _obj;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
