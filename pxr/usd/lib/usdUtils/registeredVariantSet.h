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
#ifndef USDUTILS_REGISTEREDVARIANTSET_H
#define USDUTILS_REGISTEREDVARIANTSET_H

/// \file usdUtils/registeredVariantSet.h

#include <string>

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
        /// Never
        ///
        /// This variantSet selection is meant to remain entirely within an
        /// application.  This typically represents a "session" variantSelection
        /// that should not be transmitted down the pipeline.
        Never,

        /// IfAuthored
        ///
        /// This variantSet selection should be exported if there is an authored
        /// opinion in the application.  This is only relevant if the
        /// application is able to distinguish between "default" and "set"
        /// opinions.
        IfAuthored,

        /// Authored
        ///
        /// This variantSet selection should always be exported.
        Always,
    };

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

#endif // USDUTILS_REGISTEREDVARIANTSET_H
