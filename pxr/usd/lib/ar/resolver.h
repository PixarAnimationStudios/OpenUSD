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

/// \file ar/resolver.h

#include "pxr/pxr.h"
#include "pxr/usd/ar/api.h"
#include <boost/noncopyable.hpp>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

class ArResolverContext;
class ArAssetInfo;
class VtValue;

/// \class ArResolver
///
/// Interface for the asset resolution system. An asset resolver is 
/// responsible for resolving asset information (including the asset's
/// physical path) from a logical path.
///
/// See \ref ar_implementing_resolver for information on how to customize
/// asset resolution behavior by implementing a subclass of ArResolver.
/// Clients may use #ArGetResolver to access the configured asset resolver.
///
class ArResolver 
    : public boost::noncopyable
{
public:
    AR_API
    virtual ~ArResolver();

    // --------------------------------------------------------------------- //
    /// \anchor ArResolver_resolution
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
    /// \anchor ArResolver_context
    /// \name Asset Resolver Context Operations
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
    /// \anchor ArResolver_files
    /// \name File/asset-specific Operations
    ///
    /// @{
    // --------------------------------------------------------------------- //

    /// Returns the resolved filesystem path for the file identified
    /// by \p path following the same path resolution behavior as in
    /// \ref Resolve(const std::string&).
    ///
    /// If the file identified by \p path represents an asset and
    /// \p assetInfo is not \c nullptr, the resolver should populate 
    /// \p assetInfo with whatever additional metadata it knows or can
    /// reasonably compute about the asset without actually opening it.
    ///
    /// \see Resolve(const std::string&).
    AR_API
    virtual std::string ResolveWithAssetInfo(
        const std::string& path, 
        ArAssetInfo* assetInfo) = 0;

    /// Update \p assetInfo with respect to the given \p fileVersion .
    /// \note This API is currently in flux.  In general, you should prefer
    /// to call ResolveWithAssetInfo()
    AR_API
    virtual void UpdateAssetInfo(
        const std::string& identifier,
        const std::string& filePath,
        const std::string& fileVersion,
        ArAssetInfo* assetInfo) = 0;

    /// Returns a value representing the last time the asset identified
    /// by \p path was modified. \p resolvedPath is the resolved path
    /// of the asset.
    ///
    /// Implementations may use whatever value is most appropriate
    /// for this timestamp. The value must be equality comparable, 
    /// and this function must return a different timestamp whenever 
    /// an asset has been modified. For instance, if an asset is stored 
    /// as a file on disk, the timestamp may simply be that file's mtime. 
    ///
    /// If a timestamp cannot be retrieved, returns an empty VtValue.
    AR_API
    virtual VtValue GetModificationTimestamp(
        const std::string& path,
        const std::string& resolvedPath) = 0;

    /// Fetch the asset identified by \p path to the filesystem location
    /// specified by \p resolvedPath. \p resolvedPath is the resolved path
    /// that results from calling Resolve or ResolveWithAssetInfo on 
    /// \p path.
    ///
    /// This method provides a way for consumers that expect assets 
    /// to exist as physical files on disk to retrieve data from 
    /// systems that store data in external data stores, e.g. databases,
    /// etc. 
    ///
    /// Returns true if the asset was successfully fetched to the specified
    /// \p resolvedPath or if no fetching was required. If \p resolvedPath 
    /// is not a local path or the asset could not be fetched to that path, 
    /// returns false.
    AR_API
    virtual bool FetchToLocalResolvedPath(
        const std::string& path,
        const std::string& resolvedPath) = 0;

    /// Returns true if a file may be written to the given \p path, false
    /// otherwise. 
    /// 
    /// If this function returns false and \p whyNot is not \c nullptr,
    /// it will be filled in with an explanation.
    AR_API
    virtual bool CanWriteLayerToPath(
        const std::string& path,
        std::string* whyNot) = 0;

    /// Returns true if a new file may be created using the given
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
    /// \anchor ArResolver_scopedCache
    /// \name Scoped Resolution Cache
    ///
    /// A scoped resolution cache indicates to the resolver that results of
    /// calls to Resolve should be cached for a certain scope. This is
    /// important for performance and also for consistency -- it ensures 
    /// that repeated calls to Resolve with the same parameters will
    /// return the same result.
    ///
    /// Scoped caches are managed by ArResolverScopedCache instances, which
    /// call ArResolver::_BeginCacheScope on construction and 
    /// ArResolver::_EndCacheScope on destruction. Note that these instances 
    /// may be nested. The resolver must cache the results of Resolve until 
    /// the last instance is destroyed.
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
    AR_API
    virtual void _BeginCacheScope(
        VtValue* cacheScopeData) = 0;

    /// Called by ArResolverScopedCache to mark the end of a resolution
    /// caching scope.
    ///
    /// \p cacheScopeData will contain the data stored in the 
    /// ArResolverScopedCache from the call to ArResolver::_BeginCacheScope.
    AR_API
    virtual void _EndCacheScope(
        VtValue* cacheScopeData) = 0;

    /// @}

    // --------------------------------------------------------------------- //
    /// \anchor ArResolver_contextBinder
    /// \name Asset Resolver Context Binder
    ///
    /// \see ArResolverContext
    /// \see ArResolverContextBinder
    /// @{
    // --------------------------------------------------------------------- //

    friend class ArResolverContextBinder;

    AR_API
    virtual void _BindContext(
        const ArResolverContext& context,
        VtValue* bindingData) = 0;

    AR_API
    virtual void _UnbindContext(
        const ArResolverContext& context,
        VtValue* bindingData) = 0;

    /// @}

protected:
    AR_API
    ArResolver();
};

/// Returns the configured asset resolver.
///
/// When first called, this function will check if any plugins contain a
/// subclass of ArResolver. If so, that plugin will be loaded and a new
/// instance of that subclass will be constructed. Otherwise, a new instance
/// of ArDefaultResolver will be used instead. This instance will be returned
/// by this and all subsequent calls to this function.
///
/// If ArResolver subclasses are found in multiple plugins, the subclass
/// whose typename is lexicographically first will be selected and a
/// warning will be issued.
AR_API
ArResolver& ArGetResolver();

PXR_NAMESPACE_CLOSE_SCOPE

#endif // AR_RESOLVER_H
