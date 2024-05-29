//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_SHADE_UDIM_UTILS_H
#define PXR_USD_USD_SHADE_UDIM_UTILS_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/usdShade/api.h"

#include <string>
#include <utility>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

SDF_DECLARE_HANDLES(SdfLayer);

/// \class UsdShadeUdimUtils
///
/// This class contains a set of utility functions used for working with Udim
/// texture paths
///
class UsdShadeUdimUtils {
public:

    /// Pair representing a resolved UDIM path.
    /// The first member is the fully resolved path
    /// The second number contains only the UDIM tile identifier.
    using ResolvedPathAndTile = std::pair<std::string, std::string>;

    /// Checks if \p identifier contains a UDIM token. Currently only "<UDIM>" 
    /// is supported, but other patterns such as "_MAPID_" may be supported in
    /// the future.
    USDSHADE_API
    static bool IsUdimIdentifier(const std::string &identifier);
    
    /// Replaces the UDIM pattern contained in \p identifierWithPattern
    /// with \p replacement
    USDSHADE_API
    static std::string ReplaceUdimPattern(
        const std::string &identifierWithPattern,
        const std::string &replacement);

    /// Resolves a \p udimPath containing a UDIM token. The path is first
    /// anchored with the passed \p layer if needed, then the function attempts
    /// to resolve any possible UDIM tiles. If any exist, the resolved path is
    /// returned with "<UDIM>" substituted back in. If no resolves succeed or 
    /// \p udimPath does not contain a UDIM token, an empty string is returned.
    USDSHADE_API
    static std::string ResolveUdimPath(
        const std::string &udimPath,
        const SdfLayerHandle &layer);

    /// Attempts to resolve all paths which match a path containing a UDIM
    /// pattern. The path is first anchored with the passed \p layer if needed, 
    /// then the function attempts to resolve all possible UDIM numbers in the 
    /// path.
    USDSHADE_API
    static std::vector<ResolvedPathAndTile> ResolveUdimTilePaths(
        const std::string &udimPath,
        const SdfLayerHandle &layer);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
