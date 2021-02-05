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
#ifndef PXR_USD_SDF_PATH_H
#define PXR_USD_SDF_PATH_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"
#include "pxr/usd/sdf/pool.h"
#include "pxr/usd/sdf/tokens.h"
#include "pxr/base/tf/stl.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/traits.h"

#include <boost/intrusive_ptr.hpp>
#include <boost/operators.hpp>

#include <algorithm>
#include <iterator>
#include <set>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class Sdf_PathNode;
class SdfPathAncestorsRange;

// Ref-counting pointer to a path node.
// Intrusive ref-counts are used to keep the size of SdfPath
// the same as a raw pointer.  (shared_ptr, by comparison,
// is the size of two pointers.)

typedef boost::intrusive_ptr<const Sdf_PathNode> Sdf_PathNodeConstRefPtr;

void intrusive_ptr_add_ref(Sdf_PathNode const *);
void intrusive_ptr_release(Sdf_PathNode const *);

// Tags used for the pools of path nodes.
struct Sdf_PathPrimTag;
struct Sdf_PathPropTag;

// These are validated below.
static constexpr size_t Sdf_SizeofPrimPathNode = sizeof(void *) * 3;
static constexpr size_t Sdf_SizeofPropPathNode = sizeof(void *) * 3;

using Sdf_PathPrimPartPool = Sdf_Pool<
    Sdf_PathPrimTag, Sdf_SizeofPrimPathNode, /*regionBits=*/8>;

using Sdf_PathPropPartPool = Sdf_Pool<
    Sdf_PathPropTag, Sdf_SizeofPropPathNode, /*regionBits=*/8>;

using Sdf_PathPrimHandle = Sdf_PathPrimPartPool::Handle;
using Sdf_PathPropHandle = Sdf_PathPropPartPool::Handle;

// This handle class wraps up the raw Prim/PropPartPool handles.
template <class Handle, bool Counted, class PathNode=Sdf_PathNode const>
struct Sdf_PathNodeHandleImpl {
private:
    typedef Sdf_PathNodeHandleImpl this_type;

public:
    constexpr Sdf_PathNodeHandleImpl() noexcept {};

    explicit
    Sdf_PathNodeHandleImpl(Sdf_PathNode const *p, bool add_ref = true)
        : _poolHandle(Handle::GetHandle(reinterpret_cast<char const *>(p))) {
        if (p && add_ref) {
            _AddRef(p);
        }
    }

    explicit
    Sdf_PathNodeHandleImpl(Handle h, bool add_ref = true)
        : _poolHandle(h) {
        if (h && add_ref) {
            _AddRef();
        }
    }

    Sdf_PathNodeHandleImpl(Sdf_PathNodeHandleImpl const &rhs)
        : _poolHandle(rhs._poolHandle) {
        if (_poolHandle) {
            _AddRef();
        }
    }

    ~Sdf_PathNodeHandleImpl() {
        if (_poolHandle) {
            _DecRef();
        }
    }

    Sdf_PathNodeHandleImpl &
    operator=(Sdf_PathNodeHandleImpl const &rhs) {
        if (Counted && *this == rhs) {
            return *this;
        }
        this_type(rhs).swap(*this);
        return *this;
    }

    Sdf_PathNodeHandleImpl(Sdf_PathNodeHandleImpl &&rhs) noexcept
        : _poolHandle(rhs._poolHandle) {
        rhs._poolHandle = nullptr;
    }

    Sdf_PathNodeHandleImpl &
    operator=(Sdf_PathNodeHandleImpl &&rhs) noexcept {
        this_type(std::move(rhs)).swap(*this);
        return *this;
    }

    Sdf_PathNodeHandleImpl &
    operator=(Sdf_PathNode const *rhs) noexcept {
        this_type(rhs).swap(*this);
        return *this;
    }

    void reset() noexcept {
        _poolHandle = Handle { nullptr };
    }

    Sdf_PathNode const *
    get() const noexcept {
        return reinterpret_cast<Sdf_PathNode *>(_poolHandle.GetPtr());
    }

    Sdf_PathNode const &
    operator*() const {
        return *get();
    }

    Sdf_PathNode const *
    operator->() const {
        return get();
    }

    explicit operator bool() const noexcept {
        return static_cast<bool>(_poolHandle);
    }

    void swap(Sdf_PathNodeHandleImpl &rhs) noexcept {
        _poolHandle.swap(rhs._poolHandle);
    }

    inline bool operator==(Sdf_PathNodeHandleImpl const &rhs) const noexcept {
        return _poolHandle == rhs._poolHandle;
    }
    inline bool operator!=(Sdf_PathNodeHandleImpl const &rhs) const noexcept {
        return _poolHandle != rhs._poolHandle;
    }
    inline bool operator<(Sdf_PathNodeHandleImpl const &rhs) const noexcept {
        return _poolHandle < rhs._poolHandle;
    }
private:

    void _AddRef(Sdf_PathNode const *p) const {
        if (Counted) {
            intrusive_ptr_add_ref(p);
        }
    }

    void _AddRef() const {
        _AddRef(get());
    }

    void _DecRef() const {
        if (Counted) {
            intrusive_ptr_release(get());
        }
    }

    Handle _poolHandle { nullptr };
};

using Sdf_PathPrimNodeHandle =
    Sdf_PathNodeHandleImpl<Sdf_PathPrimHandle, /*Counted=*/true>;

using Sdf_PathPropNodeHandle =
    Sdf_PathNodeHandleImpl<Sdf_PathPropHandle, /*Counted=*/false>;


/// A set of SdfPaths.
typedef std::set<class SdfPath> SdfPathSet;
/// A vector of SdfPaths.
typedef std::vector<class SdfPath> SdfPathVector;

// Tell VtValue that SdfPath is cheap to copy.
VT_TYPE_IS_CHEAP_TO_COPY(class SdfPath);

/// \class SdfPath 
///
/// A path value used to locate objects in layers or scenegraphs.
///
/// \section sec_SdfPath_Overview Overview
///
/// SdfPath is used in several ways:
/// \li As a storage key for addressing and accessing values held in a SdfLayer
/// \li As a namespace identity for scenegraph objects
/// \li As a way to refer to other scenegraph objects through relative paths
///
/// The paths represented by an SdfPath class may be either relative or
/// absolute.  Relative paths are relative to the prim object that contains them
/// (that is, if an SdfRelationshipSpec target is relative, it is relative to
/// the SdfPrimSpec object that owns the SdfRelationshipSpec object).
///
/// SdfPath objects can be readily created from and converted back to strings,
/// but as SdfPath objects, they have behaviors that make it easy and efficient
/// to work with them. The SdfPath class provides a full range of methods for
/// manipulating scene paths by appending a namespace child, appending a
/// relationship target, getting the parent path,
/// and so on.  Since the SdfPath class uses a node-based representation
/// internally, you should use the editing functions rather than converting to
/// and from strings if possible.
///
/// \section sec_SdfPath_Syntax Path Syntax
///
/// Like a filesystem path, an SdfPath is conceptually just a sequence of
/// path components.  Unlike a filesystem path, each component has a type,
/// and the type is indicated by the syntax.
///
/// Two separators are used between parts of a path. A slash ("/") following an
/// identifier is used to introduce a namespace child. A period (".") following
/// an identifier is used to introduce a property.  A property may also have
/// several non-sequential colons (':') in its name to provide a rudimentary
/// namespace within properties but may not end or begin with a colon.
///
/// A leading slash in the string representation of an SdfPath object indicates
/// an absolute path.  Two adjacent periods indicate the parent namespace.
///
/// Brackets ("[" and "]") are used to indicate relationship target paths for
/// relational attributes.
///
/// The first part in a path is assumed to be a namespace child unless
/// it is preceded by a period.  That means:
/// \li <c>/Foo</c> is an absolute path specifying the root prim Foo.
/// \li <c>/Foo/Bar</c> is an absolute path specifying namespace child Bar
///     of root prim Foo.
/// \li <c>/Foo/Bar.baz</c> is an absolute path specifying property \c baz of
///     namespace child Bar of root prim Foo.
/// \li <c>Foo</c> is a relative path specifying namespace child Foo of
///     the current prim.
/// \li <c>Foo/Bar</c> is a relative path specifying namespace child Bar of
///     namespace child Foo of the current prim.
/// \li <c>Foo/Bar.baz</c> is a relative path specifying property \c baz of
///     namespace child Bar of namespace child Foo of the current prim.
/// \li <c>.foo</c> is a relative path specifying the property \c foo of the
///     current prim.
/// \li <c>/Foo.bar[/Foo.baz].attrib</c> is a relational attribute path. The
///     relationship <c>/Foo.bar</c> has a target <c>/Foo.baz</c>. There is a
///     relational attribute \c attrib on that relationship-&gt;target pair.
///
/// \section sec_SdfPath_ThreadSafety A Note on Thread-Safety
///
/// SdfPath is strongly thread-safe, in the sense that zero additional
/// synchronization is required between threads creating or using SdfPath
/// values. Just like TfToken, SdfPath values are immutable. Internally,
/// SdfPath uses a global prefix tree to efficiently share representations
/// of paths, and provide fast equality/hashing operations, but
/// modifications to this table are internally synchronized. Consequently,
/// as with TfToken, for best performance it is important to minimize
/// the number of values created (since it requires synchronized access to
/// this table) or copied (since it requires atomic ref-counting operations).
///
class SdfPath : boost::totally_ordered<SdfPath>
{
public:
    /// The empty path value, equivalent to SdfPath().
    SDF_API static const SdfPath & EmptyPath();

    /// The absolute path representing the top of the
    /// namespace hierarchy.
    SDF_API static const SdfPath & AbsoluteRootPath();

    /// The relative path representing "self".
    SDF_API static const SdfPath & ReflexiveRelativePath();

    /// \name Constructors
    /// @{ 
    
    /// Constructs the default, empty path.
    ///
    SdfPath() noexcept {
        // This generates a single instruction instead of 2 on gcc 6.3.  Seems
        // to be fixed on gcc 7+ and newer clangs.  Remove when we're there!
        memset(this, 0, sizeof(*this));
    }

    /// Creates a path from the given string.
    ///
    /// If the given string is not a well-formed path, this will raise
    /// a Tf error.  Note that passing an empty std::string() will also
    /// raise an error; the correct way to get the empty path is SdfPath().
    ///
    /// Internal dot-dots will be resolved by removing the first dot-dot,
    /// the element preceding it, and repeating until no internal dot-dots
    /// remain.
    ///
    /// Note that most often new paths are expected to be created by
    /// asking existing paths to return modified versions of themselves.
    //
    // XXX  We may want to revisit the behavior when constructing
    // a path with an empty string ("") to accept it without error and
    // return EmptyPath.
    SDF_API explicit SdfPath(const std::string &path);

    /// @}

    /// \name Querying paths 
    /// @{

    /// Returns the number of path elements in this path.
    SDF_API size_t GetPathElementCount() const;

    /// Returns whether the path is absolute.
    SDF_API bool IsAbsolutePath() const;

    /// Return true if this path is the AbsoluteRootPath().
    SDF_API bool IsAbsoluteRootPath() const;

    /// Returns whether the path identifies a prim.
    SDF_API bool IsPrimPath() const;

    /// Returns whether the path identifies a prim or the absolute root.
    SDF_API bool IsAbsoluteRootOrPrimPath() const;

    /// Returns whether the path identifies a root prim.
    ///
    /// the path must be absolute and have a single element
    /// (for example <c>/foo</c>).
    SDF_API bool IsRootPrimPath() const;

    /// Returns whether the path identifies a property.
    ///
    /// A relational attribute is considered to be a property, so this
    /// method will return true for relational attributes as well
    /// as properties of prims.
    SDF_API bool IsPropertyPath() const;

    /// Returns whether the path identifies a prim's property.
    ///
    /// A relational attribute is not a prim property.
    SDF_API bool IsPrimPropertyPath() const;

    /// Returns whether the path identifies a namespaced property.
    ///
    /// A namespaced property has colon embedded in its name.
    SDF_API bool IsNamespacedPropertyPath() const;

    /// Returns whether the path identifies a variant selection for a
    /// prim.
    SDF_API bool IsPrimVariantSelectionPath() const;

    /// Return true if this path is a prim path or is a prim variant
    /// selection path.
    SDF_API bool IsPrimOrPrimVariantSelectionPath() const;

    /// Returns whether the path or any of its parent paths identifies
    /// a variant selection for a prim.
    SDF_API bool ContainsPrimVariantSelection() const;
    
    /// Return true if this path contains any property elements, false
    /// otherwise.  A false return indicates a prim-like path, specifically a
    /// root path, a prim path, or a prim variant selection path.  A true return
    /// indicates a property-like path: a prim property path, a target path, a
    /// relational attribute path, etc.
    bool ContainsPropertyElements() const {
        return static_cast<bool>(_propPart);
    }

    /// Return true if this path is or has a prefix that's a target path or a
    /// mapper path.
    SDF_API bool ContainsTargetPath() const;

    /// Returns whether the path identifies a relational attribute.
    ///
    /// If this is true, IsPropertyPath() will also be true.
    SDF_API bool IsRelationalAttributePath() const;

    /// Returns whether the path identifies a relationship or
    /// connection target.
    SDF_API bool IsTargetPath() const;

    /// Returns whether the path identifies a connection mapper.
    SDF_API bool IsMapperPath() const;

    /// Returns whether the path identifies a connection mapper arg.
    SDF_API bool IsMapperArgPath() const;

    /// Returns whether the path identifies a connection expression.
    SDF_API bool IsExpressionPath() const;

    /// Returns true if this is the empty path (SdfPath::EmptyPath()).
    inline bool IsEmpty() const noexcept {
        // No need to check _propPart, because it can only be non-null if
        // _primPart is non-null.
        return !_primPart; 
    }

    /// Return the string representation of this path as a TfToken.
    ///
    /// This function is recommended only for human-readable or diagnostic
    /// output.  Use the SdfPath API to manipulate paths.  It is less
    /// error-prone and has better performance.
    SDF_API TfToken GetAsToken() const;

    /// Return the string representation of this path as a TfToken lvalue.
    ///
    /// This function returns a persistent lvalue.  If an rvalue will suffice,
    /// call GetAsToken() instead.  That avoids populating internal data
    /// structures to hold the persistent token.
    ///
    /// This function is recommended only for human-readable or diagnostic
    /// output.  Use the SdfPath API to manipulate paths.  It is less
    /// error-prone and has better performance.
    SDF_API TfToken const &GetToken() const;

    /// Return the string representation of this path as a std::string.
    ///
    /// This function is recommended only for human-readable or diagnostic
    /// output.  Use the SdfPath API to manipulate paths.  It is less
    /// error-prone and has better performance.
    SDF_API std::string GetAsString() const;

    /// Return the string representation of this path as a std::string.
    ///
    /// This function returns a persistent lvalue.  If an rvalue will suffice,
    /// call GetAsString() instead.  That avoids populating internal data
    /// structures to hold the persistent string.
    ///
    /// This function is recommended only for human-readable or diagnostic
    /// output.  Use the SdfPath API to manipulate paths.  It is less
    /// error-prone and has better performance.
    SDF_API const std::string &GetString() const;

    /// Returns the string representation of this path as a c string.
    ///
    /// This function returns a pointer to a persistent c string.  If a
    /// temporary c string will suffice, call GetAsString().c_str() instead.
    /// That avoids populating internal data structures to hold the persistent
    /// string.
    ///
    /// This function is recommended only for human-readable or diagnostic
    /// output.  Use the SdfPath API to manipulate paths.  It is less
    /// error-prone and has better performance.
    SDF_API const char *GetText() const;

    /// Returns the prefix paths of this path.
    ///
    /// Prefixes are returned in order of shortest to longest.  The path
    /// itself is returned as the last prefix.
    /// Note that if the prefix order does not need to be from shortest to
    /// longest, it is more efficient to use GetAncestorsRange, which
    /// produces an equivalent set of paths, ordered from longest to shortest.
    SDF_API SdfPathVector GetPrefixes() const;

    /// Fills prefixes with prefixes of this path.
    /// 
    /// This avoids copy constructing the return value.
    ///
    /// Prefixes are returned in order of shortest to longest.  The path
    /// itself is returned as the last prefix.
    /// Note that if the prefix order does not need to be from shortest to
    /// longest, it is more efficient to use GetAncestorsRange, which
    /// produces an equivalent set of paths, ordered from longest to shortest.
    SDF_API void GetPrefixes(SdfPathVector *prefixes) const;

    /// Return a range for iterating over the ancestors of this path.
    ///
    /// The range provides iteration over the prefixes of a path, ordered
    /// from longest to shortest (the opposite of the order of the prefixes
    /// returned by GetPrefixes).
    SDF_API SdfPathAncestorsRange GetAncestorsRange() const;

    /// Returns the name of the prim, property or relational
    /// attribute identified by the path.
    ///
    /// Returns EmptyPath if this path is a target or mapper path.
    ///
    /// <ul>
    ///     <li>Returns "" for EmptyPath.</li>
    ///     <li>Returns "." for ReflexiveRelativePath.</li>
    ///     <li>Returns ".." for a path ending in ParentPathElement.</li>
    /// </ul>
    SDF_API const std::string &GetName() const;

    /// Returns the name of the prim, property or relational
    /// attribute identified by the path, as a token.
    SDF_API const TfToken &GetNameToken() const;
    
    /// Returns an ascii representation of the "terminal" element
    /// of this path, which can be used to reconstruct the path using
    /// \c AppendElementString() on its parent.
    ///
    /// EmptyPath(), AbsoluteRootPath(), and ReflexiveRelativePath() are
    /// \em not considered elements (one of the defining properties of
    /// elements is that they have a parent), so \c GetElementString() will
    /// return the empty string for these paths.
    ///
    /// Unlike \c GetName() and \c GetTargetPath(), which provide you "some"
    /// information about the terminal element, this provides a complete
    /// representation of the element, for all element types.
    ///
    /// Also note that whereas \c GetName(), \c GetNameToken(), \c GetText(),
    /// \c GetString(), and \c GetTargetPath() return cached results, 
    /// \c GetElementString() always performs some amount of string
    /// manipulation, which you should keep in mind if performance is a concern.
    SDF_API std::string GetElementString() const;

    /// Like GetElementString() but return the value as a TfToken.
    SDF_API TfToken GetElementToken() const;

    /// Return a copy of this path with its final component changed to
    /// \a newName.  This path must be a prim or property path.
    ///
    /// This method is shorthand for path.GetParentPath().AppendChild(newName)
    /// for prim paths, path.GetParentPath().AppendProperty(newName) for
    /// prim property paths, and
    /// path.GetParentPath().AppendRelationalAttribute(newName) for relational
    /// attribute paths.
    ///
    /// Note that only the final path component is ever changed.  If the name of
    /// the final path component appears elsewhere in the path, it will not be
    /// modified.
    ///
    /// Some examples:
    ///
    /// ReplaceName('/chars/MeridaGroup', 'AngusGroup') -> '/chars/AngusGroup'
    /// ReplaceName('/Merida.tx', 'ty') -> '/Merida.ty'
    /// ReplaceName('/Merida.tx[targ].tx', 'ty') -> '/Merida.tx[targ].ty'
    ///
    SDF_API SdfPath ReplaceName(TfToken const &newName) const;

    /// Returns the relational attribute or mapper target path
    /// for this path.
    ///
    /// Returns EmptyPath if this is not a target, relational attribute or
    /// mapper path.
    ///
    /// Note that it is possible for a path to have multiple "target" paths.
    /// For example a path that identifies a connection target for a
    /// relational attribute includes the target of the connection as well
    /// as the target of the relational attribute.  In these cases, the
    /// "deepest" or right-most target path will be returned (the connection
    /// target in this example).
    SDF_API const SdfPath &GetTargetPath() const;

    /// Returns all the relationship target or connection target
    /// paths contained in this path, and recursively all the target paths
    /// contained in those target paths in reverse depth-first order.
    ///
    /// For example, given the path: '/A/B.a[/C/D.a[/E/F.a]].a[/A/B.a[/C/D.a]]'
    /// this method produces: '/A/B.a[/C/D.a]', '/C/D.a', '/C/D.a[/E/F.a]',
    /// '/E/F.a'
    SDF_API void GetAllTargetPathsRecursively(SdfPathVector *result) const;

    /// Returns the variant selection for this path, if this is a variant
    /// selection path.
    /// Returns a pair of empty strings if this path is not a variant
    /// selection path.
    SDF_API 
    std::pair<std::string, std::string> GetVariantSelection() const;

    /// Return true if both this path and \a prefix are not the empty
    /// path and this path has \a prefix as a prefix.  Return false otherwise.
    SDF_API bool HasPrefix( const SdfPath &prefix ) const;

    /// @}

    /// \name Creating new paths by modifying existing paths
    /// @{

    /// Return the path that identifies this path's namespace parent.
    ///
    /// For a prim path (like '/foo/bar'), return the prim's parent's path
    /// ('/foo').  For a prim property path (like '/foo/bar.property'), return
    /// the prim's path ('/foo/bar').  For a target path (like
    /// '/foo/bar.property[/target]') return the property path
    /// ('/foo/bar.property').  For a mapper path (like
    /// '/foo/bar.property.mapper[/target]') return the property path
    /// ('/foo/bar.property).  For a relational attribute path (like
    /// '/foo/bar.property[/target].relAttr') return the relationship target's
    /// path ('/foo/bar.property[/target]').  For a prim variant selection path
    /// (like '/foo/bar{var=sel}') return the prim path ('/foo/bar').  For a
    /// root prim path (like '/rootPrim'), return AbsoluteRootPath() ('/').  For
    /// a single element relative prim path (like 'relativePrim'), return
    /// ReflexiveRelativePath() ('.').  For ReflexiveRelativePath(), return the
    /// relative parent path ('..').
    ///
    /// Note that the parent path of a relative parent path ('..') is a relative
    /// grandparent path ('../..').  Use caution writing loops that walk to
    /// parent paths since relative paths have infinitely many ancestors.  To
    /// more safely traverse ancestor paths, consider iterating over an
    /// SdfPathAncestorsRange instead, as returend by GetAncestorsRange().
    SDF_API SdfPath GetParentPath() const;

    /// Creates a path by stripping all relational attributes, targets,
    /// properties, and variant selections from the leafmost prim path, leaving
    /// the nearest path for which \a IsPrimPath() returns true.
    ///
    /// See \a GetPrimOrPrimVariantSelectionPath also.
    ///
    /// If the path is already a prim path, the same path is returned.
    SDF_API SdfPath GetPrimPath() const;

    /// Creates a path by stripping all relational attributes, targets,
    /// and properties, leaving the nearest path for which
    /// \a IsPrimOrPrimVariantSelectionPath() returns true.
    ///
    /// See \a GetPrimPath also.
    ///
    /// If the path is already a prim or a prim variant selection path, the same
    /// path is returned.
    SDF_API SdfPath GetPrimOrPrimVariantSelectionPath() const;

    /// Creates a path by stripping all properties and relational
    /// attributes from this path, leaving the path to the containing prim.
    ///
    /// If the path is already a prim or absolute root path, the same
    /// path is returned.
    SDF_API SdfPath GetAbsoluteRootOrPrimPath() const;

    /// Create a path by stripping all variant selections from all
    /// components of this path, leaving a path with no embedded variant
    /// selections.
    SDF_API SdfPath StripAllVariantSelections() const;

    /// Creates a path by appending a given relative path to this path.
    ///
    /// If the newSuffix is a prim path, then this path must be a prim path
    /// or a root path.
    ///
    /// If the newSuffix is a prim property path, then this path must be
    /// a prim path or the ReflexiveRelativePath.
    SDF_API SdfPath AppendPath(const SdfPath &newSuffix) const;

    /// Creates a path by appending an element for \p childName
    /// to this path.
    ///
    /// This path must be a prim path, the AbsoluteRootPath
    /// or the ReflexiveRelativePath.
    SDF_API SdfPath AppendChild(TfToken const &childName) const;

    /// Creates a path by appending an element for \p propName
    /// to this path.
    ///
    /// This path must be a prim path or the ReflexiveRelativePath.
    SDF_API SdfPath AppendProperty(TfToken const &propName) const;

    /// Creates a path by appending an element for \p variantSet
    /// and \p variant to this path.
    ///
    /// This path must be a prim path.
    SDF_API 
    SdfPath AppendVariantSelection(const std::string &variantSet,
                                   const std::string &variant) const;

    /// Creates a path by appending an element for
    /// \p targetPath.
    ///
    /// This path must be a prim property or relational attribute path.
    SDF_API SdfPath AppendTarget(const SdfPath &targetPath) const;

    /// Creates a path by appending an element for
    /// \p attrName to this path.
    ///
    /// This path must be a target path.
    SDF_API 
    SdfPath AppendRelationalAttribute(TfToken const &attrName) const;

    /// Replaces the relational attribute's target path
    ///
    /// The path must be a relational attribute path.
    SDF_API 
    SdfPath ReplaceTargetPath( const SdfPath &newTargetPath ) const;

    /// Creates a path by appending a mapper element for
    /// \p targetPath.
    ///
    /// This path must be a prim property or relational attribute path.
    SDF_API SdfPath AppendMapper(const SdfPath &targetPath) const;

    /// Creates a path by appending an element for
    /// \p argName.
    ///
    /// This path must be a mapper path.
    SDF_API SdfPath AppendMapperArg(TfToken const &argName) const;

    /// Creates a path by appending an expression element.
    ///
    /// This path must be a prim property or relational attribute path.
    SDF_API SdfPath AppendExpression() const;

    /// Creates a path by extracting and appending an element
    /// from the given ascii element encoding.
    ///
    /// Attempting to append a root or empty path (or malformed path)
    /// or attempting to append \em to the EmptyPath will raise an
    /// error and return the EmptyPath.
    ///
    /// May also fail and return EmptyPath if this path's type cannot
    /// possess a child of the type encoded in \p element.
    SDF_API SdfPath AppendElementString(const std::string &element) const;

    /// Like AppendElementString() but take the element as a TfToken.
    SDF_API SdfPath AppendElementToken(const TfToken &elementTok) const;

    /// Returns a path with all occurrences of the prefix path
    /// \p oldPrefix replaced with the prefix path \p newPrefix.
    ///
    /// If fixTargetPaths is true, any embedded target paths will also
    /// have their paths replaced.  This is the default.
    ///
    /// If this is not a target, relational attribute or mapper path this
    /// will do zero or one path prefix replacements, if not the number of
    /// replacements can be greater than one.
    SDF_API 
    SdfPath ReplacePrefix(const SdfPath &oldPrefix,
                          const SdfPath &newPrefix,
                          bool fixTargetPaths=true) const;

    /// Returns a path with maximal length that is a prefix path of
    /// both this path and \p path.
    SDF_API SdfPath GetCommonPrefix(const SdfPath &path) const;

    /// Find and remove the longest common suffix from two paths.
    ///
    /// Returns this path and \p otherPath with the longest common suffix
    /// removed (first and second, respectively).  If the two paths have no
    /// common suffix then the paths are returned as-is.  If the paths are
    /// equal then this returns empty paths for relative paths and absolute
    /// roots for absolute paths.  The paths need not be the same length.
    ///
    /// If \p stopAtRootPrim is \c true then neither returned path will be
    /// the root path.  That, in turn, means that some common suffixes will
    /// not be removed.  For example, if \p stopAtRootPrim is \c true then
    /// the paths /A/B and /B will be returned as is.  Were it \c false
    /// then the result would be /A and /.  Similarly paths /A/B/C and
    /// /B/C would return /A/B and /B if \p stopAtRootPrim is \c true but
    /// /A and / if it's \c false.
    SDF_API 
    std::pair<SdfPath, SdfPath>
    RemoveCommonSuffix(const SdfPath& otherPath,
                       bool stopAtRootPrim = false) const;

    /// Returns the absolute form of this path using \p anchor 
    /// as the relative basis.
    ///
    /// \p anchor must be an absolute prim path.
    ///
    /// If this path is a relative path, resolve it using \p anchor as the
    /// relative basis.
    ///
    /// If this path is already an absolute path, just return a copy.
    SDF_API SdfPath MakeAbsolutePath(const SdfPath & anchor) const;

    /// Returns the relative form of this path using \p anchor
    /// as the relative basis.
    ///
    /// \p anchor must be an absolute prim path.
    ///
    /// If this path is an absolute path, return the corresponding relative path
    /// that is relative to the absolute path given by \p anchor.
    ///
    /// If this path is a relative path, return the optimal relative
    /// path to the absolute path given by \p anchor.  (The optimal
    /// relative path from a given prim path is the relative path
    /// with the least leading dot-dots.
    SDF_API SdfPath MakeRelativePath(const SdfPath & anchor) const;

    /// @}

    /// \name Valid path strings, prim and property names
    /// @{

    /// Returns whether \p name is a legal identifier for any
    /// path component.
    SDF_API static bool IsValidIdentifier(const std::string &name);

    /// Returns whether \p name is a legal namespaced identifier.
    /// This returns \c true if IsValidIdentifier() does.
    SDF_API static bool IsValidNamespacedIdentifier(const std::string &name);

    /// Tokenizes \p name by the namespace delimiter.
    /// Returns the empty vector if \p name is not a valid namespaced
    /// identifier.
    SDF_API static std::vector<std::string> TokenizeIdentifier(const std::string &name);

    /// Tokenizes \p name by the namespace delimiter.
    /// Returns the empty vector if \p name is not a valid namespaced
    /// identifier.
    SDF_API
    static TfTokenVector TokenizeIdentifierAsTokens(const std::string &name);

    /// Join \p names into a single identifier using the namespace delimiter.
    /// Any empty strings present in \p names are ignored when joining.
    SDF_API 
    static std::string JoinIdentifier(const std::vector<std::string> &names);

    /// Join \p names into a single identifier using the namespace delimiter.
    /// Any empty strings present in \p names are ignored when joining.
    SDF_API 
    static std::string JoinIdentifier(const TfTokenVector& names);

    /// Join \p lhs and \p rhs into a single identifier using the
    /// namespace delimiter.
    /// Returns \p lhs if \p rhs is empty and vice verse.
    /// Returns an empty string if both \p lhs and \p rhs are empty.
    SDF_API 
    static std::string JoinIdentifier(const std::string &lhs,
                                      const std::string &rhs);

    /// Join \p lhs and \p rhs into a single identifier using the
    /// namespace delimiter.
    /// Returns \p lhs if \p rhs is empty and vice verse.
    /// Returns an empty string if both \p lhs and \p rhs are empty.
    SDF_API 
    static std::string JoinIdentifier(const TfToken &lhs, const TfToken &rhs);

    /// Returns \p name stripped of any namespaces.
    /// This does not check the validity of the name;  it just attempts
    /// to remove anything that looks like a namespace.
    SDF_API
    static std::string StripNamespace(const std::string &name);

    /// Returns \p name stripped of any namespaces.
    /// This does not check the validity of the name;  it just attempts
    /// to remove anything that looks like a namespace.
    SDF_API
    static TfToken StripNamespace(const TfToken &name);

    /// Returns (\p name, \c true) where \p name is stripped of the prefix
    /// specified by \p matchNamespace if \p name indeed starts with
    /// \p matchNamespace. Returns (\p name, \c false) otherwise, with \p name 
    /// unmodified.
    ///
    /// This function deals with both the case where \p matchNamespace contains
    /// the trailing namespace delimiter ':' or not.
    ///
    SDF_API
    static std::pair<std::string, bool> 
    StripPrefixNamespace(const std::string &name, 
                         const std::string &matchNamespace);

    /// Return true if \p pathString is a valid path string, meaning that
    /// passing the string to the \a SdfPath constructor will result in a valid,
    /// non-empty SdfPath.  Otherwise, return false and if \p errMsg is not NULL,
    /// set the pointed-to string to the parse error.
    SDF_API
    static bool IsValidPathString(const std::string &pathString,
                                  std::string *errMsg = 0);

    /// @}

    /// \name Operators
    /// @{

    /// Equality operator.
    /// (Boost provides inequality from this.)
    inline bool operator==(const SdfPath &rhs) const {
        return _AsInt() == rhs._AsInt();
    }

    /// Comparison operator.
    ///
    /// This orders paths lexicographically, aka dictionary-style.
    ///
    inline bool operator<(const SdfPath &rhs) const {
        if (_AsInt() == rhs._AsInt()) {
            return false;
        }
        if (!_primPart || !rhs._primPart) {
            return !_primPart && rhs._primPart;
        }
        // Valid prim parts -- must walk node structure, etc.
        return _LessThanInternal(*this, rhs);
    }

    template <class HashState>
    friend void TfHashAppend(HashState &h, SdfPath const &path) {
        // The hash function is pretty sensitive performance-wise.  Be
        // careful making changes here, and run tests.
        uint32_t primPart, propPart;
        memcpy(&primPart, &path._primPart, sizeof(primPart));
        memcpy(&propPart, &path._propPart, sizeof(propPart));
        h.Append(primPart);
        h.Append(propPart);
    }

    // For hash maps and sets
    struct Hash {
        inline size_t operator()(const SdfPath& path) const {
            return TfHash()(path);
        }
    };

    inline size_t GetHash() const {
        return Hash()(*this);
    }

    // For cases where an unspecified total order that is not stable from
    // run-to-run is needed.
    struct FastLessThan {
        inline bool operator()(const SdfPath& a, const SdfPath& b) const {
            return a._AsInt() < b._AsInt();
        }
    };

    /// @}

    /// \name Utilities
    /// @{

    /// Given some vector of paths, get a vector of concise unambiguous
    /// relative paths.
    ///
    /// GetConciseRelativePaths requires a vector of absolute paths. It
    /// finds a set of relative paths such that each relative path is
    /// unique.
    SDF_API static SdfPathVector
    GetConciseRelativePaths(const SdfPathVector& paths);

    /// Remove all elements of \a paths that are prefixed by other
    /// elements in \a paths.  As a side-effect, the result is left in sorted
    /// order.
    SDF_API static void RemoveDescendentPaths(SdfPathVector *paths);

    /// Remove all elements of \a paths that prefix other elements in
    /// \a paths.  As a side-effect, the result is left in sorted order.
    SDF_API static void RemoveAncestorPaths(SdfPathVector *paths);

    /// @}

private:

    // This is used for all internal path construction where we do operations
    // via nodes and then want to return a new path with a resulting prim and
    // property parts.

    // Accept rvalues.
    explicit SdfPath(Sdf_PathPrimNodeHandle &&primNode)
        : _primPart(std::move(primNode)) {}

    // Construct from prim & prop parts.
    SdfPath(Sdf_PathPrimNodeHandle const &primPart,
            Sdf_PathPropNodeHandle const &propPart)
        : _primPart(primPart)
        , _propPart(propPart) {}

    // Construct from prim & prop node pointers.
    SdfPath(Sdf_PathNode const *primPart,
            Sdf_PathNode const *propPart)
        : _primPart(primPart)
        , _propPart(propPart) {}

    friend class Sdf_PathNode;
    friend class Sdfext_PathAccess;
    friend class SdfPathAncestorsRange;

    // converts elements to a string for parsing (unfortunate)
    static std::string
    _ElementsToString(bool absolute, const std::vector<std::string> &elements);

    SdfPath _ReplacePrimPrefix(SdfPath const &oldPrefix,
                               SdfPath const &newPrefix) const;

    SdfPath _ReplaceTargetPathPrefixes(SdfPath const &oldPrefix,
                                       SdfPath const &newPrefix) const;

    SdfPath _ReplacePropPrefix(SdfPath const &oldPrefix,
                               SdfPath const &newPrefix,
                               bool fixTargetPaths) const;

    // Helper to implement the uninlined portion of operator<.
    SDF_API static bool
    _LessThanInternal(SdfPath const &lhs, SdfPath const &rhs);

    inline uint64_t _AsInt() const {
        static_assert(sizeof(*this) == sizeof(uint64_t), "");
        uint64_t ret;
        std::memcpy(&ret, this, sizeof(*this));
        return ret;
    }

    friend void swap(SdfPath &lhs, SdfPath &rhs) {
        lhs._primPart.swap(rhs._primPart);
        lhs._propPart.swap(rhs._propPart);
    }

    Sdf_PathPrimNodeHandle _primPart;
    Sdf_PathPropNodeHandle _propPart;

};


/// \class SdfPathAncestorsRange
///
/// Range representing a path and ancestors, and providing methods for
/// iterating over them.
///
/// An ancestor range represents a path and all of its ancestors ordered from
/// nearest to furthest (root-most).
/// For example, given a path like `/a/b.prop`, the range represents paths
/// `/a/b.prop`, `/a/b` and `/a`, in that order.
/// A range accepts relative paths as well: For path `a/b.prop`, the range
/// represents paths 'a/b.prop`, `a/b` and `a`.
/// If a path contains parent path elements, (`..`), those elements are treated
/// as elements of the range. For instance, given path `../a/b`, the range
/// represents paths `../a/b`, `../a` and `..`.
/// This represents the same of set of `prefix` paths as SdfPath::GetPrefixes,
/// but in reverse order.
class SdfPathAncestorsRange
{
public:

    SdfPathAncestorsRange(const SdfPath& path)
        : _path(path) {}

    const SdfPath& GetPath() const { return _path; }

    struct iterator {
        using iterator_category = std::forward_iterator_tag;
        using value_type = SdfPath;
        using difference_type = std::ptrdiff_t;
        using reference = const SdfPath&;
        using pointer = const SdfPath*;

        iterator(const SdfPath& path) : _path(path) {}

        iterator() = default;

        SDF_API
        iterator& operator++();

        const SdfPath& operator*() const { return _path; }

        const SdfPath* operator->() const { return &_path; }
        
        bool operator==(const iterator& o) const { return _path == o._path; }

        bool operator!=(const iterator& o) const { return _path != o._path; }

        /// Return the distance between two iterators.
        /// It is only valid to compute the distance between paths
        /// that share a common prefix.
        SDF_API friend difference_type  
        distance(const iterator& first, const iterator& last);

    private:
        SdfPath _path;
    };

    iterator begin() const { return iterator(_path); }

    iterator end() const { return iterator(); }

private:
    SdfPath _path;
};


// Overload hash_value for SdfPath.  Used by things like boost::hash.
inline size_t hash_value(SdfPath const &path)
{
    return path.GetHash();
}

/// Writes the string representation of \p path to \p out.
SDF_API std::ostream & operator<<( std::ostream &out, const SdfPath &path );

// Helper for SdfPathFindPrefixedRange & SdfPathFindLongestPrefix.  A function
// object that returns an SdfPath const & unchanged.
struct Sdf_PathIdentity {
    inline SdfPath const &operator()(SdfPath const &arg) const {
        return arg;
    }
};

/// Find the subrange of the sorted range [\a begin, \a end) that includes all
/// paths prefixed by \a path.  The input range must be ordered according to
/// SdfPath::operator<.  If your range's iterators' value_types are not SdfPath,
/// but you can obtain SdfPaths from them (e.g. map<SdfPath, X>::iterator), you
/// can pass a function to extract the path from the dereferenced iterator in
/// \p getPath.
template <class ForwardIterator, class GetPathFn = Sdf_PathIdentity>
std::pair<ForwardIterator, ForwardIterator>
SdfPathFindPrefixedRange(ForwardIterator begin, ForwardIterator end,
                         SdfPath const &prefix,
                         GetPathFn const &getPath = GetPathFn()) {
    using IterRef = 
        typename std::iterator_traits<ForwardIterator>::reference;

    struct Compare {
        Compare(GetPathFn const &getPath) : _getPath(getPath) {}
        GetPathFn const &_getPath;
        bool operator()(IterRef a, SdfPath const &b) const {
            return _getPath(a) < b;
        }
    };

    std::pair<ForwardIterator, ForwardIterator> result;

    // First, use lower_bound to find where \a prefix would go.
    result.first = std::lower_bound(begin, end, prefix, Compare(getPath));

    // Next, find end of range starting from the lower bound, using the
    // prefixing condition to define the boundary.
    result.second = TfFindBoundary(result.first, end,
                                   [&prefix, &getPath](IterRef iterRef) {
                                       return getPath(iterRef).HasPrefix(prefix);
                                   });

    return result;
}

template <class RandomAccessIterator, class GetPathFn>
RandomAccessIterator
Sdf_PathFindLongestPrefixImpl(RandomAccessIterator begin,
                              RandomAccessIterator end,
                              SdfPath const &path,
                              bool strictPrefix,
                              GetPathFn const &getPath)
{
    using IterRef = 
        typename std::iterator_traits<RandomAccessIterator>::reference;

    struct Compare {
        Compare(GetPathFn const &getPath) : _getPath(getPath) {}
        GetPathFn const &_getPath;
        bool operator()(IterRef a, SdfPath const &b) const {
            return _getPath(a) < b;
        }
    };

    // Search for the path in [begin, end).  If present, return it.  If not,
    // examine prior element in [begin, end).  If none, return end.  Else, is it
    // a prefix of path?  If so, return it.  Else find common prefix of that
    // element and path and recurse.

    // If empty sequence, return.
    if (begin == end)
        return end;

    Compare comp(getPath);

    // Search for where this path would lexicographically appear in the range.
    RandomAccessIterator result = std::lower_bound(begin, end, path, comp);

    // If we didn't get the end, check to see if we got the path exactly if
    // we're not looking for a strict prefix.
    if (!strictPrefix && result != end && getPath(*result) == path) {
        return result;
    }

    // If we got begin (and didn't match in the case of a non-strict prefix)
    // then there's no prefix.
    if (result == begin) {
        return end;
    }

    // If the prior element is a prefix, we're done.
    if (path.HasPrefix(getPath(*--result))) {
        return result;
    }

    // Otherwise, find the common prefix of the lexicographical predecessor and
    // look for its prefix in the preceding range.
    SdfPath newPath = path.GetCommonPrefix(getPath(*result));
    auto origEnd = end;
    do {
        end = result;
        result = std::lower_bound(begin, end, newPath, comp);

        if (result != end && getPath(*result) == newPath) {
            return result;
        }
        if (result == begin) {
            return origEnd;
        }
        if (newPath.HasPrefix(getPath(*--result))) {
            return result;
        }
        newPath = newPath.GetCommonPrefix(getPath(*result));
    } while (true);
}

/// Return an iterator to the element of [\a begin, \a end) that is the longest
/// prefix of the given path (including the path itself), if there is such an
/// element, otherwise \a end.  The input range must be ordered according to
/// SdfPath::operator<.  If your range's iterators' value_types are not SdfPath,
/// but you can obtain SdfPaths from them (e.g. vector<pair<SdfPath,
/// X>>::iterator), you can pass a function to extract the path from the
/// dereferenced iterator in \p getPath.
template <class RandomAccessIterator, class GetPathFn = Sdf_PathIdentity,
          class = typename std::enable_if<
              std::is_base_of<
                  std::random_access_iterator_tag,
                  typename std::iterator_traits<
                      RandomAccessIterator>::iterator_category
                  >::value
              >::type
          >
RandomAccessIterator
SdfPathFindLongestPrefix(RandomAccessIterator begin,
                         RandomAccessIterator end,
                         SdfPath const &path,
                         GetPathFn const &getPath = GetPathFn())
{
    return Sdf_PathFindLongestPrefixImpl(
        begin, end, path, /*strictPrefix=*/false, getPath);
}

/// Return an iterator to the element of [\a begin, \a end) that is the longest
/// prefix of the given path (excluding the path itself), if there is such an
/// element, otherwise \a end.  The input range must be ordered according to
/// SdfPath::operator<.  If your range's iterators' value_types are not SdfPath,
/// but you can obtain SdfPaths from them (e.g. vector<pair<SdfPath,
/// X>>::iterator), you can pass a function to extract the path from the
/// dereferenced iterator in \p getPath.
template <class RandomAccessIterator, class GetPathFn = Sdf_PathIdentity,
          class = typename std::enable_if<
              std::is_base_of<
                  std::random_access_iterator_tag,
                  typename std::iterator_traits<
                      RandomAccessIterator>::iterator_category
                  >::value
              >::type
          >
RandomAccessIterator
SdfPathFindLongestStrictPrefix(RandomAccessIterator begin,
                               RandomAccessIterator end,
                               SdfPath const &path,
                               GetPathFn const &getPath = GetPathFn())
{
    return Sdf_PathFindLongestPrefixImpl(
        begin, end, path, /*strictPrefix=*/true, getPath);
}

template <class Iter, class MapParam, class GetPathFn = Sdf_PathIdentity>
Iter
Sdf_PathFindLongestPrefixImpl(
    MapParam map, SdfPath const &path, bool strictPrefix,
    GetPathFn const &getPath = GetPathFn())
{
    // Search for the path in map.  If present, return it.  If not, examine
    // prior element in map.  If none, return end.  Else, is it a prefix of
    // path?  If so, return it.  Else find common prefix of that element and
    // path and recurse.

    const Iter mapEnd = map.end();

    // If empty, return.
    if (map.empty())
        return mapEnd;

    // Search for where this path would lexicographically appear in the range.
    Iter result = map.lower_bound(path);

    // If we didn't get the end, check to see if we got the path exactly if
    // we're not looking for a strict prefix.
    if (!strictPrefix && result != mapEnd && getPath(*result) == path)
        return result;

    // If we got begin (and didn't match in the case of a non-strict prefix)
    // then there's no prefix.
    if (result == map.begin())
        return mapEnd;

    // If the prior element is a prefix, we're done.
    if (path.HasPrefix(getPath(*--result)))
        return result;

    // Otherwise, find the common prefix of the lexicographical predecessor and
    // recurse looking for it or its longest prefix in the preceding range.  We
    // always pass strictPrefix=false, since now we're operating on prefixes of
    // the original caller's path.
    return Sdf_PathFindLongestPrefixImpl<Iter, MapParam>(
        map, path.GetCommonPrefix(getPath(*result)), /*strictPrefix=*/false,
        getPath);
}

/// Return an iterator pointing to the element of \a set whose key is the
/// longest prefix of the given path (including the path itself).  If there is
/// no such element, return \a set.end().
SDF_API
typename std::set<SdfPath>::const_iterator
SdfPathFindLongestPrefix(std::set<SdfPath> const &set, SdfPath const &path);

/// Return an iterator pointing to the element of \a map whose key is the
/// longest prefix of the given path (including the path itself).  If there is
/// no such element, return \a map.end().
template <class T>
typename std::map<SdfPath, T>::const_iterator
SdfPathFindLongestPrefix(std::map<SdfPath, T> const &map, SdfPath const &path)
{
    return Sdf_PathFindLongestPrefixImpl<
        typename std::map<SdfPath, T>::const_iterator,
        std::map<SdfPath, T> const &>(map, path, /*strictPrefix=*/false,
                                      TfGet<0>());
}
template <class T>
typename std::map<SdfPath, T>::iterator
SdfPathFindLongestPrefix(std::map<SdfPath, T> &map, SdfPath const &path)
{
    return Sdf_PathFindLongestPrefixImpl<
        typename std::map<SdfPath, T>::iterator,
        std::map<SdfPath, T> &>(map, path, /*strictPrefix=*/false,
                                TfGet<0>());
}

/// Return an iterator pointing to the element of \a set whose key is the
/// longest prefix of the given path (excluding the path itself).  If there is
/// no such element, return \a set.end().
SDF_API
typename std::set<SdfPath>::const_iterator
SdfPathFindLongestStrictPrefix(std::set<SdfPath> const &set,
                               SdfPath const &path);

/// Return an iterator pointing to the element of \a map whose key is the
/// longest prefix of the given path (excluding the path itself).  If there is
/// no such element, return \a map.end().
template <class T>
typename std::map<SdfPath, T>::const_iterator
SdfPathFindLongestStrictPrefix(
    std::map<SdfPath, T> const &map, SdfPath const &path)
{
    return Sdf_PathFindLongestPrefixImpl<
        typename std::map<SdfPath, T>::const_iterator,
        std::map<SdfPath, T> const &>(map, path, /*strictPrefix=*/true,
                                      TfGet<0>());
}
template <class T>
typename std::map<SdfPath, T>::iterator
SdfPathFindLongestStrictPrefix(
    std::map<SdfPath, T> &map, SdfPath const &path)
{
    return Sdf_PathFindLongestPrefixImpl<
        typename std::map<SdfPath, T>::iterator,
        std::map<SdfPath, T> &>(map, path, /*strictPrefix=*/true,
                                TfGet<0>());
}

PXR_NAMESPACE_CLOSE_SCOPE

// Sdf_PathNode is not public API, but we need to include it here
// so we can inline the ref-counting operations, which must manipulate
// its internal _refCount member.
#include "pxr/usd/sdf/pathNode.h"

PXR_NAMESPACE_OPEN_SCOPE

static_assert(Sdf_SizeofPrimPathNode == sizeof(Sdf_PrimPathNode), "");
static_assert(Sdf_SizeofPropPathNode == sizeof(Sdf_PrimPropertyPathNode), "");

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_PATH_H
