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
#ifndef AR_PACKAGE_RESOLVER_H
#define AR_PACKAGE_RESOLVER_H

/// \file ar/packageResolver.h

#include "pxr/pxr.h"
#include "pxr/usd/ar/api.h"

#include <memory>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

class ArAsset;
class VtValue;

/// \class ArPackageResolver
/// 
/// Interface for resolving assets within package assets. A package resolver
/// is responsible for processing particular package asset formats and
/// resolving information about assets stored within that package.
///
/// Each package resolver is associated with particular file formats and is 
/// invoked by asset resolution when handling package-relative paths involving 
/// those formats. ArPackageResolver instances are only used internally by Ar
/// and are not directly exposed to clients.
///
/// \section ar_implementing_package_resolver Implementing a Package Resolver
///
/// To implement a package resolver, users must create a plugin containing a
/// subclass of ArPackageResolver and register it with the plugin system
/// so that it can be discovered and instantiated at runtime.
///
/// - Implement an ArPackageResolver subclass
/// \code{.cpp}
/// class CustomPackageResolver : public ArPackageResolver {
///    // ...
/// }
/// \endcode
///
/// - In its implementation, register the ArPackageResolver subclass using
///   #AR_DEFINE_PACKAGE_RESOLVER
/// \code{.cpp}
/// # custom resolver's .cpp file
/// AR_DEFINE_PACKAGE_RESOLVER(CustomPackageResolver, ArPackageResolver);
/// \endcode
///
/// - Declare the ArPackageResolver subclass in the plugin's plugInfo.json file.
///   Note that the entry for the subclass must declare the file format it
///   handles in the 'extensions' metadata.
/// \code{.json}
/// # plugInfo.json
/// {
///     "Plugins": [
///         {
///             "Info": {
///                 "Types" : {
///                     "CustomPackageResolver" : {
///                         "bases": [ "ArPackageResolver" ],
///                         "extensions": [ "pack" ]
///                     }
///                 }
///             },
///             ... 
///         },
///         ...
///     ]
/// 
/// }
/// \endcode
///
class ArPackageResolver
{
public:
    ArPackageResolver(const ArPackageResolver&) = delete;
    ArPackageResolver& operator=(const ArPackageResolver&) = delete;

    AR_API
    virtual ~ArPackageResolver();

    // --------------------------------------------------------------------- //
    /// \name Packaged Path Resolution Operations
    ///
    /// @{
    // --------------------------------------------------------------------- //

    /// Returns the resolved path for the asset located at \p packagedPath 
    /// in the package specified by \p resolvedPackagePath if it exists. 
    /// If the asset does not exist in the package, returns an empty string.
    ///
    /// When ArResolver::Resolve is invoked on a package-relative path, the
    /// path will be parsed into the outermost package path, and the inner
    /// packaged path. The outermost package path will be resolved by the
    /// primary resolver. ArPackageResolver::Resolve will then be called on
    /// the corresponding package resolver with that resolved path and the
    /// inner packaged path. If the inner packaged path is itself a 
    /// package-relative path, this process recurses until all paths have been
    /// resolved.
    ///
    /// \see ArResolver::Resolve
    AR_API
    virtual std::string Resolve(
        const std::string& resolvedPackagePath,
        const std::string& packagedPath) = 0;
    
    /// @}

    // --------------------------------------------------------------------- //
    /// \name Asset-specific Operations
    ///
    /// @{
    // --------------------------------------------------------------------- //

    /// Returns an ArAsset object for the asset at \p resolvedPackagedPath
    /// located in the package asset at \p resolvedPackagePath. 
    /// Returns an invalid std::shared_ptr if object could not be created.
    ///
    /// \see ArResolver::OpenAsset
    AR_API
    virtual std::shared_ptr<ArAsset> OpenAsset(
        const std::string& resolvedPackagePath,
        const std::string& resolvedPackagedPath) = 0;

    // --------------------------------------------------------------------- //
    /// \name Scoped Resolution Cache
    /// 
    /// These functions are called when scoped resolution caches are enabled
    /// via ArResolver. 
    ///
    /// \see \ref ArResolver_scopedCache "Scoped Resolution Cache"
    /// @{
    // --------------------------------------------------------------------- //

    /// Mark the start of a resolution caching scope.
    ///
    /// \see ArResolver::BeginCacheScope
    AR_API
    virtual void BeginCacheScope(
        VtValue* cacheScopeData) = 0;

    /// Mark the end of a resolution caching scope.
    ///
    /// \see ArResolver::EndCacheScope
    AR_API
    virtual void EndCacheScope(
        VtValue* cacheScopeData) = 0;
    
protected:
    AR_API
    ArPackageResolver();
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // AR_PACKAGE_RESOLVER_H
