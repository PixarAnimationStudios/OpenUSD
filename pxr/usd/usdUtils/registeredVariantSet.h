//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_UTILS_REGISTERED_VARIANT_SET_H
#define PXR_USD_USD_UTILS_REGISTERED_VARIANT_SET_H

/// \file usdUtils/registeredVariantSet.h

#include "pxr/pxr.h"
#include "pxr/usd/usdUtils/api.h"
#include <string>

PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdUtilsRegisteredVariantSet
///
/// Class that holds information about variantSets that are registered with
/// the pipeline.
///
/// Registered variantSets are known variantSets in a pipeline that may need to
/// be reasoned about by apps during import/export.
///
/// \sa UsdUtilsGetRegisteredVariantSets
struct UsdUtilsRegisteredVariantSet
{
public:
    /// The name of the variantSet.
    const std::string name;

    /// This specifies how the variantSet should be treated during export.
    ///
    /// Note, in the plugInfo.json, the values for these enum's are
    /// lowerCamelCase.
    enum class SelectionExportPolicy {
        /// Never `"never"`
        ///
        /// This variantSet selection is meant to remain entirely within an
        /// application.  This typically represents a "session" variantSelection
        /// that should not be transmitted down the pipeline.
        Never,

        /// IfAuthored `"ifAuthored"`
        ///
        /// This variantSet selection should be exported if there is an authored
        /// opinion in the application.  This is only relevant if the
        /// application is able to distinguish between "default" and "set"
        /// opinions.
        IfAuthored,

        /// Authored `"authored"`
        ///
        /// This variantSet selection should always be exported.
        Always,
    };

    /// Returns the export policy from the string.
    static bool GetSelectionExportPolicyFromString(
        const std::string& selectionExportPolicyStr,
        SelectionExportPolicy* selectionExportPolicy);

    /// Specifies how to export a variant selection.
    const SelectionExportPolicy selectionExportPolicy;

    UsdUtilsRegisteredVariantSet(
            const std::string& name,
            const SelectionExportPolicy& selectionExportPolicy) :
        name(name),
        selectionExportPolicy(selectionExportPolicy)
    {
    }

    // provided so this can be stored in a std::set.
    bool operator<(const UsdUtilsRegisteredVariantSet&
            other) const {
        return this->name < other.name;
    }
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif /* PXR_USD_USD_UTILS_REGISTERED_VARIANT_SET_H */
