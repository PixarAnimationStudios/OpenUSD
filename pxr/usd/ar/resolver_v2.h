//
// Copyright 2020 Pixar
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
#ifndef INCLUDE_AR_RESOLVER
#error This file should not be included directly. Include resolver.h instead.
#endif

#ifndef PXR_USD_AR_RESOLVER_V2_H
#define PXR_USD_AR_RESOLVER_V2_H

/// \file ar/resolver_v2.h

#include "pxr/pxr.h"
#include "pxr/usd/ar/api.h"
#include "pxr/usd/ar/resolvedPath.h"
#include <memory>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class ArAsset;
class ArAssetInfo;
class ArResolverContext;
class ArWritableAsset;
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
    /// \anchor ArResolver_identifier
    /// \name Identifiers
    ///
    /// Identifiers are canonicalized asset paths that may be assigned
    /// to a logical asset to facilitate comparisons and lookups. They
    /// may be used to determine if different asset paths might refer to
    /// the same asset without performing resolution.
    ///
    /// Since identifiers are just a form of asset path, they may be used
    /// with other functions on ArResolver that require an asset path, like
    /// Resolve.
    ///
    /// If two asset paths produce the same identifier, those asset paths
    /// must refer to the same asset. However, in some cases comparing
    /// identifiers may not be sufficient to determine if assets are equal.
    /// For example, there could be two assets with the same identifier
    /// but whose contents were read from different resolved paths because
    /// different resolver contexts were bound when those assets were loaded.
    ///
    /// @{
    // --------------------------------------------------------------------- //

    /// Returns an identifier for the asset specified by \p assetPath.
    /// If \p anchorAssetPath is not empty, it is the resolved asset path
    /// that \p assetPath should be anchored to if it is a relative path.
    AR_API
    std::string CreateIdentifier(
        const std::string& assetPath,
        const ArResolvedPath& anchorAssetPath = ArResolvedPath());

    /// Returns an identifier for a new asset specified by \p assetPath.
    /// If \p anchorAssetPath is not empty, it is the resolved asset path
    /// that \p assetPath should be anchored to if it is a relative path.
    AR_API
    std::string CreateIdentifierForNewAsset(
        const std::string& assetPath,
        const ArResolvedPath& anchorAssetPath = ArResolvedPath());

    /// @}

    // --------------------------------------------------------------------- //
    /// \anchor ArResolver_resolution
    /// \name Path Resolution Operations
    ///
    /// @{
    // --------------------------------------------------------------------- //

    /// Returns the resolved path for the asset identified by the given \p
    /// assetPath if it exists. If the asset does not exist, returns an empty
    /// ArResolvedPath.
    AR_API
    ArResolvedPath Resolve(
        const std::string& assetPath);

    /// Returns the resolved path for the given \p assetPath that may be used
    /// to create a new asset. If such a path cannot be computed for
    /// \p assetPath, returns an empty ArResolvedPath.
    ///
    /// Note that an asset might or might not already exist at the returned
    /// resolved path.
    AR_API
    ArResolvedPath ResolveForNewAsset(
        const std::string& assetPath);

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
    void BindContext(
        const ArResolverContext& context,
        VtValue* bindingData);

    /// Unbind the given context from this resolver.
    ///
    /// Clients should generally use ArResolverContextBinder instead of calling
    /// this function directly.
    ///
    /// \see ArResolverContextBinder
    AR_API
    void UnbindContext(
        const ArResolverContext& context,
        VtValue* bindingData);

    /// Return an ArResolverContext that may be bound to this resolver
    /// to resolve assets when no other context is explicitly specified.
    AR_API
    ArResolverContext CreateDefaultContext();

    /// Return an ArResolverContext that may be bound to this resolver
    /// to resolve the asset located at \p assetPath when no other context is
    /// explicitly specified.
    AR_API
    ArResolverContext CreateDefaultContextForAsset(
        const std::string& assetPath);

    /// Return an ArResolverContext created from the primary ArResolver
    /// implementation using the given \p contextStr.
    AR_API
    ArResolverContext CreateContextFromString(
        const std::string& contextStr);

    /// Return an ArResolverContext created from the ArResolver registered
    /// for the given \p uriScheme using the given \p contextStr.
    ///
    /// An empty \p uriScheme indicates the primary resolver and is
    /// equivalent to CreateContextFromString(string).
    ///
    /// If no resolver is registered for \p uriScheme, returns an empty
    /// ArResolverContext.
    AR_API
    ArResolverContext CreateContextFromString(
        const std::string& uriScheme, const std::string& contextStr);

    /// Return an ArResolverContext created by combining the ArResolverContext
    /// objects created from the given \p contextStrs.
    ///
    /// \p contextStrs is a list of pairs of strings. The first element in the
    /// pair is the URI scheme for the ArResolver that will be used to create
    /// the ArResolverContext from the second element in the pair. An empty
    /// URI scheme indicates the primary resolver.
    ///
    /// For example:
    ///
    /// \code
    /// ArResolverContext ctx = ArGetResolver().CreateContextFromStrings(
    ///    { {"", "context str 1"}, 
    ///      {"my_scheme", "context str 2"} });
    /// \endcode
    /// 
    /// This will use the primary resolver to create an ArResolverContext
    /// using the string "context str 1" and use the resolver registered for
    /// the "my_scheme" URI scheme to create an ArResolverContext using
    /// "context str 2". These contexts will be combined into a single
    /// ArResolverContext and returned.
    ///
    /// If no resolver is registered for a URI scheme in an entry in
    /// \p contextStrs, that entry will be ignored.
    AR_API
    ArResolverContext CreateContextFromStrings(
        const std::vector<std::pair<std::string, std::string>>& contextStrs);

    /// Refresh any caches associated with the given context.
    AR_API
    void RefreshContext(
        const ArResolverContext& context);

    /// Returns the asset resolver context currently bound in this thread.
    ///
    /// \see ArResolver::BindContext, ArResolver::UnbindContext
    AR_API
    ArResolverContext GetCurrentContext();

    /// Returns true if \p assetPath is a context-dependent path, false
    /// otherwise.
    ///
    /// A context-dependent path may result in different resolved paths
    /// depending on what asset resolver context is bound when Resolve
    /// is called. Assets located at the same context-dependent path may not
    /// be the same since those assets may have been loaded from different
    /// resolved paths. In this case, the assets' resolved paths must be
    /// consulted to determine if they are the same.
    AR_API
    bool IsContextDependentPath(
        const std::string& assetPath);

    /// @}

    // --------------------------------------------------------------------- //
    /// \anchor ArResolver_files
    /// \name File/asset-specific Operations
    ///
    /// @{
    // --------------------------------------------------------------------- //

    /// Returns the file extension for the given \p assetPath. The returned
    /// extension does not include a "." at the beginning.
    AR_API
    std::string GetExtension(
        const std::string& assetPath);

    /// Returns an ArAssetInfo populated with additional metadata (if any)
    /// about the asset at the given \p assetPath. \p resolvedPath is the
    /// resolved path computed for the given \p assetPath.
    AR_API
    ArAssetInfo GetAssetInfo(
        const std::string& assetPath,
        const ArResolvedPath& resolvedPath);

    /// Return a value representing the last time the asset at the given 
    /// \p assetPath was modified. \p resolvedPath is the resolved path
    /// computed for the given \p assetPath. If a timestamp cannot be
    /// retrieved, return an empty VtValue.
    ///
    /// This timestamp may be equality compared to determine if an asset
    /// has been modified.
    AR_API
    VtValue GetModificationTimestamp(
        const std::string& assetPath,
        const ArResolvedPath& resolvedPath);

    /// Returns an ArAsset object for the asset located at \p resolvedPath. 
    /// Returns an invalid std::shared_ptr if object could not be created.
    ///
    /// The returned ArAsset object provides functions for accessing the
    /// contents of the specified asset. 
    AR_API
    std::shared_ptr<ArAsset> OpenAsset(
        const ArResolvedPath& resolvedPath);

    /// Enumeration of write modes for OpenAssetForWrite
    enum class WriteMode
    {
        /// Open asset for in-place updates. If the asset exists, its contents
        /// will not be discarded and writes may overwrite existing data.
        /// Otherwise, the asset will be created.
        Update = 0, 

        /// Open asset for replacement. If the asset exists, its contents will
        /// be discarded by the time the ArWritableAsset is destroyed.
        /// Otherwise, the asset will be created.
        Replace
    };

    /// Returns an ArWritableAsset object for the asset located at \p
    /// resolvedPath using the specified \p writeMode.  Returns an invalid
    /// std::shared_ptr if object could not be created.
    ///
    /// The returned ArWritableAsset object provides functions for writing data
    /// to the specified asset.
    ///
    /// Note that support for reading an asset through other APIs while it
    /// is open for write is implementation-specific. For example, writes to
    /// an asset may or may not be immediately visible to other threads or
    /// processes depending on the implementation.
    AR_API
    std::shared_ptr<ArWritableAsset> OpenAssetForWrite(
        const ArResolvedPath& resolvedPath,
        WriteMode writeMode);

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
    void BeginCacheScope(
        VtValue* cacheScopeData);

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
    void EndCacheScope(
        VtValue* cacheScopeData);

    /// @}

    // --------------------------------------------------------------------- //
    /// \anchor ArResolver_deprecated
    /// \name Deprecated APIs
    ///
    /// The functions in this section are deprecated in Ar 2.0 and slated
    /// for removal. Most have default implementations to allow subclasses
    /// to ignore them completely.
    ///
    /// @{
    // --------------------------------------------------------------------- //

    /// Configures the resolver for a given asset path
    /// Default implementation does nothing.
    /// \deprecated
    AR_API
    virtual void ConfigureResolverForAsset(const std::string& path);

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
    ///
    /// \deprecated Planned for removal in favor of CreateIdentifier.
    AR_API
    virtual std::string AnchorRelativePath(
        const std::string& anchorPath, 
        const std::string& path) = 0; 

    /// Returns true if the given path is a relative path.
    /// \deprecated
    AR_API
    virtual bool IsRelativePath(const std::string& path) = 0;

    /// Returns whether this path is a search path.
    /// The default implementation returns false.
    /// \deprecated
    AR_API
    virtual bool IsSearchPath(const std::string& path);

    /// \deprecated
    /// Returns true if the given path is a repository path.
    AR_API
    bool IsRepositoryPath(const std::string& path);

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
    ///
    /// The default implementation assumes no fetching is required and returns
    /// true.
    ///
    /// \deprecated Planned for removal in favor or using OpenAsset to read
    /// data instead of requiring assets to be fetched to disk.
    AR_API
    virtual bool FetchToLocalResolvedPath(
        const std::string& path,
        const std::string& resolvedPath);

    /// Create path needed to write a file to the given \p path. 
    ///
    /// For example:
    /// - A filesystem-based resolver might create the directories specified
    ///   in \p path.
    /// - A database-based resolver might create a new table, or it might
    ///   ignore this altogether.
    ///
    /// In practice, when writing a layer, CanWriteLayerToPath will be called
    /// first to check if writing is permitted. If this returns true, then
    /// CreatePathForLayer will be called before writing the layer out.
    ///
    /// Returns true on success, false otherwise.
    ///
    /// \deprecated Planned for removal in favor of making OpenAssetForWrite
    /// responsible for creating any intemediate path that might be needed.
    AR_API
    virtual bool CreatePathForLayer(
        const std::string& path) = 0;

    /// Returns true if a file may be written to the given \p path, false
    /// otherwise. 
    ///
    /// In practice, when writing a layer, CanWriteLayerToPath will be called
    /// first to check if writing is permitted. If this returns true, then
    /// CreatePathForLayer will be called before writing the layer out.
    /// 
    /// If this function returns false and \p whyNot is not \c nullptr,
    /// it will be filled in with an explanation.
    ///
    /// The default implementation returns true.
    ///
    /// \deprecated Planned for removal in favor of making OpenAssetForWrite
    /// responsible for determining if a layer can be written to a given path.
    AR_API
    virtual bool CanWriteLayerToPath(
        const std::string& path,
        std::string* whyNot);

    /// Returns true if a new file may be created using the given
    /// \p identifier, false otherwise.
    ///
    /// If this function returns false and \p whyNot is not \c nullptr,
    /// it will be filled in with an explanation.
    ///
    /// The default implementation returns true.
    ///
    /// \deprecated Planned for removal in favor of using ResolveForNewAsset
    /// to determine if a new layer can be created with a given identifier.
    AR_API
    virtual bool CanCreateNewLayerWithIdentifier(
        const std::string& identifier, 
        std::string* whyNot);

    /// @}

protected:
    AR_API
    ArResolver();

    // --------------------------------------------------------------------- //
    /// \anchor ArResolver_implementation
    /// \name Implementation
    ///
    /// @{

    /// Return an identifier for the given \p assetPath. If \p anchorAssetPath
    /// is non-empty, it should be used as the anchoring asset if \p assetPath
    /// is relative.
    ///
    /// Two different (assetPath, anchorAssetPath) inputs should return the
    /// same identifier only if they refer to the same asset. Identifiers may 
    /// be compared to determine given paths refer to the same asset, so
    /// implementations should take care to canonicalize and normalize the
    /// returned identifier to a consistent format.
    virtual std::string _CreateIdentifier(
        const std::string& assetPath,
        const ArResolvedPath& anchorAssetPath) = 0;

    /// Return an identifier for a new asset at the given \p assetPath.  If
    /// \p anchorAssetPath is non-empty, it should be used as the anchoring
    /// asset if \p assetPath is relative.
    ///
    /// This is similar to _CreateIdentifier but is used to create identifiers
    /// for new assets that are being created.
    virtual std::string _CreateIdentifierForNewAsset(
        const std::string& assetPath,
        const ArResolvedPath& anchorAssetPath) = 0;

    /// Return the resolved path for the given \p assetPath or an empty
    /// ArResolvedPath if no asset exists at that path.
    virtual ArResolvedPath _Resolve(
        const std::string& assetPath) = 0;

    /// Return the resolved path for the given \p assetPath that may be used
    /// to create a new asset or an empty ArResolvedPath if such a path cannot
    /// be computed.
    virtual ArResolvedPath _ResolveForNewAsset(
        const std::string& assetPath) = 0;

    /// Bind the given \p context to this resolver. \p bindingData may be
    /// populated with additional information that will be kept alive while
    /// \p context is bound. Both \p context and \p bindingData will be
    /// passed to UnbindContext when the context is being unbound.
    ///
    /// Contexts may be nested; if multiple contexts are bound, the context
    /// that was most recently bound must take precedence and block all
    /// previously bound contexts.
    ///
    /// Context binding is thread-specific; contexts bound in a thread must
    /// only affect other resolver calls in the same thread.
    ///
    /// The default implementation does nothing.
    AR_API
    virtual void _BindContext(
        const ArResolverContext& context,
        VtValue* bindingData);

    /// Unbind the given \p context from this resolver.
    ///
    /// It is an error if the context being unbound is not the currently
    /// bound context.
    ///
    /// The default implementation does nothing.
    AR_API
    virtual void _UnbindContext(
        const ArResolverContext& context,
        VtValue* bindingData);

    /// Return a default ArResolverContext that may be bound to this resolver
    /// to resolve assets when no other context is explicitly specified.
    ///
    /// This function should not automatically bind this context, but should
    /// create one that may be used later.
    ///
    /// The default implementation returns a default-constructed
    /// ArResolverContext.
    AR_API
    virtual ArResolverContext _CreateDefaultContext();

    /// Return an ArResolverContext that may be bound to this resolver
    /// to resolve the asset located at \p assetPath when no other context is
    /// explicitly specified.
    ///
    /// This function should not automatically bind this context, but should
    /// create one that may be used later.
    ///
    /// The default implementation returns a default-constructed
    /// ArResolverContext.
    AR_API
    virtual ArResolverContext _CreateDefaultContextForAsset(
        const std::string& assetPath);

    /// Return an ArResolverContext created from the given \p contextStr.
    ///
    /// The default implementation returns a default-constructed
    /// ArResolverContext.
    AR_API
    virtual ArResolverContext _CreateContextFromString(
        const std::string& contextStr);

    /// Refresh any caches associated with the given context.
    ///
    /// The default implementation does nothing.
    AR_API
    virtual void _RefreshContext(
        const ArResolverContext& context);

    /// Return the currently bound context. Since context binding is 
    /// thread-specific, this should return the context that was most recently
    /// bound in this thread.
    ///
    /// The default implementation returns a default-constructed 
    /// ArResolverContext.
    AR_API
    virtual ArResolverContext _GetCurrentContext();

    /// Return true if the result of resolving the given \p assetPath may
    /// differ depending on the asset resolver context that is bound when
    /// Resolve is called, false otherwise.
    ///
    /// The default implementation returns false.
    AR_API
    virtual bool _IsContextDependentPath(
        const std::string& assetPath);

    /// Return the file extension for the given \p assetPath. This extension
    /// should not include a "." at the beginning of the string.
    virtual std::string _GetExtension(
        const std::string& assetPath) = 0;

    /// Return an ArAssetInfo populated with additional metadata (if any)
    /// about the asset at the given \p assetPath. \p resolvedPath is the
    /// resolved path computed for the given \p assetPath.
    /// The default implementation returns a default-constructed ArAssetInfo.
    AR_API
    virtual ArAssetInfo _GetAssetInfo(
        const std::string& assetPath,
        const ArResolvedPath& resolvedPath);

    /// Return a value representing the last time the asset at the given 
    /// \p assetPath was modified. \p resolvedPath is the resolved path
    /// computed for the given \p assetPath. If a timestamp cannot be
    /// retrieved, return an empty VtValue.
    ///
    /// Implementations may use whatever value is most appropriate
    /// for this timestamp. The value must be equality comparable, 
    /// and this function must return a different timestamp whenever 
    /// an asset has been modified. For instance, if an asset is stored 
    /// as a file on disk, the timestamp may simply be that file's mtime. 
    virtual VtValue _GetModificationTimestamp(
        const std::string& assetPath,
        const ArResolvedPath& resolvedPath) = 0;

    /// Return an ArAsset object for the asset located at \p resolvedPath.
    /// Return an invalid std::shared_ptr if object could not be created
    /// (for example, if the asset at the given path could not be opened).
    ///
    /// Note that clients may still be using the data associated with 
    /// this object even after the last shared_ptr has been destroyed. For 
    /// example, a client may have created a memory mapping using the FILE* 
    /// presented in the ArAsset object; this would preclude truncating or
    /// overwriting any of the contents of that file.
    AR_API
    virtual std::shared_ptr<ArAsset> _OpenAsset(
        const ArResolvedPath& resolvedPath) = 0;

    /// Return an ArWritableAsset object for the asset at \p resolvedPath
    /// using the specified \p writeMode. Return an invalid std::shared_ptr
    /// if object could not be created (for example, if writing to the
    /// given path is not allowed).
    ///
    /// Implementations should create any parent paths that are necessary
    /// to write this asset. The returned ArWritableAsset must obey the
    /// behaviors for the given \p writeMode, see the documentation for
    /// the WriteMode enum for more details.
    AR_API
    virtual std::shared_ptr<ArWritableAsset>
    _OpenAssetForWrite(
        const ArResolvedPath& resolvedPath,
        WriteMode writeMode) = 0;

    /// Mark the start of a resolution caching scope. 
    ///
    /// Resolvers may fill \p cacheScopeData with arbitrary data. Clients may
    /// also pass in a \p cacheScopeData populated by an earlier call to
    /// BeginCacheScope to allow the resolver access to that information.
    AR_API
    virtual void _BeginCacheScope(
        VtValue* cacheScopeData) = 0;

    /// Mark the end of a resolution caching scope.
    ///
    /// \p cacheScopeData should contain the data that was populated by the
    /// previous corresponding call to BeginCacheScope.
    AR_API
    virtual void _EndCacheScope(
        VtValue* cacheScopeData) = 0;

    /// \deprecated
    /// Return true if the given path is a repository path, false otherwise.
    /// Default implementation returns false.
    AR_API
    virtual bool _IsRepositoryPath(
        const std::string& path);

    /// @}
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

#endif
