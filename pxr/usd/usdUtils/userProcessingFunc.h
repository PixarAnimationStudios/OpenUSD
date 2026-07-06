//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_USD_UTILS_USER_PROCESSING_FUNC
#define PXR_USD_USD_UTILS_USER_PROCESSING_FUNC

/// \file usdUtils/userProcessingFunc.h

#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usdUtils/api.h"

#include <functional>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// Class containing information from a processed dependency.
/// A UsdUtilsDependencyInfo object is passed into the user processing function
/// and contains relevant asset path and dependency information.
/// Additionally, a UsdUtilsDependencyInfo object is also returned from the 
/// user processing function and communicates back to the asset localization 
/// routine any changes that were made during processing.
class UsdUtilsDependencyInfo {
public:
    USDUTILS_API UsdUtilsDependencyInfo() = default;
    USDUTILS_API explicit UsdUtilsDependencyInfo(const std::string &assetPath) 
        : _assetPath(assetPath) {}

    USDUTILS_API UsdUtilsDependencyInfo(
        const std::string &assetPath, 
        const std::vector<std::string> &dependencies) 
        : _assetPath(assetPath), _dependencies(dependencies) {}

private:
    /// This constructor is meant to be used by the asset localization system.
    /// The raw asset path and expression variables are not considered when
    /// UsdUtilsDependencyInfo is returned from a callback function.
    USDUTILS_API UsdUtilsDependencyInfo(
        const std::string &assetPath,
        const std::vector<std::string> &dependencies,
        const std::string &rawAssetPath,
        const VtDictionary *expressionVariables) 
        : _assetPath(assetPath), _dependencies(dependencies),
          _rawAssetPath(rawAssetPath), _expressionVariables(expressionVariables)
          {}

    friend class UsdUtils_LocalizationClient;

public:
    /// Returns the asset value path for the dependency.  The string returned
    /// by this function will contain the result of evaluating any variable
    /// expressions present in the asset path string.
    ///
    /// When UsdUtilsDependencyInfo is returned as a parameter from a user
    /// processing function, the localization system compares the value
    /// with the value that was originally authored in the layer.
    ///
    /// If the values are the same, no special action is taken and processing
    /// will continue as normal.
    ///
    /// If the returned value is an empty string, the system will ignore this
    /// path as well as any dependencies associated with it.
    ///
    /// If the returned value differs from what what was originally
    /// authored into the layer, the system will instead operate on the updated.
    /// value.  If the updated path can be opened as a layer, it will be 
    /// enqueued and searched for additional dependencies.
    ///
    /// Note: A coding error will be issued if a user processing function
    /// attempts to modify an asset path contained in an existing package.
    USDUTILS_API const std::string& GetAssetPath() const {
        return _assetPath;
    }

    /// Gets the raw asset path that was authored in the layer without any
    /// substitutions or evaluations.  This value is ignored when
    /// UsdUtilsDependencyInfo is returned from a callback functions.
    USDUTILS_API const std::string& GetRawAssetPath() const {
        return _rawAssetPath;
    }

    /// Gets the current set of variable expressions relevant to this
    /// dependency. This value is ignored when UsdUtilsDependencyInfo is
    /// returned from a callback function.
    USDUTILS_API const VtDictionary &GetExpressionVariables() const {
        return _expressionVariables ? 
            *_expressionVariables : VtGetEmptyDictionary();
    }

    /// Returns a list of dependencies related to the asset path.
    /// Paths in this vector are specified relative to their containing layer. 
    /// When passed into the user processing function, if this array is
    /// populated, then the asset path resolved to one or more values, such as
    /// in the case of UDIM tiles or clip asset path template strings.
    ///
    /// When this structure is returned from a processing function, each path
    /// contained within will in turn be processed by the system. Any path 
    /// that can be opened as a layer, will be enqueued and searched for 
    /// additional dependencies.
    USDUTILS_API const std::vector<std::string>& GetDependencies() const {
        return _dependencies;
    }

    /// Equality: Asset path and dependencies are the same
    bool operator==(const UsdUtilsDependencyInfo &rhs) const {
        return _assetPath == rhs._assetPath &&
               _dependencies == rhs._dependencies;
    }

    /// Inequality operator
    /// \sa UsdUtilsDependencyInfo::operator==(const UsdUtilsDependencyInfo&)
    bool operator!=(const UsdUtilsDependencyInfo& rhs) const {
        return !(*this == rhs);
    }

private:
    std::string _assetPath;
    std::vector<std::string> _dependencies;
    std::string _rawAssetPath;
    const VtDictionary *_expressionVariables = nullptr;
};

/// Signature for user supplied processing function.
/// \param layer The layer containing this dependency.
/// \param dependencyInfo contains asset path information for this dependency.
using UsdUtilsProcessingFunc = UsdUtilsDependencyInfo(
    const SdfLayerHandle &layer, 
    const UsdUtilsDependencyInfo &dependencyInfo);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_UTILS_USER_PROCESSING_FUNC
