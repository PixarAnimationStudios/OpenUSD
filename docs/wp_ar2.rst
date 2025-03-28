=========================
Asset Resolution (Ar) 2.0
=========================

.. include:: rolesAndUtils.rst
.. include:: <isonum.txt>

.. note::
   This proposal has been implemented. This document exists for historical
   reference and may be outdated. For up-to-date documentation, see the
   `Ar overview page <api/ar_page_front.html>`_.

Copyright |copy| 2020, Pixar Animation Studios,  *version 1.0*

.. contents:: :local:

Background and Goals
####################

The Ar 2.0 project is an effort to revise the Ar (Asset Resolution) library used
by USD to interface with a user's asset management and storage infrastructure.

The initial version of Ar was implemented prior to USD's open source release
in 2016. The primary focus at the time was to extract Pixar-specific
implementation details from the USD codebase in preparation for USD's public
release. Although Ar was intended to be a general-purpose interface, it was not
entirely successful in that regard. For example, Ar was geared towards assets
stored on a traditional filesystem, and parts of USD still assume that asset
paths are filesystem paths that can be manipulated as such. Also, some
Pixar-specific concepts like repository paths and search paths are still present
in the interface.

Ar 2.0 aims to address these issues, as well as incorporate feedback and add
support for new use cases that have developed over the last few years.

Tasks
#####

General Cleanup
***************

The :cpp:`ArResolver` interface will be reimplemented using the non-virtual
interface (NVI) pattern (as described `here
<http://www.gotw.ca/publications/mill18.htm>`__) to provide more flexibility and
potentially allow future changes without breaking the public interface.

In the sections below, we will be presenting the proposed new public API for
:cpp:`ArResolver`. See the proposed headers for details on the virtual functions
used to override behavior.

The API on :cpp:`ArResolver` will all be marked const to aid with writing
const-correct code and as an indicator that these functions may be called
concurrently and must be thread-safe (following the semantics used by the
Standard Library).

Add Documentation and Examples
******************************

More extensive documentation and example resolver implementations will be
provided as part of this work.

Add Identifier Concept
**********************

The current :cpp:`ArResolver` interface contains a number of functions that are
used primarily in :mono:`Sdf` to compute identifiers for :cpp:`SdfLayer` objects
given an asset path or an (anchoring asset path, relative asset path) pair,
e.g., for calls to :cpp:`SdfLayer::FindOrOpen` or :cpp:`CreateNew`. These
functions lack clear documentation and introduce confusing concepts, some of
which are remnants of Pixar-specific implementation details. To simplify the
interface, these functions will be removed:

.. code-block:: cpp
   :caption: Removed

   ArResolver::AnchorRelativePath
   ArResolver::IsRelativePath
   ArResolver::IsRepositoryPath
   ArResolver::IsSearchPath
   ArResolver::ComputeNormalizedPath
   ArResolver::ComputeRepositoryPath

and replaced with:

.. code-block:: cpp
   :caption: Added

   ArIdentifier
   ArResolver::CreateIdentifier(
       const std::string& assetPath) const;
   
   ArIdentifier
   ArResolver::CreateIdentifier(
       const ArIdentifier& anchorIdentifier,
       const std::string& assetPath) const;
   
   ArIdentifier
   ArResolver::CreateIdentifierForNewAsset(
       const std::string& assetPath) const;
   
   ArIdentifier
   ArResolver::CreateIdentifierForNewAsset(
       const ArIdentifier& anchorIdentifier,
       const std::string& assetPath) const;

These functions are responsible for taking an asset path and computing an
:cpp:`ArIdentifier` that refers to a logical asset. An :cpp:`ArIdentifier` is a
simple wrapper around a :cpp:`std::string` to allow clients to more clearly
indicate intent, as opposed to using bare :cpp:`std::string` objects everywhere.

This identifier has consequences for operations like :cpp:`SdfLayer::Reload,`
which re-resolve the identifier to see if an update is necessary. In the
simplest cases, the identifier may just be the same as the input asset path, but
for more complex resolution behaviors determining the identifier may be more
involved.

For example, the :cpp:`ArDefaultResolver` implementation included with USD
supports normal filesystem paths, like :file:`/Dir/File.usd` or
:file:`./Dir/File.usd`. In these cases, the corresponding identifier would
simply be the normalized, absolutized (anchored either to the current working
directory or the supplied anchor path) forms of these paths. So, calling
:cpp:`SdfLayer::FindOrOpen` with these paths would return SdfLayer objects with
identifiers :file:`/Dir/File.usd` and :file:`/current/working/dir/Dir/File.usd`.

:cpp:`ArDefaultResolver` also supports "search paths" that look like
:file:`Dir/File.usd`. For these paths, :cpp:`ArDefaultResolver` searches an
:ordered list of directories specified by the :cpp:`ArDefaultResolverContext` by
:prepending those directories to the path and returning the first one that
:exists on disk. For these search paths, the corresponding identifier is the
:search path itself. So, calling :cpp:`SdfLayer::FindOrOpen` with the above path
:would return an SdfLayer with identifier :file:`Dir/File.usd`. Calling
::cpp:`SdfLayer::Reload` on this layer would cause the identifier to be resolved
:again, but since the identifier was kept in search path form
::cpp:`ArDefaultResolver` knows to re-run the search to see if the asset had
:been installed in a new directory in the search order.

The :cpp:`CreateIdentifierForNewAsset` functions are separated from
:cpp:`CreateIdentifier` to support existing functionality where calling
:cpp:`SdfLayer::CreateNew` with a particular asset path may return an
:cpp:`SdfLayer` with a different identifier than calling
:cpp:`SdfLayer::FindOrOpen` with the same asset path. This distinction will show
up again in the :cpp:`Resolve` functions described in the next section.

For example, when using :cpp:`ArDefaultResolver` calling
:cpp:`SdfLayer::CreateNew` will always anchor the given path to the current
working directory. Calling :cpp:`SdfLayer::CreateNew` with a search path like
:file:`Dir/File.usd` will always result in an :cpp:`SdfLayer` with identifier
:file:`/current/working/dir/Dir/File.usd` and create the new layer at that
path. Calling :cpp:`SdfLayer::FindOrOpen` with that same search path would
instead return an :cpp:`SdfLayer` with identifier :file:`Dir/File.usd` as
described above.

Remove Repository and Search Path
*********************************

The addition of the identifier concept above allows us to remove the repository
path and search path concepts from Ar. Aside from the functions mentioned above,
the following will also be removed from Ar and related libraries:

.. code-block:: cp
   :caption: Removed

   ArAssetInfo::repoPath
   SdfLayer::GetRepositoryPath

Repository paths are currently used as an additional index in the
:cpp:`SdfLayer` registry. This will also be removed as part of this work.

Improve Resolve and Asset Info
******************************

The Resolve functions on :cpp:`ArResolver` will be modified to:

.. code-block:: cpp
   :caption: Changed

   ArResolvedPath Resolve(
       const ArIdentifier& identifier,
       ArAssetInfo* assetInfo = nullptr) const;
   
   ArResolvedPath ResolveForNewAsset(
       const ArIdentifier& identifier,
       ArAssetInfo* assetInfo = nullptr) const;

Similar to :cpp:`ArIdentifier,` an :cpp:`ArResolvedPath` is a simple wrapper
around a :cpp:`std::string` that makes it clear to clients when they are
interacting with a resolved vs. unresolved path. This will help resolve
confusion around whether other :cpp:`ArResolver` APIs are expected to handle
resolved or unresolved paths -- the APIs that require a resolved path will be
updated to take these objects as parameters instead of a bare
:cpp:`std::string`.

We considered the idea of allowing resolvers to store arbitrary blind data in an
:cpp:`ArResolvedPath.` This would allow resolvers to store additional
information during asset resolution that could be used later when calling other
:cpp:`ArResolver` functions, like :cpp:`OpenAsset.` However, there were concerns
that clients would fill the blind data with information *required* to consume
the resolved path in these APIs. This would potentially break the ability to
replace an authored asset path in a USD layer with its resolved path to 'bake
in' the asset resolution for workflows like asset isolation/freezing. These
concerns led to this idea being dropped, at least for now.

Like :cpp:`CreateIdentifierForNewAsset,` :cpp:`ResolveForNewAsset` is required
primarily for :cpp:`SdfLayer::CreateNew` to resolve the given asset path to a
location where a new layer may be written. :cpp:`Resolve` cannot be used for
this purpose, since it is expected to return an empty resolved path if no asset
exists at a given location. :cpp:`ResolveForNewAsset` is a replacement for
:cpp:`ArResolver::ComputeLocalPath` and will allow us to remove a number of
other related functions for layer creation as well:

.. code-block:: cpp
   :caption: Removed

   ArResolver::ComputeLocalPath
   ArResolver::CanCreateNewLayerWithIdentifier
   ArResolver::CanWriteLayerToPath

Both :cpp:`Resolve` and :cpp:`ResolveForNewAsset` will accept an optional
:cpp:`ArAssetInfo` to populate during resolution. In the old :cpp:`ArResolver`
interface, this was done via a separate :cpp:`ResolveWithAssetInfo` function to
avoid adding a default parameter to a public virtual function. Using the public
NVI pattern avoids this issue, allowing us to remove :cpp:`ResolveWithAssetInfo`
and reduce the size of the C++ API. (Note, however, that
:cpp:`ResolveWithAssetInfo` and :cpp:`ResolveForNewAssetWithAssetInfo` will
still exist in the Python API since there is no concept of an optional output
parameter in Python.) These functions will be the only ones expected to populate
an :cpp:`ArAssetInfo;` the :cpp:`UpdateAssetInfo` function will be removed.

.. code-block:: cpp
   :caption: Removed

   ArResolver::ResolveWithAssetInfo
   ArResolver::UpdateAssetInfo

Remove Filesystem-specific Code
*******************************

Remnants of Ar's filesystem-centric origins still exist in the ArResolver
interface and various parts of the USD codebase. A large part of the Ar 2.0
project will be finding and replacing these remnants with Ar-based abstractions.

The :cpp:`ArResolver::FetchToLocalResolvedPath` function was an early attempt at
supporting asset systems that did not store their data directly on disk. The
idea was that USD would call this function to save assets from remote data
stores onto the local filesystem. This was necessary because, at the time, USD
only supported reading data from files on disk. Since then, much of the codebase
has been updated to use the :cpp:`ArAsset` API, which provides a more general
interface for reading data from any backing store and does not require assets to
be saved locally. So, these functions will be removed:

.. code-block:: cpp
   :caption: Removed

   ArResolver::FetchToLocalResolvedPath
   SdfFileFormat::LayersAreFileBased

The primary downside of this removal is that it potentially makes interacting
with 3rd-party libraries that require a file on disk more difficult. For
example, the Alembic library requires a file path to open an Alembic archive. To
support non-filesystem asset stores, the usdAbc plugin could have used
FetchToLocalResolvedPath to save a :filename:`.abc` file to the local disk and
passed that file path to the Alembic API. This would give the :cpp:`ArResolver`
implementation control over how and where the .abc file would be saved. Without
this function, an alternate approach would be needed. One possibility is that
the usdAbc plugin could use the :cpp:`ArAsset` API to open the .abc file from
its data store and then save the file to a temporary location. But, that would
move control of the localization process to the usdAbc plugin itself.

Add Asset Writing Interface
***************************

The :cpp:`ArAsset` API allows USD to read data from non-filesystem locations and
most of the USD codebase that reads data from assets has been updated to take
advantage of this. However, parts of USD that write data out (for example, the
:filename:`.usd/.usda/.usdc` file formats) still use filesystem-specific API to
do so. To fully separate USD from filesystems, an API for writing data will be
added to :cpp:`ArResolver:`

.. code-block:: cpp
   :caption: Added

   std::shared_ptr<ArWritableAsset>
   ArResolver::OpenAssetForUpdate(
       const ArResolvedPath& resolvedPath) const;
   
   std::shared_ptr<ArWritableAsset>
   ArResolver::OpenAssetForReplace(
       const ArResolvedPath& resolvedPath) const;
   
   /// \class ArWritableAsset
   /// Interface for writing to an asset.
   class ArWritableAsset
   {
   public:
       virtual size_t Write(const void* buffer, size_t count, size_t offset) = 0;
   };

These functions will return instances of a new :cpp:`ArWritableAsset` interface
that provide an interface for writing bytes to some destination. This interface
has been kept as minimal as possible to make it easier to implement. However,
resolvers are not required to implement these functions and may return a NULL
pointer if they do not support writes.

The USD codebase (particularly the file formats that ship with USD) will be
modified to use this new API for writing data. This will allow us to remove
:cpp:`ArResolver::CreatePathForLayer`.

.. code-block:: cpp
   :caption: Removed

   ArResolver::CreatePathForLayer

Add URI Resolvers
*****************

Ar 2.0 will add native support for multiple resolver implementations based on
URI scheme. This will allow users to easily plug in support for multiple asset
systems and forms of asset paths without needing to modify the existing resolver
implementation. Ar will still require a "primary" resolver that will handle
asset paths without an associated URI or an unhandled URI.

To implement a URI resolver, users would implement an :cpp:`ArResolver` subclass
and associate it with a given URI scheme or schemes in that subclass' entry in
the corresponding :filename:`plugInfo.json` file. At runtime, Ar would parse
given asset paths to determine their URI scheme and dispatch it to the
associated URI resolver. If no URI scheme is present in the asset path or no URI
resolver for the scheme is found, Ar will dispatch the asset path to the primary
resolver that is used today.

For example, a user might implement an :cpp:`HTTPResolver` that handles
:mono:`http:` and :mono:`https:` asset paths like:

.. code-block:: cpp
   :caption: URI Resolver Example

   httpResolver.h/.cpp:
   
   /// \class HTTPResolver
   /// ArResolver implementation handling http and https asset paths.
   class HTTPResolver : public ArResolver
   {
       // ... implementation!
   };
   
   plugInfo.json:
   {
       "Plugins": [
           "Info": {
               "Types": {
                   "HTTPResolver" : {
                       "bases": ["ArResolver"],
                       "uriSchemes": ["http", "https"]
                   }
               }
           },
           ...
        ],
        ...
   }

At runtime, the :cpp:`HTTPResolver` would be registered alongside the default
:cpp:`ArDefaultResolver.` Ar would handle dispatching to the appropriate
resolver automatically, so:

.. code-block:: cpp

   ArGetResolver().Resolve("https://...") => HTTPResolver
   ArGetResolver().Resolve("/foo/bar/...") => ArDefaultResolver

:cpp:`ArResolverContext` is a container used to supply additional data to
resolve implementations for use during asset resolution. Support for multiple
resolvers will be added so that context objects for different resolvers can be
held in a single :cpp:`ArResolverContext` object and passed through to
:cpp:`UsdStage` and other consumers as they are today. For example,
:cpp:`ArDefaultResolver` has an associated :cpp:`ArDefaultResolverContext` that
allows clients to specify directories to search when resolving a search
path. The :cpp:`HTTPResolver` might have a context object to allow users to
specify credentials for retrieving assets from a web server:

.. code-block:: cpp

   httpResolverContext.h/.cpp:
   
   /// \class HTTPResolverContext
   /// Context object for use with ArResolverContext that provides extra
   /// information when resolving http and https asset paths.
   class HTTPResolverContext
   {
       HTTPResolverContext(const std::string& username, const std::string& password);
       // ...
   };

A client could create a :cpp:`UsdStage` using these context objects like:

.. code-block:: cpp

   ArResolverContext ctx({
       HTTPResolverContext("user", "12345"),
       ArDefaultResolverContext({"/search/dir/1", "/search/dir/2"})
   });
   
   UsdStage newStage = UsdStage::Open(..., ctx);

Note that binding an :cpp:`ArResolverContext` via :cpp:`ArResolverContextBinder`
will override any :cpp:`ArResolverContext` that had previously been bound. This
behavior ensures consistency, for example:

.. code-block:: cpp

   ArResolverContext ctx_1({
       ArDefaultResolverContext({"/search/dir/3", "/search/dir/4"})
   });
   
   ArResolverContext ctx_2({
       HTTPResolverContext("user", "12345"),
       ArDefaultResolverContext({"/search/dir/1", "/search/dir/2"})
   });
   
   ArResolverContextBinder binder_2(ctx_2);
   
   // Create a new UsdStage that will bind ctx_1 when resolving asset paths.
   // In particular, this means that the HTTPResolverContext specified in
   // ctx_2 should not be used when resolving asset paths on this stage,
   // even though no HTTPResolverContext object was specified in ctx_1.
   UsdStage newStage = UsdStage::Open(..., ctx_1);

Allow Creation of ArResolverContext From Strings
************************************************

The :cpp:`ArResolver` interface currently allows implementations to create a
default resolver context or a resolver context for a given asset via
:cpp:`CreateDefaultContext` and :cpp:`CreateDefaultContextForAsset.` However,
this level of control is insufficient for DCCs and plugins that want to allow
users to specify other resolver contexts that are specific to their resolver
implementation. To support this, a new function will be added to allow
:cpp:`ArResolverContext` objects to be created from strings:

.. code-block:: cpp
   :caption: Added

   static ArResolverContext
   ArResolverContext::CreateFromString(
       const std::string& str);
   
   static ArResolverContext
   ArResolverContext::CreateFromString(
       const std::vector<std::pair<std::string, std::string> >& strs);

These functions will call protected virtual functions on :cpp:`ArResolver` to
allow resolvers to parse the given string and return an appropriate
:cpp:`ArResolverContext` object. Using the first overload would pass the given
string to the primary resolver to parse and return an :cpp:`ArResolverContext.`
An example usage might look like:

.. code-block:: cpp

   ArResolverContext ctx = ArResolverContext::CreateFromString(
       "/search/dir/1:/search/dir/2"
   );

The second form is used to create an :cpp:`ArResolverContext` for multiple
resolvers. An example usage might look like:

.. code-block:: cpp

   ArResolverContext ctx = ArResolverContext::CreateFromString({
       { "http", "username=user,pw=12345" },
       { "", "/search/dir/1:/search/dir/2" }
   });

In this case, :cpp:`ArResolverContext` will use the URI resolver associated with
the :mono:`http` scheme to create a context using the "username=..." string, use the
primary resolver to create a context using the "/search/..." string, then
combine them into the returned :cpp:`ArResolverContext` object.

Note that the public API is defined on :cpp:`ArResolverContext.` This made more
sense for organization and avoids some ambiguities that would arise
(particularly for multiple resolver support) if this API was put on
:cpp:`ArResolver` instead. For consistency, the existing functions for creating
default contexts will also be moved to :cpp:`ArResolverContext`.

.. code-block:: cpp
   :caption: Added

   static ArResolverContext
   ArResolverContext::CreateDefault();
   
   static ArResolverContext
   ArResolverContext::CreateDefaultForAsset(
       const std::string& assetPath);

.. code-block:: cpp
   :caption: Removed

   ArResolver::CreateDefaultContext
   ArResolver::CreateDefaultContextForAsset

Remove ArResolver::ConfigureResolverForAsset
********************************************

This function allows resolvers the opportunity to configure themselves in
applications that only load a single asset. This seems like an odd and
unnecessarily specific case to call out and does not cover any use cases where
multiple assets may be loaded. Animal Logic's Maya USD plugin actually hijacks
this function and allows users to pass in a general configuration string instead
of an asset path; this will hopefully no longer be necessary with the new
ability to create :cpp:`ArResolverContext` objects from strings.

To simplify the :cpp:`ArResolver` API, this function will be removed.

.. code-block:: cpp
   :caption: Removed

   ArResolver::ConfigureResolverForAsset

Rollout and Transition
######################

Ar 2.0 will substantially modify the :cpp:`ArResolver` interface and other
related classes. To maintain backwards compatibility with existing
:cpp:`ArResolver` implementations and client code sites, users will be able to
specify whether they want to enable Ar 2.0 at build time. If enabled, the
:cpp:`AR_VERSION` flag will be defined and to 2 and the Ar 2.0 changes will be
built and installed, causing old code to break if not updated to accommodate
those changes. If disabled, :cpp:`AR_VERSION` will be set to 1 and Ar will
revert back to the code that exists today, ensuring no change in behavior or
build breakages.

For its initial release, Ar 2.0 will be disabled by default. The current Ar
implementation will remain the default for backwards compatibility, but will be
deprecated. Users will be able to enable Ar 2.0 using the build flag to begin
testing and transitioning their resolver implementations and client code to the
new interface. Ar 2.0 will be enabled by default in the subsequent release, and
the old Ar implementation will be removed at that time.

Proposed API
############

The following are rough sketches of the proposed new interface on
:cpp:`ArResolver` and related classes. These are not final and subject to
change.

.. code-block:: cpp
   :caption: ar/resolver.h

   /// \class ArResolver
   ///
   class ArResolver
   {
       /// Identifiers
       /// @{
   public:
       ArIdentifier CreateIdentifier(
           const std::string& assetPath) const;
   
       ArIdentifier CreateIdentifier(
           const ArIdentifier& anchorIdentifier,
           const std::string& assetPath) const;
   
       ArIdentifier CreateIdentifierForNewAsset(
           const std::string& assetPath) const;
   
       ArIdentifier CreateIdentifierForNewAsset(
           const ArIdentifier& anchorIdentifier,
           const std::string& assetPath) const;
   
   protected:
       virtual ArIdentifier _CreateIdentifier(
           const std::string& assetPath) const = 0;
   
       virtual ArIdentifier _CreateIdentifier(
           const ArIdentifier& anchorIdentifier,
           const std::string& assetPath) const = 0;
   
       virtual ArIdentifier _CreateIdentifierForNewAsset(
           const std::string& assetPath) const = 0;
   
       virtual ArIdentifier _CreateIdentifierForNewAsset(
           const ArIdentifier& anchorIdentifier,
           const std::string& assetPath) const = 0;
       /// @}
   
       /// Resolve
       /// @{
   public:
       ArResolvedPath Resolve(
           const ArIdentifier& identifier,
           ArAssetInfo* assetInfo = nullptr) const;
   
       /// By default this function is just a convenience wrapper that
       /// is equivalent to Resolve(CreateIdentifier(assetPath), assetInfo);
       ArResolvedPath Resolve(
           const std::string& assetPath,
           ArAssetInfo* assetInfo = nullptr) const;
   
       ArResolvedPath ResolveForNewAsset(
           const ArIdentifier& identifier,
           ArAssetInfo* assetInfo = nullptr) const;
   
       /// By default this function is just a convenience wrapper that
       /// is equivalent to Resolve(CreateIdentifierForNewAsset(assetPath), assetInfo);
       ArResolvedPath ResolveForNewAsset(
           const std::string& assetPath,
           ArAssetInfo* assetInfo = nullptr) const;
   
   protected:
       virtual ArResolvedPath _Resolve(
           const std::string& assetPath
           ArAssetInfo* assetInfo) const
       { return _Resolve(CreateIdentifier(assetPath), assetInfo); }
   
       virtual ArResolvedPath _Resolve(
           const ArIdentifier& identifier,
           ArAssetInfo* assetInfo) const = 0;
   
       virtual ArResolvedPath _ResolveForNewAsset(
           const std::string& assetPath
           ArAssetInfo* assetInfo) const
       { return _ResolveForNewAsset(CreateIdentifierForNewAsset(assetPath), assetInfo); }
   
       virtual ArResolvedPath _ResolveForNewAsset(
           const ArIdentifier& identifier,
           ArAssetInfo* assetInfo) const = 0;
   
       /// @}
   
       /// Asset Access and Metadata
       /// @{
   public:
       std::shared_ptr<ArAsset> OpenAsset(
           const ArResolvedPath& resolvedPath) const;
   
       // By default returns NULL pointer to indicate that the
       // resolver has no write capabilities.
       std::shared_ptr<ArWritableAsset> OpenAssetForReplace(
           const ArResolvedPath& resolvedPath) const;
   
       // By default returns NULL pointer to indicate that the
       // resolver has no write capabilities.
       std::shared_ptr<ArWritableAsset> OpenAssetForUpdate(
           const ArResolvedPath& resolvedPath) const;
   
       std::string GetExtension(
           const ArResolvedPath& resolvedPath) const;
   
       VtValue GetModificationTimestamp(
           const ArResolvedPath& resolvedPath) const;
   
   protected:
       virtual std::shared_ptr<ArAsset> _OpenAsset(
           const ArResolvedPath& resolvedPath) const = 0;
   
       virtual std::shared_ptr<ArWritableAsset> _OpenAssetForReplace(
           const ArResolvedPath& resolvedPath) const
       { return nullptr; }
   
       virtual std::shared_ptr<ArWritableAsset> _OpenAssetForUpdate(
           const ArResolvedPath& resolvedPath) const
       { return nullptr; }
   
       virtual std::string _GetExtension(
           const ArResolvedPath& resolvedPath) const = 0;
   
       virtual VtValue _GetModificationTimestamp(
           const ArResolvedPath& resolvedPath) const = 0;
   
       /// @}
   
       /// Context Management
       /// @{
   public:
       void BindContext(
           const ArResolverContext& context,
           VtValue* bindingData);
   
       void UnbindContext(
           const ArResolverContext& context,
           VtValue* bindingData);
   
       void RefreshContext(
           const ArResolverContext& context);
   
       ArResolverContext GetCurrentContext() const;
   
   protected:
       virtual void _BindContext(
           const ArResolverContext& context,
           VtValue* bindingData) = 0;
   
       virtual void _UnbindContext(
           const ArResolverContext& context,
           VtValue* bindingData) = 0;
   
       virtual void _RefreshContext(
           const ArResolverContext& context) = 0;
   
       virtual ArResolverContext _GetCurrentContext() const = 0;
   
       // These functions are called by public API on ArResolverContext
       virtual ArResolverContext _CreateDefaultContext() const = 0;
       virtual ArResolverContext _CreateContextForAsset(
           const std::string& assetPath) const = 0;
       virtual ArResolverContext _CreateContextFromString(
           const std::string& configStr) const = 0;
   
       /// @}
   
       /// Scoped Caches
       /// @{
   public:
       void BeginCacheScope(
           VtValue* cacheScopeData);
   
       void EndCacheScope(
           VtValue* cacheScopeData);
   
   protected:
       virtual void BeginCacheScope(
           VtValue* cacheScopeData) = 0;
   
       virtual void EndCacheScope(
           VtValue* cacheScopeData) = 0;
   
       /// @}
   };

.. code-block:: cpp
   :caption: ar/identifier.h

   class ArIdentifier
   {
   public:
       ArIdentifier();
       explicit ArIdentifier(const std::string&);
       ArIdentifier(const ArIdentifier&);
       ArIdentifier(ArIdentifier&&);
       ~ArIdentifier();
   
       ArIdentifier& operator=(const ArIdentifier&);
       ArIdentifier& operator=(ArIdentifier&&); 
       bool operator<(const ArIdentifier&) const;
       bool operator==(const ArIdentifier&) const;
    
       // Return true if this identifier has a value, false if empty.
       explicit operator bool() const;
    
       // Return the identifier's value.
       const std::string& GetValue() const;
   };

.. code-block:: cpp
   :caption: ar/resolvedPath.h

   class ArResolvedPath
   {
   public:
       explicit ArResolvedPath(const std::string&);
       ArResolvedPath(const ArResolvedPath&);
       ArResolvedPath(ArResolvedPath&&);
       ~ArResolvedPath();
   
       ArResolvedPath& operator=(const ArResolvedPath&);
       ArResolvedPath& operator=(ArResolvedPath&&); 
       bool operator<(const ArResolvedPath&) const;
       bool operator==(const ArResolvedPath&) const;
   
   
       // Return true if this object has a non-empty resolved path, false otherwise.
       explicit operator bool() const;
   
       // Return resolved path.
       const std::string& GetValue() const;
   };

.. code-block:: cpp
   :caption: ar/resolverContext.h

   class ArResolverContext
   {
   public:
       // These factory methods call into protected methods on ArResolver
       // to allow resolvers to customize behavior.
       static ArResolverContext CreateDefault();
       static ArResolverContext CreateForAsset(const std::string& assetPath);
       static ArResolverContext CreateFromString(const std::string& configStr);
       static ArResolverContext CreateFromString(
           const std::vector<std::pair<std::string, std::string>>& configStr);
   
   
       ArResolverContext();
   
   
       template <class ... Context>
       ArResolverContext(const Context&... contexts);
   
   
       // ...
   };

.. code-block:: cpp
   :caption: ar/writableAsset.h

   class ArWritableAsset
   {
   public:
       virtual ~ArWritableAsset();
       virtual size_t Write(const void* buffer, size_t count, size_t offset) = 0;
   };

