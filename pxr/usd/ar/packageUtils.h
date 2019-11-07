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
#ifndef PXR_USD_AR_PACKAGE_UTILS_H
#define PXR_USD_AR_PACKAGE_UTILS_H

/// \file ar/packageUtils.h
/// Utility functions for working with package assets

#include "pxr/pxr.h"
#include "pxr/usd/ar/api.h"

#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

// --------------------------------------------------------------------- //
/// \anchor Ar_packagePaths
/// \name Package Relative Paths
///
/// Assets within package assets can be addressed via "package-relative"
/// paths. For example, given a hypothetical package <tt>"Model.package"</tt>, 
/// the asset <tt>"Geom.file"</tt>, can be referred to using the path
/// <tt>"Model.package[Geom.file]"</tt>.
///
/// A package-relative path consists of two parts:
///
/// - The outer "package" path is the path to the containing package asset.
///   This path can be whatever is appropriate to the client's asset system.
///
/// - The inner "packaged" path is the path to an asset contained within the
///   package asset. This path must be a relative path delimited by forward
///   slashes '/', with no leading slashes or drive or device letters. Note
///   that packaged paths may themselves be package-relative paths, since
///   package assets may be nested.
///
/// Examples:
/// - <tt>/path/to/Model.package[a/b/Geom.file]</tt>
/// - <tt>/path/to/Model.package[a/b/Sub.package[c/d/Geom.file]]</tt>
///
/// @{
// --------------------------------------------------------------------- //

/// Return true if \p path is a package-relative path, false otherwise.
AR_API
bool
ArIsPackageRelativePath(const std::string& path);

/// Combines the given \p paths into a single package-relative path, nesting 
/// paths as necessary.
///
/// \code
/// ArJoinPackageRelativePath(["a.pack", "b.pack"])
///    => "a.pack[b.pack]"
///
/// ArJoinPackageRelativePath(["a.pack", "b.pack", "c.pack"])
///    => "a.pack[b.pack[c.pack]]"
///
/// ArJoinPackageRelativePath(["a.pack[b.pack]", "c.pack"])
///    => "a.pack[b.pack[c.pack]]"
/// \endcode
AR_API
std::string
ArJoinPackageRelativePath(const std::vector<std::string>& paths);

/// \overload
AR_API
std::string
ArJoinPackageRelativePath(const std::pair<std::string, std::string>& paths);

/// \overload
AR_API
std::string
ArJoinPackageRelativePath(
    const std::string& packagePath, const std::string& packagedPath);

/// Split package-relative path \p path into a (package path, packaged path)
/// pair. If \p packageRelativePath contains nested package-relative paths
/// the package path will be the outermost package path, and the packaged path 
/// will be the inner package-relative path.
///
/// \code
/// ArSplitPackageRelativePathOuter("a.pack[b.pack]")
///    => ("a.pack", "b.pack")
///
/// ArSplitPackageRelativePathOuter("a.pack[b.pack[c.pack]]")
///    => ("a.pack", "b.pack[c.pack]")
/// \endcode
AR_API
std::pair<std::string, std::string>
ArSplitPackageRelativePathOuter(const std::string& path);

/// Split package-relative path \p path into a (package path, packaged path)
/// pair. If \p packageRelativePath contains nested package-relative paths
/// the package path will be the outermost package-relative path, and the 
/// packaged path will be the innermost packaged path.
///
/// \code
/// ArSplitPackageRelativePathInner("a.pack[b.pack]")
///    => ("a.pack", "b.pack")
///
/// ArSplitPackageRelativePathInner("a.pack[b.pack[c.pack]]")
///    => ("a.pack[b.pack]", "c.pack")
/// \endcode
AR_API
std::pair<std::string, std::string>
ArSplitPackageRelativePathInner(const std::string& path);

/// @}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_AR_PACKAGE_UTILS_H
