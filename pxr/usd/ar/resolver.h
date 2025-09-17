//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_AR_RESOLVER_H
#define PXR_USD_AR_RESOLVER_H

/// \file ar/resolver.h

#include "pxr/pxr.h"
#include "pxr/usd/ar/api.h"
#include "pxr/usd/ar/ar.h"
#include "pxr/usd/ar/resolvedPath.h"
#include "pxr/usd/ar/resolverContext.h"
#include "pxr/usd/ar/timestamp.h"

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
        const ArResolvedPath& anchorAssetPath = ArResolvedPath()) const;

    /// Returns an identifier for a new asset specified by \p assetPath.
    /// If \p anchorAssetPath is not empty, it is the resolved asset path
    /// that \p assetPath should be anchored to if it is a relative path.
    AR_API
    std::string CreateIdentifierForNewAsset(
        const std::string& assetPath,
        const ArResolvedPath& anchorAssetPath = ArResolvedPath()) const;

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
        const std::string& assetPath) const;

    /// Returns the resolved path for the given \p assetPath that may be used
    /// to create a new asset. If such a path cannot be computed for
    /// \p assetPath, returns an empty ArResolvedPath.
    ///
    /// Note that an asset might or might not already exist at the returned
    /// resolved path.
    AR_API
    ArResolvedPath ResolveForNewAsset(
        const std::string& assetPath) const;

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
    ///
    /// The returned ArResolverContext will contain the default context 
    /// returned by the primary resolver and all URI/IRI resolvers.
    AR_API
    ArResolverContext CreateDefaultContext() const;

    /// Return an ArResolverContext that may be bound to this resolver
    /// to resolve the asset located at \p assetPath or referenced by
    /// that asset when no other context is explicitly specified.
    ///
    /// The returned ArResolverContext will contain the default context 
    /// for \p assetPath returned by the primary resolver and all URI/IRI
    /// resolvers.
    AR_API
    ArResolverContext CreateDefaultContextForAsset(
        const std::string& assetPath) const;

    /// Return an ArResolverContext created from the primary ArResolver
    /// implementation using the given \p contextStr.
    AR_API
    ArResolverContext CreateContextFromString(
        const std::string& contextStr) const;

    /// Return an ArResolverContext created from the ArResolver registered
    /// for the given \p uriScheme using the given \p contextStr.
    ///
    /// An empty \p uriScheme indicates the primary resolver and is
    /// equivalent to CreateContextFromString(string).
    ///
    /// If no resolver is registered for \p uriScheme, returns an empty
    /// ArResolverContext.
    ///
    /// \note 'uriScheme' can be used to register IRI resolvers
    AR_API
    ArResolverContext CreateContextFromString(
        const std::string& uriScheme, const std::string& contextStr) const;

    /// Return an ArResolverContext created by combining the ArResolverContext
    /// objects created from the given \p contextStrs.
    ///
    /// \p contextStrs is a list of pairs of strings. The first element in the
    /// pair is the URI/IRI scheme for the ArResolver that will be used to
    /// create the ArResolverContext from the second element in the pair. An
    /// empty resource identifier scheme indicates the primary resolver.
    ///
    /// For example:
    ///
    /// \code
    /// ArResolverContext ctx = ArGetResolver().CreateContextFromStrings(
    ///    { {"", "context str 1"}, 
    ///      {"my-scheme", "context str 2"} });
    /// \endcode
    /// 
    /// This will use the primary resolver to create an ArResolverContext
    /// using the string "context str 1" and use the resolver registered for
    /// the "my-scheme" URI/IRI scheme to create an ArResolverContext using
    /// "context str 2". These contexts will be combined into a single
    /// ArResolverContext and returned.
    ///
    /// If no resolver is registered for a URI/IRI scheme in an entry in
    /// \p contextStrs, that entry will be ignored.
    AR_API
    ArResolverContext CreateContextFromStrings(
        const std::vector<
            std::pair<std::string, std::string>>& contextStrs) const;

    /// Refresh any caches associated with the given context. If doing so
    /// would invalidate asset paths that had previously been resolved,
    /// an ArNotice::ResolverChanged notice will be sent to inform clients
    /// of this.
    ///
    /// Avoid calling RefreshContext() on the same context from more than one 
    /// thread concurrently as ArNotice::ResolverChanged notice listeners may 
    /// mutate their state in response to receiving the notice.
    ///
    /// Avoid calling RefreshContext() with a context that is active (bound to 
    /// a resolver). Unbind the context before refreshing it.
    ///
    /// \see \ref Usd_Page_MultiThreading "Threading Model and Performance Considerations"
    AR_API
    void RefreshContext(
        const ArResolverContext& context);

    /// Returns the asset resolver context currently bound in this thread.
    ///
    /// \see ArResolver::BindContext, ArResolver::UnbindContext
    AR_API
    ArResolverContext GetCurrentContext() const;

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
        const std::string& assetPath) const;

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
        const std::string& assetPath) const;

    /// Returns an ArAssetInfo populated with additional metadata (if any)
    /// about the asset at the given \p assetPath. \p resolvedPath is the
    /// resolved path computed for the given \p assetPath.
    AR_API
    ArAssetInfo GetAssetInfo(
        const std::string& assetPath,
        const ArResolvedPath& resolvedPath) const;

    /// Returns an ArTimestamp representing the last time the asset at
    /// \p assetPath was modified. \p resolvedPath is the resolved path
    /// computed for the given \p assetPath. If a timestamp cannot be
    /// retrieved, return an invalid ArTimestamp.
    AR_API
    ArTimestamp GetModificationTimestamp(
        const std::string& assetPath,
        const ArResolvedPath& resolvedPath) const;

    /// Returns an ArAsset object for the asset located at \p resolvedPath. 
    /// Returns an invalid std::shared_ptr if object could not be created.
    ///
    /// The returned ArAsset object provides functions for accessing the
    /// contents of the specified asset. 
    AR_API
    std::shared_ptr<ArAsset> OpenAsset(
        const ArResolvedPath& resolvedPath) const;

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
        WriteMode writeMode) const;

    /// Returns true if an asset may be written to the given \p resolvedPath,
    /// false otherwise. If this function returns false and \p whyNot is not
    /// \c nullptr, it may be filled with an explanation.
    AR_API
    bool CanWriteAssetToPath(
        const ArResolvedPath& resolvedPath,
        std::string* whyNot = nullptr) const;

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

    /// \deprecated
    /// Returns true if the given path is a repository path.
    AR_API
    bool IsRepositoryPath(const std::string& path) const;

    /// @}

protected:
    AR_API
    ArResolver();

    // --------------------------------------------------------------------- //
    /// \anchor ArResolver_implementation
    /// \name Implementation
    ///
    /// @{

    /// Return an identifier for the asset at the given \p assetPath. 
    /// See \ref ArResolver_identifier "Identifiers" for more information.
    ///
    /// If \p anchorAssetPath is non-empty, it should be used as the anchoring 
    /// asset if \p assetPath is relative. For example, for a filesystem-based
    /// implementation _CreateIdentifier might return:
    ///
    /// _CreateIdentifier(
    ///     /* assetPath = */ "/abs/path/to/model.usd",
    ///     /* anchorAssetPath = */ ArResolvedPath("/abs/path/to/shot.usd"))
    ///      => "/abs/path/to/model.usd"
    ///
    /// _CreateIdentifier(
    ///     /* assetPath = */ "relative/model.usd",
    ///     /* anchorAssetPath = */ ArResolvedPath("/abs/path/to/shot.usd"))
    ///      => "/abs/path/to/relative/model.usd"
    ///
    /// Identifiers may be compared to determine if given paths refer to the
    /// same asset, so implementations should take care to canonicalize and
    /// normalize the returned identifier to a consistent format.
    ///
    /// If either \p assetPath or \p anchorAssetPath have a URI/IRI scheme,
    /// this function will be called on the resolver associated with that
    /// URI/IRI scheme, if any.
    ///
    /// Example uses:
    /// - When opening a layer via SdfLayer::FindOrOpen or Find,
    ///   CreateIdentifier will be called with the asset path given to those
    ///   functions and no anchoring asset path.  The result will be used as the
    ///   layer's identifier.
    ///
    /// - When processing composition arcs that refer to other layers, this
    ///   function will be called with the asset path of the referenced layer
    ///   and the resolved path of the layer where the composition arc was
    ///   authored. The result will be passed to SdfLayer::FindOrOpen to
    ///   open the referenced layer.
    virtual std::string _CreateIdentifier(
        const std::string& assetPath,
        const ArResolvedPath& anchorAssetPath) const = 0;

    /// Return an identifier for a new asset at the given \p assetPath.
    ///
    /// This is similar to _CreateIdentifier but is used to create identifiers
    /// for assets that may not exist yet and are being created.
    ///
    /// Example uses:
    /// - When creating a new layer via SdfLayer::CreateNew,
    ///   CreateIdentifierForNewAsset will be called with the asset path given
    ///   to the function. The result will be used as the new layer's
    ///   identifier.
    virtual std::string _CreateIdentifierForNewAsset(
        const std::string& assetPath,
        const ArResolvedPath& anchorAssetPath) const = 0;

    /// Return the resolved path for the given \p assetPath or an empty
    /// ArResolvedPath if no asset exists at that path.
    virtual ArResolvedPath _Resolve(
        const std::string& assetPath) const = 0;

    /// Return the resolved path for the given \p assetPath that may be used
    /// to create a new asset or an empty ArResolvedPath if such a path cannot
    /// be computed.
    virtual ArResolvedPath _ResolveForNewAsset(
        const std::string& assetPath) const = 0;

    /// @}

    // --------------------------------------------------------------------- //
    /// \anchor ArResolver_contextImplementation
    /// \name Context Operations Implementation
    ///
    /// If any of these functions are implemented in a subclass, the plugin
    /// metadata for that subclass in the plugin library's plugInfo.json 
    /// must specify:
    /// \code
    /// "implementsContexts" : true.
    /// \endcode
    ///
    /// If a subclass indicates that it implements any of these functions,
    /// its plugin library will be loaded and these functions will be called
    /// when the corresponding public ArResolver API is called. Otherwise,
    /// these functions will not be called.
    ///
    /// @{

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
    /// ArResolver itself manages thread-local stacks of bound contexts.
    /// Subclasses can retrieve the most recent context object which was passed
    /// to BindContext using _GetCurrentContextObject. Because of this,
    /// subclasses typically do not need to implement this function unless they
    /// need to be informed when a context object is bound. For example, this
    /// may be needed if the context needs to be passed on to another subsystem
    /// that manages these bindings itself.
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
    /// Subclasses typically do not need to implement this function since
    /// ArResolver itself keeps track of the contexts that are bound via calls
    /// to BindContext. However, subclasses may need to implement this function
    /// if they are managing these bindings itself.
    ///
    /// The default implementation does nothing.
    AR_API
    virtual void _UnbindContext(
        const ArResolverContext& context,
        VtValue* bindingData);

    /// Return a default ArResolverContext that may be bound to this resolver
    /// to resolve assets when no other context is explicitly specified.
    ///
    /// When CreateDefaultContext is called on the configured asset resolver,
    /// Ar will call this method on the primary resolver and all URI/IRI
    /// resolvers and merge the results into a single ArResolverContext that
    /// will be returned to the consumer.
    ///
    /// This function should not automatically bind this context, but should
    /// create one that may be used later.
    ///
    /// The default implementation returns a default-constructed
    /// ArResolverContext.
    ///
    /// Example uses: 
    /// - UsdStage will call CreateDefaultContext when creating a new stage with
    ///   an anonymous root layer and without a given context. The returned
    ///   context will be bound when resolving asset paths on that stage.
    AR_API
    virtual ArResolverContext _CreateDefaultContext() const;

    /// Return an ArResolverContext that may be bound to this resolver
    /// to resolve the asset located at \p assetPath or referenced by
    /// that asset when no other context is explicitly specified.
    ///
    /// When CreateDefaultContextForAsset is called on the configured asset
    /// resolver, Ar will call this method on the primary resolver and all
    /// URI/IRI resolvers and merge the results into a single ArResolverContext
    /// that will be returned to the consumer.
    ///
    /// Note that this means this method may be called with asset paths that
    /// are not associated with this resolver. For example, this method may
    /// be called on a URI/IRI resolver with a non-URI/IRI asset path. This is
    /// to support cases where the asset at \p assetPath references other
    /// assets with URI/IRI schemes that differ from the URI/IRI scheme
    /// (if any) in \p assetPath.
    ///
    /// This function should not automatically bind this context, but should
    /// create one that may be used later.
    ///
    /// The default implementation returns a default-constructed
    /// ArResolverContext.
    ///
    /// Example uses: 
    /// - UsdStage will call CreateDefaultContextForAsset when creating a new
    ///   stage with a non-anonymous root layer and without a given context. The
    ///   resolved path of the root layer will be passed in as the
    ///   \p assetPath. The returned context will be bound when resolving asset
    ///   paths on that stage.
    AR_API
    virtual ArResolverContext _CreateDefaultContextForAsset(
        const std::string& assetPath) const;

    /// Return an ArResolverContext created from the given \p contextStr.
    ///
    /// The default implementation returns a default-constructed
    /// ArResolverContext.
    AR_API
    virtual ArResolverContext _CreateContextFromString(
        const std::string& contextStr) const;

    /// Refresh any caches associated with the given context. If doing so
    /// would invalidate asset paths that had previously been resolved,
    /// this function should send an ArNotice::ResolverChanged notice to
    /// inform clients of this. See documentation on that class for more
    /// details.
    ///
    /// The default implementation does nothing.
    AR_API
    virtual void _RefreshContext(
        const ArResolverContext& context);

    /// Return the currently bound context. Since context binding is 
    /// thread-specific, this should return the context that was most recently
    /// bound in this thread.
    ///
    /// Subclasses typically do not need to implement this function since
    /// ArResolver itself keeps track of the contexts that are bound via calls
    /// to BindContext. However, if a subclass is managing bound contexts itself
    /// and allows clients to bind context objects via other API outside of
    /// BindContext, this function should return the context object as described
    /// above. This typically happens with subclasses that are wrappers around
    /// other resolution subsystems. \see _BindContext for more information.
    ///
    /// The default implementation returns a default-constructed 
    /// ArResolverContext.
    AR_API
    virtual ArResolverContext _GetCurrentContext() const;

    /// Return true if the result of resolving the given \p assetPath may
    /// differ depending on the asset resolver context that is bound when
    /// Resolve is called, false otherwise.
    ///
    /// The default implementation returns false.
    ///
    /// Example uses:
    /// - SdfLayer will call this function to check if the identifier given
    ///   to SdfLayer::Find or SdfLayer::FindOrOpen is context-dependent.
    ///   If it is and a layer exists with the same identifier, SdfLayer
    ///   can return it without resolving the identifier. If it is not,
    ///   SdfLayer must resolve the identifier and search for a layer with
    ///   the same resolved path, even if a layer exists with the same
    ///   identifier.
    AR_API
    virtual bool _IsContextDependentPath(
        const std::string& assetPath) const;

    /// @}

    // --------------------------------------------------------------------- //
    /// \anchor ArResolver_assetImplementation
    /// \name Asset Operations Implementation
    ///
    /// @{

    /// Return the file extension for the given \p assetPath. This extension
    /// should not include a "." at the beginning of the string.
    ///
    /// The default implementation returns the string after the last "."
    /// in \p assetPath. If \p assetPath begins with a ".", the extension
    /// will be empty unless there is another "." in the path. If 
    /// \p assetPath has components separated by '/' (or '\' on Windows),
    /// only the last component will be considered.
    AR_API
    virtual std::string _GetExtension(
        const std::string& assetPath) const;

    /// Return an ArAssetInfo populated with additional metadata (if any)
    /// about the asset at the given \p assetPath. \p resolvedPath is the
    /// resolved path computed for the given \p assetPath.
    /// The default implementation returns a default-constructed ArAssetInfo.
    AR_API
    virtual ArAssetInfo _GetAssetInfo(
        const std::string& assetPath,
        const ArResolvedPath& resolvedPath) const;

    /// Return an ArTimestamp representing the last time the asset at
    /// \p assetPath was modified. \p resolvedPath is the resolved path
    /// computed for the given \p assetPath. If a timestamp cannot be
    /// retrieved, return an invalid ArTimestamp.
    ///
    /// The default implementation returns an invalid ArTimestamp.
    ///
    /// Example uses:
    /// - SdfLayer will call GetModificationTimestamp when opening a
    ///   layer and store the returned timestamp. When SdfLayer::Reload
    ///   is called on that layer, this method will be called again.
    ///   If the returned timestamp differs from the stored timestamp,
    ///   or if it is invalid, the layer will be reloaded.
    AR_API
    virtual ArTimestamp _GetModificationTimestamp(
        const std::string& assetPath,
        const ArResolvedPath& resolvedPath) const;

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
        const ArResolvedPath& resolvedPath) const = 0;

    /// Return true if an asset may be written to the given \p resolvedPath,
    /// false otherwise. If this function returns false and \p whyNot is not
    /// \c nullptr, it may be filled with an explanation.  The default
    /// implementation returns true.
    AR_API
    virtual bool _CanWriteAssetToPath(
        const ArResolvedPath& resolvedPath,
        std::string* whyNot) const;

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
        WriteMode writeMode) const = 0;

    /// @}

    // --------------------------------------------------------------------- //
    /// \anchor ArResolver_scopedCacheImplementation
    /// \name Scoped Resolution Cache Implementation
    ///
    /// If any of these functions are implemented in a subclass, the plugin
    /// metadata for that subclass in the plugin library's plugInfo.json 
    /// must specify:
    /// \code
    /// "implementsScopedCaches" : true.
    /// \endcode
    ///
    /// If a subclass indicates that it implements these functions, ArResolver
    /// will assume the subclass is handling all caching of resolved paths
    /// and will call these functions when a caching scope is opened and closed.
    /// Otherwise, these functions will not be called. Instead, ArResolver
    /// itself will handle caching and returning resolved paths as needed.
    ///
    /// @{

    /// Mark the start of a resolution caching scope. 
    ///
    /// Resolvers may fill \p cacheScopeData with arbitrary data. Clients may
    /// also pass in a \p cacheScopeData populated by an earlier call to
    /// BeginCacheScope to allow the resolver access to that information.
    ///
    /// See \ref ArResolver_scopedCacheImplementation "Scoped Resolution Cache Implementation" 
    /// for more implementation details.
    AR_API
    virtual void _BeginCacheScope(
        VtValue* cacheScopeData);

    /// Mark the end of a resolution caching scope.
    ///
    /// \p cacheScopeData should contain the data that was populated by the
    /// previous corresponding call to BeginCacheScope.
    ///
    /// See \ref ArResolver_scopedCacheImplementation "Scoped Resolution Cache Implementation" 
    /// for more implementation details.
    AR_API
    virtual void _EndCacheScope(
        VtValue* cacheScopeData);

    /// @}

    /// \deprecated
    /// Return true if the given path is a repository path, false otherwise.
    /// Default implementation returns false.
    AR_API
    virtual bool _IsRepositoryPath(
        const std::string& path) const;

    // --------------------------------------------------------------------- //
    /// \anchor ArResolver_implementationUtils
    /// \name Implementation Utilities
    ///
    /// Utility functions for implementations.
    ///
    /// @{

    /// Returns a pointer to the context object of type \p ContextObj from
    /// the last ArResolverContext that was bound via a call to BindContext,
    /// or \c NULL if no context object of that type exists.
    ///
    /// Typically, a subclass might use this in their _Resolve function to
    /// get the currently bound context to drive their resolution behavior.
    ///
    /// \code
    /// if (const MyContextObject* ctx = 
    ///        _GetCurrentContextObject<MyContextObject>()) {
    ///
    ///     // Use information in ctx to resolve given path
    /// }
    /// else {
    ///     // Resolve given path with no context object
    /// }
    /// \endcode
    ///
    /// This is the same as GetCurrentContext().Get<ContextObj>() but more
    /// efficient, since it does not make a copy of the ArResolverContext.
    /// However, it is *not* the same as _GetCurrentContext().Get<ContextObj>().
    /// Subclasses that manage context binding themselves may have overridden
    /// _GetCurrentContext to return a context that was bound without calling
    /// BindContext. These subclasses should not use this function and should
    /// retrieve the current context from their own internal data structures.
    template <class ContextObj>
    const ContextObj* _GetCurrentContextObject() const
    {
        const ArResolverContext* ctx = _GetInternallyManagedCurrentContext();
        return ctx ? ctx->Get<ContextObj>() : nullptr;
    }

    /// @}

private:
    // Returns pointer to ArResolverContext that was most recently bound
    // via BindContext. This is *not* the same as GetCurrentContext,
    // since subclasses may return an ArResolverContext that hasn't
    // been bound via BindContext in their implementations.
    AR_API
    const ArResolverContext* _GetInternallyManagedCurrentContext() const;

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

/// Returns list of all URI schemes for which a resolver has been
/// registered. Schemes are returned in all lower-case and in alphabetically
/// sorted order.
AR_API
const std::vector<std::string>& ArGetRegisteredURISchemes();

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
