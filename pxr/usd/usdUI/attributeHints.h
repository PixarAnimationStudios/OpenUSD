//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_USDUI_ATTRIBUTE_HINTS_H
#define PXR_USD_USDUI_ATTRIBUTE_HINTS_H

#include "pxr/usd/usdUI/propertyHints.h"
#include "pxr/usd/usd/attribute.h"

#include "pxr/base/vt/dictionary.h"
#include "pxr/base/vt/value.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdUIAttributeHints
///
/// A "schema-like" wrapper that provides API for retrieving and authoring UI
/// hint values within the \c uiHints dictionary field on a UsdAttribute
/// instance.
///
/// UsdUIAttributeHints is "schema-like" in that it interprets fields belonging
/// to a core object type (in this case UsdAttribute, but see also
/// UsdUIObjectHints, UsdUIPrimHints, and UsdUIPropertyHints), and provides
/// convenient API for using those fields. However, it is not formally a schema
/// and does not derive from UsdSchemaBase.
///
/// See \ref usdUI_hintsOverview for an overview of UI hints.
class UsdUIAttributeHints : public UsdUIPropertyHints
{
public:
    /// Default constructor that creates an invalid hints object.
    ///
    /// Calling "set" operations on this object will post errors. "Get"
    /// operations will return fallback values.
    USDUI_API
    UsdUIAttributeHints();

    /// Construct a hints object for the given UsdAttribute \p attr.
    USDUI_API
    explicit UsdUIAttributeHints(const UsdAttribute& attr);

    /// Return the attribute that this hints instance is interpreting.
    UsdAttribute GetAttribute() const { return _attr; }

    /// Return the attribute's value labels dictionary.
    ///
    /// This dictionary associates user-facing labels for display in the UI with
    /// underlying values to be authored to the attribute when selected.
    USDUI_API
    VtDictionary GetValueLabels() const;

    /// Set the attribute's value labels dictionary. Return \c true if
    /// successful.
    ///
    /// Note that since this field is dictionary-valued, its composed value will
    /// be the combination of all its entries as specified across all relevant
    /// edit targets. Overrides occur per-entry rather than the dictionary as a
    /// whole.
    USDUI_API
    bool SetValueLabels(const VtDictionary& labels);

    /// Return the value labels order, indicating the order in which value
    /// labels should appear.
    USDUI_API
    VtTokenArray GetValueLabelsOrder() const;

    /// Set the attribute's value labels order. Return \c true if successful.
    USDUI_API
    bool SetValueLabelsOrder(const VtTokenArray& order);

    /// Author the value associated with the given \p label to the attribute.
    /// If \p label is not a key in the attribute's value labels dictionary,
    /// return \c false. Otherwise return \c true if \p label was successfully
    /// applied.
    USDUI_API
    bool ApplyValueLabel(const std::string& label);

private:
    UsdAttribute _attr;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
