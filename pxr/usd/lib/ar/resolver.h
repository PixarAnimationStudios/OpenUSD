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
#ifndef AR_RESOLVER_H
#define AR_RESOLVER_H

#include "pxr/usd/ar/api.h"
#include <boost/noncopyable.hpp>
#include <string>

class ArResolverContext;
class ArAssetInfo;
class VtValue;

/// \class ArResolver
///
/// Interface for the asset resolution system. An asset resolver is 
/// responsible for resolving asset information (including the asset's
/// physical path) from a logical path.
class ArResolver 
    : public boost::noncopyable
{
public:
    AR_API
    virtual ~ArResolver();

    // --------------------------------------------------------------------- //
    /// \name Path Resolution Operations
    ///
    /// @{
    // --------------------------------------------------------------------- //

    /// Configures the resolver for a given asset path
    AR_API
    virtual void ConfigureResolverForAsset(const std::string& path) = 0;

    /// Returns the path formed by anchoring \p path to \p anchorPath.
    ///
    /// If \p anchorPath ends with a trailing '/', it is treated as
    /// a directory to which \p path will be anchored. Otherwise, it
    /// is treated as a file and \p path will be anchored to its
    /// containing directory.
    ///
    /// If \p anchorPath is empty, \p path will be returned as-is.
    ///
    /// If \p path is empty or not a relative path, it will be 
    /// returned as-is.
    AR_API
    virtual std::string AnchorRelativePath(
        const std::string& anchorPath, 
        const std::string& path) = 0; 

    /// Returns true if the given path is a relative path.
    AR_API
    virtual bool IsRelativePath(const std::string& path) = 0;

    /// Returns true if the given path is a repository path.
    AR_API
    virtual bool IsRepositoryPath(const std::string& path) = 0;

    /// Returns whether this path is a search path.
    AR_API
    virtual bool IsSearchPath(const std::string& path) = 0;

    /// Returns the normalized extension for the given \p path. 
    AR_API
    virtual std::string GetExtension(const std::string& path) = 0;

    /// Returns a normalized version of the given \p path
    AR_API
    virtual std::string ComputeNormalizedPath(const std::string& path) = 0;

    /// Returns the computed repository path using the current resolver 
    AR_API
    virtual std::string ComputeRepositoryPath(const std::string& path) = 0;

    /// Returns the local path for the given \p path.
    AR_API
    virtual std::string ComputeLocalPath(const std::string& path) = 0;

    /// Returns the resolved filesystem path for the file identified by
    /// the given \p path if it exists. If the file does not exist,
    /// returns an empty string.
    AR_API
    virtual std::string Resolve(const std::string& path) = 0;

    /// @}

    // --------------------------------------------------------------------- //
    /// \name Path Resolver Context Operations
    /// @{

    /// Return a default ArResolverContext that may be bound to this resolver
    /// to resolve assets when no other context is explicitly specified.
    ///
    /// This function should not automatically bind this context, but should
    /// create one that may be used later.
    AR_API
    virtual ArResolverContext CreateDefaultContext() = 0;

    /// Return a default ArResolverContext that may be bound to this resolver
    /// to resolve the asset located at \p filePath when no other context is
    /// explicitly specified.
    ///
    /// This function should not automatically bind this context, but should
    /// create one that may be used later.
    AR_API
    virtual ArResolverContext CreateDefaultContextForAsset(
        const std::string& filePath) = 0;

    /// Return a default ArResolverContext that may be bound to this resolver
    /// to resolve assets located in the given \p fileDirectory when no other
    /// context is explicitly specified.
    ///
    /// This function should not automatically bind this context, but should
    /// create one that may be used later.
    AR_API
    virtual ArResolverContext CreateDefaultContextForDirectory(
        const std::string& fileDirectory) = 0;

    /// Refresh any caches associated with the given context.
    AR_API
    virtual void RefreshContext(const ArResolverContext& context) = 0;

    /// Returns the currently-bound asset resolver context.
    AR_API
    virtual ArResolverContext GetCurrentContext() = 0;

    /// @}

    // --------------------------------------------------------------------- //
    /// \name Layer-specific Operations
    ///
    /// \see SdfLayer
    /// @{
    // --------------------------------------------------------------------- //

    /// Returns the resolved filesystem path for the layer identified
    /// by \p path following the same path resolution behavior as in
    /// \ref Resolve(const std::string&).
    ///
    /// If the layer identified by \p path represents an asset and
    /// \p assetInfo is not \c nullptr, it will be populated with
    /// additional information about the asset.
    ///
    /// \see Resolve(const std::string&).
    AR_API
    virtual std::string ResolveWithAssetInfo(
        const std::string& path, 
        ArAssetInfo* assetInfo) = 0;

    /// Update \p assetInfo with respect to the given \p fileVersion
    AR_API
    virtual void UpdateAssetInfo(
        const std::string& identifier,
        const std::string& filePath,
        const std::string& fileVersion,
        ArAssetInfo* assetInfo) = 0;

    /// Returns true if a layer may be written to the given \p path, false
    /// otherwise. 
    /// 
    /// If this function returns false and \p whyNot is not \c nullptr,
    /// it will be filled in with an explanation.
    AR_API
    virtual bool CanWriteLayerToPath(
        const std::string& path,
        std::string* whyNot) = 0;

    /// Returns true if a new layer may be created using the given
    /// \p identifier, false otherwise.
    ///
    /// If this function returns false and \p whyNot is not \c nullptr,
    /// it will be filled in with an explanation.
    AR_API
    virtual bool CanCreateNewLayerWithIdentifier(
        const std::string& identifier, 
        std::string* whyNot) = 0;

    /// @}

protected:
    // --------------------------------------------------------------------- //
    /// \name Scoped Resolution Caches
    ///
    /// A scoped resolution cache indicates to the resolver that results of
    /// calls to Resolve should be cached for a certain scope. This is
    /// important for performance and also for consistency -- it ensures 
    /// that repeated calls to Resolve with the same parameters will
    /// return the same result.
    ///
    /// Scoped caches are managed by ArResolverScopedCache instances, which
    /// call _BeginCacheScope on construction and _EndCacheScope on 
    /// destruction. Note that these instances may be nested. The resolver 
    /// must cache the results of Resolve until the last instance is 
    /// destroyed.
    ///
    /// ArResolverScopedCache instances only apply to the thread in
    /// which they are created. If multiple threads are running and an
    /// ArResolverScopedCache is created in one of those threads, caching
    /// should be enabled in that thread only. 
    ///
    /// Resolvers can populate an ArResolverScopedCache with data
    /// for implementation-specific purposes. An ArResolverScopedCache
    /// may share this data with other instances, including instances that
    /// are created in different threads. This allows cache data to be
    /// shared across threads, which means the resolver must ensure it 
    /// is safe to access this data concurrently.
    ///
    /// \see ArResolverScopedCache
    /// @{
    // --------------------------------------------------------------------- //

    friend class ArResolverScopedCache;

    /// Called by ArResolverScopedCache to mark the start of a resolution
    /// caching scope. 
    ///
    /// Resolvers may fill \p cacheScopeData with arbitrary data, which will
    /// be stored in the ArResolverScopedCache. If an ArResolverScopedCache
    /// is constructed with data shared from another ArResolverScopedCache
    /// instance, \p cacheScopeData will contain a copy of that data.
    virtual void _BeginCacheScope(
        VtValue* cacheScopeData) = 0;

    /// Called by ArResolverScopedCache to mark the end of a resolution
    /// caching scope.
    ///
    /// \p cacheScopeData will contain the data stored in the 
    /// ArResolverScopedCache from the call to _BeginCacheScope.
    virtual void _EndCacheScope(
        VtValue* cacheScopeData) = 0;

    /// @}

    // --------------------------------------------------------------------- //
    /// \name Path Resolver Context Operations
    ///
    /// \see ArResolverContext
    /// \see ArResolverContextBinder
    /// @{
    // --------------------------------------------------------------------- //

    friend class ArResolverContextBinder;

    virtual void _BindContext(
        const ArResolverContext& context,
        VtValue* bindingData) = 0;

    virtual void _UnbindContext(
        const ArResolverContext& context,
        VtValue* bindingData) = 0;

    /// @}

protected:
    ArResolver();
};

/// Returns the configured asset resolver.
AR_API
ArResolver& ArGetResolver();

#endif // AR_RESOLVER_H
