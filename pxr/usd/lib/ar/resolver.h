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
#include <memory>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class ArAsset;
class ArAssetInfo;
class ArResolverContext;
class TfType;
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
{
public:
    AR_API
    virtual ~ArResolver();

    // Disallow copies
    ArResolver(const ArResolver&) = delete;
    ArResolver& operator=(const ArResolver&) = delete;

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
    ///
    /// @{
    // --------------------------------------------------------------------- //

    /// Binds the given context to this resolver.
    ///
    /// Clients should generally use ArResolverContextBinder instead of calling
    /// this function directly.
    ///
    /// \see ArResolverContextBinder
    AR_API
    virtual void BindContext(
        const ArResolverContext& context,
        VtValue* bindingData) = 0;

    /// Unbind the given context from this resolver.
    ///
    /// Clients should generally use ArResolverContextBinder instead of calling
    /// this function directly.
    ///
    /// \see ArResolverContextBinder
    AR_API
    virtual void UnbindContext(
        const ArResolverContext& context,
        VtValue* bindingData) = 0;

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

    /// Refresh any caches associated with the given context.
    AR_API
    virtual void RefreshContext(const ArResolverContext& context) = 0;

    /// Returns the currently-bound asset resolver context.
    ///
    /// \see ArResolver::BindContext, ArResolver::UnbindContext
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

    /// Returns an ArAsset object for the asset located at \p resolvedPath. 
    /// Returns an invalid std::shared_ptr if object could not be created.
    ///
    /// The returned ArAsset object provides functions for accessing the
    /// contents of the specified asset. 
    ///
    /// Note that clients may still be using the data associated with 
    /// this object even after the last shared_ptr has been destroyed. For 
    /// example, a client may have created a memory mapping using the FILE* 
    /// presented in the ArAsset object; this would preclude truncating or
    /// overwriting any of the contents of that file.
    AR_API
    virtual std::shared_ptr<ArAsset> OpenAsset(
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
    /// A resolution cache scope is opened by a call to BeginCacheScope and
    /// must be closed with a matching call to EndCacheScope. The resolver must
    /// cache the results of Resolve until the scope is closed. Note that these
    /// calls may be nested.
    ///
    /// Cache scopes are thread-specific: if multiple threads are running and
    /// a cache scope is opened in one of those threads, caching should be
    /// enabled in that thread only.
    ///
    /// When opening a scope, a resolver may return additional data for
    /// implementation-specific purposes. This data may be shared across 
    /// threads, so long as it is safe to access this data concurrently.
    /// 
    /// ArResolverScopedCache is an RAII object for managing cache scope 
    /// lifetimes and data. Clients should generally use that class rather
    /// than calling the BeginCacheScope and EndCacheScope functions manually.
    ///
    /// \see ArResolverScopedCache
    /// @{
    // --------------------------------------------------------------------- //

    /// Mark the start of a resolution caching scope. 
    ///
    /// Clients should generally use ArResolverScopedCache instead of calling
    /// this function directly.
    ///
    /// Resolvers may fill \p cacheScopeData with arbitrary data. Clients may
    /// also pass in a \p cacheScopeData populated by an earlier call to
    /// BeginCacheScope to allow the resolver access to that information.
    ///
    /// \see ArResolverScopedCache
    AR_API
    virtual void BeginCacheScope(
        VtValue* cacheScopeData) = 0;

    /// Mark the end of a resolution caching scope.
    ///
    /// Clients should generally use ArResolverScopedCache instead of calling
    /// this function directly.
    ///
    /// \p cacheScopeData should contain the data that was populated by the
    /// previous corresponding call to BeginCacheScope.
    ///
    /// \see ArResolverScopedCache
    AR_API
    virtual void EndCacheScope(
        VtValue* cacheScopeData) = 0;

    /// @}

protected:
    AR_API
    ArResolver();
};

/// Returns the configured asset resolver.
///
/// When first called, this function will determine the ArResolver subclass
/// to use for asset resolution via the following process:
///
/// - If a preferred resolver has been set via \ref ArSetPreferredResolver,
///   it will be selected.
///
/// - Otherwise, a list of available ArResolver subclasses in plugins will
///   be generated. If multiple ArResolver subclasses are found, the list
///   will be sorted by typename. ArDefaultResolver will be added as the last
///   element of this list, and the first resolver in the list will be
///   selected. 
///
/// - The plugin for the selected subclass will be loaded and an instance
///   of the subclass will be constructed.
///
/// - If an error occurs, an ArDefaultResolver will be constructed.
///
/// The constructed ArResolver subclass will be cached and used to service
/// function calls made on the returned resolver.
///
/// Note that this function may not return the constructed subclass itself, 
/// meaning that dynamic casts to the subclass type may fail. See
/// ArGetUnderlyingResolver if access to this object is needed.
AR_API
ArResolver& ArGetResolver();

/// Set the preferred ArResolver subclass used by ArGetResolver.
///
/// Consumers may override ArGetResolver's plugin resolver discovery and
/// force the use of a specific resolver subclass by calling this 
/// function with the typename of the implementation to use. 
///
/// If the subclass specified by \p resolverTypeName cannot be found, 
/// ArGetResolver will issue a warning and fall back to using 
/// ArDefaultResolver.
///
/// This must be called before the first call to ArGetResolver.
AR_API
void ArSetPreferredResolver(const std::string& resolverTypeName);

/// \name Advanced API
///
/// \warning These functions should typically not be used by consumers except
/// in very specific cases. Consumers who want to retrieve an ArResolver to
/// perform asset resolution should use \ref ArGetResolver.
/// 
/// @{

/// Returns the underlying ArResolver instance used by ArGetResolver.
///
/// This function returns the instance of the ArResolver subclass used by 
/// ArGetResolver and can be dynamic_cast to that type.
///
/// \warning This functions should typically not be used by consumers except
/// in very specific cases. Consumers who want to retrieve an ArResolver to
/// perform asset resolution should use \ref ArGetResolver.
AR_API
ArResolver& ArGetUnderlyingResolver();

/// Returns list of TfTypes for available ArResolver subclasses.
///
/// This function returns the list of ArResolver subclasses used to determine 
/// the resolver implementation returned by \ref ArGetResolver. See 
/// documentation on that function for more details.
///
/// If this function is called from within a call (or calls) to 
/// \ref ArCreateResolver, the ArResolver subclass(es) being created will 
/// be removed from the returned list.
///
/// This function is not safe to call concurrently with itself or 
/// \ref ArCreateResolver.
///
/// \warning This functions should typically not be used by consumers except
/// in very specific cases. Consumers who want to retrieve an ArResolver to
/// perform asset resolution should use \ref ArGetResolver.
AR_API
std::vector<TfType> ArGetAvailableResolvers();

/// Construct an instance of the ArResolver subclass specified by 
/// \p resolverType.
///
/// This function will load the plugin for the given \p resolverType and
/// construct and return a new instance of the specified ArResolver subclass.
/// If an error occurs, coding errors will be emitted and this function
/// will return an ArDefaultResolver instance.
///
/// Note that this function *does not* change the resolver used by 
/// \ref ArGetResolver to an instance of \p resolverType.
///
/// This function is not safe to call concurrently with itself or 
/// \ref ArGetAvailableResolvers.
///
/// \warning This functions should typically not be used by consumers except
/// in very specific cases. Consumers who want to retrieve an ArResolver to
/// perform asset resolution should use \ref ArGetResolver.
AR_API
std::unique_ptr<ArResolver> ArCreateResolver(const TfType& resolverType);

/// @}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // AR_RESOLVER_H
